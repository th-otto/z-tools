#define	VERSION	     0x0201
#define NAME        "Calamus (Raster Graphic)"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
Calamus Raster Graphic    *.CRG

10 bytes    file id, 'CALAMUSCRG'
1 word      version? [$03E8]
1 word      version? [$0002]
1 long      file size, minus 24
1 word      code for the page size? [$0080]
1 long      image width in pixels
1 long      image height in pixels
1 long      unknown [varies]
1 byte      unknown [$01]
1 byte      unknown [$00]
1 long      unknown [$FFFFFFFF]
1 long      size of compressed image data in bytes
--------
42 bytes    total for header

??          monochrome image adata

RLE compression scheme: The compressed data consists of a code byte, followed
by one or more data bytes, followed by another code byte, and so on.

  To decompress:  Code byte (N)    Instructions
                  N <= 127         Emit the next N+1 bytes literally.
                  N >= 128         Emit the next byte N-127 times.

  After decompression, pixels are in left-to-right, top-to-bottom order. The
  format is 8 pixels per byte, most significant bit first, white is 0. Rows are
  padded to the next byte boundary.

Additional information:
  After the compressed data is 2 extra bytes. The purpose of these extra bytes
  is unknown. They can be safely ignored. Values encountered:
    $FFFF
    $0D0A (EC-Paint always writes this value)
*/

struct file_header {
	char magic[10];
	uint16_t ver1;
	uint16_t ver2;
	uint32_t length;
	uint16_t page_size;
	uint32_t width;
	uint32_t height;
	uint32_t unk1;
	uint8_t unk2;
	uint8_t unk3;
	uint32_t unk4;
	uint32_t datasize;
};

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("CRG\0");

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


static void decode_packbits(const unsigned char *src, unsigned char *dst, size_t cnt)
{
	while (cnt > 0)
	{
		uint8_t cmd = *src++;

		if (cmd <= 127)
		{								/* literals */
			cmd++;
			
			if (cmd > cnt)
				cmd = cnt;
			cnt -= cmd;
			while (cmd)
			{
				*dst++ = *src++;
				cmd--;
			}
		} else
		{
			uint8_t c;
			
			cmd -= (128 - 1);
			if (cmd > cnt)
				cmd = cnt;
			cnt -= cmd;
			c = *src++;
			while (cmd)
			{
				*dst++ = c;
				cmd--;
			}
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
	uint8_t *filedata;
	int16_t handle;
	size_t file_size;
	uint32_t datasize;
	uint32_t linesize;
	struct file_header *header;
	
	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	filedata = malloc(file_size + 2);
	if (filedata == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, file_size, filedata) != file_size)
	{
		free(filedata);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	header = (struct file_header *)filedata;
	if (memcmp(header->magic, "CALAMUSCRG", 10) != 0)
	{
		free(filedata);
		return FALSE;
	}
	if (header->length + 24 != file_size)
	{
		free(filedata);
		return FALSE;
	}
	linesize = (header->width + 7) >> 3;
	datasize = linesize * header->height;
	bmap = malloc(datasize + 256);
	if (bmap == NULL)
	{
		free(filedata);
		return FALSE;
	}
	decode_packbits(filedata + sizeof(*header), bmap, datasize);
	free(filedata);

	info->width = header->width;
	info->height = header->height;
	info->planes = 1;
	info->components = 1;
	info->indexed_color = FALSE;
	info->colors = 1L << 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	strcpy(info->info, "Calamus (Raster Graphic)");
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
	uint8_t *bmap;
	uint32_t pos;
	uint8_t b;
	size_t x;

	pos = info->_priv_var;
	bmap = info->_priv_ptr;
	x = 0;
	do
	{								/* 1-bit mono v1.00 */
		b = bmap[pos++];
		buffer[x++] = b & 0x80 ? 1 : 0;
		buffer[x++] = b & 0x40 ? 1 : 0;
		buffer[x++] = b & 0x20 ? 1 : 0;
		buffer[x++] = b & 0x10 ? 1 : 0;
		buffer[x++] = b & 0x08 ? 1 : 0;
		buffer[x++] = b & 0x04 ? 1 : 0;
		buffer[x++] = b & 0x02 ? 1 : 0;
		buffer[x++] = b & 0x01 ? 1 : 0;
	} while (x < info->width);

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
