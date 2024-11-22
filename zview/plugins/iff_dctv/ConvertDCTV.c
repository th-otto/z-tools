/*
 * Code originally written by cholok
 * https://drive.google.com/file/d/1ejrHgC4ZeTjMRe4CYqaeftedBJY-6PjZ/view?usp%3Dsharing
 */

#include <stdint.h>
#include <stdlib.h>
#include "dctv.h"
#include "dctvtabs.h"

#ifndef FALSE
#  define FALSE 0
#  define TRUE 1
#endif

#define ASM_NAME(x) __asm__(x)
void c2p(uint8_t *chunky, uint8_t **planetab, uint16_t linesize) ASM_NAME("c2p");
void p2c(uint8_t *chunky, struct BitMap *bitmap, uint16_t linesize) ASM_NAME("p2c");


int16_t minmax(int16_t pix, int16_t min, int16_t max)
{
	if (pix < min)
		pix = min;
	else if (pix > max)
		pix = max;

	return pix;
}


static void pal2direct(struct DCTVCvtHandle *cvt)
{
	int16_t cnt = cvt->Width;
	uint8_t *buf = cvt->Chunky;

	while (cnt > 0)
	{
		*buf = cvt->Palette[*buf];		/* pixel=palette[pixel] */
		buf++;
		cnt--;
	}
}


static void join(uint8_t *chunky, uint8_t *lbuf, int16_t sw, int16_t zzflag)
{
	int16_t cnt;
	int8_t pix;

	cnt = (sw >> 1) - 1 - zzflag;
	chunky += zzflag + 1;

	*lbuf++ = 0x40;
	*lbuf++ = 0x40;
	if (zzflag)
	{
		*lbuf++ = 0x40;
		*lbuf++ = 0x40;
	}
	/*  read chunky */
	/*  $0054xxyy...zz0000 zzflag=1 */
	/*  $14xxyy.......zz00 zzflag=0 */
	/*  write composite */
	/*  $40404040xyxy...zz */
	/*  $4040xyxy.......zz */

	while (cnt > 0)
	{
		pix = *chunky++;				/* %0x0x0x0x (4 bit value) */
		pix <<= 1;
		pix |= *chunky++;				/* %xyxyxyxy (8 bit value) */
		if (!pix)
			pix = 0x40;					/* luma=0 */
		*lbuf++ = pix;
		*lbuf++ = pix;					/* store 2 8-bit pixels */
		cnt--;
	}
}


static void chroma(uint8_t *lbuf, int8_t *cbuf, int16_t sw, int16_t zzflag)
{
	int16_t cnt;
	int16_t c1;
	int16_t c2;
	int16_t c3;
	int8_t sign = 0;

	cbuf += zzflag;
	lbuf += zzflag + 1;

	/*  $40404040xxxx...zz zzflag=1 */
	/*  $4040xxxx.......zz zzflag=0 */
	/*  read composite */
	/*  $0000xx00yy...zz00 */
	/*  $00xx00yy.....00zz */
	/*  write chroma */
	/*  $00xxyy...zz00 */
	/*  $xxyy.....zz00 */

	cnt = (sw >> 1) - 1 - zzflag;

	c3 = *lbuf;
	c2 = 0x40;

	while (cnt > 0)
	{
		c1 = c2;
		c2 = c3;
		lbuf += 2;

		c3 = *lbuf;						/* get composite */

		if ((c1 = c2 + c2 - c1 - c3 + 2) < 0)
			c1 += 3;

		c1 >>= 2;

		if (sign ^= 1)
			c1 = -c1;

		*cbuf++ = (int8_t) minmax(c1, -127, 127);

		cnt--;
	}
}


static void luma(uint8_t *lbuf, int8_t *cbuf, int16_t sw, int16_t zzflag)
{
	int16_t cnt;
	int16_t c1;
	int16_t c2;
	int16_t c3;
	int16_t c4;
	int16_t c5;
	int16_t c6;
	int16_t c7;
	int8_t sign = 0;

	cbuf += zzflag;
	lbuf += zzflag + 1;

	/*  $40404040xxxx...zz zzflag=1 */
	/*  $4040xxxx.......zz zzflag=0 */
	/*  read composite */
	/*  $0000xx00yy...zz00 */
	/*  $00xx00yy.....00zz */
	/*  write luma */
	/*  $0000xxxxyy...zz00 */
	/*  $00xxxxyy.....zz00 */
	/*  write chroma */
	/*  $00xxyy...zz00 */
	/*  $xxyy.....zz00 */

	cnt = (sw >> 1) - 1 - zzflag;

	c1 = c2 = c3 = c4 = 0;
	c5 = *cbuf++;
	c6 = *cbuf++;
	c7 = *cbuf++;

	while (cnt > 0)
	{
		c1 = c2;
		c2 = c3;
		c3 = c4;
		c4 = c5;
		c5 = c6;
		c6 = c7;
		c7 = 0;
		if (cnt >= 4)
			c7 = *cbuf++;

		c1 = (c1 + c2 + c3 + c4 + c4 + c5 + c6 + c7 + 4) >> 3;

		if (sign ^= 1)
			c1 = -c1;

		c1 = minmax(*lbuf - c1, 64, 224);

		*lbuf++ = c1;
		*lbuf++ = c1;

		cnt--;
	}

	if (!zzflag)
		lbuf--;

	*lbuf = 0x40;
}


static void yvu2rgb(uint8_t *lbuf, int8_t *vbuf, int8_t *ubuf, struct DCTVCvtHandle *cvt)
{
	int16_t cnt;
	int16_t d0;
	int16_t luma;
	int16_t v;
	int16_t u;
	uint8_t *rbuf;
	uint8_t *gbuf;
	uint8_t *bbuf;

	cnt = cvt->Width >> 1;
	rbuf = cvt->Red;
	gbuf = cvt->Green;
	bbuf = cvt->Blue;

	while (cnt > 0)
	{
		v = *vbuf++;
		u = *ubuf++;

		luma = tables[*lbuf++];

		d0 = tables[v + 0x180] + luma;
		*rbuf++ = minmax(d0 >> 4, 0, 255);

		d0 = tables[v + 0x380] + (int16_t) tables[u + 0x480] + luma;
		*gbuf++ = minmax(d0 >> 4, 0, 255);

		d0 = tables[u + 0x280] + luma;
		*bbuf++ = minmax(d0 >> 4, 0, 255);

		luma = (int16_t) tables[*lbuf++];

		d0 = tables[v + 0x180] + luma;
		*rbuf++ = minmax(d0 >> 4, 0, 255);

		d0 = tables[v + 0x380] + (int16_t) tables[u + 0x480] + luma;
		*gbuf++ = minmax(d0 >> 4, 0, 255);

		d0 = tables[u + 0x280] + luma;
		*bbuf++ = minmax(d0 >> 4, 0, 255);

		cnt--;
	}
}


static void grey(uint8_t *lbuf, int16_t sw, struct DCTVCvtHandle *cvt)
{
	int16_t cnt;
	int16_t luma;
	uint8_t *rbuf;
	uint8_t *gbuf;
	uint8_t *bbuf;

	cnt = cvt->Width;
	rbuf = cvt->Red;
	gbuf = cvt->Green;
	bbuf = cvt->Blue;

	while (cnt > 0)
	{
		luma = tables[*lbuf++];
		luma = minmax(luma >> 4, 0, 255);

		*rbuf++ = luma;
		*gbuf++ = luma;
		*bbuf++ = luma;

		cnt--;
	}
}


static void filter(uint8_t *rgbbuf, int16_t sw)
{
	int16_t cnt;
	uint8_t d1, d2, d3;
	uint8_t *buf;

	cnt = sw - 3;
	buf = rgbbuf + 3;

	d2 = rgbbuf[1];
	d3 = rgbbuf[2];

	while (cnt > 0)
	{
		d1 = d2;
		d2 = d3;
		d3 = *buf++;
		*rgbbuf++ = (d1 + d2 + d2 + d3) >> 2;
		cnt--;
	}

	*rgbbuf++ = (d2 + d3 + d3) >> 2;
	*rgbbuf++ = d3 >> 2;
	*rgbbuf = 0;
}


static void dc2rgb(struct DCTVCvtHandle *cvt, uint16_t linenr)
{
	uint8_t *lbuf;
	int8_t *cbuf;
	int8_t *vbuf;
	int8_t *ubuf;
	int16_t sw;
	int16_t mode;
	int16_t field;
	int16_t zzflag;

	/* plannar to chunky */
	p2c(cvt->Chunky, cvt->BitMap, linenr);

	/* palette to direct value */
	pal2direct(cvt);

	sw = cvt->Width;

	if (cvt->Lace)
	{
		field = (linenr + 1) & 1;
		linenr >>= 1;
	} else
	{
		field = 0;
	}

	/*  line0    signature */
	/*  line1    $0054... zzflag=1 odd */
	/*  line2    $14..... zzflag=0 even */

	zzflag = linenr & 1;

	{
		uint8_t cb1 = cvt->Chunky[0];
		uint8_t cb2 = cvt->Chunky[1];

		if (cb1 || cb2)
			mode = 1;					/* color */
		else
			mode = 0;					/* grey */
	}

	if (field == 0)
	{
		lbuf = cvt->FBuf1[0];
		vbuf = (int8_t *)cvt->FBuf1[1];
		ubuf = (int8_t *)cvt->FBuf1[2];
	} else
	{
		lbuf = cvt->FBuf2[0];
		vbuf = (int8_t *)cvt->FBuf2[1];
		ubuf = (int8_t *)cvt->FBuf2[2];
	}

	if (zzflag)
		cbuf = vbuf;
	else
		cbuf = ubuf;

	/* join two 4-bit pixels into one 8-bit */
	join(cvt->Chunky, lbuf, sw, zzflag);

	if (mode)
	{
		/* decode chroma */
		chroma(lbuf, cbuf, sw, zzflag);

		/* decode luma */
		luma(lbuf, cbuf, sw, zzflag);

		/* convert yuv to rgb */
		yvu2rgb(lbuf, vbuf, ubuf, cvt);
	} else
	{
		grey(lbuf, sw, cvt);
	}

	/* filtering for removing artifacts */
	filter(cvt->Red, sw);
	filter(cvt->Green, sw);
	filter(cvt->Blue, sw);
}


static void blank(struct DCTVCvtHandle *cvt, int16_t offset, int16_t size)
{
	uint8_t *red;
	uint8_t *green;
	uint8_t *blue;

	red = cvt->Red + offset;
	green = cvt->Green + offset;
	blue = cvt->Blue + offset;

	while (size > 0)
	{
		*red++ = 0;
		*green++ = 0;
		*blue++ = 0;
		size--;
	}
}


/*	Convert one line of DCTV image to 24-bit RGB.
	Zigzag column must be first column in the DCTV image
*/
void ConvertDCTVLine(struct DCTVCvtHandle *cvt)
{
	uint16_t line;

	line = cvt->LineNum;				/*  from 0 to sh-1 */

	/*  first line or two (lace) is signature */
	if (line > cvt->Lace && line < cvt->Height)
	{
		dc2rgb(cvt, line);
		blank(cvt, 0, 4);				/* blank first 4 pixels */
		blank(cvt, cvt->Width - 4, 4);	/* blank last 4 pixels (2 more) */
	} else
	{
		blank(cvt, 0, cvt->Width);		/* blank full line (signature) */
	}

	cvt->LineNum++;
}


/*	Check for DCTV signature.
	Sinature typically is in highest plane.
	Has 32 bytes data, so min width of image is 32*8=256 pixels.
	Max of depth for DCTV image is 4, but dctv.lib allow to 8.
	( planes > 4 must be cleared ).
*/
int CheckDCTV(struct BitMap *bmp)
{
	uint8_t sign[] = { 0xD5, 0xAB, 0x57, 0xAF, 0x5F, 0xBF, 0x7F, 0xFF };
	uint8_t buf[32];
	uint8_t *plane;
	int16_t np;
	int16_t bpr;
	int16_t i;
	int16_t j;
	int16_t k;
	int16_t d0;
	int16_t d1;
	int16_t d2;

	np = bmp->Depth;
	bpr = bmp->BytesPerRow;

	if (bpr < 32 || np > 4)
		return FALSE;

	while (np > 0)
	{
		np--;

		/*  find first nonzero byte */
		plane = bmp->Planes[np];
		k = bpr - 31;
		while (*plane == 0 && k != 0)
		{
			plane++;
			k--;
		}
		if (k == 0)
			continue;

		/*  find nonzero hi bit */
		d0 = *plane;
		k = 7;
		while ((d0 & 0x80) == 0)
		{								/* d0 nonzero value */
			d0 <<= 1;
			k -= 1;
		}

		/*  generate signature */
		d1 = sign[k];
		for (i = 0; i < 32; i++)
		{
			d2 = 0;
			for (j = 0; j < 8; j++)
			{
				d0 = (d1 & 0xC3) + 0x41;
				d0 = (d0 & 0x82) + 0x7E;
				d0 = (d0 >> 7) & 1;		/* test bit 7 */
				d1 = d1 + d1 + d0;
				d2 = d2 + d2 + 1 - d0;
			}
			buf[i] = d2;
		}
		if (k == 7)
			buf[31] &= 0xFE;

		/*  compare signatures */
		for (i = 0; i < 31; i++)
		{
			d0 = *plane++;
			d1 = buf[i];
			if (d0 != d1)
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}


/*	This function convert 12-bit Amiga palette
	to special DCTV values (Digital RGBI).
	Only 4 bits of 12 are used.
	Standard palettes are:
	for 4 planes
	0x000,0x786,0x778,0x788,0x876,0x886,0x878,0x888
	0x001,0x787,0x779,0x789,0x877,0x887,0x879,0x889
	for 3 planes
	0x000,      0x778,      0x876,      0x878
	0x001,      0x779,      0x877,      0x879

*/
void SetmapDCTV(struct DCTVCvtHandle *cvt, const uint16_t *cmap)
{
	uint8_t cnt;
	uint8_t val;
	uint8_t *pal;
	uint16_t col;

	pal = cvt->Palette;
	cnt = 1 << cvt->Depth;

	while (cnt > 0)
	{
		col = *cmap++;
		val = 0;
		if (col & 1)
			val = 64;					/*  DI */
		if (col & 0x800)
			val |= 16;					/*  DR */
		if (col & 8)
			val |= 4;					/*  DB */
		if (col & 0x80)
			val |= 1;					/*  DG */
		*pal++ = val;
		cnt--;
	}
}


/*	Allocate structure and some buffers for handle conversion.
	For free use FreeVec( cvt )
*/
struct DCTVCvtHandle *AllocDCTV(struct BitMap *bmp, int interlace)
{
	struct DCTVCvtHandle *cvt;
	int16_t sw;
	int16_t sh;
	int16_t np;
	int16_t fields;
	int16_t i;
	uint8_t *buf;

	if (!bmp)
		return NULL;

	sw = bmp->BytesPerRow << 3;
	sh = bmp->Rows;
	np = bmp->Depth;
	interlace &= 1;							/*  1=lace, 0=nonlace */
	fields = interlace + 1;

	if (sw < 256 || sh < (2 * fields) || np > 4)
		return NULL;

	if ((cvt = calloc(sizeof(struct DCTVCvtHandle) + (sw * 10), 1)) != NULL)
	{
		cvt->BitMap = bmp;
		cvt->Width = sw;
		cvt->Height = sh;
		cvt->Depth = np;
		cvt->Lace = interlace;

		cvt->Red = (uint8_t *) cvt + sizeof(struct DCTVCvtHandle);
		cvt->Green = cvt->Red + sw;
		cvt->Blue = cvt->Green + sw;
		cvt->Chunky = cvt->Blue + sw;

		buf = cvt->Chunky + sw;

		for (i = 0; i < 3; i++)
		{
			cvt->FBuf1[i] = buf + sw * i;	/*  first field */
			cvt->FBuf2[i] = buf + sw * (i + 3);	/*  second field */
		}
	}
	return cvt;
}
