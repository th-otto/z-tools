#define	VERSION	    0x200
#define NAME        "PaintPro ST/PlusPaint ST"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
PaintPro ST/PlusPaint ST    *.PIC

These drawing programs allow double high images.

               standard    double
ST low:         320x200   320x400
ST medium:      640x200   640x400
ST high:        640x400   640x800
Size in bytes:    32034     64034

The 32034 size files are identical to the original uncompressed Degas format.

The 64034 size files are essentially expanded Degas files, the only difference
is the bitmap is twice the size.

See also DEGAS file format
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("PIC\0");

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

typedef struct
{										/* 34 bytes */
	uint16_t resolution;
	uint16_t palette[16];				/* xbios format, xbios order */
} HEADER;

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
	HEADER header;
	size_t file_size;
	
	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	file_size -= sizeof(header);
	if ((file_size != SCREEN_SIZE && file_size != 2 * SCREEN_SIZE) ||
		Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		return FALSE;
	}

	switch (header.resolution)
	{
	case 0:
		info->planes = 4;
		info->components = 3;
		info->width = 320;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 1:
		info->planes = 2;
		info->components = 3;
		info->width = 640;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 2:
		info->planes = 1;
		info->components = 1;
		info->width = 640;
		info->height = 400;
		info->indexed_color = FALSE;
		break;
	default:
		Fclose(handle);
		return FALSE;
	}

	strcpy(info->info, "PaintPro ST/PlusPaint ST");
	if (file_size == 2 * SCREEN_SIZE)
	{
		info->height *= 2;
		strcat(info->info, " (Double)");
	} else
	{
		strcat(info->info, " (Standard)");
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
	
	if (info->planes == 1 && (header.palette[0] & 1) == 0)
	{
		size_t i;
		
		for (i = 0; i < file_size; i++)
			bmap[i] = ~bmap[i];
	}

	info->colors = 1L << info->planes;

	if (info->indexed_color)
	{	
		int i;

		for (i = 0; i < (int)info->colors; i++)
		{
			info->palette[i].red = (((header.palette[i] >> 7) & 0x0e) + ((header.palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((header.palette[i] >> 3) & 0x0e) + ((header.palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((header.palette[i] << 1) & 0x0e) + ((header.palette[i] >> 3) & 0x01)) * 17;
		}
	}

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
	switch (info->planes)
	{
	case 1:
		{
			size_t pos = info->_priv_var;
			uint8_t *bmap = info->_priv_ptr;
			int16_t byte;
			int x;
			
			x = info->width >> 3;
			do
			{
				byte = bmap[pos++];
				*buffer++ = (byte >> 7) & 1;
				*buffer++ = (byte >> 6) & 1;
				*buffer++ = (byte >> 5) & 1;
				*buffer++ = (byte >> 4) & 1;
				*buffer++ = (byte >> 3) & 1;
				*buffer++ = (byte >> 2) & 1;
				*buffer++ = (byte >> 1) & 1;
				*buffer++ = (byte >> 0) & 1;
			} while (--x > 0);
			info->_priv_var = pos;
		}
		break;
	case 2:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t *ptr;
			
			ptr = (uint16_t *)(info->_priv_ptr + info->_priv_var);
			info->_priv_var += info->width >> 2;
			x = info->width >> 4;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2);
					plane0 <<= 1;
					plane1 <<= 1;
				}
			} while (--x > 0);
		}
		break;
	case 4:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t *ptr;
			
			ptr = (uint16_t *)(info->_priv_ptr + info->_priv_var);
			info->_priv_var += info->width >> 1;
			x = info->width >> 4;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				plane2 = *ptr++;
				plane3 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4) |
						((plane3 >> 12) & 8);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
				}
			} while (--x > 0);
		}
		break;
	}
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
