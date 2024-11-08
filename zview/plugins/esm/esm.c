#define	VERSION	     0x0200
#define NAME        "TmS Cranach (Paint/Studio)"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"
#define NF_DEBUG 0
#include "nfdebug.h"

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
	case OPTION_EXTENSIONS:
		return (long) ("ESM\0");

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


#define ESM_MAGIC 0x544D5300L /* 'TMS\0' */

typedef struct ESM_Header {
	uint32_t magic;
	uint16_t head_size;
	uint16_t width;
	uint16_t height;
	uint16_t planes;
	uint16_t components;
	uint16_t red_depth;
	uint16_t green_depth;
	uint16_t blue_depth;
	uint16_t black_depth;
	uint16_t version;
	uint16_t xdpi;
	uint16_t ydpi;
	uint16_t file_height;
	uint16_t first_line;
	uint16_t last_line;
	uint16_t mask;
	unsigned char red_tab[256];
	unsigned char green_tab[256];
	unsigned char blue_tab[256];
	int32_t reserved[2];
} ESM_Header;


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
	uint32_t datasize;
	uint32_t line_size;
	int i;
	uint8_t *bmap;
	int16_t handle;
	ESM_Header header;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		return FALSE;
	}
	if (header.magic != ESM_MAGIC)
	{
		Fclose(handle);
		return FALSE;
	}
	switch (header.planes)
	{
	case 24:
		info->indexed_color = FALSE;
		line_size = (uint32_t)header.width * 3;
		break;
	case 8:
		info->indexed_color = TRUE;
		line_size = header.width;
		break;
	case 1:
		info->indexed_color = FALSE;
		line_size = ((int32_t)header.width + 7) / 8;
		break;
	default:
		Fclose(handle);
		return FALSE;
	}
	datasize = line_size * header.height;
	bmap = malloc(datasize + 256);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	Fseek(header.head_size, handle, SEEK_SET);
	if ((uint32_t)Fread(handle, datasize, bmap) != datasize)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);
	info->planes = header.planes;
	info->components = info->planes == 1 ? 1 : 3;
	info->width = header.width;
	info->height = header.height;
	info->colors = 1L << MIN(info->planes, 24);
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	strcpy(info->info, "TmS Cranach (Paint/Studio)");
	strcpy(info->compression, "None");
	if (info->indexed_color)
	{
		for (i = 0; i < 256; i++)
		{
			info->palette[i].red = header.red_tab[i];
			info->palette[i].green = header.green_tab[i];
			info->palette[i].blue = header.blue_tab[i];
		}
	}

	info->_priv_ptr = bmap;
	info->_priv_var = 0;				/* y offset */
	info->_priv_ptr_more = (void *)line_size;

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
	uint16_t x;
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	uint32_t pos = info->_priv_var;
	uint32_t line_size = (uint32_t)info->_priv_ptr_more;
	uint16_t w;
	uint8_t b;

	if (info->planes == 1)
	{
		for (w = 0, x = 0; w < line_size; w++)
		{
			b = bmap[pos++];
			buffer[x++] = b & 0x80 ? 1 : 0;
			buffer[x++] = b & 0x40 ? 1 : 0;
			buffer[x++] = b & 0x20 ? 1 : 0;
			buffer[x++] = b & 0x10 ? 1 : 0;
			buffer[x++] = b & 0x08 ? 1 : 0;
			buffer[x++] = b & 0x04 ? 1 : 0;
			buffer[x++] = b & 0x02 ? 1 : 0;
			buffer[x++] = b & 0x01 ? 1 : 0;
		}
	} else
	{
		memcpy(buffer, &bmap[pos], line_size);
		pos += line_size;
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
	int i;
	ESM_Header header;
	int16_t handle;
	uint32_t line_size;

	if ((handle = (int16_t) Fcreate(name, 0)) < 0)
	{
		return FALSE;
	}
	header.magic = ESM_MAGIC;
	header.head_size = sizeof(header);
	header.width = info->width;
	header.height = info->height;
	header.planes = 24;
	header.components = 3;
	header.red_depth = 8;
	header.green_depth = 8;
	header.blue_depth = 8;
	header.black_depth = 0;
	header.version = 4;
	header.xdpi = 100;
	header.ydpi = 100;
	header.file_height = info->height;
	header.first_line = 0;
	header.last_line = info->height - 1;
	header.mask = 0;
	header.reserved[0] = 0;
	header.reserved[1] = 0;
	for (i = 0; i < 256; i++)
	{
		header.blue_tab[i] = i;
		header.green_tab[i] = i;
		header.red_tab[i] = i;
	}
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

	line_size = (uint32_t)info->width * 3;

	info->_priv_var = handle;
	info->_priv_ptr_more = (void *)line_size;

	return TRUE;
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	int16_t handle = (int16_t)info->_priv_var;
	uint32_t line_size = (uint32_t)info->_priv_ptr_more;

	if ((uint32_t)Fwrite(handle, line_size, buffer) != line_size)
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
}
