#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"
#define NF_DEBUG 0
#include "nfdebug.h"

/*
COKE format    *.TG1

12 bytes    file ID ["COKE format."]
1 word      image width in pixels
1 word      image height in pixels
1 word      offset to image data from start of file [value of 18 = v1.00]
--------
18 bytes    total for header

??              Image data:
   width*height*2 bytes Falcon true color data,  word RRRRRGGGGGGBBBBB
*/

struct file_header {
	char magic[12];
	int16_t width;
	int16_t height;
	uint16_t offset;
};


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
	size_t datasize;
	uint8_t *bmap;
	int16_t handle;
	struct file_header header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	if ((size_t)Fread(handle, sizeof(header), &header) != sizeof(header) ||
		memcmp(header.magic, "COKE format.", 12) != 0)
	{
		Fclose(handle);
		return FALSE;
	}
	Fseek(header.offset, handle, SEEK_SET);
	datasize = (size_t)header.width * header.height * 2;
	
	bmap = malloc(datasize);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, datasize, bmap) != datasize)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);
	
	info->planes = 16;
	info->real_width = info->width = header.width;
	info->real_height = info->height = header.height;
	info->components = 3;
	info->indexed_color = FALSE;
	info->colors = 1L << 16;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_ptr = bmap;
	info->_priv_var = 0;				/* y position in bmap */

	strcpy(info->info, "COKE");
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


boolean __CDECL encoder_init(const char *name, IMGINFO info)
{
	struct file_header header;
	int16_t handle;
	uint32_t line_size;
	uint8_t *outline;
	
	line_size = (uint32_t)info->width * 2;
	outline = malloc(line_size);
	if (outline == NULL)
	{
		return FALSE;
	}

	if ((handle = (int16_t) Fcreate(name, 0)) < 0)
	{
		free(outline);
		return FALSE;
	}

	memcpy(header.magic, "COKE format.", sizeof(header.magic));
	header.width = info->width;
	header.height = info->height;
	header.offset = sizeof(header);
	if ((size_t)Fwrite(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		free(outline);
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
	info->_priv_var_more = line_size;
	info->_priv_ptr = outline;
	
	return TRUE;
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	int16_t handle;
	uint32_t line_size;
	uint16_t *outline = (uint16_t *)info->_priv_ptr;
	uint16_t x;
	uint16_t color;

	for (x = 0; x < info->width; x++)
	{
		color = (*buffer++ & 0xf8) << 8;
		color |= (*buffer++ & 0xfc) << 3;
		color |= *buffer++ >> 3;
		outline[x] = color;
	}
	handle = (int16_t)info->_priv_var;
	line_size = (uint32_t)info->_priv_var_more;
	if ((size_t)Fwrite(handle, line_size, outline) != line_size)
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
	free(info->_priv_ptr);
	info->_priv_ptr = 0;
}
