#define	VERSION	     0x208
#define NAME        "RGB Intermediate Format"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
RGB Intermediate Format    *.RGB (ST low resolution only)

This format was invented by Lars Michael to facilitate conversions between
standard ST picture formats and higher resolution formats like GIF and IFF. It
supports 12 bits of color resolution by keeping the red, green and blue
components in separate bit planes.

1 word          resolution (ignored)
16 word         palette (ignored)
16000 words     red plane (screen memory)
1 word          resolution (ignored)
16 word         palette (ignored)
16000 words     green plane (screen memory)
1 word          resolution (ignored)
16 word         palette (ignored)
16000 words     blue plane (screen memory)
------------
96102 bytes     total

The format was derived by concatenating three DEGAS .PI1 files together -- one
for each color gun. The RGB value for a pixel is constructed by looking at the
appropriate pixel in the red plane, green plane, and blue plane. The bitplanes
are in standard ST low resolution screen RAM format, but where pixel values in
screen RAM refer to palette entries (0 through 15), pixel values here
correspond to absolute R, G, and B values. The red, green, and blue components
for each pixel range from 0 to 15 (4 bits), yielding a total of 12 bits of
color information per pixel. Not coincidentally, this is exactly the format of
ST palette entries (although on ST's without the extended palette only the
lower 3 bits of each color component are used).

You can view a single bit plane on a standard ST by splitting the .RGB file
into its three DEGAS .PI1 components and setting the palette to successively
brighter shades of gray.
*/


#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("RGB\0");

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

#define SCREEN_SIZE 32000L

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
	uint8_t *bmap;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	if (Fseek(0, handle, SEEK_END) != 96102L)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	bmap = malloc(SCREEN_SIZE * 3);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if (Fseek(34, handle, SEEK_SET) < 0 ||
		Fread(handle, SCREEN_SIZE, bmap) != SCREEN_SIZE ||
		Fseek(34, handle, SEEK_CUR) < 0 ||
		Fread(handle, SCREEN_SIZE, bmap + SCREEN_SIZE) != SCREEN_SIZE ||
		Fseek(34, handle, SEEK_CUR) < 0 ||
		Fread(handle, SCREEN_SIZE, bmap + 2 * SCREEN_SIZE) != SCREEN_SIZE)
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);

	info->width = 320;
	info->height = 200;
	info->planes = 12;
	info->colors = 1L << 12;
	info->indexed_color = FALSE;
	info->components = 3;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "RGB Intermediate Format");
	strcpy(info->compression, "None");
	
	RETURN_SUCCESS();
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
	int x;
	int bit;
	uint16_t *red_plane;
	uint16_t *green_plane;
	uint16_t *blue_plane;
	
	red_plane = (uint16_t *)(info->_priv_ptr + info->_priv_var);
	green_plane = red_plane + SCREEN_SIZE / 2;
	blue_plane = green_plane + SCREEN_SIZE / 2;
	x = info->width >> 4;
	info->_priv_var += x << 3;
	do
	{
		for (bit = 15; bit >= 0; bit--)
		{
			*buffer++ =
				((((red_plane[0] >> bit) & 1) << 0) |
				 (((red_plane[1] >> bit) & 1) << 1) |
				 (((red_plane[2] >> bit) & 1) << 2) |
				 (((red_plane[3] >> bit) & 1) << 3)) << 4;
			*buffer++ =
				((((green_plane[0] >> bit) & 1) << 0) |
				 (((green_plane[1] >> bit) & 1) << 1) |
				 (((green_plane[2] >> bit) & 1) << 2) |
				 (((green_plane[3] >> bit) & 1) << 3)) << 4;
			*buffer++ =
				((((blue_plane[0] >> bit) & 1) << 0) |
				 (((blue_plane[1] >> bit) & 1) << 1) |
				 (((blue_plane[2] >> bit) & 1) << 2) |
				 (((blue_plane[3] >> bit) & 1) << 3)) << 4;
		}
		red_plane += 4;
		green_plane += 4;
		blue_plane += 4;
	} while (--x > 0);

	RETURN_SUCCESS();
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
