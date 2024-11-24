/* -------------------------------------------------------------------
 Management of a basic pixel buffer with export to BMP file format.
 by Zerkman / Sector One
------------------------------------------------------------------- */

/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */

#ifndef _PIXBUF_H_
#define _PIXBUF_H_

#if !defined(__ZVIEW_IMGINFO_H__)
#if defined(__PUREC__) && !defined(_COMPILER_H)
/*
 * far from being a complete replacement for <stdint.h>,
 * but good enough for our purposes
 */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed long int32_t;
typedef unsigned long uint32_t;
typedef long intptr_t;
#define __CDECL cdecl
#else
#include <stdint.h>
#endif
#endif

typedef union pixel
{
	uint32_t rgb;
	struct
	{
		uint8_t a, r, g, b;
	} cp;
} Pixel;

typedef struct _pixbuf
{
	long width;
	long height;
	Pixel *array;
	void *_ptr;
} PixBuf;

typedef struct
{
	long id;
	long ncolors;
	long nfixed;
	long border0;
	long x0;
	long (*xinc)(long);
	long width;
	long height;
} ModeD;


PixBuf *pixbuf_new(long width, long height);
void pixbuf_delete(PixBuf * pb);
void pixbuf_export_bmp(const PixBuf * pb, const char *filename);
PixBuf *decode_mpp(const ModeD *mode, long ste, long extra, int in);
void mpp2bmp(const char *_mppfilename, int _mode, long ste, long extra, int doubl, const char *bmpfilename);

#endif
