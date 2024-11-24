/* -------------------------------------------------------------------
    MPP to BMP file converter.
    by Zerkman / Sector One
------------------------------------------------------------------- */

/* This program is free software. It comes without any warranty, to
* the extent permitted by applicable law. You can redistribute it
* and/or modify it under the terms of the Do What The Fuck You Want
* To Public License, Version 2, as published by Sam Hocevar. See
* http://sam.zoy.org/wtfpl/COPYING for more details. */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__PUREC__) && !defined(_COMPILER_H)
#include <tos.h>
#else
#include <osbind.h>
#endif

#include "pixbuf.h"


static long xinc0(long c)
{
	return c == 15 ? 88 : c == 31 ? 12 : c == 37 ? 100 : 4;
}

static long xinc1(long c)
{
	return c & 1 ? 16 : 4;
}

static long xinc2(long c)
{
	(void)c;
	return 8;
}

static long xinc3(long c)
{
	return c == 15 ? 112 : c == 31 ? 12 : c == 37 ? 100 : 4;
}

static ModeD const modes[4] = {
	{ 0, 54, 0, 1, 33, xinc0, 320, 199 },
	{ 1, 48, 0, 1, 9, xinc1, 320, 199 },
	{ 2, 56, 0, 1, 4, xinc2, 320, 199 },
	{ 3, 54, 6, 0, 69, xinc3, 416, 273 }
};

static uint32_t read32(const void *ptr)
{
	const unsigned char *p = ptr;

	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}


PixBuf *decode_mpp(const ModeD *mode, long ste, long extra, int in)
{
	long bits = 3 * (3 + ste + extra);
	long palette_size = (mode->ncolors - mode->nfixed) * mode->height;
	long packed_palette_size =
		((bits * (mode->ncolors - mode->nfixed - mode->border0 * 2) * mode->height + 15) / 8) & -2;
	long pal_buf_offset = palette_size * sizeof(Pixel) - packed_palette_size;
	long image_size = mode->width / 2 * mode->height;
	Pixel *pal = malloc(palette_size * sizeof(Pixel));
	unsigned char *img = malloc(image_size);
	unsigned char *pal_buf = (unsigned char *) pal + pal_buf_offset;
	long bitbuf = 0;
	long bbnbits = 0;
	long i;

	{
		if (pal == NULL ||
			img == NULL ||
			Fread(in, packed_palette_size, pal_buf) != packed_palette_size ||
			Fread(in, image_size, img) != image_size)
		{
			free(img);
			free(pal);
			return NULL;
		}

		for (i = 0; i < palette_size; i++)
		{
			long x = i % mode->ncolors;

			if (mode->border0 && (x == 0 || x == ((mode->ncolors - 1) & -16)))
			{
				pal[i].rgb = 0;
				continue;
			}
			while (bbnbits < bits)
			{
				bitbuf = (bitbuf << 8) | *pal_buf++;
				bbnbits += 8;
			}

			bbnbits -= bits;

			{
				unsigned short c = (bitbuf >> bbnbits) & ((1 << bits) - 1);
				Pixel p;

				p.cp.a = 0;
				switch ((int)bits)
				{
				case 9:
					p.cp.r = (c >> 6);
					p.cp.g = (c >> 3) & 7;
					p.cp.b = c & 7;
					break;
				case 12:
				case 15:
					p.cp.r = ((c >> 7) & 0xe) | ((c >> 11) & 1);
					p.cp.g = ((c >> 3) & 0xe) | ((c >> 7) & 1);
					p.cp.b = ((c << 1) & 0xe) | ((c >> 3) & 1);
					if (bits == 15)
					{
						p.cp.r = (p.cp.r << 1) | ((c >> 14) & 1);
						p.cp.g = (p.cp.g << 1) | ((c >> 13) & 1);
						p.cp.b = (p.cp.b << 1) | ((c >> 12) & 1);
					}
					break;
				default:
					/* printf("error at 'switch (bits)'"); */
					free(img);
					free(pal);
					return NULL;
				}
				p.cp.r <<= 8 - (bits / 3);
				p.cp.g <<= 8 - (bits / 3);
				p.cp.b <<= 8 - (bits / 3);
				pal[i] = p;
			}
		}
	}


	{
		PixBuf *pix = pixbuf_new(mode->width, mode->height);
		long l;
		long k;
		long x;
		long y;
		unsigned short b0;
		unsigned short b1;
		unsigned short b2;
		unsigned short b3;
		Pixel palette[16];

		if (pix != NULL)
		{
			memset(pix->array, 0, mode->width * mode->height * sizeof(Pixel));
	
			l = 0;
			k = 0;
			b0 = b1 = b2 = b3 = 0;
	
			memset(palette, 0, sizeof(palette));
	
			for (y = 0; y < mode->height; ++y)
			{
				Pixel *ppal = pal + y * (mode->ncolors - mode->nfixed);
				long nextx = mode->x0;
				long nextc = 0;
	
				for (i = mode->nfixed; i < 16; ++i)
					palette[i] = *ppal++;
	
				for (x = 0; x < mode->width; ++x)
				{
					if (x == nextx)
					{
						palette[nextc & 0xf] = *ppal++;
						nextx += mode->xinc(nextc);
						++nextc;
					}
					if ((x & 0xf) == 0)
					{
						b0 = (img[k + 0] << 8) | (img[k + 1]);
						b1 = (img[k + 2] << 8) | (img[k + 3]);
						b2 = (img[k + 4] << 8) | (img[k + 5]);
						b3 = (img[k + 6] << 8) | (img[k + 7]);
						k += 8;
					}
					i = ((b3 >> 12) & 8) | ((b2 >> 13) & 4) | ((b1 >> 14) & 2) | ((b0 >> 15) & 1);
					pix->array[l++] = palette[i];
					b0 <<= 1;
					b1 <<= 1;
					b2 <<= 1;
					b3 <<= 1;
				}
			}
		}
		free(img);
		free(pal);

		return pix;
	}
}


#if 0
void mpp2bmp(const char *_mppfilename, int _mode, long ste, long extra, int doubl, const char *bmpfilename)
{
	const ModeD *mode = &modes[_mode];
	int in = (int)Fopen(_mppfilename, FO_READ);

	PixBuf *pix = decode_mpp(mode, ste, extra, in);

	if (doubl)
	{
		PixBuf *pix2 = decode_mpp(mode, ste, extra, in);
		long k;

		for (k = 0; k < mode->width * mode->height; ++k)
		{
			Pixel a = pix->array[k];
			Pixel b = pix2->array[k];

			a.cp.r = ((int) a.cp.r + b.cp.r) / 2;
			a.cp.g = ((int) a.cp.g + b.cp.g) / 2;
			a.cp.b = ((int) a.cp.b + b.cp.b) / 2;
			pix->array[k] = a;
		}
		pixbuf_delete(pix2);
	}

	Fclose(in);

	pixbuf_export_bmp(pix, bmpfilename);
	pixbuf_delete(pix);
}
#endif

