#include "plugin.h"
#include "zvplugin.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"

/*
See https://en.wikipedia.org/wiki/Silicon_Graphics_Image
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
	uint16_t magic;
	uint8_t compression;
	uint8_t bytes_per_channel;
	uint16_t dimension;
	uint16_t width;
	uint16_t height;
	uint16_t components;
	uint32_t minval;
	uint32_t maxval;
	uint32_t reserved1;
	char imagename[80];
	uint32_t colormap_id;
	char reserved2[404];
};


static void decode_sgi(uint8_t *dst, uint8_t *src)
{
	uint8_t c;
	int count;
	
	for (;;)
	{
		c = *src++;
		count = c & 127;
		if (count == 0)
			break;
		if (c & 0x80)
		{
			while (--count >= 0)
				*dst++ = *src++;
		} else
		{
			c = *src++;
			while (--count >= 0)
				*dst++ = c;
		}
	}
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
	size_t bytes_per_row;
	size_t image_size;
	struct file_header header;
	
	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fopen);
	}
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (header.magic != 0x1da)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}	
	if (header.colormap_id != 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_ColorMapType);
	}
	/* TODO: header.bytes_per_channel = 2 */
	if (header.bytes_per_channel != 1 /* && header.bytes_per_channel != 2 */)
	{
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}

	if (header.components == 1)
	{
		int i;

		/*
		 * TODO: handle colormap_id == 2
		 */		
		info->planes = 8;
		info->colors = 1L << 8;
		info->components = 1;
		info->indexed_color = FALSE;
		for (i = 0; i < 256; i++)
		{
			info->palette[i].red =
			info->palette[i].green =
			info->palette[i].blue = i;
		}
	} else if (header.components == 3)
	{
		info->planes = 24;
		info->colors = 1L << 24;
		info->components = 3;
		info->indexed_color = FALSE;
	} else if (header.components == 4)
	{
		info->planes = 32;
		info->colors = 1L << 24;
		info->components = 3;
		info->indexed_color = FALSE;
	} else
	{
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}
	
	bytes_per_row = (size_t)header.bytes_per_channel * header.width;
	image_size = bytes_per_row * header.components * header.height;
	bmap = (void *)Malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}

	if (header.compression == 0)
	{
		if ((size_t)Fread(handle, image_size, bmap) != image_size)
		{
			Mfree(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		strcpy(info->compression, "None");
	} else if (header.compression == 1)
	{
		uint32_t *offset_table;
		uint32_t *length_table;
		uint8_t *temp;
		size_t i;
		size_t num_offsets = (size_t)header.height * header.components;
		size_t table_size = num_offsets * sizeof(*offset_table);
		size_t tempsize = bytes_per_row + 256;
		
		offset_table = (void *)Malloc(table_size * 2 + tempsize);
		if (offset_table == NULL)
		{
			Mfree(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		length_table = offset_table + num_offsets;
		temp = (uint8_t *)(length_table + num_offsets);
		if ((size_t)Fread(handle, table_size * 2, offset_table) != table_size * 2)
		{
			Mfree(offset_table);
			Mfree(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		for (i = 0; i < num_offsets; i++)
		{
			size_t offset = offset_table[i];
			size_t length = length_table[i];
			
			Fseek(offset, handle, SEEK_SET);
			if (length > tempsize || (size_t)Fread(handle, length, temp) != length)
			{
				RETURN_ERROR(EC_Fread);
			}
			decode_sgi(&bmap[i * header.width], temp);
		}
		Mfree(offset_table);
		strcpy(info->compression, "RLE");
	} else
	{
		Mfree(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_CompType);
	}
	
	Fclose(handle);
	
	info->width = header.width;
	info->height = header.height;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;

	strcpy(info->info, "Silicon Graphics Image");

	/* TODO: make imagename available as text */

	info->_priv_ptr = bmap;
	info->_priv_var = 0;		/* y offset */

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
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	size_t bytes_per_row;
	
	switch (info->planes)
	{
	case 8:
		/* 8bit grayscale or colormap */
		bytes_per_row = info->width;
		info->_priv_var += bytes_per_row;
		bmap += bytes_per_row * info->height;
		bmap -= info->_priv_var;
		memcpy(buffer, bmap, bytes_per_row);
		break;

	case 24:
		/* RGB */
		/* channels are stored in separate planes, upside-down */
		{
			size_t planesize;
			int x;
			
			bytes_per_row = info->width;
			info->_priv_var += bytes_per_row;
			planesize = bytes_per_row * info->height;
			bmap += planesize;
			bmap -= info->_priv_var;
			for (x = info->width; --x >= 0; )
			{
				*buffer++ = bmap[0];
				*buffer++ = bmap[planesize];
				*buffer++ = bmap[planesize * 2];
				bmap++;
			}
		}
		break;

	case 32:
		/* RGBA */
		/* channels are stored in separate planes, upside-down */
		{
			size_t planesize;
			int x;
			
			bytes_per_row = info->width;
			info->_priv_var += bytes_per_row;
			planesize = bytes_per_row * info->height;
			bmap += planesize;
			bmap -= info->_priv_var;
			for (x = info->width; --x >= 0; )
			{
				*buffer++ = bmap[0];
				*buffer++ = bmap[planesize];
				*buffer++ = bmap[planesize * 2];
				bmap++;
			}
		}
		break;
	}
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
