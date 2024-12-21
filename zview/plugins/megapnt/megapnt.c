#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
MegaPaint    *.BLD

1 word     image width in pixels [if negative, file is compressed]
1 word     image height in pixels
-------
4 bytes    total for header

??         monochrome image data [max. 1440x1856]
           uncompressed size = ((width+7)/8)*height

Compression scheme:

For a given control byte 'n':
  n = 0         : use the next byte + 1 times and repeat 0
  n = 255       : use the next byte + 1 times and repeat 255
  n>0 and n<255 : output n (literal)
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
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
	int16_t width;
	int16_t height;
};


static void decompress_bld(uint8_t *src, uint8_t *dst, size_t dstlen)
{
	uint8_t n;
	int16_t c;
	uint8_t *end = dst + dstlen;

	do
	{
		n = *src++;
		if (n == 0)
		{
			c = *src++ + 1;
			while (--c >= 0)
				*dst++ = 0;
		} else if (n == 0xff)
		{
			c = *src++ + 1;
			while (--c >= 0)
				*dst++ = 0xff;
		} else
		{
			*dst++ = n;
		}
	} while (dst < end);
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
	size_t image_size;
	size_t bytes_per_row;
	int16_t handle;
	uint8_t *bmap;
	int16_t compressed;
	struct file_header header;
	
	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (file_size <= sizeof(header) ||
		Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		return FALSE;
	}
	if (header.width & 0x8000)
	{
		compressed = TRUE;
		header.width = -header.width;
	} else
	{
		compressed = FALSE;
	}
	header.width++;
	header.height++;
	bytes_per_row = ((size_t)header.width + 7) >> 3;
	image_size = bytes_per_row * header.height;
	
	bmap = malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if (compressed)
	{
		uint8_t *temp;

		temp = malloc(file_size);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		file_size -= sizeof(header);
		if ((size_t)Fread(handle, file_size, temp) != file_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		decompress_bld(temp, bmap, image_size);
		free(temp);
		strcpy(info->compression, "RLE");
	} else
	{
		if ((size_t)Fread(handle, image_size, bmap) != image_size)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		strcpy(info->compression, "None");
	}
	
	Fclose(handle);
	
	info->planes = 1;
	info->width = header.width;
	info->height = header.height;
	info->components = 1;
	info->indexed_color = FALSE;
	info->colors = 1L << 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "MegaPaint (Raster)");
	
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
