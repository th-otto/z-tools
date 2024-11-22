/*
 * Code originally written by cholok
 * https://drive.google.com/file/d/1ejrHgC4ZeTjMRe4CYqaeftedBJY-6PjZ/view?usp%3Dsharing
 */

#include "dctv.h"
#include "dctvtabs.h"

void c2p(uint8_t *chunky, uint8_t **planetab, uint16_t linesize) ASM_NAME("c2p");
void p2c(uint8_t *chunky, struct BitMap *bitmap, uint16_t linesize) ASM_NAME("p2c");

#ifdef __GNUC__
void p2c(uint8_t *Chunky, struct BitMap *bitmap, uint16_t linesize)
{
	register uint32_t BytesPerRow __asm__("d0") = bitmap->BytesPerRow;
	register uint32_t sw __asm__("d1") = linesize;
	register uint32_t depth __asm__("d2") = bitmap->Depth;
	register void *chunky __asm__("a0") = Chunky;
	register uint8_t **planetab __asm__("a1") = bitmap->Planes;

	__asm__ __volatile__(
	" mulu.w     %%d0,%%d1\n"	/* offset=line*bpr */
	" asl.l      #3,%%d0\n"	/* width=bpr*8 */
	" lea.l      p2coffs(%%pc),%%a2\n"
	" add.l      %%d2,%%d2\n"
	" move.w     -2(%%a2,%%d2.l),%%d2\n"
	" add.l      %%d2,%%a2\n"
	" jsr        (%%a2)\n"
	" bra 1f\n"

"p2coffs:\n"
	" .dc.w p2c1-p2coffs\n"
	" .dc.w p2c2-p2coffs\n"
	" .dc.w p2c3-p2coffs\n"
	" .dc.w p2c4-p2coffs\n"

/* --------------------------------------------------------------------------- */

/* a0=chunky a1=plane[] d1=sw */
"p2c1:\n"
	" movea.l    %%a0,%%a4\n"          /* chunky */
	" lea.l      0(%%a4,%%d0.l),%%a5\n"  /* end of chunky */
	" movem.l    (%%a1),%%a0\n"	      /* planes */
	" adda.l     %%d1,%%a0\n"
	" move.l     #0x0F0F0F0F,%%d5\n" /* d5 = constant 0x0f0f0f0f */
	" move.l     #0x55555555,%%d6\n" /* d6 = constant 0x55555555 */
	" move.l     #0x3333CCCC,%%d7\n" /* d7 = constant 0x3333cccc */
"p2c1_1:\n"
	" moveq.l    #0,%%d0\n" /* get next chunky pixel in d0 */
	" move.b     (%%a0)+,%%d0\n"
#ifdef __mcoldfire__
	" moveq      #24,%%d2\n"
	" lsl.l      %%d2,%%d0\n"
#else
	" ror.l      #8,%%d0\n"
#endif

	" move.l     %%d0,%%d2\n"
	" and.l      %%d5,%%d2\n"
	" eor.l      %%d2,%%d0\n"
	" lsr.l      #4,%%d0\n"
	" move.l     %%d2,%%d3\n"
	" and.l      %%d7,%%d3\n"
	" swap       %%d2\n"
	" and.l      %%d7,%%d2\n"
#ifdef __mcoldfire__
	/*
	xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx 
	xxxxxxxx xxxxxxxx 00xxxxxx xxxxxxxx
	xxxxxxxx xxxxxx00 xxxxxxxx xxxxxx00
	xxxxxxxx xxxxxx00 00xxxxxx xxxxxxxx
	*/
	" move.l     %%d2,%%d4\n"
	" lsr.l      #2,%%d4\n"
	" lsl.l      #2,%%d2\n"
	" and.l      #0xfffcffff,%%d2\n"
	" and.l      #0xffffcfff,%%d4\n"
	" move.w     %%d4,%%d2\n"
#else
	" lsr.w      #2,%%d2\n"
	" lsl.l      #2,%%d2\n"
	" lsr.w      #2,%%d2\n"
#endif
	" or.l       %%d2,%%d3\n"
	" move.l     %%d0,%%d1\n"
	" and.l      %%d7,%%d1\n"
	" swap       %%d0\n"
	" and.l      %%d7,%%d0\n"
#ifdef __mcoldfire__
	" move.l     %%d0,%%d4\n"
	" lsr.l      #2,%%d4\n"
	" lsl.l      #2,%%d0\n"
	" and.l      #0xfffcffff,%%d0\n"
	" and.l      #0xffffcfff,%%d4\n"
	" move.w     %%d4,%%d0\n"
#else
	" lsr.w      #2,%%d0\n"
	" lsl.l      #2,%%d0\n"
	" lsr.w      #2,%%d0\n"
#endif
	" or.l       %%d0,%%d1\n"
	" move.l     %%d1,%%d2\n"
	" lsr.l      #8,%%d2\n"
	" move.l     %%d1,%%d0\n"
	" and.l      %%d6,%%d0\n"
	" eor.l      %%d0,%%d1\n"
	" move.l     %%d2,%%d4\n"
	" and.l      %%d6,%%d4\n"
	" eor.l      %%d4,%%d2\n"
	" lsr.l      #1,%%d2\n"
	" or.l       %%d2,%%d1\n"
	" lsl.l      #1,%%d0\n"
	" or.l       %%d4,%%d0\n"
	" move.b     %%d1,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" swap       %%d0\n"
	" swap       %%d1\n"
	" move.b     %%d1,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" move.l     %%d3,%%d2\n"
	" lsr.l      #8,%%d2\n"
	" move.l     %%d3,%%d0\n"
	" and.l      %%d6,%%d0\n"
	" eor.l      %%d0,%%d3\n"
	" move.l     %%d2,%%d4\n"
	" and.l      %%d6,%%d4\n"
	" eor.l      %%d4,%%d2\n"
	" lsr.l      #1,%%d2\n"
	" or.l       %%d2,%%d3\n"
	" lsl.l      #1,%%d0\n"
	" or.l       %%d4,%%d0\n"
	" move.b     %%d3,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" swap       %%d0\n"
	" swap       %%d3\n"
	" move.b     %%d3,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" cmpa.l     %%a4,%%a5\n"
	" bgt.w      p2c1_1\n"
	" rts\n"

/* --------------------------------------------------------------------------- */

/* a0=chunky a1=plane[] d1=sw */
"p2c2:\n"
	" movea.l    %%a0,%%a4\n"
	" lea.l      0(%%a4,%%d0.l),%%a5\n"
	" movem.l    (%%a1),%%a0-%%a1\n"	      /* planes */
	" adda.l     %%d1,%%a0\n"
	" adda.l     %%d1,%%a1\n"
	" move.l     #0x0F0F0F0F,%%d5\n" /* d5 = constant 0x0f0f0f0f */
	" move.l     #0x55555555,%%d6\n" /* d6 = constant 0x55555555 */
	" move.l     #0x3333CCCC,%%d7\n" /* d7 = constant 0x3333cccc */
"p2c2_1:\n"
	" moveq.l    #0,%%d0\n" /* get next 2 chunky pixels in d0 */
	" move.b     (%%a0)+,%%d0\n"
#ifdef __mcoldfire__
	" lsl.l      #8,%%d0\n"
#else
	" lsl.w      #8,%%d0\n"
#endif
	" move.b     (%%a1)+,%%d0\n"
	" swap       %%d0\n"

	" move.l     %%d0,%%d2\n"
	" and.l      %%d5,%%d2\n"
	" eor.l      %%d2,%%d0\n"
	" lsr.l      #4,%%d0\n"
	" move.l     %%d2,%%d3\n"
	" and.l      %%d7,%%d3\n"
	" swap       %%d2\n"
	" and.l      %%d7,%%d2\n"
#ifdef __mcoldfire__
	" move.l     %%d2,%%d4\n"
	" lsr.l      #2,%%d4\n"
	" lsl.l      #2,%%d2\n"
	" and.l      #0xfffcffff,%%d2\n"
	" and.l      #0xffffcfff,%%d4\n"
	" move.w     %%d4,%%d2\n"
#else
	" lsr.w      #2,%%d2\n"
	" lsl.l      #2,%%d2\n"
	" lsr.w      #2,%%d2\n"
#endif
	" or.l       %%d2,%%d3\n"
	" move.l     %%d0,%%d1\n"
	" and.l      %%d7,%%d1\n"
	" swap       %%d0\n"
	" and.l      %%d7,%%d0\n"
#ifdef __mcoldfire__
	" move.l     %%d0,%%d4\n"
	" lsr.l      #2,%%d4\n"
	" lsl.l      #2,%%d0\n"
	" and.l      #0xfffcffff,%%d0\n"
	" and.l      #0xffffcfff,%%d4\n"
	" move.w     %%d4,%%d0\n"
#else
	" lsr.w      #2,%%d0\n"
	" lsl.l      #2,%%d0\n"
	" lsr.w      #2,%%d0\n"
#endif
	" or.l       %%d0,%%d1\n"
	" move.l     %%d1,%%d2\n"
	" lsr.l      #8,%%d2\n"
	" move.l     %%d1,%%d0\n"
	" and.l      %%d6,%%d0\n"
	" eor.l      %%d0,%%d1\n"
	" move.l     %%d2,%%d4\n"
	" and.l      %%d6,%%d4\n"
	" eor.l      %%d4,%%d2\n"
	" lsr.l      #1,%%d2\n"
	" or.l       %%d2,%%d1\n"
	" lsl.l      #1,%%d0\n"
	" or.l       %%d4,%%d0\n"
	" move.b     %%d1,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" swap       %%d0\n"
	" swap       %%d1\n"
	" move.b     %%d1,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" move.l     %%d3,%%d2\n"
	" lsr.l      #8,%%d2\n"
	" move.l     %%d3,%%d0\n"
	" and.l      %%d6,%%d0\n"
	" eor.l      %%d0,%%d3\n"
	" move.l     %%d2,%%d4\n"
	" and.l      %%d6,%%d4\n"
	" eor.l      %%d4,%%d2\n"
	" lsr.l      #1,%%d2\n"
	" or.l       %%d2,%%d3\n"
	" lsl.l      #1,%%d0\n"
	" or.l       %%d4,%%d0\n"
	" move.b     %%d3,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" swap       %%d0\n"
	" swap       %%d3\n"
	" move.b     %%d3,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" cmpa.l     %%a4,%%a5\n"
	" bgt.w      p2c2_1\n"
	" rts\n"

/* --------------------------------------------------------------------------- */

/* a0=chunky a1=plane[] d1=sw */
"p2c3:\n"
	" movea.l    %%a0,%%a4\n"
	" lea.l      0(%%a4,%%d0.l),%%a5\n"
	" movem.l    (%%a1),%%a0-%%a2	      /* planes */\n"
	" adda.l     %%d1,%%a0\n"
	" adda.l     %%d1,%%a1\n"
	" adda.l     %%d1,%%a2\n"
	" move.l     #0x0F0F0F0F,%%d5\n" /* d5 = constant 0x0f0f0f0f */
	" move.l     #0x55555555,%%d6\n" /* d6 = constant 0x55555555 */
	" move.l     #0x3333CCCC,%%d7\n" /* d7 = constant 0x3333cccc */
"p2c3_1:\n"
	" moveq.l    #0,%%d0\n" /* get next 3 chunky pixels in d0 */
#ifdef __mcoldfire__
	" move.b     (%%a0)+,%%d0\n"
	" lsl.l      #8,%%d0\n"
	" move.b     (%%a1)+,%%d0\n"
	" swap       %%d0\n"
	" moveq.l    #0,%%d2\n"
	" move.b     (%%a2)+,%%d2\n"
	" lsl.l      #8,%%d2\n"
	" move.w     %%d2,%%d0\n"
#else
	" move.b     (%%a0)+,%%d0\n"
	" lsl.w      #8,%%d0\n"
	" move.b     (%%a1)+,%%d0\n"
	" swap       %%d0\n"
	" move.b     (%%a2)+,%%d0\n"
	" lsl.w      #8,%%d0\n"
#endif

	" move.l     %%d0,%%d2\n"
	" and.l      %%d5,%%d2\n"
	" eor.l      %%d2,%%d0\n"
	" lsr.l      #4,%%d0\n"
	" move.l     %%d2,%%d3\n"
	" and.l      %%d7,%%d3\n"
	" swap       %%d2\n"
	" and.l      %%d7,%%d2\n"
#ifdef __mcoldfire__
	" move.l     %%d2,%%d4\n"
	" lsr.l      #2,%%d4\n"
	" lsl.l      #2,%%d2\n"
	" and.l      #0xfffcffff,%%d2\n"
	" and.l      #0xffffcfff,%%d4\n"
	" move.w     %%d4,%%d2\n"
#else
	" lsr.w      #2,%%d2\n"
	" lsl.l      #2,%%d2\n"
	" lsr.w      #2,%%d2\n"
#endif
	" or.l       %%d2,%%d3\n"
	" move.l     %%d0,%%d1\n"
	" and.l      %%d7,%%d1\n"
	" swap       %%d0\n"
	" and.l      %%d7,%%d0\n"
#ifdef __mcoldfire__
	" move.l     %%d0,%%d4\n"
	" lsr.l      #2,%%d4\n"
	" lsl.l      #2,%%d0\n"
	" and.l      #0xfffcffff,%%d0\n"
	" and.l      #0xffffcfff,%%d4\n"
	" move.w     %%d4,%%d0\n"
#else
	" lsr.w      #2,%%d0\n"
	" lsl.l      #2,%%d0\n"
	" lsr.w      #2,%%d0\n"
#endif
	" or.l       %%d0,%%d1\n"
	" move.l     %%d1,%%d2\n"
	" lsr.l      #8,%%d2\n"
	" move.l     %%d1,%%d0\n"
	" and.l      %%d6,%%d0\n"
	" eor.l      %%d0,%%d1\n"
	" move.l     %%d2,%%d4\n"
	" and.l      %%d6,%%d4\n"
	" eor.l      %%d4,%%d2\n"
	" lsr.l      #1,%%d2\n"
	" or.l       %%d2,%%d1\n"
	" lsl.l      #1,%%d0\n"
	" or.l       %%d4,%%d0\n"
	" move.b     %%d1,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" swap       %%d0\n"
	" swap       %%d1\n"
	" move.b     %%d1,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" move.l     %%d3,%%d2\n"
	" lsr.l      #8,%%d2\n"
	" move.l     %%d3,%%d0\n"
	" and.l      %%d6,%%d0\n"
	" eor.l      %%d0,%%d3\n"
	" move.l     %%d2,%%d4\n"
	" and.l      %%d6,%%d4\n"
	" eor.l      %%d4,%%d2\n"
	" lsr.l      #1,%%d2\n"
	" or.l       %%d2,%%d3\n"
	" lsl.l      #1,%%d0\n"
	" or.l       %%d4,%%d0\n"
	" move.b     %%d3,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" swap       %%d0\n"
	" swap       %%d3\n"
	" move.b     %%d3,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" cmpa.l     %%a4,%%a5\n"
	" bgt        p2c3_1\n"
	" rts\n"

/* --------------------------------------------------------------------------- */

/* a0=chunky a1=plane[] d1=sw */
"p2c4:\n"
	" movea.l    %%a0,%%a4\n"
	" lea.l      0(%%a4,%%d0.l),%%a5\n"
	" movem.l    (%%a1),%%a0-%%a3	      /* planes */\n"
	" adda.l     %%d1,%%a0\n"
	" adda.l     %%d1,%%a1\n"
	" adda.l     %%d1,%%a2\n"
	" adda.l     %%d1,%%a3\n"
	" move.l     #0x0F0F0F0F,%%d5\n" /* d5 = constant 0x0f0f0f0f */
	" move.l     #0x55555555,%%d6\n" /* d6 = constant 0x55555555 */
	" move.l     #0x3333CCCC,%%d7\n" /* d7 = constant 0x3333cccc */
"p2c4_1:\n"
#ifdef __mcoldfire__
	" move.b     (%%a0)+,%%d0\n" /* get next 4 chunky pixels in d0 */
	" lsl.l      #8,%%d0\n"
	" move.b     (%%a1)+,%%d0\n"
	" swap       %%d0\n"
	" move.b     (%%a2)+,%%d2\n"
	" lsl.l      #8,%%d2\n"
	" move.b     (%%a3)+,%%d2\n"
	" move.w     %%d2,%%d0\n"
#else
	" move.b     (%%a0)+,%%d0\n" /* get next 4 chunky pixels in d0 */
	" lsl.w      #8,%%d0\n"
	" move.b     (%%a1)+,%%d0\n"
	" swap       %%d0\n"
	" move.b     (%%a2)+,%%d0\n"
	" lsl.w      #8,%%d0\n"
	" move.b     (%%a3)+,%%d0\n"
#endif
	" move.l     %%d0,%%d2\n"
	" and.l      %%d5,%%d2\n"
	" eor.l      %%d2,%%d0\n"
	" lsr.l      #4,%%d0\n"
	" move.l     %%d2,%%d3\n"
	" and.l      %%d7,%%d3\n"
	" swap       %%d2\n"
	" and.l      %%d7,%%d2\n"
#ifdef __mcoldfire__
	" move.l     %%d2,%%d4\n"
	" lsr.l      #2,%%d4\n"
	" lsl.l      #2,%%d2\n"
	" and.l      #0xfffcffff,%%d2\n"
	" and.l      #0xffffcfff,%%d4\n"
	" move.w     %%d4,%%d2\n"
#else
	" lsr.w      #2,%%d2\n"
	" lsl.l      #2,%%d2\n"
	" lsr.w      #2,%%d2\n"
#endif
	" or.l       %%d2,%%d3\n"
	" move.l     %%d0,%%d1\n"
	" and.l      %%d7,%%d1\n"
	" swap       %%d0\n"
	" and.l      %%d7,%%d0\n"
#ifdef __mcoldfire__
	" move.l     %%d0,%%d4\n"
	" lsr.l      #2,%%d4\n"
	" lsl.l      #2,%%d0\n"
	" and.l      #0xfffcffff,%%d0\n"
	" and.l      #0xffffcfff,%%d4\n"
	" move.w     %%d4,%%d0\n"
#else
	" lsr.w      #2,%%d0\n"
	" lsl.l      #2,%%d0\n"
	" lsr.w      #2,%%d0\n"
#endif
	" or.l       %%d0,%%d1\n"
	" move.l     %%d1,%%d2\n"
	" lsr.l      #8,%%d2\n"
	" move.l     %%d1,%%d0\n"
	" and.l      %%d6,%%d0\n"
	" eor.l      %%d0,%%d1\n"
	" move.l     %%d2,%%d4\n"
	" and.l      %%d6,%%d4\n"
	" eor.l      %%d4,%%d2\n"
	" lsr.l      #1,%%d2\n"
	" or.l       %%d2,%%d1\n"
	" lsl.l      #1,%%d0\n"
	" or.l       %%d4,%%d0\n"
	" move.b     %%d1,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" swap       %%d0\n"
	" swap       %%d1\n"
	" move.b     %%d1,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" move.l     %%d3,%%d2\n"
	" lsr.l      #8,%%d2\n"
	" move.l     %%d3,%%d0\n"
	" and.l      %%d6,%%d0\n"
	" eor.l      %%d0,%%d3\n"
	" move.l     %%d2,%%d4\n"
	" and.l      %%d6,%%d4\n"
	" eor.l      %%d4,%%d2\n"
	" lsr.l      #1,%%d2\n"
	" or.l       %%d2,%%d3\n"
	" lsl.l      #1,%%d0\n"
	" or.l       %%d4,%%d0\n"
	" move.b     %%d3,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" swap       %%d0\n"
	" swap       %%d3\n"
	" move.b     %%d3,(%%a4)+\n"
	" move.b     %%d0,(%%a4)+\n"
	" cmpa.l     %%a4,%%a5\n"
	" bgt        p2c4_1\n"
	" rts\n"

"1:\n"
	:
	: "r"(BytesPerRow), "r"(sw), "r"(depth), "a"(chunky), "a"(planetab)
	: "a2", "a3", "a4", "a5", "d3", "d4", "d5", "d6", "d7", "cc" AND_MEMORY);
}
#endif


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


static void join(uint8_t *chunky, uint8_t *lbuf, uint16_t sw, uint16_t zzflag)
{
	uint16_t cnt;
	uint8_t pix;

	cnt = (sw >> 1) - 1 - zzflag;
	chunky = chunky + zzflag + 1;

	lbuf = lbuf + (zzflag + 1) * 2;
	/* read chunky */
	/* $0054xxyy...zz0000 zzflag=1 */
	/* $14xxyy.......zz00 zzflag=0 */
	/* write composite */
	/* $40404040xyxy...zz */
	/* $4040xyxy.......zz */

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


static void filter(uint8_t *rgbbuf, int16_t sw)
{
	uint8_t *buf;
	uint8_t d1;
	uint8_t d2;
	uint8_t d3;
	
	sw -= 2;
	buf = rgbbuf + 3;
	d2 = rgbbuf[1];
	d3 = rgbbuf[2];
	while (sw > 0)
	{
		d1 = d2;
		d2 = d3;
		d3 = *buf++;
		*rgbbuf++ = (d1 + d2 + d2 + d3) >> 2;
		sw--;
	}
	*rgbbuf++ = (d2 + d3 + d3) >> 2;
	*rgbbuf = d3 >> 1;
}


static int16_t minmax(int16_t pix, int16_t min, int16_t max)
{
	if (pix < min)
		pix = min;
	else if (pix > max)
		pix = max;

	return pix;
}


static void yvu2rgb(uint8_t *lbuf, int8_t *vbuf, int8_t *ubuf, struct DCTVCvtHandle *cvt)
{
	int16_t cnt;
	int16_t luma;
	int16_t v;
	int16_t u;
	int16_t o6;
	int16_t d4;
	int16_t d7;
	uint8_t *rbuf;
	uint8_t *gbuf;
	uint8_t *bbuf;

	cnt = cvt->Width - 1;
	rbuf = cvt->Red;
	gbuf = cvt->Green;
	bbuf = cvt->Blue;
	v = *vbuf++;
	o6 = 0;
	
	while (cnt >= 0)
	{
		if (cnt & 1)
		{
			u = *ubuf++;
			d7 = v;
			d4 = (u + o6) >> 1;
			o6 = u;
		} else if (cnt != 0)
		{
			u = *vbuf++;
			d4 = o6;
			d7 = (u + v) >> 1;
			v = u;
		} else
		{
			u = 0;
			d4 = o6;
			d7 = (u + v) >> 1;
			v = u;
		}

		luma = tables[*lbuf++];

		u = tables[d7 + 0x180] + luma;
		*rbuf++ = minmax(u >> 4, 0, 255);

		u = tables[d7 + 0x380] + tables[d4 + 0x480] + luma;
		*gbuf++ = minmax(u >> 4, 0, 255);

		u = tables[d4 + 0x280] + luma;
		*bbuf++ = minmax(u >> 4, 0, 255);

		cnt--;
	}
}


static void chroma(uint8_t *buf, uint16_t sw, uint16_t zzflag)
{
	uint8_t *cbuf;
	uint8_t *lbuf;
	int16_t cnt;
	int16_t c1;
	int16_t c2;
	int16_t c3;
	int16_t tmp;
	uint8_t sign;
	
	sign = 0;
	cbuf = buf + sw + 1;
	lbuf = buf + zzflag + 1;
	
	/*  $40404040xxxx...zz zzflag=1 */
	/*  $4040xxxx.......zz zzflag=0 */
	/*  read composite */
	/*  $0000xx00yy...zz00 */
	/*  $00xx00yy.....00zz */
	/*  write chroma */
	/*  $00xxyy...zz00 */
	/*  $xxyy.....zz00 */

	cnt = (sw >> 1) + zzflag - 2;

	c3 = *lbuf;
	c2 = (0x40 + c3) >> 1;

	while (cnt > 0)
	{
		c1 = c2;
		c2 = c3;
		lbuf += 2;

		c3 = *lbuf;						/* get composite */

		if ((tmp = c2 + c2 - c1 - c3 + 2) < 0)
			tmp += 3;

		tmp >>= 2;

		if (sign ^= 1)
			tmp = -tmp;

		*cbuf++ = minmax(tmp, -127, 127);

		cnt--;
	}
}


static void luma(uint8_t *buf, uint16_t sw, uint16_t zzflag)
{
	uint8_t *ptr;
	int8_t *cptr;
	int8_t *cbuf;
	uint8_t *lbuf;
	int16_t cnt;
	int16_t c1;
	int16_t c2;
	int16_t c3;
	int16_t c4;
	int16_t c5;
	int16_t c6;
	int16_t c7;
	int16_t d4;
	int16_t d3;
	int16_t d7;
	uint8_t sign;

	ptr = buf;
	cptr = (int8_t *)ptr + sw;
	cbuf = cptr + 1;
	lbuf = ptr + zzflag + 1;
	ptr = lbuf;
	
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

	cnt = (sw >> 1) + zzflag - 3;
	sign = 1;
	
	c5 = *cbuf++;
	c6 = *cbuf++;
	c7 = *cbuf++;
	d4 = c5 + c6 + c7 + 4;
	c1 = c2 = c3 = c4 = 0;

	if (zzflag == 0)
	{
		sign = 0;
		c4 = c5;
		c5 = c6;
		c6 = c7;
		c7 = *cbuf++;
		d4 = d4 + c7 + c4;
		d3 = *ptr + (d4 >> 3);
		d3 = d3 + d3 - 64;
		d7 = minmax(d3, 64, 224);
		d3 -= d7;
		if (d3 != 0)
		{
			d3 <<= 2;
			d3 += c4;
			d3 = minmax(d3, -127, 127);
			d4 = d4 - c4 - c4 + d3 + d3;
			cbuf[-4] = d3;
			c4 = d3;
		}
		*ptr++ = 0x40;
		*ptr++ = d7;
	}
	
	while (cnt > 0)
	{
		d3 = d4 - c1 - c4;
		c1 = c2;
		c2 = c3;
		c3 = c4;
		c4 = c5;
		c5 = c6;
		c6 = c7;
		c7 = 0;
		if (cnt >= zzflag + 3)
			c7 = *cbuf++;
		
		d4 = d3 + c7 + c4;
		d3 = d4 >> 3;

		if ((sign ^= 1) == 0)
			d3 = -d3;

		d7 = minmax(*ptr - d3, 64, 224);

		*ptr++ = d7;
		*ptr++ = d7;

		cnt--;
	}

	if (zzflag)
	{
		ptr[0] = 0x40;
		ptr[1] = 0x40;
	} else
	{
		d7 = (d4 - c1 - c4 + c5) >> 3;
		if (sign)
			d7 = -d7;
		d7 = *ptr - d7;
		d3 = minmax(d7, 64, 224);
		d7 -= d3;
		if (d7 != 0)
		{
			d7 <<= 2;
			cbuf[-1] = c5 - d7;
		}
		*ptr++ = d3;
		*ptr++ = 0x40;
	}
	
	cbuf = cptr;
	d3 = (int)((uint8_t *)cbuf - ptr);
	if (d3 > 0)
	{
		d3--;
		while (d3 >= 0)
		{
			*ptr++ = 0x40;
			--d3;
		}
	}
	*cbuf++ = 0;
	if (zzflag)
		cbuf[0] = cbuf[1];
	cbuf = cptr + (sw >> 1) - 1;
	d3 = *cbuf;
	if (d3 < 0)
		d3++;
	d3 >>= 1;
	*cbuf = d3;
}


static void dc2rgb(struct DCTVCvtHandle *cvt, uint16_t linenr)
{
	uint8_t *lbuf;
	uint8_t *uvbuf;
	int8_t *ubuf;
	int8_t *vbuf;
	uint16_t sw;
	size_t field;
	int zzflag;

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

	if (field == 0)
	{
		lbuf = cvt->FBuf1[linenr & 3];
		uvbuf = cvt->FBuf1[(linenr - 1) & 3];
	} else
	{
		lbuf = cvt->FBuf2[linenr & 3];
		uvbuf = cvt->FBuf2[(linenr - 1) & 3];
	}

	zzflag = cvt->Chunky[1] == 0x54;

	/* join two 4-bit pixels into one 8-bit */
	join(cvt->Chunky, lbuf, sw, zzflag);

	/* decode chroma */
	chroma(lbuf, sw, zzflag);

	/* decode luma */
	luma(lbuf, sw, zzflag);

	/* convert yuv to rgb */
	ubuf = (int8_t *)uvbuf + sw;
	vbuf = (int8_t *)lbuf + sw;
	if (zzflag)
		yvu2rgb(lbuf, vbuf, ubuf, cvt);
	else
		yvu2rgb(lbuf, ubuf, vbuf, cvt);
		
	/* filtering for removing artifacts */
	filter(cvt->Red, sw);
	filter(cvt->Green, sw);
	filter(cvt->Blue, sw);
}


static void blank(struct DCTVCvtHandle *cvt, uint16_t offset, uint16_t size)
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

	/* first line or two (lace) is signature */
	if (line > cvt->Lace && line < cvt->Height)
	{
		dc2rgb(cvt, line);
		blank(cvt, 0, 4);				/* blank first 4 pixels */
		blank(cvt, cvt->Width - 2, 2);	/* blank last 4 pixels (2 more) */
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
	static uint8_t const sign[8] = { 0xd5, 0xab, 0x57, 0xaf, 0x5f, 0xbf, 0x7f, 0xff };
	
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
	uint16_t sw;
	int16_t swb;
	uint16_t sh;
	uint16_t np;
	uint16_t fields;
	int16_t i;
	uint8_t *buf;
	uint8_t *ptr;

	if (!bmp)
		return NULL;

	sw = bmp->BytesPerRow << 3;
	sh = bmp->Rows;
	np = bmp->Depth;
	interlace &= 1;							/*  1=lace, 0=nonlace */
	fields = interlace + 1;

	if (sw < 256 || sh < (2 * fields) || np > 4)
		return NULL;

	if ((cvt = calloc(sizeof(*cvt) + (sw * 16), 1)) != NULL)
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
		ptr = buf;
		for (i = 0; i < sw; i++)
			*buf++ = 0x40;
		
		swb = ((sw >> 1) + sw);
		buf = &ptr[swb * 4];
		memcpy(ptr + swb, ptr, sw);
		memcpy(ptr + swb * 2, ptr, swb * 2);
		memcpy(buf, ptr, swb * 4);
		
		for (i = 0; i < 4; i++)
		{
			cvt->FBuf1[i] = ptr + swb * i;	/*  first field */
			cvt->FBuf2[i] = buf + swb * i;	/*  second field */
		}
	}
	return cvt;
}

