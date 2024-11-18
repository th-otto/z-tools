#define	VERSION	     0x106
#define NAME        "Falcon True Color"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
Falcon True Color    *.FTC *.XGA (Falcon high-color)

The file extentions are interchangeable. There's no header allowing easy
identification. The only way to determine the resolution is the file size.

Files encountered:

Resolution  File size in bytes
----------  ------------------
320x200	    128000
320x240     153600
320x480     307200
384x240     184320
384x480     368640
384x576     442368
640x480     614400
768x480     737280
768x576     884736
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("FTC\0" "XGA\0");

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
boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	size_t file_size;
	int16_t handle;
	uint8_t *bmap;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (file_size == 128000L)
	{
		info->width = 320;
		info->height = 200;
	} else if (file_size == 153600L)
	{
		info->width = 320;
		info->height = 240;
	} else if (file_size == 307200L)
	{
		info->width = 320;
		info->height = 480;
	} else if (file_size == 184320L)
	{
		info->width = 384;
		info->height = 240;
	} else if (file_size == 196608L)
	{
		info->width = 384;
		info->height = 256;
	} else if (file_size == 368640L)
	{
		info->width = 384;
		info->height = 480;
	} else if (file_size == 442368L)
	{
		info->width = 384;
		info->height = 576;
	} else if (file_size == 614400L)
	{
		info->width = 640;
		info->height = 480;
	} else if (file_size == 737280L)
	{
		info->width = 768;
		info->height = 480;
	} else if (file_size == 884736L)
	{
		info->width = 768;
		info->height = 576;
	} else if (file_size == 475392L)
	{
		info->width = 384;
		info->height = 619;
	} else
	{
		Fclose(handle);
		return FALSE;
	}
		
	bmap = malloc(file_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, file_size, bmap) != file_size)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}

	Fclose(handle);
	
	info->planes = 16;
	info->components = 3;
	info->indexed_color = FALSE;
	info->colors = 1L << 16;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "Falcon True Color");
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
	int16_t x;
	const uint16_t *screen16;
	uint16_t color;
	
	screen16 = (const uint16_t *)info->_priv_ptr + info->_priv_var;
	for (x = 0; x < info->width; x++)
	{
		color = *screen16++;
		*buffer++ = ((color >> 11) & 0x1f) << 3;
		*buffer++ = ((color >> 5) & 0x3f) << 2;
		*buffer++ = ((color) & 0x1f) << 3;
	}
	info->_priv_var += info->width;
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
