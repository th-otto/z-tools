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
	size_t image_size;
	int16_t handle;
	uint16_t magic;
	uint8_t c;
	uint8_t type;
	int16_t count;
	size_t pos;
	uint8_t pixel;
	
	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fopen);
	}
	if (Fread(handle, sizeof(magic), &magic) != sizeof(magic))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (magic != 0x1b47)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	if (Fread(handle, sizeof(type), &type) != sizeof(type))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}

	strcpy(info->info, "CompuServe RLE");
	switch (type)
	{
	case 0x4d:
		info->width = 128;
		info->height = 96;
		strcat(info->info, " (Medium)");
		break;
	case 0x48:
		info->width = 256;
		info->height = 192;
		strcat(info->info, " (High)");
		break;
	case 0x53:
		info->width = 640;
		info->height = 200;
		strcat(info->info, " (Super)");
		break;
	default:
		Fclose(handle);
		RETURN_ERROR(EC_ResolutionType);
	}

	image_size = (size_t)info->width * info->height;
	bmap = (uint8_t *)Malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}

	pixel = 1;
	pos = 0;
	for (;;)
	{
		if (Fread(handle, sizeof(c), &c) != sizeof(c))
			break;
		if (c == 0x1b)
			break;
		if (c < 32 || c > 127)
		{
			if (pos == image_size - 1)
			{
				/* last pixel is often missing */
				bmap[pos++] = pixel;
				break;
			}
			Mfree(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_DecompError);
		}
		for (count = c - 32; count > 0; count--)
		{
			if (pos < image_size)
				bmap[pos++] = pixel;
		}
		pixel ^= 1;
	}
		
	Fclose(handle);
	while (pos < image_size)
	{
		bmap[pos++] = 0;
	}
	
	info->palette[0].red =
	info->palette[0].green =
	info->palette[0].blue = 0;
	info->palette[1].red =
	info->palette[1].green =
	info->palette[1].blue = 255;

	info->planes = 1;
	info->indexed_color = FALSE;
	info->components = 1;
	info->colors = 1L << 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	strcpy(info->compression, "RLE");

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
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	
	bmap += info->_priv_var;
	info->_priv_var += info->width;
	memcpy(buffer, bmap, info->width);

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
