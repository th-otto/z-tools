#define	VERSION	     0x0206
#define NAME        "Quantum Paintbox"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__
#define MISC_INFO   "Some code by Hans Wessels"

#include "plugin.h"
#include "zvplugin.h"

/*
QuantumPaint    *.PBX (ST low and ST medium resolution)

3 bytes         0
1 byte          mode, 0x00 =  128 color low resolution pciture,
                      0x01 =   32 color medium resolution picture,
                      0x80 =  512 color picture low resolution picture,
                      0x81 = 4096 color picture low resolution picture,
1 word          0x8001 = compressed
                QuantumPaint v2.00 sets it to 0x8000 for uncompressed files
                QuantumPaint v1.00 seems to use 0x0000 for low res and 0x0333
                for medium res bytes. The PBX viewer only tests for 0x8001.
122 bytes       ??
                total 128 header bytes
?? bytes        palette data;
                for uncompressed normal pictures 8 palette datasets follow:
                    16 words: 16 palettes
                     1 word: number of the first line this palette is used
                     1 word: 0 the palette is not active,
                             else this pallete is active
                     1 word: first color of color cycle
                     1 word: last color of color cycle
                     1 word: cycle speed, number of vbl's between steps,
                             0 = fastest
                     1 word: 0: no cycling, 1 cycle right, 0xffff cycle left
                     2 word: ?? should be zero
                    __
                    48 bytes per palette
                    The first palette should always be active and start on line 0
                   ___
                   384 bytes palette data for low (128 color) and medium (32 color) pictures
                for uncompressed 512 color pictures 6400 words (32 colors per scanline)
                for uncompressed 4096 color pictures 12800 words (32 colors per scanline,
                2 palette sets for alternating screens, two 6400 word blocks)
?? bytes        screen data, for uncompressed pictures this is 32000 bytes for all modes
---------
> 128 bytes total

Compression scheme:
Palette and screen data are compressed into one block using packbits
compression. The expanded screen data is not simply screen memory bitmap data;
instead, the data is divided into four sets of vertical columns. (This results
in better compression.) A column consists of one specific word taken from each
scan line, going from top to bottom. For example, column 1 consists of word 1
on scanline 1 followed by word 1 on scanline 2, etc., followed by word 1 on
scanline 200.
   The columns appear in the following order:

   1st set contains columns 1, 5,  9, 13, ..., 69, 73, 77 in order
   2nd set contains columns 2, 6, 10, 14, ..., 70, 74, 78 in order
   3rd set contains columns 3, 7, 11, 15, ..., 71, 75, 79 in order
   4th set contains columns 4, 8, 12, 16, ..., 72, 76, 80 in order
This column ordering is the same as in the Tiny format.
For medium resolution it is the same only with two stes for the two medium res
bitplanes.
*/


#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("PBX\0");

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

#define SCREEN_W 320
#define SCREEN_H 200
#define SCREEN_SIZE ((long)SCREEN_W * SCREEN_H * 3)

typedef uint16_t stcolor_t;
typedef long coord_t;

/* FIXME: statics */
static uint8_t palette[256 * 3];

#include "../degas/packbits.c"

static uint8_t *create_dip(coord_t x_size, coord_t y_size, uint16_t bits, uint16_t used_colors, stcolor_t *atari_colors)
{
	size_t bmpx;
	size_t size;
	uint8_t *data;
	
	/* line size always multiple longs */
	bmpx = (((3 + ((x_size * (size_t)bits + 7) >> 3)) & -4) * 8) / bits;
	size = (bmpx * y_size * bits) / 8;
	if ((data = malloc(size)) != NULL)
	{
		if (atari_colors != NULL)
		{
			int16_t i;
			int j;
			uint16_t tmp;
			
			for (j = i = 0; i < used_colors; i++)
			{
				tmp = atari_colors[i];
				palette[j++] = colortable[tmp & 0xf] & 0xff;
				palette[j++] = colortable[(tmp >> 4) & 0xf] & 0xff;
				palette[j++] = colortable[(tmp >> 8) & 0xf] & 0xff;
			}
		}
	}
	return data;
}


/*
 *  Given an x-coordinate and a color index, returns the corresponding
 *  QuantumPaint palette index.
 *
 *  by Hans Wessels; placed in the public domain January, 2008.
 */
static int find_pbx_index(int x, int c)
{
	int x1 = 10 * c;
	if (1 & c)
	{	/* If c is odd */
		x1 = x1 - 5;
	} else
	{	/* If c is even */
		x1 = x1 + 1;
	}
	if (c > 7)
	{
	 	x1 += 12;
	}
	if (x >= (x1 + 75))
	{
		c = c + 16;
	}
	return c;
}


/* 320 x 200, 4 bitplanes */
static void reorder_column_bitplanes(uint8_t *dst, uint8_t *src)
{
	int i;
	uint16_t *src = (uint16_t *)_src;
	uint16_t *dst = (uint16_t *)_dst;

	for (i = 0; i < SCREEN_H; i++)
	{
		int j;

		for (j = 0; j < SCREEN_W / 16; j++)
		{
			*dst++ = src[j * SCREEN_H + 0 * SCREEN_H * (SCREEN_W / 16)];
			*dst++ = src[j * SCREEN_H + 1 * SCREEN_H * (SCREEN_W / 16)];
			*dst++ = src[j * SCREEN_H + 2 * SCREEN_H * (SCREEN_W / 16)];
			*dst++ = src[j * SCREEN_H + 3 * SCREEN_H * (SCREEN_W / 16)];
		}
		src++;
	}
}


/* 640 x 200, 2 bitplanes */
static void reorder_column_bitplanes_med(uint8_t *dst, uint8_t *src)
{
	int i;

	for (i = 0; i < 200; i++)
	{
		int j;

		for (j = 0; j < 40; j++)
		{
			*dst++ = src[i * 2 + j * 400];
			*dst++ = src[i * 2 + j * 400 + 1];
			*dst++ = src[i * 2 + j * 400 + 16000];
			*dst++ = src[i * 2 + j * 400 + 16001];
		}
	}
}


static uint16_t get_word(const uint8_t *p)
{
	uint16_t res;

	res = *p++;
	res <<= 8;
	res += *p;
	return res;
}


static void get_colors(const void *p, stcolor_t *colors, int cnt)
{
	int i;
	const uint8_t *q = p;

	for (i = 0; i < cnt; i++)
	{
		colors[i] = get_word(q + 2 * i);
	}
}


/* special with rasters for Quantumpaint */
static uint8_t *convert_2bit32(coord_t x_size, coord_t y_size, uint8_t *screen, stcolor_t *colors, coord_t *rasterlines)
{
	uint8_t *dst;
	size_t bmx_line;

	dst = create_dip(x_size, y_size * 2, 8, 32, colors);
	bmx_line = (x_size + 3) & -4;	/* line size always multiple longs */
	x_size = ((x_size + 15) & -16);
	if (dst != NULL)
	{
		coord_t y;
		uint8_t color_index = 0;

		for (y = 0; y < y_size; y++)
		{
			uint8_t *q = screen + y * (x_size / 4);
			coord_t x;
			size_t bmx = 0;

			for (x = 0; x < x_size / 16; x++)
			{
				uint16_t plane0;
				uint16_t plane1;
				int i;

				plane0 = *q++;
				plane0 <<= 8;
				plane0 += *q++;
				plane1 = *q++;
				plane1 <<= 8;
				plane1 += *q++;
				for (i = 0; i < 16; i++)
				{
					uint8_t data;

					data = color_index;
					data += (uint8_t) ((plane1 >> 14) & 2);
					data += (uint8_t) ((plane0 >> 15) & 1);
					plane0 <<= 1;
					plane1 <<= 1;
					if (bmx < bmx_line)
					{
						dst[2 * y * bmx_line + bmx] = data;
						dst[(2 * y + 1) * bmx_line + bmx] = data;
						bmx++;
					}
				}
			}
			if ((y + 1) == *rasterlines)
			{
				color_index += 4;
				rasterlines++;
			}
		}
	}
	return dst;
}


static uint8_t *convert_4bitrasters(coord_t x_size, coord_t y_size, uint8_t *screen, stcolor_t *colors, coord_t *rasterlines)
{
	uint8_t *dst;
	size_t bmx_line;

	dst = create_dip(x_size, y_size, 8, 128, colors);
	bmx_line = (x_size + 3) & -4;	/* line size always multiple longs */
	/* picture x-line is multiple of 16 pixels */
	/* 1 uint8_t per pixel -> there are x-sizes where BMP line length < scanline length */
	x_size = ((x_size + 15) & -16);
	if (dst != NULL)
	{
		coord_t y;
		uint8_t color_index = 0;

		for (y = 0; y < y_size; y++)
		{
			uint8_t *q = screen + y * (x_size / 2);
			coord_t x;
			size_t bmx = 0;

			for (x = 0; x < x_size / 16; x++)
			{
				uint16_t plane0;
				uint16_t plane1;
				uint16_t plane2;
				uint16_t plane3;
				int i;

				plane0 = *q++;
				plane0 <<= 8;
				plane0 += *q++;
				plane1 = *q++;
				plane1 <<= 8;
				plane1 += *q++;
				plane2 = *q++;
				plane2 <<= 8;
				plane2 += *q++;
				plane3 = *q++;
				plane3 <<= 8;
				plane3 += *q++;
				for (i = 0; i < 16; i++)
				{
					uint8_t data;

					data = color_index;
					data += (uint8_t) ((plane3 >> 12) & 8);
					data += (uint8_t) ((plane2 >> 13) & 4);
					data += (uint8_t) ((plane1 >> 14) & 2);
					data += (uint8_t) ((plane0 >> 15) & 1);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
					if (bmx < bmx_line)
					{
						dst[y * bmx_line + bmx] = data;
						bmx++;
					}
				}
			}
			if ((y + 1) == *rasterlines)
			{
				color_index += 16;
				rasterlines++;
			}
		}
	}
	return dst;
}


static uint8_t *convert_pbx(uint8_t *gfx, uint8_t *pal0, uint8_t *pal1)
{
	uint8_t *dst;

	dst = create_dip(SCREEN_W, SCREEN_H, 24, 0, NULL);
	if (dst != NULL)
	{
		uint8_t *p = dst;
		long y;

		for (y = 0; y < SCREEN_H; y++)
		{
			uint8_t *q = gfx + y * 160;
			uint8_t *cp0 = pal0 + y * 64;
			uint8_t *cp1 = pal1 + y * 64;
			int x;

			for (x = 0; x < SCREEN_W / 16; x++)
			{
				uint16_t plane0;
				uint16_t plane1;
				uint16_t plane2;
				uint16_t plane3;
				int i;

				plane0 = *q++;
				plane0 <<= 8;
				plane0 += *q++;
				plane1 = *q++;
				plane1 <<= 8;
				plane1 += *q++;
				plane2 = *q++;
				plane2 <<= 8;
				plane2 += *q++;
				plane3 = *q++;
				plane3 <<= 8;
				plane3 += *q++;
				for (i = 0; i < 16; i++)
				{
					uint8_t data;
					stcolor_t color0;
					stcolor_t color1;

					data = (uint8_t) ((plane3 >> 12) & 8);
					data += (uint8_t) ((plane2 >> 13) & 4);
					data += (uint8_t) ((plane1 >> 14) & 2);
					data += (uint8_t) ((plane0 >> 15) & 1);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
					color0 = find_pbx_index(x * 16 + i, data);
					color1 = (cp1[color0 * 2] << 8) + cp1[color0 * 2 + 1];
					color0 = (cp0[color0 * 2] << 8) + cp0[color0 * 2 + 1];
					*p++ = ((colortable[(color0 >> 8) & 0xf] & 0xff) + (colortable[(color1 >> 8) & 0xf] & 0xff)) / 2;
					*p++ = ((colortable[(color0 >> 4) & 0xf] & 0xff) + (colortable[(color1 >> 4) & 0xf] & 0xff)) / 2;
					*p++ = ((colortable[color0 & 0xf] & 0xff) + (colortable[color1 & 0xf] & 0xff)) / 2;
				}
			}
		}
	}
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
	uint8_t *file;
	int type;
	int compressed;
	size_t file_size;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	file = malloc(120000L);
	if (file == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, file_size, file) != file_size)
	{
		free(file);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);

	if (/* file_size < 128 || */
		file[0] != 0 || file[1] != 0 || file[2] != 0 ||
		(file[3] != 0 && file[3] != 1 && file[3] != 0x80 && file[3] != 0x81))
	{
		/* sprintf(errorText, "This is not a Quantum Paint file!"); */
		free(file);
		RETURN_ERROR(EC_FileId);
	}
	
	if (file[3] == 0)
	{							/* 128 color low */
		uint16_t *p;
		uint16_t rasterdata[8 * 16];
		coord_t rasterlines[8];
		uint8_t *pic;
		uint8_t *cols;

		if (file[4] == 0x80 && file[5] == 1)
		{						/* compressed */
			decode_packbits(file + 60000L, file + 128, 32384);
			reorder_column_bitplanes(file, file + 60384L);
			cols = file + 60000L;
			pic = file;
			compressed = TRUE;
		} else
		{
			cols = file + 128;
			pic = file + 512;
		}
		{						/* atari raster picture */
			/* full palette for every line */
			int i;
			coord_t *q;
			
			p = rasterdata;
			q = rasterlines;
			get_colors(cols, p, 16);
			p += 16;
			cols += 48;
			for (i = 0; i < 7; i++)
			{
				if (cols[34] | cols[35])
				{
					coord_t line;

					line = cols[33];
					*q++ = line;
					get_colors(cols, p, 16);
					p += 16;
				}
				cols += 48;
			}
			*q = -1;			/* close rasters */
			type = 0;
			bmap = convert_4bitrasters(320, 200, pic, rasterdata, rasterlines);
		}
	} else if (file[3] == 1)
	{							/* 32 color med */
		uint16_t *p;
		stcolor_t colors[32];
		coord_t rasterlines[8];
		uint8_t *pic;
		uint8_t *cols;

		if (file[4] == 0x80 && file[5] == 1)
		{						/* compressed */
			decode_packbits(file + 60000L, file + 128, 32384);
			reorder_column_bitplanes_med(file, file + 60384L);
			cols = file + 60000L;
			pic = file;
			compressed = TRUE;
		} else
		{
			cols = file + 128;
			pic = file + 512;
		}
		{
			int i;
			coord_t *q;

			p = colors;
			q = rasterlines;
			get_colors(cols, p, 4);
			p += 4;
			cols += 48;
			for (i = 0; i < 7; i++)
			{
				if (cols[34] | cols[35])
				{				/* pallette on! */
					coord_t line;

					line = cols[33];
					*q++ = line;
					get_colors(cols, p, 4);
					p += 4;
				}
				cols += 48;
			}
			*q = -1;			/* close rasters */
		}
		type = 1;
		bmap = convert_2bit32(640, 200, pic, colors, rasterlines);
	} else if (file[3] == 0x80 || file[3] == 0x81)
	{							/* 512 and 4096 color */
		uint8_t *gfx;
		uint8_t *col0 = file + 60000L;
		uint8_t *col1;

		if (file[4] == 0x80 && file[5] == 1)
		{						/* compressed */
			size_t decode_size;

			if (file[3] == 0x80)
			{
				decode_size = 44800L;
				col1 = col0;
			} else
			{
				decode_size = 57600L;
				col1 = col0 + 12800;
			}
			gfx = file;
			decode_packbits(file + 60000L, file + 128, decode_size);
			reorder_column_bitplanes(file, col1 + 12800);
			compressed = TRUE;
		} else
		{
			col0 = file + 128;
			if (file[3] == 0x80)
			{
				col1 = col0;
			} else
			{
				col1 = col0 + 12800;
			}
			gfx = col1 + 12800;
		}
		type = 2;
		bmap = convert_pbx(gfx, col0, col1);
	} else
	{
		free(file);
		RETURN_ERROR(EC_ImageType);
	}
	
	free(file);
	if (bmap == NULL)
	{
		RETURN_ERROR(EC_Malloc);
	}
	
	if (compressed)
		strcpy(info->compression, "RLE");
	else
		strcpy(info->compression, "None");
	strcpy(info->info, "Quantum Paintbox");
	switch (type)
	{
	case 0:
		strcat(info->info, " (Low 128)");
		info->planes = 7;
		info->colors = 1L << 7;
		info->width = SCREEN_W;
		info->height = SCREEN_H;
		info->components = 1;
		info->indexed_color = TRUE;
		break;
	case 1:
		strcat(info->info, " (Medium 32)");
		info->planes = 5;
		info->colors = 1L << 5;
		info->width = SCREEN_W * 2;
		info->height = SCREEN_H * 2;
		info->components = 1;
		info->indexed_color = TRUE;
		break;
	case 2:
		strcat(info->info, " (Low 512/4096)");
		info->planes = 12;
		info->colors = 1L << 12;
		info->width = SCREEN_W;
		info->height = SCREEN_H;
		info->components = 3;
		info->indexed_color = FALSE;
		break;
	}

	if (info->indexed_color)
	{
		int i, j;
		
		for (i = j = 0; i < (int)info->colors; i++)
		{
			info->palette[i].blue = palette[j++];
			info->palette[i].green = palette[j++];
			info->palette[i].red = palette[j++];
		}
	}
	
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

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
	uint8_t *bmap = info->_priv_ptr;

	if (info->planes > 8)
	{
		memcpy(buffer, &bmap[info->_priv_var], SCREEN_W * 3);
		info->_priv_var += SCREEN_W * 3;
	} else
	{
		memcpy(buffer, &bmap[info->_priv_var], info->width);
		info->_priv_var += info->width;
	}
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
