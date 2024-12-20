#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
ImageLab    *.B&W *.B_W

The two file extensions are interchangeable.
Storage of 256 level gray scale images.

6 bytes     file id, "B&W256"
1 word      image width in pixels
1 word      image height in pixels
--------
10 bytes    total for header

?           image date:
image data size = width*height, 1 pixel per byte, 0 = black, 255 = white
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
	char magic[6];
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
	int16_t handle;
	struct file_header header;
	uint8_t *bmap;

	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	if (Fread(handle, sizeof(header), &header) != sizeof(header) ||
		memcmp(header.magic, "B&W256", 6) != 0)
	{
		Fclose(handle);
		return FALSE;
	}
	image_size = (size_t)header.width * header.height;

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
	info->planes = 8;
	info->indexed_color = TRUE;
	info->components = 3;
	info->colors = 1L << 8;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "ImageLab");
	strcpy(info->compression, "None");
	
	{
		unsigned int i;
		
		for (i = 0; i < 256; i++)
		{
			info->palette[i].red =
			info->palette[i].green =
			info->palette[i].blue = i;
		}
	}
	
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
	uint8_t *bmap = info->_priv_ptr;
	
	memcpy(buffer, &bmap[info->_priv_var], info->width);
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
