#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

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
	uint32_t magic;
	uint32_t reserved;
	uint32_t width;
	uint32_t height;
	uint32_t planes;
	uint32_t image_size;
	uint32_t extra_len;
	uint8_t id_byte;
	uint8_t pack_byte;
	uint8_t special_byte;
	uint8_t padding;
};

/*
 * Same as STAD decompression
 */
static int decode_mgf(uint8_t *src, uint8_t *dst, size_t dstlen, uint8_t id_byte, uint8_t pack_byte, uint8_t special_byte)
{
	uint8_t c;
	uint16_t count;
	
	do
	{
		c = *src++;
		if (c == id_byte)
		{
			count = *src++;
			count++;
			if (count > dstlen)
				count = dstlen;
			dstlen -= count;
			while (count > 0)
			{
				*dst++ = pack_byte;
				count--;
			}
		} else if (c == special_byte)
		{
			c = *src++;
			count = *src++;
			count++;
			if (count > dstlen)
				count = dstlen;
			dstlen -= count;
			while (count > 0)
			{
				*dst++ = c;
				count--;
			}
		} else
		{
			*dst++ = c;
			dstlen--;
		}
	} while (dstlen > 0);
	return dstlen == 0;
}


static void flip_orient(uint8_t *src, uint8_t *dst, uint16_t width, uint16_t height, size_t image_size)
{
	uint8_t *end = src + image_size;
	size_t dstpos = 0;
	uint16_t x = 0;
	uint16_t y = 0;
	size_t bytes_per_line;
	
	bytes_per_line = ((width + 15) >> 4) * 2;
	do
	{
		dst[dstpos] = *src++;
		dstpos += bytes_per_line;
		if (++y == height)
		{
			y = 0;
			++x;
			dstpos = x;
		}
	} while (src < end);
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
	uint8_t *bmap;
	int16_t handle;
	struct file_header header;
	size_t file_size;
	size_t image_size;
	int compressed;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}

	if (header.magic == 0x4D474630L) /* 'MGF0' */
	{
		strcpy(info->compression, "None");
		compressed = 0;
	} else if (header.magic == 0x4D474631L) /* 'MGF1' */
	{
		strcpy(info->compression, "RLEH");
		compressed = 1;
	} else if (header.magic == 0x4D474632L) /* 'MGF2' */
	{
		strcpy(info->compression, "RLEV");
		compressed = 2;
	} else
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	if (header.planes != 1)
	{
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}
	if (header.width > 65535L || header.height > 65535L)
	{
		Fclose(handle);
		RETURN_ERROR(EC_InvalidHeader);
	}

	file_size -= sizeof(header);
	if (header.extra_len != 0)
	{
		Fseek(header.extra_len, handle, SEEK_CUR);
		file_size -= header.extra_len;
	}
	
	info->planes = 1;
	info->colors = 1L << 1;
	info->components = 1;
	info->indexed_color = FALSE;
	info->width = header.width;
	info->height = header.height;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;

	strcpy(info->info, "GemView MGF");

	image_size = ((header.width + 15) >> 4) * 2 * header.height;

	if (compressed)
	{
		uint8_t *temp;
		size_t size;
		
		size = file_size;
		if (compressed == 2 && image_size > file_size)
			size = image_size;
		bmap = (void *)Malloc(image_size + size + 256);
		if (bmap == NULL)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		temp = bmap + image_size;
		if ((size_t)Fread(handle, file_size, temp) != file_size)
		{
			Mfree(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		if (decode_mgf(temp, bmap, image_size, header.id_byte, header.pack_byte, header.special_byte) == FALSE)
		{
			Mfree(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_DecompError);
		}
		if (compressed == 2)
		{
			flip_orient(bmap, temp, info->width, info->height, image_size);
			memcpy(bmap, temp, image_size);
		}
	} else
	{
		bmap = (void *)Malloc(image_size);
		if (bmap == NULL)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		if ((size_t)Fread(handle, image_size, bmap) != image_size)
		{
			Mfree(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
	}
	Fclose(handle);

	info->_priv_ptr = bmap;
	info->_priv_var = 0;		/* y offset */

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
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	int16_t byte;
	int x;
	
	bmap += info->_priv_var;
	x = ((info->width + 15) >> 4) * 2;
	info->_priv_var += x;
	
	do
	{
		byte = *bmap++;
		*buffer++ = (byte >> 7) & 1;
		*buffer++ = (byte >> 6) & 1;
		*buffer++ = (byte >> 5) & 1;
		*buffer++ = (byte >> 4) & 1;
		*buffer++ = (byte >> 3) & 1;
		*buffer++ = (byte >> 2) & 1;
		*buffer++ = (byte >> 1) & 1;
		*buffer++ = (byte >> 0) & 1;
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
	Mfree(info->_priv_ptr);
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
