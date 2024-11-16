#include "plugin.h"
#include "zvplugin.h"

#define VERSION		0x0201
#define NAME        "Degas Extended, FuckPaint"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

/*
FuckPaint       *.PI4, *.PI9

FuckPaint was an early Atari Falcon drawing program. The format is still
popular because its simplicity. The PI4 extension is used for 320x240 pictures,
the PI9 for 320x200 pictures. Except for the extension the two formats are
exactly the same.

256 longs      palette 256 colors, in Falcon format, R, G, 0, B
38400 words    image data, 8 interleaved bitplanes
-----------
77824 bytes    total
*/


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("PI4\0" "PI5\0" "PI6\0" "PI7\0" "PI9\0");

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
boolean __CDECL reader_init( const char *name, IMGINFO info)
{
	int16_t handle;
	uint8_t *bmap;
	uint32_t file_size;
	uint32_t datasize;
	uint16_t color;
	int degas;
	int i, colors;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (file_size == 154114L)
	{
		/* PI4 */
		info->width = 320;
		info->height = 480;
		info->planes = 8;
		degas = TRUE;
	} else if (file_size == 153634L)
	{
		/* PI5 */
		info->width = 640;
		info->height = 480;
		info->planes = 4;
		degas = TRUE;
	} else if (file_size == 153606L)
	{
		/* PI6 */
		info->width = 1280;
		info->height = 960;
		info->planes = 1;
		degas = TRUE;
	} else if (file_size == 77824L)
	{
		/* PI9 */
		info->width = 320;
		info->height = 240;
		info->planes = 8;
		degas = FALSE;
	} else if (file_size == 308224L)
	{
		/* PI7 */
		info->width = 640;
		info->height = 480;
		info->planes = 8;
		degas = FALSE;
	} else if (file_size == 65024L)
	{
		/* PI9 */
		info->width = 320;
		info->height = 200;
		info->planes = 8;
		degas = FALSE;
	} else
	{
		Fclose(handle);
		return FALSE;
	}
	datasize = (((size_t)info->width * info->planes) / 8) * info->height;
	
	bmap = malloc(datasize);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}

	colors = 1 << info->planes;
	if (degas)
	{
		int16_t res;
		
		Fread(handle, sizeof(res), &res);
		for (i = 0; i < colors; i++)
		{
			Fread(handle, sizeof(color), &color);
			info->palette[i].red = ((color >> 8) & 0xF) * 17;
			info->palette[i].green = ((color >> 4) & 0x0f) * 17;
			info->palette[i].blue = (color & 0xf) * 17;
		}
		strcpy(info->info, "Degas Extended");
	} else
	{
		char dummy;
		
		for (i = 0; i < colors; i++)
		{
			Fread(handle, 1, &info->palette[i].red);
			Fread(handle, 1, &info->palette[i].green);
			Fread(handle, 1, &dummy);
			Fread(handle, 1, &info->palette[i].blue);
		}
		strcpy(info->info, "FuckPaint");
	}
	
	if ((size_t)Fread(handle, datasize, bmap) != datasize)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	info->indexed_color = info->planes != 1;
	info->colors = colors;
	info->components = info->planes == 1 ? 1 : 3;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->compression, "None");

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
	int x;
	int32_t offset;
	
	offset = info->_priv_var;
	if (info->planes == 1)
	{
		int16_t p0;
		uint8_t *bmap = (uint8_t *)info->_priv_ptr + offset;

		x = info->width >> 3;
		info->_priv_var += x;
		do
		{								/* 1-bit mono v1.00 */
			p0 = *bmap++;
			*buffer++ = (p0 >> 7) & 1;
			*buffer++ = (p0 >> 6) & 1;
			*buffer++ = (p0 >> 5) & 1;
			*buffer++ = (p0 >> 4) & 1;
			*buffer++ = (p0 >> 3) & 1;
			*buffer++ = (p0 >> 2) & 1;
			*buffer++ = (p0 >> 1) & 1;
			*buffer++ = (p0 >> 0) & 1;
		} while (--x > 0);
	} else
	{
		int16_t ndx;
		int pln;
		int bit;
		uint16_t *xmap = (uint16_t *)info->_priv_ptr + offset;	/* as word array */

		x = info->width >> 4;
		info->_priv_var += x * info->planes;
		do
		{								/* 1-bit to 8-bit atari st word interleaved bitmap v1.01 */
			for (bit = 15; bit >= 0; bit--)
			{
				ndx = 0;
				for (pln = 0; pln < info->planes; pln++)
				{
					if ((xmap[pln] >> bit) & 1)
					{
						ndx |= 1 << pln;
					}
				}
				*buffer++ = ndx;
			}
			xmap += info->planes;	/* next plane */
		} while (--x > 0);		/* next x */
	}

	return TRUE;
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
void __CDECL reader_get_txt( IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
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
	info->_priv_ptr = NULL;
}
