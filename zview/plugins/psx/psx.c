#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

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
	uint32_t len;
	uint32_t flags;
};
struct clut_header {
	uint32_t len;
	uint16_t x;
	uint16_t y;
	uint16_t colors;
	uint16_t palette_count;
};
struct bitmap_header {
	uint32_t len;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
};


static uint16_t swap16(uint16_t w)
{
	return (w >> 8) | (w << 8);
}


static uint32_t swap32(uint32_t l)
{
	return ((l >> 24) & 0xffL) | ((l << 8) & 0xff0000L) | ((l >> 8) & 0xff00L) | ((l << 24) & 0xff000000UL);
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
	int16_t handle;
	uint8_t *bmap;
	size_t bytes_per_row;
	uint16_t palette[256];
	struct file_header header;
	struct bitmap_header bitmap;
	struct clut_header clut;
	int bpp;
	int has_clut;
	size_t image_size;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	header.len = swap32(header.len);
	header.flags = swap32(header.flags);
	if (header.len != 16)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	
	bpp = (int)(header.flags & 7);
	has_clut = (int)(header.flags & 8);
	if (bpp == 0 || bpp == 1)
	{
		if (!has_clut)
		{
			Fclose(handle);
			RETURN_ERROR(EC_ColorMapLength);
		}
		if (Fread(handle, sizeof(clut), &clut) != sizeof(clut))
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		clut.len = swap32(clut.len) - sizeof(clut);
		clut.palette_count = swap16(clut.palette_count);
		clut.colors = swap16(clut.colors);
		if ((bpp == 0 && clut.colors != 16) ||
			(bpp == 1 && clut.colors != 256))
		{
			Fclose(handle);
			RETURN_ERROR(EC_ColorMapLength);
		}
	} else
	{
		clut.len = 0;
		clut.palette_count = 0;
		clut.colors = 0;
	}
	if (has_clut)
	{
		if ((size_t)Fread(handle, (size_t)clut.colors * 2, palette) != (size_t)clut.colors * 2)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		if (clut.palette_count > 1)
			Fseek(clut.len - 2 * (size_t)clut.colors, handle, SEEK_CUR);
	}
	if ((size_t)Fread(handle, sizeof(bitmap), &bitmap) != sizeof(bitmap))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	bitmap.len = swap32(bitmap.len) - sizeof(bitmap);
	bitmap.width = swap16(bitmap.width);
	bitmap.height = swap16(bitmap.height);
	
	switch (bpp)
	{
	case 3:
		/* truecolor */
		info->planes = 24;
		info->indexed_color = FALSE;
		bitmap.width = (bitmap.width * 2) / 3;
		bytes_per_row = (size_t)bitmap.width * 3;
		break;
	case 2:
		/* hicolor */
		info->planes = 15;
		info->indexed_color = FALSE;
		bytes_per_row = (size_t)bitmap.width * 2;
		break;
	case 1:
		/* 256 colors */
		info->planes = 8;
		info->indexed_color = TRUE;
		bitmap.width = bitmap.width * 2;
		bytes_per_row = (size_t)bitmap.width;
		break;
	case 0:
		/* 16 colors */
		info->planes = 4;
		info->indexed_color = TRUE;
		bitmap.width = bitmap.width * 4;
		bytes_per_row = (size_t)bitmap.width / 2;
		break;
	default:
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}
	
	image_size = bytes_per_row * bitmap.height;
	
	if (image_size > bitmap.len)
	{
		Fclose(handle);
		RETURN_ERROR(EC_BitmapLength);
	}

	bmap = malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, image_size, bmap) != image_size)
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);

	info->width = bitmap.width;
	info->height = bitmap.height;
	info->components = 3;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;
	info->_priv_var_more = bytes_per_row;

	strcpy(info->info, "PlayStation");
	strcpy(info->compression, "None");

	if (info->indexed_color)
	{
		int i;
		uint16_t color;
		
		for (i = 0; i < (int)info->colors; i++)
		{
			color = swap16(palette[i]);
			info->palette[i].red = (color & 0x1f) << 3;
			info->palette[i].green = ((color >> 5) & 0x1f) << 3;
			info->palette[i].blue = ((color >> 10) & 0x1f) << 3;
		}
	}
	
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
	uint8_t *bmap = info->_priv_ptr;
	size_t src = info->_priv_var;
	size_t bytes_per_row = info->_priv_var_more;

	switch (info->planes)
	{
	case 4:
		{
			unsigned int x;
			
			for (x = 0; x < info->width; x += 2)
			{
				uint8_t byte = bmap[src++];
				*buffer++ = byte & 15;
				*buffer++ = byte >> 4;
			}
		}
		break;
	case 15:
	case 16:
		{
			unsigned int x;
			
			for (x = 0; x < info->width; x++)
			{
				uint16_t color;
				color = bmap[src++];
				color |= bmap[src++] << 8;
				*buffer++ = (color & 0x1f) << 3;
				*buffer++ = ((color >> 5) & 0x1f) << 3;
				*buffer++ = ((color >> 10) & 0x1f) << 3;
			}
		}
		break;
	case 8:
	case 24:
		memcpy(buffer, &bmap[src], bytes_per_row);
		break;
	}
	info->_priv_var += bytes_per_row;

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
