#define	VERSION	     0x0103
#define NAME        "Cybermate (Animation)"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
                               Stereo CAD-3D 2.0
                     Communication Pipeline Specification
                                 by Tom Hudson
                                  April, 1987
                           Copyright 1987 Tom Hudson
                              All rights reserved


CYBERMATE COMPRESSED DELTA FILE FORMAT

The Cybermate animation system uses a "delta" compression technique for storage
of animated sequences. A delta compression is a simple technique which compares
a frame of animation to the previous frame, storing only the changes (or
deltas) that occurred from one frame to the next.

In the current application of the delta compression technique, the first frame
of the animation sequence is stored in a DEGAS-format picture file, and the
remainder of the animation sequence is stored as a series of delta values in a
.DLT file.

Each frame of the animation is recorded as a series of delta values, each of
which is stored as a WORD value from 0 to 31996 which indicate an offset into
the 32000-byte display bitmap memory, then a LONG value which must be EOR'ed at
the specified point in the display memory. This changes the previous frame's
LONG value to the new frame's value. The EOR technique allows animations to be
played in reverse as well.

Each frame has a WORD which indicates the number of deltas that are present for
that frame. When all those are processed, a new frame delta count WORD is read
and the process is repeated.

The .DLT file has the following format:

WORD -- The number of deltas in this frame. A zero in this flag indicates the
end of the file.  Frames with no deltas (the same as the previous frame) are
special cases and a dummy delta offset and LONG EOR value of zero are provided
in the delta data which follows.

The following structure is repeated the number of times specified in the delta
count for this frame.

WORD -- Offset into 32000-byte screen RAM for the delta data. This number is a
        multiple of 4 from 0 to 31996. It should be used as an offset from the
        start of screen RAM to EOR the following LONG.

LONG -- Delta data. This value is EOR'ed with the screen data at [screenbase +
        offset] to change the previous frame's data to the new frame's.

Once all deltas for a particular frame have been processed, the program reads
the number of deltas for the next frame, and continues with this process until
all frames have been processed (delta count = 0).
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("DLT\0");

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

#define SCREEN_SIZE 32000

/* FIXME */
static uint32_t frame_positions[ZVIEW_MAX_IMAGES];
static int cur_y;

/* to be fixed: */
#define bail(msg)

#include "../degas/packbits.c"
#include "../degas/tanglbts.c"

static int fexists(const char *name)
{
	long ret = Fattrib(name, 0, 0);
	if (ret < 0)
		return FALSE;
	return TRUE;
}


static int find_degas(const char *name, char *degas_name)
{
	char *ext;
	
	strcpy(degas_name, name);
	ext = degas_name + strlen(degas_name) - 3;
	strcpy(ext, "pi1");
	if (fexists(degas_name))
		return 0;
	strcpy(ext, "pc1");
	if (fexists(degas_name))
		return 0;
	strcpy(ext, "pi2");
	if (fexists(degas_name))
		return 1;
	strcpy(ext, "pc2");
	if (fexists(degas_name))
		return 1;
	strcpy(ext, "pi3");
	if (fexists(degas_name))
		return 2;
	strcpy(ext, "pc3");
	if (fexists(degas_name))
		return 2;
	return -1;
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
	size_t file_size;
	int16_t handle;
	int fh;
	uint8_t *bmap;
	uint16_t palette[16];
	uint16_t num_deltas;
	uint16_t res;
	int degas_res;
	int num_frames;
	char degas_name[256];

	handle = (int16_t)Fopen(name, FO_READ);
	if (handle < 0)
	{
		bail("Invalid path");
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	degas_res = find_degas(name, degas_name);
	if (degas_res < 0)
	{
		bail("Invalid path");
		Fclose(handle);
		return FALSE;
	}
	bmap = malloc(SCREEN_SIZE);
	if (bmap == NULL)
	{
		bail("Malloc(bitmap) failed");
		Fclose(handle);
		return FALSE;
	}

	num_frames = 1;
	for (;;)
	{
		if ((size_t) Fread(handle, sizeof(num_deltas), &num_deltas) != sizeof(num_deltas))
		{
			bail("Fread failed");
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		if (num_deltas == 0)
			break;
		if (num_frames >= ZVIEW_MAX_IMAGES)
			break;
		frame_positions[num_frames] = Fseek(0, handle, SEEK_CUR) - 2;
		/* skip offset & delta value */
		Fseek(6 * num_deltas, handle, SEEK_CUR);
		num_frames++;
	}
	
	Fseek(0, handle, SEEK_SET);
	
	fh = (int)Fopen(degas_name, FO_READ);
	if (fh < 0)
	{
		bail("Invalid path");
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	
	file_size = Fseek(0, fh, SEEK_END);
	Fseek(0, fh, SEEK_SET);
	if (Fread(fh, sizeof(res), &res) != sizeof(res) ||
		Fread(fh, sizeof(palette), palette) != sizeof(palette) ||
		(res & 3) > 2)
	{
		bail("Fread failed");
		Fclose(fh);
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	
	info->planes = 4 >> (res & 3);
	switch (info->planes)
	{
	case 4:
		info->width = 320;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 2:
		info->width = 640;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 1:
		info->width = 640;
		info->height = 400;
		info->indexed_color = FALSE;
		break;
	}

	if (res & 0x8000)
	{
		uint8_t *temp;
		
		file_size -= sizeof(res) + sizeof(palette);
		if ((size_t)Fread(fh, file_size, bmap) != file_size)
		{
			bail("Fread failed");
			Fclose(fh);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		temp = malloc(SCREEN_SIZE);
		if (temp == NULL)
		{
			bail("Malloc(bitmap) failed");
			Fclose(fh);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		decode_packbits(temp, bmap, SCREEN_SIZE);
		tangle_bitplanes(bmap, temp, info->width, info->height, info->planes);
		free(temp);
	} else
	{
		if (Fread(fh, SCREEN_SIZE, bmap) != SCREEN_SIZE)
		{
			bail("Fread failed");
			Fclose(fh);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
	}
	Fclose(fh);
	
	if (info->indexed_color)
	{
		int colors = 1 << info->planes;
		int i;

		for (i = 0; i < colors; i++)
		{
			info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((palette[i] << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 0x11;
		}
	}

	info->components = info->planes == 1 ? 1 : 3;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = num_frames;			/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = handle;
	info->_priv_ptr = bmap;
	cur_y = 0;

	strcpy(info->info, "Cybermate (Animation)");
	strcpy(info->compression, "DLTa");

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
	size_t pos = info->_priv_var;
	
	if (cur_y == 0)
	{
		if (info->page_wanted != 0)
		{
			uint32_t *bmap32;
			uint16_t i;
			uint16_t num_deltas;
			uint16_t delta_offset;
			uint32_t delta_value;
			int handle = info->_priv_var_more;
			
			bmap32 = (uint32_t *)info->_priv_ptr;
			Fseek(frame_positions[info->page_wanted], handle, SEEK_SET);
			Fread(handle, sizeof(num_deltas), &num_deltas);
			for (i = 0; i < num_deltas; i++)
			{
				if (Fread(handle, sizeof(delta_offset), &delta_offset) != sizeof(delta_offset) ||
					Fread(handle, sizeof(delta_value), &delta_value) != sizeof(delta_value))
					break;
				if (delta_offset < SCREEN_SIZE - 4)
					bmap32[delta_offset >> 2] ^= delta_value;
			}
		}
		info->delay = 10;
		pos = 0;
	}
	
	switch (info->planes)
	{
	case 1:
		{
			int16_t x;
			int16_t byte;
			uint8_t *bmap8;
			
			x = 0;
			bmap8 = info->_priv_ptr;
			do
			{								/* 1-bit mono v1.00 */
				byte = bmap8[pos++];
				*buffer++ = (byte >> 7) & 1;
				*buffer++ = (byte >> 6) & 1;
				*buffer++ = (byte >> 5) & 1;
				*buffer++ = (byte >> 4) & 1;
				*buffer++ = (byte >> 3) & 1;
				*buffer++ = (byte >> 2) & 1;
				*buffer++ = (byte >> 1) & 1;
				*buffer++ = (byte >> 0) & 1;
				x += 8;
			} while (x < info->width);
		}
		break;
	default:
		{
			int16_t x;
			int16_t bit;
			uint16_t byte;
			int16_t plane;
			const uint16_t *bmap16 = (const uint16_t *)info->_priv_ptr;
			
			x = 0;
			do
			{
				for (bit = 15; bit >= 0; bit--)
				{
					for (plane = byte = 0; plane < info->planes; plane++)
					{
						if ((bmap16[pos + plane] >> bit) & 1)
							byte |= 1 << plane;
					}
					buffer[x] = byte;
					x++;
				}
				pos += info->planes;
			} while (x < info->width);
		}
		break;
	}
	
	++cur_y;
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
	if (info->_priv_var_more > 0)
	{
		Fclose(info->_priv_var_more);
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
