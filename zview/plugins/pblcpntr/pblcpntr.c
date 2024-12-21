#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
Public Painter    *.CMP

Always monochrome.

1 byte    flag [used during decompression]
1 byte    size [0 = A5/640x400, 200 = A4/640x800]
-------
2 bytes   total for header

?         image data:

The image data is always RLE compressed.

Depending on the size field the uncompressed image will be 32000 or 64000 bytes.

Decompression code -> parameters:
  data = source address (compressed image data)
  bmap = destination address (uncompressed monochrome image data)
  flag = first item from file header
  cds  = compressed data size (file size minus sizeof(header))
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


#define SCREEN_SIZE 32000L

struct file_header {
	uint8_t flag;
	uint8_t size;
};

static void decode_cmp(uint8_t *data, uint8_t *dest, uint8_t flg, size_t cds)
{
	uint8_t byt, cmd;
	int16_t cnt;
	uint8_t *end;
	
	end = data + cds;
	do
	{
		cmd = *data++;
		if (cmd == flg)						/* repeat? */
		{
			cnt = 1 + *data++;
			byt = *data++;
			while (--cnt >= 0)
			{
			    *dest++ = byt;
			}
		} else                              /* literal ? */
		{
		    *dest++ = cmd;
		}
	} while (data < end);
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
	uint8_t *temp;
	struct file_header header;
	size_t file_size;
	
	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		return FALSE;
	}
	file_size -= sizeof(header);

	strcpy(info->info, "Public Painter");
	
	switch (header.size)
	{
	case 0:
		info->width = 640;
		info->height = 400;
		strcat(info->info, " (A5)");
		bmap = malloc(SCREEN_SIZE);
		break;
	case 200:
		info->width = 640;
		info->height = 800;
		strcat(info->info, " (A4)");
		bmap = malloc(SCREEN_SIZE * 2);
		break;
	default:
		Fclose(handle);
		return FALSE;
	}

	temp = malloc(file_size);
	if (bmap == NULL || temp == NULL)
	{
		free(temp);
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
	decode_cmp(temp, bmap, header.flag, file_size);
	
	info->planes = 1;
	info->colors = 1L << 1;
	info->components = 1;
	info->indexed_color = FALSE;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->compression, "RLE");

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
	uint8_t *bmap = info->_priv_ptr;
	int16_t byte;
	int x;
	
	x = info->width >> 3;
	do
	{
		byte = bmap[pos++];
		*buffer++ = (byte >> 7) & 1;
		*buffer++ = (byte >> 6) & 1;
		*buffer++ = (byte >> 5) & 1;
		*buffer++ = (byte >> 4) & 1;
		*buffer++ = (byte >> 3) & 1;
		*buffer++ = (byte >> 2) & 1;
		*buffer++ = (byte >> 1) & 1;
		*buffer++ = (byte >> 0) & 1;
	} while (--x > 0);
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
