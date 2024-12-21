#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
PaintShop plus    *.PSC (st high resolution)
                  *.DA4 (st high resolution, double height)

*.DA4 files are simple screen dumps:

64000 bytes    image data (uncompressed, thus 640x800)
-----------
64000 bytes    total for file

_______________________________________________________________________________

*.PSC files are compressed:

6 bytes     file id, "tm89PS"
1 word      version [$CB = 2.03]
1 byte      ? [always 2]
1 byte      ? [always 1]
1 word      image width in pixels - 1 [always 639]
1 word      image height in pixels - 1 [always 399]
--------
14 bytes    total for header

?           image data:
The image data is not compressed based on pixels, but rather scan lines.
Starting at the top and working downward. There are 9 unique codes:

Codes:
  0    set current line all white ($00)
 10    copy line above (next byte + 1) times
 12    copy line above (next byte + 256 + 1) times
 99    copy from stream 80 * 400 lines (full image)
100    fill next line with (next byte) as line style [(b << 8 ) | b]
102    fill next line with (next 2 bytes) as line style [(b1 << 8) | b2]
110    copy from stream 80 bytes to current line
200    set current line all black ($FF)
255    marks end of file (stop process codes)

'line style' mentioned above is a reference to VDI function vsl_udsty(), set
user-defined line style pattern. Essentially, fill that line with the specified
bit pattern.

See also https://github.com/thmuch/tosgem-image-reader/blob/main/docs/PaintShopCompressed.md

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
	uint32_t magic;
	union {
		uint32_t l;
		uint16_t s;
	} magic2;
	uint8_t reserved;
	uint8_t hlen;
	uint16_t width;
	uint16_t height;
};


static uint8_t *decode_psc(uint8_t *src, uint8_t *dst)
{
	uint8_t cmd;
	
	for (;;)
	{
		cmd = *src++;
		if (cmd == 0)
		{
			memset(dst, 0, 80);
			dst += 80;
		} else if (cmd == 10)
		{
			uint8_t *p;
			int count;

			count = *src++ + 1;
			p = dst - 80;
			do
			{
				memcpy(dst, p, 80);
				dst += 80;
			} while (--count > 0);
		} else if (cmd == 12)
		{
			uint8_t *p;
			int count;

			count = *src++ + 256 + 1; /* hm, +1 not according to doc & java reader, but recoil does the same */
			p = dst - 80;
			do
			{
				memcpy(dst, p, 80);
				dst += 80;
			} while (--count > 0);
		} else if (cmd == 99)
		{
			/* should only occur as first byte */
			memcpy(dst, src, SCREEN_SIZE);
			src += SCREEN_SIZE;
			dst += SCREEN_SIZE;
		} else if (cmd == 100)
		{
			uint8_t line_style = *src++;

			memset(dst, line_style, 80);
			dst += 80;
		} else if (cmd == 102)
		{
			uint8_t line_style = *src++;
			uint8_t line_style2 = *src++;
			int count = 40;
			
			do
			{
				*dst++ = line_style;
				*dst++ = line_style2;
			} while (--count > 0);
		} else if (cmd == 110)
		{
			memcpy(dst, src, 80);
			src += 80;
			dst += 80;
		} else if (cmd == 200)
		{
			memset(dst, 0xff, 80);
			dst += 80;
		} else
		{
			break;
		}
	}
	if (cmd != 0xff)
		dst = NULL;
	return dst;
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
	size_t file_size;
	struct file_header header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	if (file_size == 2 * SCREEN_SIZE)
	{
		bmap = malloc(2 * SCREEN_SIZE);
		if (bmap == NULL)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		info->width = 640;
		info->height = 800;
		if (Fread(handle, 2 * SCREEN_SIZE, bmap) != 2 * SCREEN_SIZE)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		strcpy(info->compression, "None");
	} else
	{
		uint8_t *temp;

		if (Fread(handle, sizeof(header), &header) != sizeof(header))
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		if (header.magic != 0x746d3839L || /* 'tm89' */
			(header.magic2.s != 0x5053 && header.magic2.l != 0x4d424954L) || /* 'MBIT' */
			header.hlen != 1 ||
			header.width != 640 - 1 ||
			header.height != 400 -1)
		{
			Fclose(handle);
			RETURN_ERROR(EC_FileId);
		}
		bmap = malloc(SCREEN_SIZE);
		temp = malloc(file_size);
		if (bmap == NULL || temp == NULL)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		file_size -= sizeof(header);
		if ((size_t)Fread(handle, file_size, temp) != file_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		info->width = header.width + 1;
		info->height = header.height + 1;
		if (decode_psc(temp, bmap) - bmap != SCREEN_SIZE)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_DecompError);
		}
		free(temp);
		strcpy(info->compression, "RLE");
	}
	Fclose(handle);
	
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

	strcpy(info->info, "PaintShop+");

	RETURN_SUCCESS();
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
	int x;
	int16_t byte;
	uint8_t *bmap = info->_priv_ptr;
	size_t src = info->_priv_var;

	x = info->width >> 3;
	info->_priv_var += x;
	do
	{
		byte = bmap[src++];
		*buffer++ = (byte >> 7) & 1;
		*buffer++ = (byte >> 6) & 1;
		*buffer++ = (byte >> 5) & 1;
		*buffer++ = (byte >> 4) & 1;
		*buffer++ = (byte >> 3) & 1;
		*buffer++ = (byte >> 2) & 1;
		*buffer++ = (byte >> 1) & 1;
		*buffer++ = (byte >> 0) & 1;
	} while (--x > 0);

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
