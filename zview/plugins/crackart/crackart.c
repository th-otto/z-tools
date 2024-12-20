#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
CrackArt        *.CA1 (ST low resolution)
                *.CA2 (ST medium resolution)
                *.CA3 (ST high resolution)

1 word          file id, 'CA'
1 byte          compression: 00 = uncompressed, 01 = compressed
1 byte          resolution: 00 = low res, 01 = medium res, 02 = high res
?? words        palette
                For low resolution pictures there are 16 palette entries
                For med resolution pictures there are 4 palette entries
                For high resolution pictures there are 0 palette entries
?? bytes        Image data:
                For uncompressed images 32000 bytes follow
                For compressed images the compressed data follows
-------------
< 32057 bytes   total

CrackArt data compression:
1 byte          Escape character
1 byte          Delta: initially the 32000 screen memory are
                filled with the delta byte
1 word          Offset: offset between two packed bytes;
                Offset is added to the current byte position to calculate the
                next byte position. When the next byte position is greater
                32000, the next byte position is the first position of screen
                memory not written to
                if Offset = 0, the whole screen if filled with the delta byte,
                no packed bytes follow

1 byte          Cmd: Command byte
                Cmd <> Escape character: Cmd = one literal byte
                Cmd = Escape charater:
                  read 1 control byte
                    0: read one byte, n, and one byte, b;
                       repeat b (n+1) times
                    1: read one word, n, and one byte, b;
                       repeat b (n+1) times
                    2: read one byte, if zero: stop else read
                       one extra byte and to make word n:
                       repeat Delta (n+1) times
                    Escape character: output one escape charater
                    else: read one byte, b;
                       repeat b cmd times
Decompression stops when Escape 2,0 is found or 32000 bytes have been written

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

#define SCREEN_SIZE 32000

struct file_header {
	uint16_t magic;
	uint8_t compression;
	uint8_t resolution;
};

#include "ca_unpac.c"

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
	uint8_t *bmap;
	uint8_t *file_buffer;
	struct file_header file_header;
	size_t data_offset;

	handle = (int16_t)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	if ((size_t) Fread(handle, sizeof(file_header), &file_header) != sizeof(file_header) ||
		file_header.magic != 0x4341 ||
		file_header.compression > 1 ||
		file_header.resolution > 2)
	{
		Fclose(handle);
		return FALSE;
	}

	info->planes = 4 >> file_header.resolution;
	switch (info->planes)
	{
	case 4:
		info->width = 320;
		info->height = 200;
		info->indexed_color = TRUE;
		data_offset = sizeof(file_header) + 16 * 2;
		break;
	case 2:
		info->width = 640;
		info->height = 200;
		info->indexed_color = TRUE;
		data_offset = sizeof(file_header) + 4 * 2;
		break;
	case 1:
		info->width = 640;
		info->height = 400;
		info->indexed_color = FALSE;
		data_offset = sizeof(file_header);
		break;
	default:
		Fclose(handle);
		return FALSE;
	}
	info->colors = 1L << info->planes;
	if (info->indexed_color)
	{
		int i, colors;
		uint16_t color;

		colors = 1 << info->planes;
		for (i = 0; i < colors; i++)
		{
			if ((size_t) Fread(handle, sizeof(color), &color) != sizeof(color))
			{
				Fclose(handle);
				return FALSE;
			}
			info->palette[i].red = (((color >> 7) & 0x0e) + ((color >> 11) & 0x01)) * 17;
			info->palette[i].green = (((color >> 3) & 0x0e) + ((color >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((color << 1) & 0x0e) + ((color >> 3) & 0x01)) * 17;
		}
	}
	
	bmap = malloc(SCREEN_SIZE);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if (file_header.compression)
	{
		file_size -= data_offset;
		file_buffer = malloc(file_size);
		if (file_buffer == NULL)
		{
			Fclose(handle);
			return FALSE;
		}
		if ((size_t) Fread(handle, file_size, file_buffer) != file_size)
		{
			free(file_buffer);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		ca_decompress(file_buffer, bmap);
		free(file_buffer);
		strcpy(info->compression, "RLE");
	} else
	{
		if ((size_t) Fread(handle, SCREEN_SIZE, bmap) != SCREEN_SIZE)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		strcpy(info->compression, "None");
	}

	Fclose(handle);

	info->components = info->planes == 1 ? 1 : 3;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "CrackArt");
	
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
	
	switch (info->planes)
	{
	case 1:
		{
			int x;
			int width = info->width;
			int byte;
			int bit;
			const uint8_t *bmap = info->_priv_ptr;

			x = 0;
			do
			{								/* 1-bit mono v1.00 */
				byte = bmap[pos++];
				for (bit = 7; bit >= 0; bit--)
				{
					buffer[x++] = (byte >> bit) & 1;
				}
			} while (x < width);
		}
		break;
	default:
		{
			int width = info->width;
			const uint16_t *bmap16;
			int x;
			int bit;
			int byte;
			int plane;
	
			x = 0;
			bmap16 = (const uint16_t *)info->_priv_ptr;
			do
			{
				for (bit = 15; bit >= 0; bit--)
				{
					for (plane = byte = 0; plane < info->planes; plane++)
					{
						if ((bmap16[pos + plane] >> bit) & 1)
							byte |= 1 << plane;
					}
					buffer[x] = byte;
					x++;
				}
				pos += info->planes;
			} while (x < width);
		}
		break;
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
