#define	VERSION	     0x0108
#define NAME        "C.O.L.R. Object Editor"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
ComputerEyes Raw Data    *.CE1 (low)
                         *.CE2 (medium)
                         *.CE3 (high)

1 long          file ID, $45594553 'EYES'
1 word          resolution [0 = low res, 1 = medium res, 2 = high res]
1 word          brightness adjust [0-191]
1 word          contrast adjust   [0-191]
1 word          red adjust        [0-191]
1 word          green adjust      [0-191]
1 word          blue adjust       [0-191]
1 word          reserved? [0]
1 word          reserved? [0]
1 word          reserved? [0]
If resolution = 0 {
64000 bytes     red plane,   320 x 200, 1 pixel per byte
64000 bytes     green plane, 320 x 200, 1 pixel per byte
64000 bytes     blue plane,  320 x 200, 1 pixel per byte
------------
192022 bytes    total
}
else If resolution = 1 {
128000 words    640 x 200, 1 pixel per word
------------
256022 bytes    total
}
else if resolution = 2 {
256000 bytes    640 x 400, 1 pixel per byte
------------
256022 bytes    total
}

   This is almost three formats in one:

        Low resolution:

        The planes are arranged vertically, instead of horizontally. The first
        byte is the red component of pixel (0,0), the second is (0,1), and the
        third (0,2). The 201st corresponds to (1,0), etc. The 64001st byte is
        the green component of (0,0).

        Only the lower six bits of each byte are used.

        Medium resolution:

        The picture is arranged vertically, instead of horizontally. The first
        word is pixel (0,0), the second is (0,1), and the third (0,2).  The
        200th is (1,0) etc.

        Each word is divided up into the RGB values for the corresponding
        pixel, as follows:

          Bit:  (MSB) 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00 (LSB)
                      -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
                       0 R4 R3 R2 R1 R0 G4 G3 G2 G1 G0 B4 B3 B2 B1 B0

          Bit 15 is not used.

        High resolution:

        The picture is arranged vertically, instead of horizontally. The first
        byte is pixel (0,0), the second is (0,2), and the third (0,4). The
        200th is (0,1), followed by (0,3), and (0,5), etc.

        Each byte represent a value from 0 to 191, which is the result of
        6-bits each of R, G, and B added together, but not divided by 3.

In order to reduce overhead the pixel arrangement matches the devices scanning
sequence. It's also assumed the divide by 3 was left out of the high resolution
dump for the same reason. The re-arrangement of the pixels and the divide could
be done during post processing anyway.

These files are created by CE.PRG which comes with the ComputerEyes hardware.
It's not documented in the manual how to create these files. You must hold down
the Alternate key while clicking Save or Load in the File drop-down menu.

Further more the program PicSwitch v1.0.1 can process these files. This feature
is also not documented.

Indecently CE.PRG and PicSwitch are both written by John Brochu.
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("CE1\0" "CE2\0" "CE3\0");

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
	uint32_t magic;
	uint16_t resolution;
	uint16_t brightness;
	uint16_t contrast;
	uint16_t red_adjust;
	uint16_t green_adjust;
	uint16_t blue_adjust;
	uint16_t reserved[3];
};


#define LOW_WIDTH 320L
#define LOW_HEIGHT 200L
#define LOW_FILESIZE (LOW_WIDTH * LOW_HEIGHT * 3)
#define MED_WIDTH 640L
#define MED_HEIGHT 200L
#define MED_FILESIZE (MED_WIDTH * MED_HEIGHT * 2)
#define HIGH_WIDTH 640L
#define HIGH_HEIGHT 400L
#define HIGH_FILESIZE (HIGH_WIDTH * HIGH_HEIGHT)

static uint8_t *red_plane;
static uint8_t *green_plane;
static uint8_t *blue_plane;
static int16_t const indexed_tab[3] = { FALSE, FALSE, TRUE };
static int16_t component_tab[3] = { 3, 3, 1 };
static int16_t const planetab[3] = { 18, 15, 6 };
static int16_t const width_tab[3] = { LOW_WIDTH, MED_WIDTH, HIGH_WIDTH };
static int16_t const height_tab[3] = { LOW_HEIGHT, MED_HEIGHT, HIGH_HEIGHT };
static size_t filesizes[3] = { LOW_FILESIZE, MED_FILESIZE, HIGH_FILESIZE };


static void convert_low_resolution(uint8_t *file_buffer)
{
	size_t offset;
	size_t x;
	size_t y;
	
	offset = 0;
	for (x = 0; x < LOW_WIDTH; x++)
	{
		for (y = 0; y < LOW_HEIGHT; y++)
		{
			red_plane[y * LOW_WIDTH + x] = file_buffer[offset];
			offset++;
		}
	}
	for (x = 0; x < LOW_WIDTH; x++)
	{
		for (y = 0; y < LOW_HEIGHT; y++)
		{
			green_plane[y * LOW_WIDTH + x] = file_buffer[offset];
			offset++;
		}
	}
	for (x = 0; x < LOW_WIDTH; x++)
	{
		for (y = 0; y < LOW_HEIGHT; y++)
		{
			blue_plane[y * LOW_WIDTH + x] = file_buffer[offset];
			offset++;
		}
	}
}


static void convert_med_resolution(const uint8_t *src, uint8_t *dst)
{
	size_t offset;
	size_t x;
	size_t y;
	uint16_t *bmap16;
	const uint16_t *file_buffer16;
	
	offset = 0;
	bmap16 = (uint16_t *)dst;
	file_buffer16 = (const uint16_t *)src;
	for (x = 0; x < MED_WIDTH; x++)
	{
		for (y = 0; y < MED_HEIGHT; y++)
		{
			bmap16[y * MED_WIDTH + x] = file_buffer16[offset];
			offset++;
		}
	}
}


static void convert_high_resolution(const uint8_t *src, uint8_t *dst)
{
	int y;
	int x;
	size_t offset;
	
	x = (int)HIGH_WIDTH;
	do
	{
		offset = 0;
		y = (int)HIGH_HEIGHT / 2;
		do
		{
			dst[offset] = *src++;
			offset += HIGH_WIDTH * 2;
		} while (--y > 0);

		offset = HIGH_WIDTH;
		y = (int)HIGH_HEIGHT / 2;
		do
		{
			dst[offset] = *src++;
			offset += HIGH_WIDTH * 2;
		} while (--y > 0);
		dst++;
	} while (--x > 0);
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
	uint8_t *bmap;
	uint8_t *file_buffer;
	struct file_header file_header;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	if (file_size != LOW_FILESIZE + sizeof(file_header) &&
		file_size != MED_FILESIZE + sizeof(file_header) &&
		file_size != HIGH_FILESIZE + sizeof(file_header))
	{
		Fclose(handle);
		return FALSE;
	}
	Fseek(0, handle, SEEK_SET);
	
	if ((size_t)Fread(handle, sizeof(file_header), &file_header) != sizeof(file_header) ||
		file_header.magic != 0x45594553L ||
		file_header.resolution >= 3)
	{
		Fclose(handle);
		return FALSE;
	}
	
	bmap = malloc(filesizes[file_header.resolution]);
	file_buffer = malloc(filesizes[file_header.resolution]);
	if (bmap == NULL || file_buffer == NULL)
	{
		free(file_buffer);
		free(bmap);
		return FALSE;
	}
	if ((size_t)Fread(handle, filesizes[file_header.resolution], file_buffer) != filesizes[file_header.resolution])
	{
		free(file_buffer);
		free(bmap);
		return FALSE;
	}
	
	strcpy(info->info, "ComputerEyes");
	switch (file_header.resolution)
	{
	case 0:
		red_plane = bmap;
		green_plane = bmap + LOW_WIDTH * LOW_HEIGHT;
		blue_plane = bmap + LOW_WIDTH * LOW_HEIGHT * 2;
		convert_low_resolution(file_buffer);
		strcat(info->info, " (Low/6-bit RGB)");
		break;
	case 1:
		convert_med_resolution(file_buffer, bmap);
		strcat(info->info, " (Medium/5-bit RGB)");
		break;
	case 2:
		convert_high_resolution(file_buffer, bmap);
		strcat(info->info, " (High/6-bit Grayscale)");
		break;
	}
	Fclose(handle);
	free(file_buffer);
	
	info->planes = planetab[file_header.resolution];
	info->width = width_tab[file_header.resolution];
	info->height = height_tab[file_header.resolution];
	info->components = component_tab[file_header.resolution];
	info->indexed_color = indexed_tab[file_header.resolution];
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->compression, "None");
	
	if (info->indexed_color)
	{
		int i;
		uint16_t color;
		
		color = 0;
		for (i = 0; i < 64; i++)
		{
			info->palette[i].red =
			info->palette[i].green =
			info->palette[i].blue = color;
			color += 4;
		}
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
	uint16_t x;
	uint16_t w;
	size_t pos = info->_priv_var;
	
	w = 0;
	switch (info->planes)
	{
	case 18:
		{
			for (x = 0; x < info->width; x++)
			{
				buffer[w] = red_plane[pos] << 2;
				w++;
				buffer[w] = green_plane[pos] << 2;
				w++;
				buffer[w] = blue_plane[pos] << 2;
				w++;
				pos++;
			}
		}
		break;
	case 15:
		{
			const uint16_t *bmap16 = (const uint16_t *)info->_priv_ptr;
			uint16_t color;
			
			for (x = 0; x < info->width; x++)
			{
				color = bmap16[pos++];
				buffer[w] = ((color >> 10) & 0x1f) << 3;
				w++;
				buffer[w] = ((color >> 5) & 0x1f) << 3;
				w++;
				buffer[w] = (color & 0x1f) << 3;
				w++;
			}
		}
		break;
	case 6:
		{
			const uint8_t *bmap = (const uint8_t *)info->_priv_ptr;

			for (x = 0; x < info->width; x++)
			{
				buffer[w] = (bmap[pos++] / 3);
				w++;
			}
		}
		break;
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
