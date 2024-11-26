#define	VERSION	    0x202
#define NAME        "Public Painter"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__
#define MISC_INFO   "Some code by Hans Wessels"


#include "plugin.h"
#include "zvplugin.h"

/*
PhotoChrome     *.PCS (low resolution only)

1 word          0x0140 (hex) x-resolution (always 320)
1 word          0x00C8 (hex) y-resolution (always 200)
1 byte          00 = single screen mode
                nonzero value's = alternating screen mode,
                bit 0 (0x01) not set: second screen data is xor'ed with first
                screen
                bit 1 (0x02) not set: second color palette is xor'ed with first
1 byte          bit 0 (0x01) set: 50 Hz mode, when not set 60 Hz mode
?? bytes        compressed screen data
?? bytes        compressed palette
?? bytes        compressed second screen data (in case of alternating screens)
?? bytes        compressed second palette
-----------
 > 18 bytes     total

Screen compression scheme:
1 word          total number of control bytes that follow
1 control byte  x
                x < 0 : copy -x literal bytes (1 - 128)
                x = 0 : next word specifies the number of times the following
                        byte has to be copied (0-65535)
                x = 1 : next word specifies the number of bytes to be copied
                        (0-65535)
                x > 1 : repeat the next byte x times (2-127)

The 4 bitplanes are separated, first 8000 bytes of the first bitplane, then
8000 bytes for the second, 8000 bytes for the third and 8000 bytes for the
fourth; a total of 32000 bytes.

Palette compression scheme:
The palette compression scheme is the same as the screen compression but it
works with 16 bit words
1 word          total number of control bytes that follow
1 control byte  x
                x < 0 : copy -x literal words (1 - 128)
                x = 0 : next word specifies the number of times the following
                        word has to be copied (0-65535)
                x = 1 : next word specifies the number of words to be copied
                        (0-65535)
                x > 1 : repeat the next word x times (2-127)

In total 9616 palette entries are stored. 200 scanlines with 3 palettes (48
colors) each, plus one final palette (16 colors). PhotoChrome files with a few
extra entries have been found. The first scanline usually contains no data.
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("PCS\0");

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
	case INFO_MISC:
		return (long)MISC_INFO;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif


static uint8_t const colortable[16] = {
	0x00,								/* 0000 */
	0x22,								/* 0001 */
	0x44,								/* 0010 */
	0x66,								/* 0011 */
	0x88,								/* 0100 */
	0xAA,								/* 0101 */
	0xCC,								/* 0110 */
	0xEE,								/* 0111 */
	0x11,								/* 1000 */
	0x33,								/* 1001 */
	0x55,								/* 1010 */
	0x77,								/* 1011 */
	0x99,								/* 1100 */
	0xBB,								/* 1101 */
	0xDD,								/* 1110 */
	0xFF								/* 1111 */
};


struct file_header {
	uint16_t width;
	uint16_t height;
	uint8_t screen_flags;
	uint8_t hz50;
};


#define SCREEN_W 320
#define SCREEN_H 200
#define SCREEN_SIZE ((long)SCREEN_W * SCREEN_H * 3)


/*
 * pcs functions ported from the IrfanView module
 * atarist.c from Hans Wessels
 */

static uint8_t *decode_pcs(uint8_t *dst, uint8_t *src)
{
	uint16_t cnt = 32256;					/* sometimes some overshoot is needed for correct depack... */
	uint16_t cmd_cnt;

	cmd_cnt = *src++;
	cmd_cnt <<= 8;
	cmd_cnt += *src++;
	while (cmd_cnt > 0)
	{
		uint8_t cmd = *src++;

		cmd_cnt--;
		if (cmd == 0)
		{
			uint8_t data;
			uint16_t tmp;

			tmp = *src++;
			tmp <<= 8;
			tmp += *src++;
			data = *src++;
			if (tmp > cnt)
			{
				tmp = cnt;
			}
			cnt -= tmp;
			while (tmp > 0)
			{
				*dst++ = data;
				tmp--;
			}
		} else if (cmd == 1)
		{
			uint16_t tmp;

			tmp = *src++;
			tmp <<= 8;
			tmp += *src++;
			if (tmp > cnt)
			{
				tmp = cnt;
			}
			cnt -= tmp;
			while (tmp > 0)
			{
				*dst++ = *src++;
				tmp--;
			}
		} else if ((cmd & 0x80) == 0x80)
		{
			int i = 128 - (cmd & 0x7f);

			if ((uint16_t)i > cnt)
			{
				i = cnt;
			}
			cnt -= i;
			while (i > 0)
			{
				*dst++ = *src++;
				i--;
			}
		} else
		{
			int i = cmd & 0x7f;
			uint8_t data;

			data = *src++;
			if ((uint16_t)i > cnt)
			{
				i = cnt;
			}
			cnt -= i;
			while (i > 0)
			{
				*dst++ = data;
				i--;
			}
		}
	}
	return src;
}


static uint8_t *decode_pcs_colors(uint8_t *dst, uint8_t *src)
{
	uint16_t cnt = 9700;					/* max 9700 colors, official is 9616 but some files have more */
	uint16_t cmd_cnt;

	cmd_cnt = *src++;
	cmd_cnt <<= 8;
	cmd_cnt += *src++;
	while (cmd_cnt > 0)
	{
		uint8_t cmd = *src++;

		cmd_cnt--;
		if (cmd == 0)
		{
			uint8_t data0;
			uint8_t data1;
			uint16_t tmp;

			tmp = *src++;
			tmp <<= 8;
			tmp += *src++;
			data0 = *src++;
			data1 = *src++;
			if (tmp > cnt)
			{
				tmp = cnt;
			}
			cnt -= tmp;
			while (tmp > 0)
			{
				*dst++ = data0;
				*dst++ = data1;
				tmp--;
			}
		} else if (cmd == 1)
		{
			uint16_t tmp;

			tmp = *src++;
			tmp <<= 8;
			tmp += *src++;
			if (tmp > cnt)
			{
				tmp = cnt;
			}
			cnt -= tmp;
			while (tmp > 0)
			{
				*dst++ = *src++;
				*dst++ = *src++;
				tmp--;
			}
		} else if ((cmd & 0x80) == 0x80)
		{
			int i = 128 - (cmd & 0x7f);

			if ((uint16_t)i > cnt)
			{
				i = cnt;
			}
			cnt -= i;
			while (i > 0)
			{
				*dst++ = *src++;
				*dst++ = *src++;
				i--;
			}
		} else
		{
			int i = cmd & 0x7f;
			uint8_t data0;
			uint8_t data1;

			data0 = *src++;
			data1 = *src++;
			if ((uint16_t)i > cnt)
			{
				i = cnt;
			}
			cnt -= i;
			while (i > 0)
			{
				*dst++ = data0;
				*dst++ = data1;
				i--;
			}
		}
	}
	return src;
}


static void reorder_bitplanes(uint8_t *dst, uint8_t *src, long offset)
{
	uint8_t *plane0 = src;
	uint8_t *plane1 = plane0 + offset;
	uint8_t *plane2 = plane1 + offset;
	uint8_t *plane3 = plane2 + offset;
	
	offset >>= 1;
	while (offset > 0)
	{
		*dst++ = *plane0++;
		*dst++ = *plane0++;
		*dst++ = *plane1++;
		*dst++ = *plane1++;
		*dst++ = *plane2++;
		*dst++ = *plane2++;
		*dst++ = *plane3++;
		*dst++ = *plane3++;
		offset--;
	}
}


/*
 *  Given an x-coordinate and a color index, returns the corresponding
 *  Photochrome palette index.
 *
 *  by Hans Wessels; placed in the public domain January, 2008.
 */
static int find_pcs_index(int x, int c)
{
	int x1 = 4 * c;
	int index = c;

	if (x >= x1)
	{
		index += 16;
	}
	if (x >= (x1 + 64 + 12) && c < 14)
	{
		index += 16;
	}
	if (x >= (132 + 16) && c == 14)
	{
		index += 16;
	}
	if (x >= (132 + 20) && c == 15)
	{
		index += 16;
	}
	x1 = 10 * c;
	if ((c & 1) == 1)
	{
		x1 -= 6;
	}
	if (x >= (176 + x1) && c < 14)
	{
		index += 16;
	}
	return index;
}


static uint8_t *convert_pcs(uint8_t *gfx0, uint8_t *pal0, uint8_t *gfx1, uint8_t *pal1)
{
	uint8_t *data;

	data = malloc(SCREEN_SIZE);
	if (data != NULL)
	{
		uint8_t *p = data;
		long y;

		/* 1st line is black */
		for (y = 0; y < SCREEN_W * 3; y++)
		{
			*p++ = 0;
		}
		for (y = 1; y < SCREEN_H; y++)
		{
			uint8_t *q0 = gfx0 + y * 160;
			uint8_t *cp0 = pal0 + (y - 1) * 96;
			uint8_t *q1 = gfx1 + y * 160;
			uint8_t *cp1 = pal1 + (y - 1) * 96;
			int x;

			for (x = 0; x < (SCREEN_W / 16); x++)
			{
				uint16_t plane00;
				uint16_t plane01;
				uint16_t plane02;
				uint16_t plane03;
				uint16_t plane10;
				uint16_t plane11;
				uint16_t plane12;
				uint16_t plane13;
				int i;

				plane00 = *q0++;
				plane00 <<= 8;
				plane00 += *q0++;
				plane01 = *q0++;
				plane01 <<= 8;
				plane01 += *q0++;
				plane02 = *q0++;
				plane02 <<= 8;
				plane02 += *q0++;
				plane03 = *q0++;
				plane03 <<= 8;
				plane03 += *q0++;
				plane10 = *q1++;
				plane10 <<= 8;
				plane10 += *q1++;
				plane11 = *q1++;
				plane11 <<= 8;
				plane11 += *q1++;
				plane12 = *q1++;
				plane12 <<= 8;
				plane12 += *q1++;
				plane13 = *q1++;
				plane13 <<= 8;
				plane13 += *q1++;
				for (i = 0; i < 16; i++)
				{
					uint8_t data0;
					uint16_t color0;
					uint8_t data1;
					uint16_t color1;

					data0 = ((plane03 >> 12) & 8);
					data0 += ((plane02 >> 13) & 4);
					data0 += ((plane01 >> 14) & 2);
					data0 += ((plane00 >> 15) & 1);
					data1 = ((plane13 >> 12) & 8);
					data1 += ((plane12 >> 13) & 4);
					data1 += ((plane11 >> 14) & 2);
					data1 += ((plane10 >> 15) & 1);
					plane00 <<= 1;
					plane01 <<= 1;
					plane02 <<= 1;
					plane03 <<= 1;
					plane10 <<= 1;
					plane11 <<= 1;
					plane12 <<= 1;
					plane13 <<= 1;
					if ((x * 16 + i) < 316 || data0 != 0)
					{
						color0 = find_pcs_index(x * 16 + i, data0);
						color0 = (cp0[color0 * 2] << 8) + cp0[color0 * 2 + 1];
					} else
					{
						color0 = 0;
					}
					if ((x * 16 + i) < 316 || data1 != 0)
					{
						color1 = find_pcs_index(x * 16 + i, data1);
						color1 = (cp1[color1 * 2] << 8) + cp1[color1 * 2 + 1];
					} else
					{
						color1 = 0;
					}
					*p++ = ((colortable[(color0 >> 8) & 0xf] & 0xff) + (colortable[(color1 >> 8) & 0xf] & 0xff)) / 2;
					*p++ = ((colortable[(color0 >> 4) & 0xf] & 0xff) + (colortable[(color1 >> 4) & 0xf] & 0xff)) / 2;
					*p++ = ((colortable[color0 & 0xf] & 0xff) + (colortable[color1 & 0xf] & 0xff)) / 2;
				}
			}
		}
	}
	return data;
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
	uint8_t *file;
	uint8_t *screen;
	uint8_t *scr0;
	uint8_t *scr1;
	uint8_t *pal0;
	uint8_t *pal1;
	uint8_t *src;
	size_t file_size;
	struct file_header header;

	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	file_size -= sizeof(header);
	if (file_size > 105000L)
	{
		Fclose(handle);
		return FALSE;
	}
	
	file = malloc(242000L); /* 105000 for packed data, 105000 for depacked data and 32000 for work screen */
	if (file == NULL)
	{
		Fclose(handle);
		return FALSE;
	}

	if (Fread(handle, sizeof(header), &header) != sizeof(header) ||
		header.width != SCREEN_W ||
		header.height != SCREEN_H ||
		(size_t)Fread(handle, file_size, file) != file_size)
	{
		free(file);
		Fclose(handle);
		return FALSE;
	}
				
	Fclose(handle);
	
	screen = file + 105000L;
	scr0 = screen + 32256L;
	scr1 = scr0;
	pal0 = scr0 + 32256L;
	pal1 = pal0;
	src = file;
	src = decode_pcs(screen, src);
	reorder_bitplanes(scr0, screen, 8000);
	src = decode_pcs_colors(pal0, src);
	
	strcpy(info->info, "PhotoChrome");
	if (header.screen_flags != 0)
	{
		/* alternating screens */
		strcat(info->info, "+");
		scr1 = file + 105000L + 32256L + 32256L + 19400L;
		pal1 = file + 105000L + 32256L + 32256L + 32256L + 19400L;
		src = decode_pcs(screen, src);
		reorder_bitplanes(scr1, screen, 8000);
		src = decode_pcs_colors(pal1, src);
		if ((header.screen_flags & 1) == 0)
		{						/* xor-ed screens */
			int i;

			for (i = 0; i < 32000; i++)
			{
				scr1[i] ^= scr0[i];
			}
		}
		if ((header.screen_flags & 2) == 0)
		{						/* xor-ed palette */
			int i;

			for (i = 0; i < 19232; i++)
			{
				pal1[i] ^= pal0[i];
			}
		}
	}

	bmap = convert_pcs(scr0, pal0, scr1, pal1);
	free(file);
	if (bmap == NULL)
	{
		return FALSE;
	}
	
	info->planes = 24;
	info->width = SCREEN_W;
	info->height = SCREEN_H;
	info->components = 3;
	info->indexed_color = FALSE;
	info->colors = 1L << 18;
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
	uint8_t *bmap = info->_priv_ptr;

	memcpy(buffer, bmap + info->_priv_var, SCREEN_W * 3);
	info->_priv_var += SCREEN_W * 3;
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
