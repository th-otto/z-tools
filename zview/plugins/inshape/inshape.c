#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
InShape     *.IIM

8 bytes     file ID, "IS_IMAGE"
1 word      type [0 = 1 plane, 1 = 8 planes gray, 4 = 24 planes, 5 = 32 planes]
1 word      bit planes [inconsistent as some older files contain '72']
1 word      image width in pixels
1 word      image height in pixels
--------
16 bytes    total for header

??          Image data:
   1 plane is monochrome, width assumed to be rounded up to the nearest byte
   8 planes is 256 level gray scale, where 0 = black and 255 = white
   24 planes is true-color, RGB format, 3 bytes per pixel in R, G, B order
   32 planes is true-color, ARGB format 4 bytes per pixel in A, R, G, B order

From the InShape website:
  The IIM format has several variants:
    - Duochrome.
    - 256 Grayscale.
    - 24 bit (16 million colors).
    - 32 bit (24 bit alpha channel for mask).
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
	case OPTION_EXTENSIONS:
		return (long) (EXTENSIONS);

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
#ifdef MISC_INFO
	case INFO_MISC:
		return (long)MISC_INFO;
#endif
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif


struct file_header {
	char magic[8];
	uint16_t type;
	uint16_t planes;
	uint16_t width;
	uint16_t height;
};

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
	size_t image_size;
	size_t bytes_per_row;
	uint8_t *bmap;
	int16_t handle;
	struct file_header header;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	if (Fread(handle, sizeof(header), &header) != sizeof(header) ||
		memcmp(header.magic, "IS_IMAGE", 8) != 0)
	{
		Fclose(handle);
		return FALSE;
	}

	switch (header.type)
	{
	case 0:
		bytes_per_row = ((size_t)header.width + 7) >> 3;
		info->planes = 1;
		info->indexed_color = FALSE;
		info->components = 1;
		break;
	case 1:
		bytes_per_row = header.width;
		info->planes = 8;
		info->indexed_color = TRUE;
		info->components = 3;
		break;
	case 2:
		bytes_per_row = ((size_t)header.width + 1) >> 1;
		info->planes = 4;
		info->indexed_color = TRUE;
		info->components = 3;
		Fclose(handle);
		return FALSE;
	case 3:
		bytes_per_row = header.width;
		info->planes = 8;
		info->indexed_color = TRUE;
		info->components = 3;
		Fclose(handle);
		return FALSE;
	case 4:
		bytes_per_row = (size_t)header.width * 3;
		info->planes = 24;
		info->indexed_color = FALSE;
		info->components = 3;
		break;
	case 5:
		bytes_per_row = (size_t)header.width * 4;
		info->planes = 32;
		info->indexed_color = FALSE;
		info->components = 3;
		break;
	default:
		Fclose(handle);
		return FALSE;
	}

	image_size = bytes_per_row * header.height;
	bmap = malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, image_size, bmap) != image_size)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	info->width = header.width;
	info->height = header.height;
	info->colors = 1L << (info->planes > 24 ? 24 : info->planes);
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;

	strcpy(info->info, "InShape");
	strcpy(info->compression, "None");

	if (info->indexed_color)
	{
		int i;
		
		for (i = 0; i < 256; i++)
		{
			info->palette[i].red =
			info->palette[i].green =
			info->palette[i].blue = i;
		}
	}
	
	info->_priv_ptr = bmap;
	info->_priv_var = 0;				/* y offset */

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
	uint8_t *bmap = info->_priv_ptr;

	switch (info->planes)
	{
	case 1:
		{
			int16_t byte;
			int x;
			
			x = (info->width + 7) >> 3;
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
		}
		break;
	case 4:
		{
			uint8_t byte;
			int x;
			
			x = (info->width + 1) >> 1;
			do
			{
				byte = bmap[pos++];
				*buffer++ = byte >> 4;
				*buffer++ = byte & 15;
			} while (--x > 0);
		}
		break;
	case 8:
		memcpy(buffer, &bmap[pos], info->width);
		pos += info->width;
		break;
	case 24:
		{
			size_t line_size = (size_t)info->width * 3;
			memcpy(buffer, &bmap[pos], line_size);
			pos += line_size;
		}
		break;
	case 32:
		{
			int x;
			
			x = info->width;
			do
			{
				pos++;
				*buffer++ = bmap[pos++];
				*buffer++ = bmap[pos++];
				*buffer++ = bmap[pos++];
			} while (--x > 0);
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


boolean __CDECL encoder_init(const char *name, IMGINFO info)
{
	struct file_header header;
	int16_t handle;

	if ((handle = (int16_t) Fcreate(name, 0)) < 0)
	{
		return FALSE;
	}
	memcpy(header.magic, "IS_IMAGE", 8);
	header.type = 4;
	header.planes = 24;
	header.width = info->width;
	header.height = info->height;
	if ((size_t)Fwrite(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		return FALSE;
	}

	info->planes = 24;
	info->components = 3;
	info->colors = 1L << 24;
	info->orientation = UP_TO_DOWN;
	info->memory_alloc = TT_RAM;
	info->indexed_color = FALSE;
	info->page = 1;

	info->_priv_var = handle;
	info->_priv_ptr = NULL;

	return TRUE;
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	int16_t handle;
	size_t line_size;

	handle = (int16_t)info->_priv_var;
	line_size = (size_t)info->width * 3;
	if ((size_t)Fwrite(handle, line_size, buffer) != line_size)
	{
		return FALSE;
	}
	return TRUE;
}


void __CDECL encoder_quit(IMGINFO info)
{
	int16_t handle = (int16_t)info->_priv_var;

	if (handle > 0)
	{
		Fclose(handle);
		info->_priv_var = 0;
	}
	info->_priv_ptr = 0;
}
