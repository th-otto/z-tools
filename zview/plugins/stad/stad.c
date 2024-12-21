#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"
#define NF_DEBUG 0
#include "nfdebug.h"

/*
STAD       *.PAC (ST high resolution)

4 bytes    file id, 'pM86' (vertically packed) or 'pM85' (horizontally packed)
1 byte     id byte
1 byte     pack byte (most frequently occurring byte in bitmap)
1 byte     "special" byte
-------
7 bytes    total for header

? bytes    compressed data

The data is encoded as follows.  For each byte x in the data section:

    x = id byte             Read one more byte, n. Use pack byte n + 1 times.
    x = "special" byte      Read two more bytes, d, and n (in order).
                            Use byte d n + 1 times.
    otherwise               Use byte x literally.
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


#define STAD_MAGIC1 0x704d3835l /* horizontal compression */
#define STAD_MAGIC2 0x704d3836l /* vertical compression */

#define SCREEN_SIZE 32000L




static void unpack_stad(uint8_t *dst, const uint8_t *src)
{
	uint8_t id_byte;
	uint8_t pack_byte;
	uint8_t special_byte;
	uint8_t c;
	int16_t count;
	int16_t size;
	
	id_byte = *src++;
	pack_byte = *src++;
	special_byte = *src++;

	size = SCREEN_SIZE;
	while (size > 0)
	{
		c = *src++;
		if (c == id_byte)
		{
			count = *src++;
			count++;
			if (count > size)
				count = size;
			size -= count;
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
			if (count > size)
				count = size;
			size -= count;
			while (count > 0)
			{
				*dst++ = c;
				count--;
			}
		} else
		{
			--size;
			*dst++ = c;
		}
	}	
}


static void cleanup(IMGINFO info)
{
	int16_t handle = info->_priv_var_more;
	
	if (handle > 0)
	{
		Fclose(handle);
		info->_priv_var_more = 0;
	}
	if (info->_priv_ptr != NULL)
	{
		Mfree(info->_priv_ptr);
		info->_priv_ptr = NULL;
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
	uint32_t filesize;
	uint32_t filetype;
	int16_t handle;
	uint8_t *screen_buffer;
	uint8_t *file_buffer;

	info->_priv_ptr = NULL;
	info->_priv_ptr_more = NULL;
	handle = (int16_t) Fopen(name, 0);
	if (handle < 0)
	{
		return FALSE;
	}
	info->_priv_var_more = handle;
	filesize = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	file_buffer = info->_priv_ptr = (uint8_t *)Malloc(SCREEN_SIZE + 256);
	screen_buffer = (uint8_t *)Malloc(SCREEN_SIZE + 256);
	if (file_buffer == NULL || screen_buffer == NULL)
	{
		Mfree(screen_buffer);
		cleanup(info);
		return FALSE;
	}
	
	if (Fread(handle, filesize, file_buffer) != (long)filesize)
	{
		Mfree(screen_buffer);
		cleanup(info);
		return FALSE;
	}
	Fclose(handle);
	info->_priv_var_more = 0;
	filetype = *((uint32_t *)file_buffer);

	if (filetype != STAD_MAGIC1 && filetype != STAD_MAGIC2)
	{
		Mfree(screen_buffer);
		cleanup(info);
		return FALSE;
	}
	
	unpack_stad(screen_buffer, file_buffer + 4);
	if (filetype == STAD_MAGIC1)
	{
		info->_priv_ptr = screen_buffer;
		Mfree(file_buffer);
		strcpy(info->compression, "RLEH");
	} else
	{
		int16_t x, y;
		size_t i = 0;
		
		for (x = 0; x < 80; x++)
		{
			for (y = 0; y < 400; y++)
			{
				file_buffer[y * 80 + x] = screen_buffer[i];
				i++;
			}
		}
		Mfree(screen_buffer);
		strcpy(info->compression, "RLEV");
	}
	
	info->planes = 1;
	info->width = 640;
	info->height = 400;
	info->components = 1;
	info->indexed_color = 0;
	info->colors = 1L << 1;
	
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	info->_priv_var = 0;
	
	strcpy(info->info, "ST Aided Design");
	
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
	} while (x < info->width);
	info->_priv_var = i;
	return TRUE;
}


/*==================================================================================*
 * void __CDECL reader_quit:														*
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
	cleanup(info);
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
