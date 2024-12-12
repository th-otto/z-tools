#define	VERSION	     0x0105
#define NAME        "Cyber Paint (Animation)"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
Cyber Paint Sequence    *.SEQ (ST low resolution only)

This format, while fairly complex, yields excellent compression of animated
images while offering reasonably fast decompression times.

1 word          file ID, [$FEDB = Cyber Paint, $FEDC = Flicker]
1 word          version number
1 long          number of frames
1 word          speed (high byte is vblanks per frame)
1 byte          resolution [always 0]
1 byte          flags
52 bytes        reserved [0]
64 bytes        padding
---------
128 bytes       total for main header

for each frame {
1 word          type (always -1, used as double check)
1 word          resolution [always 0]
16 words        palette
12 bytes        filename [usually "        .   "]
1 word          color animation segment [not used]
1 byte          color animation active [not used]
1 byte          color animation speed [not used]
1 word          slide time, 60Hz ticks until next picture
1 word          x offset for this frame [0 - 319]
1 word          y offset for this frame [0 - 199]
1 word          width of this frame, in pixels (may be 0, see below)
1 word          height of this frame, in pixels (may be 0, see below)
1 byte          operation [0 = copy, 1 = exclusive or]
1 byte          storage method [0 = uncompressed, 1 = compressed]
1 long          length of data in bytes (if the data is compressed, this will
                be the size of the compressed data BEFORE decompression)
30 bytes        reserved [0]
30 bytes        padding
--------
128 bytes       total for frame header

? bytes         data
}

Frames are "delta-compressed," meaning that only the changes from one frame to
the next are stored. On the ST, .SEQ files are always full-screen low
resolution animations, so the sequence resulting from expanding all the data
will be n 320 by 200 pixel low resolution screens, where n is given in the .SEQ
header.

Since only the changes from frame to frame are stored, image data for a frame
will rarely be 320x200 (except for the very first frame, which will always be a
full screen). Instead what is stored is the smallest rectangular region on the
screen that contains all the changes from the previous frame to the current
frame. The x offset and y offset in the frame header determine where the upper
left corner of the "change box" lies, and the width and height specify the
box's size.

Additionally, each "change box" is stored in one of five ways. For each of
these, the screen is assumed to have the full-screen image from the last frame
on it.

    o uncompressed copy: The data for this frame is uncompressed image data,
      and is simply copied onto the screen at position (x, y) specified in the
      frame header.

    o uncompressed eor: The data for this frame is exclusive or'ed with the
      screen at position (x, y).

    o compressed copy: The data for this frame must be decompressed (see
      below), and then copied onto the screen at position (x, y) specified in
      the frame header.

    o compressed eor: The data for this frame must be decompressed (see below),
      and then exclusive or'ed with the screen RAM at position (x, y).

    o null frame: The width and/or height of this frame is 0, so this frame is
      the same as the previous frame.

    Of the 5 methods above, the one that results in the smallest amount of data
    being stored for a particular is used for that frame.

Compression Scheme:

Compression is similar to that employed by Tiny STuff, but is not quite as
space efficient.

Control word meanings:

        For a given control word, x:

        x < 0   Absolute value specifies the number of unique words to take
                from the data section (from 1 to 32767).
        x > 0   Specifies the number of times to repeat the next word taken
                from the data section (from 1 to 32767).

        Note that a control word of 0 is possible but meaningless.

Format of expanded data:

The expanded data is not simply screen memory bitmap data; instead the four
bitplanes are separated, and the data within each bitplane is presented
vertically instead of horizontally. (This results in better compression.)

    To clarify, data for a full screen would appear in the following order:

    bitplane 0, word 0, scanline 0
    bitplane 0, word 0, scanline 1
    ...
    bitplane 0, word 0, scanline 199
    bitplane 0, word 1, scanline 0
    bitplane 0, word 1, scanline 1
    ...
    bitplane 0, word 1, scanline 199
    ...
    bitplane 0, word 79, scanline 199
    bitplane 1, word 0, scanline 0
    ...
    bitplane 3, word 79, scanline 199

Note however, that the data does not usually refer to an entire screen, but
rather to the smaller "change box," whose size is given in the frame header.
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("SEQ\0");

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif


struct file_header {
	uint16_t id;
	uint16_t version;
	uint32_t num_frames;
	uint16_t speed;
	uint8_t resolution;
	uint8_t flags;
	uint8_t reserved[52];
	uint8_t pad[64];
};

struct frame_header {
	uint16_t type;
	uint16_t resolution;
	uint16_t palette[16];
	char filename[12];
	uint16_t segment;
	uint8_t active;
	uint8_t speed;
	uint16_t slide;
	uint16_t x_offset;
	uint16_t y_offset;
	uint16_t width;
	uint16_t height;
	uint8_t operation;
	uint8_t compression;
	uint32_t datasize;
	uint8_t reserved[30];
	uint8_t pad[30];
};

#define SCREEN_SIZE 32000

static int16_t frame_x_offset;
static int16_t frame_y_offset;
static int16_t frame_x_end;
static int16_t frame_y_end;
static uint8_t *bmap;
static uint8_t *frame_data;
static uint8_t *screen;
static int cur_y;
static uint32_t *frame_positions;
static int const plane_bytes = 40;
static int const bytes_per_line = 160;
static int const screen_width = 320;
static struct file_header file_header;
static struct frame_header frame_header;


static void flip_orientation(uint8_t *screenptr, uint8_t *buffer, int width, int height, int planes)
{
	int wdwidth;
	int x;
	int y;
	int pln;
	
	wdwidth = (width + 15) >> 4;
	if (planes == 1)
	{
		memcpy(screenptr, buffer, wdwidth * 2 * height);
	} else
	{
		y = height;
		while (y > 0)
		{
			for (x = 0; x < wdwidth; x++)
			{
				for (pln = 0; pln < planes; pln++)
				{
					long pos = (pln * wdwidth + x) * 2;
					*screenptr++ = buffer[pos + 0];
					*screenptr++ = buffer[pos + 1];
				}
			}
			buffer += planes * 2 * wdwidth;
			y--;
		}
	}
}


static void decode_uncompressed_copy(void)
{
}


static void decode_uncompressed_eor(void)
{
}


static void decode_compressed_copy(void)
{
	uint32_t src_offset;
	int x;
	int y;
	int dst_offset;
	int x_inc;
	int bitpos;
	int count;
	uint8_t c1;
	uint8_t c2;
	uint8_t c3;
	int i;
	
	src_offset = 0;
	x = frame_x_offset;
	y = frame_y_offset;
	bitpos = x & 7;
	x_inc = 0;
	dst_offset = y * bytes_per_line + (x >> 3);
	memset(bmap, 0, SCREEN_SIZE);
	if (bitpos == 0)
	{
		while (src_offset < frame_header.datasize)
		{
			c1 = frame_data[src_offset++];
			c2 = frame_data[src_offset++];
			if (((count = (c1 << 8) | c2) & 0x8000) == 0)
			{
				/* repeat next word */
				c1 = frame_data[src_offset++];
				c2 = frame_data[src_offset++];
				for (i = 0; i < count; i++)
				{
					bmap[dst_offset] = c1;
					if (x < screen_width - 8)
						bmap[dst_offset + 1] = c2;
					y++;
					dst_offset += bytes_per_line;
					if (y >= frame_y_end)
					{
						y = frame_y_offset;
						x += 16;
						if (x >= frame_x_end)
						{
							x = frame_x_offset;
							x_inc++;
						}
						dst_offset = x_inc * plane_bytes + y * bytes_per_line + (x >> 3);
					}
				}
			} else
			{
				/* literal copy */
				count ^= 0x8000;
				for (i = 0; i < count; i++)
				{
					c1 = frame_data[src_offset++];
					c2 = frame_data[src_offset++];
					bmap[dst_offset] = c1;
					if (x < screen_width - 8)
						bmap[dst_offset + 1] = c2;
					y++;
					dst_offset += bytes_per_line;
					if (y >= frame_y_end)
					{
						y = frame_y_offset;
						x += 16;
						if (x >= frame_x_end)
						{
							x = frame_x_offset;
							x_inc++;
						}
						dst_offset = x_inc * plane_bytes + y * bytes_per_line + (x >> 3);
					}
				}
			}
		}
	} else
	{
		int shiftcount;
		int beg_mask;
		int end_mask;

		shiftcount = 8 - bitpos;
		beg_mask = (0xff << shiftcount) & 0xff;
		end_mask = (0xff >> bitpos);
		while (src_offset < frame_header.datasize)
		{
			c1 = frame_data[src_offset++];
			c2 = frame_data[src_offset++];
			if (((count = (c1 << 8) | c2) & 0x8000) == 0)
			{
				/* repeat next word */
				c1 = frame_data[src_offset++];
				c2 = frame_data[src_offset++];
				c3 = c2 << shiftcount;
				c2 = ((c1 << shiftcount) & beg_mask) | (c2 >> bitpos);
				c1 = c1 >> bitpos;
				for (i = 0; i < count; i++)
				{
					bmap[dst_offset] = (bmap[dst_offset] & beg_mask) | c1;
					if (x < screen_width - 8)
					{
						bmap[dst_offset + 1] = c2;
						if (x < screen_width - 16)
							bmap[dst_offset + 2] = (bmap[dst_offset + 2] & end_mask) | c3;
					}
					y++;
					dst_offset += bytes_per_line;
					if (y >= frame_y_end)
					{
						y = frame_y_offset;
						x += 16;
						if (x >= frame_x_end)
						{
							x = frame_x_offset;
							x_inc++;
						}
						dst_offset = x_inc * plane_bytes + y * bytes_per_line + (x >> 3);
					}
				}
			} else
			{
				/* literal copy */
				count ^= 0x8000;
				for (i = 0; i < count; i++)
				{
					c1 = frame_data[src_offset++];
					c2 = frame_data[src_offset++];
					c3 = c2 << shiftcount;
					c2 = ((c1 << shiftcount) & beg_mask) | (c2 >> bitpos);
					c1 = c1 >> bitpos;
					bmap[dst_offset] = (bmap[dst_offset] & beg_mask) | c1;
					if (x < screen_width - 8)
					{
						bmap[dst_offset + 1] = c2;
						if (x < screen_width - 16)
							bmap[dst_offset + 2] = (bmap[dst_offset + 2] & end_mask) | c3;
					}
					y++;
					dst_offset += bytes_per_line;
					if (y >= frame_y_end)
					{
						y = frame_y_offset;
						x += 16;
						if (x >= frame_x_end)
						{
							x = frame_x_offset;
							x_inc++;
						}
						dst_offset = x_inc * plane_bytes + y * bytes_per_line + (x >> 3);
					}
				}
			}
		}
	}
}


static void decode_compressed_eor(void)
{
	uint32_t src_offset;
	int x;
	int y;
	int dst_offset;
	int x_inc;
	int bitpos;
	int count;
	uint8_t c1;
	uint8_t c2;
	uint8_t c3;
	int i;
	
	src_offset = 0;
	x = frame_x_offset;
	y = frame_y_offset;
	bitpos = x & 7;
	x_inc = 0;
	dst_offset = y * bytes_per_line + (x >> 3);
	if (bitpos == 0)
	{
		while (src_offset < frame_header.datasize)
		{
			c1 = frame_data[src_offset++];
			c2 = frame_data[src_offset++];
			if (((count = (c1 << 8) | c2) & 0x8000) == 0)
			{
				/* repeat next word */
				c1 = frame_data[src_offset++];
				c2 = frame_data[src_offset++];
				for (i = 0; i < count; i++)
				{
					bmap[dst_offset] ^= c1;
					if (x < screen_width - 8)
						bmap[dst_offset + 1] ^= c2;
					y++;
					dst_offset += bytes_per_line;
					if (y >= frame_y_end)
					{
						y = frame_y_offset;
						x += 16;
						if (x >= frame_x_end)
						{
							x = frame_x_offset;
							x_inc++;
						}
						dst_offset = x_inc * plane_bytes + y * bytes_per_line + (x >> 3);
					}
				}
			} else
			{
				/* literal copy */
				count ^= 0x8000;
				for (i = 0; i < count; i++)
				{
					c1 = frame_data[src_offset++];
					c2 = frame_data[src_offset++];
					bmap[dst_offset] ^= c1;
					if (x < screen_width - 8)
						bmap[dst_offset + 1] ^= c2;
					y++;
					dst_offset += bytes_per_line;
					if (y >= frame_y_end)
					{
						y = frame_y_offset;
						x += 16;
						if (x >= frame_x_end)
						{
							x = frame_x_offset;
							x_inc++;
						}
						dst_offset = x_inc * plane_bytes + y * bytes_per_line + (x >> 3);
					}
				}
			}
		}
	} else
	{
		int shiftcount;
		int beg_mask;
		int end_mask;

		shiftcount = 8 - bitpos;
		beg_mask = (0xff << shiftcount) & 0xff;
		end_mask = (0xff >> bitpos);
		while (src_offset < frame_header.datasize)
		{
			c1 = frame_data[src_offset++];
			c2 = frame_data[src_offset++];
			if (((count = (c1 << 8) | c2) & 0x8000) == 0)
			{
				/* repeat next word */
				c1 = frame_data[src_offset++];
				c2 = frame_data[src_offset++];
				c3 = c2 << shiftcount;
				c2 = ((c1 << shiftcount) & beg_mask) | (c2 >> bitpos);
				c1 = c1 >> bitpos;
				for (i = 0; i < count; i++)
				{
					bmap[dst_offset] = ((bmap[dst_offset] & end_mask) ^ c1) | (bmap[dst_offset] & beg_mask);
					if (x < screen_width - 8)
					{
						bmap[dst_offset + 1] ^= c2;
						if (x < screen_width - 16)
							bmap[dst_offset + 2] = ((bmap[dst_offset + 2] & beg_mask) ^ c3) | (bmap[dst_offset + 2] & end_mask);
					}
					y++;
					dst_offset += bytes_per_line;
					if (y >= frame_y_end)
					{
						y = frame_y_offset;
						x += 16;
						if (x >= frame_x_end)
						{
							x = frame_x_offset;
							x_inc++;
						}
						dst_offset = x_inc * plane_bytes + y * bytes_per_line + (x >> 3);
					}
				}
			} else
			{
				/* literal copy */
				count ^= 0x8000;
				for (i = 0; i < count; i++)
				{
					c1 = frame_data[src_offset++];
					c2 = frame_data[src_offset++];
					c3 = c2 << shiftcount;
					c2 = ((c1 << shiftcount) & beg_mask) | (c2 >> bitpos);
					c1 = c1 >> bitpos;
					bmap[dst_offset] = ((bmap[dst_offset] & end_mask) ^ c1) | (bmap[dst_offset] & beg_mask);
					if (x < screen_width - 8)
					{
						bmap[dst_offset + 1] ^= c2;
						if (x < screen_width - 16)
							bmap[dst_offset + 2] = ((bmap[dst_offset + 2] & beg_mask) ^ c3) | (bmap[dst_offset + 2] & end_mask);
					}
					y++;
					dst_offset += bytes_per_line;
					if (y >= frame_y_end)
					{
						y = frame_y_offset;
						x += 16;
						if (x >= frame_x_end)
						{
							x = frame_x_offset;
							x_inc++;
						}
						dst_offset = x_inc * plane_bytes + y * bytes_per_line + (x >> 3);
					}
				}
			}
		}
	}
}



static void decode_framedata(void)
{
	frame_x_offset = frame_header.x_offset;
	frame_y_offset = frame_header.y_offset;
	frame_x_end = frame_header.x_offset + frame_header.width;
	frame_y_end = frame_header.y_offset + frame_header.height;
	switch ((frame_header.operation * 2) | frame_header.compression)
	{
	case 0:
		decode_uncompressed_copy();
		break;
	case 1:
		decode_compressed_copy();
		break;
	case 2:
		decode_uncompressed_eor();
		break;
	case 3:
		decode_compressed_eor();
		break;
	}
}


/*==================================================================================*
 * boolean __CDECL reader_init:														*
 *		Open the file "name", fit the "info" struct. ( see zview.h) and make others	*
 *		things needed by the decoder.												*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		name		->	The file to open.											*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	int16_t handle;
	int i;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	if ((size_t)Fread(handle, sizeof(file_header), &file_header) != sizeof(file_header) ||
		(file_header.id != 0xfedc && file_header.id != 0xfedb))
	{
		Fclose(handle);
		return FALSE;
	}
	bmap = malloc(SCREEN_SIZE * 3 + file_header.num_frames * sizeof(*frame_positions));
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	frame_data = bmap + SCREEN_SIZE;
	screen = frame_data + SCREEN_SIZE;
	frame_positions = (uint32_t *)(screen + SCREEN_SIZE);
	
	if ((size_t)Fread(handle, file_header.num_frames * sizeof(*frame_positions), frame_positions) != file_header.num_frames * sizeof(*frame_positions) ||
		(size_t)Fread(handle, sizeof(frame_header), &frame_header) != sizeof(frame_header) ||
		frame_header.type != (uint16_t)-1)
	{
		free(bmap);
		bmap = NULL;
		Fclose(handle);
		return FALSE;
	}

	file_header.speed = file_header.speed / 31;
	if (file_header.speed == 0)
		file_header.speed = 10;

	info->planes = 4;
	info->width = 320;
	info->height = 200;
	info->indexed_color = TRUE;
	info->components = 3;
	info->colors = 1L << 4;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = file_header.num_frames;	/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = handle;
	info->_priv_ptr = bmap;
	cur_y = 0;
	if (file_header.id == 0xfedc)
		strcpy(info->info, "Flicker");
	else if (file_header.id == 0xfedb)
		strcpy(info->info, "Cyber Paint");
	strcat(info->info, " (Animation)");
	strcpy(info->compression, "RLE");
	
	for (i = 0; i < 16; i++)
	{
		info->palette[i].red = (((frame_header.palette[i] >> 7) & 0x0e) + ((frame_header.palette[i] >> 11) & 0x01)) * 17;
		info->palette[i].green = (((frame_header.palette[i] >> 3) & 0x0e) + ((frame_header.palette[i] >> 7) & 0x01)) * 17;
		info->palette[i].blue = (((frame_header.palette[i] << 1) & 0x0e) + ((frame_header.palette[i] >> 3) & 0x01)) * 17;
	}

	return TRUE;
}


/*==================================================================================*
 * boolean __CDECL reader_read:														*
 *		This function fits the buffer with image data								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		buffer		->	The destination buffer.										*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	uint32_t pos = info->_priv_var;
	const uint16_t *screen16;
	int16_t x;
	int16_t i;
	uint16_t byte;
	int16_t plane;
	int16_t width = (info->width + 15) & -16;

	x = 0;
	if (cur_y == 0)
	{
		int handle = (int)info->_priv_var_more;
		Fseek(frame_positions[info->page_wanted], handle, SEEK_SET);
		if ((size_t)Fread(handle, sizeof(frame_header), &frame_header) != sizeof(frame_header))
			return FALSE;
		if (frame_header.datasize != 0)
		{
			if ((size_t)Fread(handle, frame_header.datasize, frame_data) != frame_header.datasize)
				return FALSE;
			decode_framedata();
			flip_orientation(screen, bmap, 320, 200, 4);
		}
		info->delay = file_header.speed;
	}
	
	screen16 = (const uint16_t *)screen;
	do
	{
		for (i = 15; i >= 0; i--)
		{
			for (plane = byte = 0; plane < info->planes; plane++)
			{
				if ((screen16[pos + plane] >> i) & 1)
					byte |= 1 << plane;
			}
			buffer[x] = byte;
			x++;
		}
		pos += info->planes;
	} while (x < width);

	cur_y++;
	if (cur_y == info->height)
	{
		cur_y = 0;
		pos = 0;
	}
	info->_priv_var = pos;
	
	return TRUE;
}


/*==================================================================================*
 * boolean __CDECL reader_quit:														*
 *		This function makes the last job like close the file handler and free the	*
 *		allocated memory.															*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_quit(IMGINFO info)
{
	free(info->_priv_ptr);
	info->_priv_ptr = 0;
	bmap = NULL;
	screen = NULL;
	frame_data = NULL;
	frame_positions = NULL;
	if (info->_priv_var_more > 0)
	{
		Fclose((int)info->_priv_var_more);
		info->_priv_var_more = 0;
	}
}


/*==================================================================================*
 * boolean __CDECL reader_get_txt													*
 *		This function , like other function must be always present.					*
 *		It fills txtdata struct. with the text present in the picture ( if any).	*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		txtdata		->	The destination text buffer.								*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
