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
	uint16_t magic;
	uint16_t palette[16];
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
	size_t file_size;
	size_t image_size;
	int16_t handle;
	uint8_t *bmap;
	struct file_header header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (file_size == 63054L)
	{
		info->width = 460;
		info->height = 274;
	} else if (file_size == 62506L)
	{
		info->width = 456;
		info->height = 271;
	} else if (file_size == 61410L)
	{
		info->width = 448;
		info->height = 266;
	} else
	{
		Fclose(handle);
		return FALSE;
	}
	if (!(Kbshift(-1) & K_ALT))
		info->width = 448;

	if (Fread(handle, sizeof(header), &header) != sizeof(header) ||
		(header.magic != 0x4B44 &&
		 header.magic != 0x4B76 &&
		 header.magic != 0x4B84))
	{
		Fclose(handle);
		return FALSE;
	}
		
	image_size = file_size - sizeof(header);
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
	
	info->planes = 4;
	info->components = 3;
	info->indexed_color = TRUE;
	info->colors = 1L << 4;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "FULLSHOW");
	strcpy(info->compression, "None");
	
	{
		int i;

		for (i = 0; i < 16; i++)
		{
			info->palette[i].red = (((header.palette[i] >> 7) & 0xE) + ((header.palette[i] >> 11) & 0x1)) * 17;
			info->palette[i].green = (((header.palette[i] >> 3) & 0xE) + ((header.palette[i] >> 7) & 0x1)) * 17;
			info->palette[i].blue = (((header.palette[i] << 1) & 0xE) + ((header.palette[i] >> 3) & 0x1)) * 17;
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
	const uint16_t *screen16;
	uint8_t *end;
	int bit;
	uint16_t plane0;
	uint16_t plane1;
	uint16_t plane2;
	uint16_t plane3;
	
	screen16 = (const uint16_t *)info->_priv_ptr + info->_priv_var;
	end = buffer + info->width;
	do
	{
		plane0 = *screen16++;
		plane1 = *screen16++;
		plane2 = *screen16++;
		plane3 = *screen16++;
		for (bit = 0; bit < 16; bit++)
		{
			*buffer++ = ((plane0 >> 15) & 1) | ((plane1 >> 14) & 2) | ((plane2 >> 13) & 4) | ((plane3 >> 12) & 8);
			plane0 <<= 1;
			plane1 <<= 1;
			plane2 <<= 1;
			plane3 <<= 1;
		}
	} while (buffer < end);
	/* info->_priv_var += info->width >> 2; */
	info->_priv_var += 460 >> 2;
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
