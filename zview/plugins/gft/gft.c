#define	VERSION	     0x100
#define NAME        "GFA Artist (Font)"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("GFT\0");

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


static uint16_t bitreverse(uint16_t val)
{
	uint16_t newval;
	int j;
	
	newval = 0;
	for (j = 16; --j >= 0; )
	{
		if (val & 1)
			newval |= 1 << j;
		val >>= 1;
	}
	return newval;
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
	int16_t handle;
	uint16_t *bmap;
	uint16_t *temp;
	int i;
	int j;
	int k;
	int src;
	int dst;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (file_size != 5280)
	{
		Fclose(handle);
		return FALSE;
	}

	bmap = malloc(file_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	temp = malloc(file_size);
	if (temp == NULL)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}

	if ((size_t)Fread(handle, file_size, temp) != file_size)
	{
		free(temp);
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	dst = 0;
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 16; j++)
		{
			src = i * 512 + j;
			for (k = 0; k < 32; k++)
			{
				bmap[dst++] = bitreverse(temp[src]);
				src += 16;
			}
		}
	}
	free(temp);
	
	info->width = 512;
	info->height = 80;
	info->planes = 1;
	info->indexed_color = FALSE;
	info->components = 1;
	info->colors = 1L << 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "GFA Artist (Font)");
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
	size_t pos = info->_priv_var;
	uint16_t *bmap = info->_priv_ptr;
	int width = info->width;
	uint8_t *end = buffer + width;
	uint16_t byte;
	
	do
	{
		byte = bmap[pos++];
		*buffer++ = (byte >> 15) & 1;
		*buffer++ = (byte >> 14) & 1;
		*buffer++ = (byte >> 13) & 1;
		*buffer++ = (byte >> 12) & 1;
		*buffer++ = (byte >> 11) & 1;
		*buffer++ = (byte >> 10) & 1;
		*buffer++ = (byte >> 9) & 1;
		*buffer++ = (byte >> 8) & 1;
		*buffer++ = (byte >> 7) & 1;
		*buffer++ = (byte >> 6) & 1;
		*buffer++ = (byte >> 5) & 1;
		*buffer++ = (byte >> 4) & 1;
		*buffer++ = (byte >> 3) & 1;
		*buffer++ = (byte >> 2) & 1;
		*buffer++ = (byte >> 1) & 1;
		*buffer++ = (byte >> 0) & 1;
	} while (buffer < end);
	
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
