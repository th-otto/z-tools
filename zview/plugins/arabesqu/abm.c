#include "plugin.h"
#include "zvplugin.h"
#define NF_DEBUG 0
#include "nfdebug.h"

#define VERSION		0x0201
#define NAME        "Arabesque Bitmap"
#define DATE        __DATE__ " " __TIME__
#define AUTHOR      "Thorsten Otto"

/*
 * Format	ABM   compression horizontal, 2 headers !
 *			-----------
 * +0		'ESO8'	Id ABM
 * +4		'9a'	Id format
 * +6		?	image width in pixels
 * +8		?	image height
 * +10		1   number of planes
 * +12		?	image block size    = File size - 14
 * +14		?	id_byte		
 * +15		?	id_pac		
 * +16		?	special byte	
 * +17		?	compressed data	
 *
 * +0		'ESO8'	Id ABM
 * +4		'8b'	Id format
 * +6		?	image width in pixels
 * +8		?	image height
 * +10		?	size of the first image block	; Total =
 * +12		?	size of the second image block	;   file size - 18
 * +14		?	size of the third image block	;
 * +16		?	size of the third image block	;.............
 * +18		?	id_byte		
 * +19		?	id_pac		
 * +20		?	special byte	
 * +21		?	compressed data	
 */

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("ABM\0" "PUF\0");

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


static uint32_t filesize(int16_t fhand)
{
	uint32_t flen = Fseek(0, fhand, SEEK_END);
	Fseek(0, fhand, SEEK_SET);					/* reset */
	return flen;
}


static long unpack_stad(uint8_t *dst, const uint8_t *src, long size, long pos)
{
	uint8_t id_byte;
	uint8_t pack_byte;
	uint8_t special_byte;
	uint8_t c;
	int16_t count;

	id_byte = *src++;
	pack_byte = *src++;
	special_byte = *src++;
	size -= 3;

	while (size > 0)
	{
		c = *src++;
		size--;
		if (c == id_byte)
		{
			count = *src++;
			count++;
			size--;
			while (count > 0)
			{
				dst[pos++] = pack_byte;
				count--;
			}
		} else if (c == special_byte)
		{
			c = *src++;
			count = *src++;
			count++;
			size -= 2;
			while (count > 0)
			{
				dst[pos++] = c;
				count--;
			}
		} else
		{
			dst[pos++] = c;
		}
	}
	return pos;
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
	uint16_t i;
	uint32_t file_size;
	int16_t handle;
	uint8_t *data;
	uint8_t *ptr;
	uint8_t *bmap;
	uint16_t width;
	uint16_t height;
	uint16_t format;
	uint16_t blocks;
	uint16_t sizes[4];
	uint32_t magic;
	uint16_t wdwidth;
	uint32_t screensize;
	long offset;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}

	file_size = filesize(handle);
	data = malloc(file_size);
	if (data == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((uint32_t)Fread(handle, file_size, data) != file_size)
	{
		free(data);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	ptr = data;
	magic = *((uint32_t *)ptr);
	ptr += 4;
	if (magic != 0x45534F38L)  /* 'ESO8' */
	{
		free(data);
		return FALSE;
	}
	format = *((uint16_t *)ptr);
	ptr += 2;
	width = *((uint16_t *)ptr);
	ptr += 2;
	height = *((uint16_t *)ptr);
	ptr += 2;
	nf_debugprintf(("width=%u height %u format=$%04x\n", width, height, format));

	if (format == 0x3961)
	{
		blocks = *((uint16_t *)ptr);
		ptr += 2;
		for (i = 0; i < blocks && i < 4; i++)
		{
			sizes[i] = *((uint16_t *)ptr);
			ptr += 2;
			nf_debugprintf(("size[%u] = %u\n", i, sizes[i]));
		}
	} else if (format == 0x3862)
	{
		blocks = 4;
		for (i = 0; i < blocks; i++)
		{
			sizes[i] = *((uint16_t *)ptr);
			ptr += 2;
			nf_debugprintf(("size[%u] = %u\n", i, sizes[i]));
		}
	} else if (format == 0x3861)
	{
		blocks = 1;
		sizes[0] = file_size - 10;
	} else
	{
		free(data);
		return FALSE;
	}

	wdwidth = (width + 15) & -16;
	screensize = ((uint32_t)wdwidth >> 3) * height;
	
	bmap = malloc(screensize + 256);
	if (bmap == NULL)
	{
		free(data);
		return FALSE;
	}

	strcpy(info->info, "Arabesque Bitmap");
	offset = 0;
	if (format == 0x3961)
	{
		for (i = 0; i < blocks; i++)
		{
			if (sizes[i] > 0)
			{
				offset = unpack_stad(bmap, ptr, sizes[i], offset);
				offset -= 2;
				ptr += sizes[i];
				nf_debugprintf(("offset=%lu fpos=%lu\n", offset, ptr - data));
			}
		}
		strcat(info->info, " (Type 9a)");
	} else if (format == 0x3862)
	{
		for (i = 0; i < blocks; i++)
		{
			if (sizes[i] > 0)
			{
				offset = unpack_stad(bmap, ptr, sizes[i], offset);
				offset -= 1;
				ptr += sizes[i];
			}
		}
		strcat(info->info, " (Type 8b)");
	} else if (format == 0x3861)
	{
		offset = unpack_stad(bmap, ptr, sizes[0], offset);
		strcat(info->info, " (Type 8a)");
	} 
	free(data);

	info->planes = 1;
	info->width = width;
	info->height = height;
	info->components = 1;
	info->indexed_color = FALSE;
	info->real_width = info->width;
	info->real_height = info->height;
	info->colors = 1L << 1;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;
	strcpy(info->compression, "RLEH");
	
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
	int16_t byte;
	uint16_t x;
	uint8_t *screen_buffer = info->_priv_ptr;
	int32_t i = info->_priv_var;
	uint16_t wdwidth = (info->width + 15) & -16;

	x = 0;
	do
	{
		byte = screen_buffer[i++];
		*buffer++ = (byte >> 7) & 1;
		*buffer++ = (byte >> 6) & 1;
		*buffer++ = (byte >> 5) & 1;
		*buffer++ = (byte >> 4) & 1;
		*buffer++ = (byte >> 3) & 1;
		*buffer++ = (byte >> 2) & 1;
		*buffer++ = (byte >> 1) & 1;
		*buffer++ = (byte >> 0) & 1;
		x += 8;
	} while (x < wdwidth);
	info->_priv_var = i;
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
	uint8_t *file_buffer = info->_priv_ptr;
	
	free(file_buffer);
	info->_priv_ptr = NULL;
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
