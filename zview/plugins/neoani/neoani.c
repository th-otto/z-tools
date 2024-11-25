#define	VERSION	     0x0205
#define NAME        "NEOchrome (Animation)"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("ANI\0");

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

/*
NEOchrome Animation    *.ANI (ST low resolution)

Note: To activate this hidden feature on versions 0.9 and later select the
Grabber icon and click both mouse buttons in the eye of the second R in the
word GRABBER.

Interestingly enough, some versions of NEO only require you to press the right
mouse button, not both.  Hmmm...

1 long      file ID, $BABEEBEA (seems to be ignored)
1 word      width of image in bytes (always divisible by 8)
1 word      height of image in scan lines
1 word      size of image in bytes + 10 (!)
1 word      x coordinate of image (must be divisible by 16) - 1
1 word      y coordinate of image - 1
1 word      number of frames
1 word      animation speed (# vblanks to delay between frames)
1 long      reserved (0)
--------
22 bytes    total for header

? words     image data (words of screen memory) for each frame, in order

NEOchrome Master adds an additional raster mode with 1 palette per scan line.
It offers two ways to store these images.

1) Standard 320x200 *.NEO image along with a separate *.RST palette file.
   The same base file name is used, example: TEST.NEO -> TEST.RST
   The *.RST file size is always 6800 bytes.

2) *.IFF image with an added 'RAST' chunk containing the palettes.
   Contrary to the IFF specifications the 'RAST' chunk comes after the 'BODY'
   chunk. The chunk size will be 6800 bytes.

File size: (2 + 32) * 200 = 6800
Palettes are stored in xbios format.

Both methods store the palette in the same format. From the NEOchrome Master
documentation (*.RST):

----->
|       WORD y_position;        / * shows the position of this raster * /
|       WORD palette[16];       / * The colors of the raster * /
|
------  This is repeated 200 times.

The first entry of the file is always the vbl-palette. It's y-position is set
to zero. All following rasters with a y_position of zero are not active.

It's also possible that the y_positions of the rasters are not sorted. It's
exactly the same setting as made in NEOchrome Master.

See also NEOchrome and IFF file formats.
*/

struct file_header {
	uint32_t magic;
	uint16_t width;
	uint16_t height;
	uint16_t frame_size;
	int16_t xpos;
	int16_t ypos;
	uint16_t num_frames;
	uint16_t speed;
	uint32_t reserved;
};

struct palette {
	uint16_t y_position;
	uint16_t palette[16];
};

/* FIXME: statics */
static size_t frame_size;
static uint32_t *frame_positions;
static uint16_t speed;
static uint16_t cur_y;

static uint8_t const colortab[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };


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
	uint16_t palette[16];
	int i;
	char tempname[256];
	uint8_t *bmap;
	struct file_header header;
	uint16_t wdwidth;
	int neofd;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	if (Fread(handle, sizeof(header), &header) != sizeof(header) ||
		header.magic != 0xBABEEBEAL)
	{
		Fclose(handle);
		return FALSE;
	}

	wdwidth = ((header.width >> 1) >> 2) << 4;
	frame_positions = malloc((size_t)header.num_frames * 2 * sizeof(*frame_positions));
	if (frame_positions == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	frame_size = header.frame_size - 10;
	
	bmap = malloc(frame_size + 256);
	if (bmap == NULL)
	{
		free(frame_positions);
		frame_positions = NULL;
		Fclose(handle);
		return FALSE;
	}

	for (i = 0; i < header.num_frames; i++)
	{
		frame_positions[i] = Fseek(0, handle, SEEK_CUR);
		Fseek(frame_size, handle, SEEK_CUR);
	}
	speed = header.speed * 2;
	if (speed == 0)
		speed = 10;
	if (Kbshift(-1) & K_ALT)
	{
		int j;
		uint32_t tmp;
		
		j = header.num_frames - 1;
		for (i = 0; i < j; i++, j--)
		{
			tmp = frame_positions[i];
			frame_positions[i] = frame_positions[j];
			frame_positions[j] = tmp;
		}
	}
	if (Kbshift(-1) & K_CAPSLOCK)
	{
		int k;
		int m;
		int j;
		
		/*
		 * build reverse frame map
		 */
		m = header.num_frames;
		j = header.num_frames - 1;
		k = header.num_frames;
		for (i = 0; i < k; j--, m++, header.num_frames++, i++)
			frame_positions[m] = frame_positions[j];
	}
		
	info->planes = 4;
	info->width = wdwidth;
	info->height = header.height;
	info->indexed_color = TRUE;
	info->components = 3;
	info->colors = 1L << 4;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = header.num_frames;		/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = handle;
	info->_priv_ptr = bmap;
	cur_y = 0;

	strcpy(info->info, "NEOchrome (Animation)");
	strcpy(info->compression, "RLE");
	
	strcpy(tempname, name);
	tempname[strlen(name) - 3] = 'n';
	tempname[strlen(name) - 2] = 'e';
	tempname[strlen(name) - 1] = 'o';
	
	for (i = 0; i < 16; i++)
	{
		info->palette[i].red =
		info->palette[i].green =
		info->palette[i].blue = colortab[i];
	}
	
	neofd = (int)Fopen(tempname, FO_READ);
	if (neofd >= 0)
	{
		if (Fseek(0, neofd, SEEK_END) == 32128L)
		{
			Fseek(4, neofd, SEEK_SET);
			Fread(neofd, sizeof(palette), palette);
			for (i = 0; i < 16; i++)
			{
				info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 17;
				info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 17;
				info->palette[i].blue = (((palette[i] << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 17;
			}
		}
		Fclose(neofd);
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

	screen16 = (const uint16_t *)info->_priv_ptr;

	if (cur_y == 0)
	{
		int handle = info->_priv_var_more;
		Fseek(frame_positions[info->page_wanted], handle, SEEK_SET);
		if ((size_t)Fread(handle, frame_size, screen16) != frame_size)
			return FALSE;
		info->delay = speed;
		pos = 0;
	}
	
	x = 0;
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
	} while (x < info->width);

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
	free(frame_positions);
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
