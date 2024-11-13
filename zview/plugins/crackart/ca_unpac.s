/*
 * Dekomprimierung von CRACK ART Bildern (CA?)
 * Copyright (c) Detlef Roettger 04.03.1990
 */

	.globl ca_decompress

	.text

/* void __CDECL ca_decompress(const uint8_t *filedata, uint8_t *screen); */

ca_decompress:
	movem.l    4(a7),a0-a1
	lea        -44(a7),a7
	movem.l    d2-d7/a2-a6,(a7)
	movea.l    a1,a2
	lea.l      32000(a1),a3  /* end of screen */
	clr.w      d7
	clr.w      d6
	move.b     (a0)+,d7		/* get escape character */
	move.b     (a0)+,d6		/* get fill byte */
	move.w     (a0)+,d0		/* get packet offset */
	and.l      #0x00007FFF,d0
	movea.l    d0,a5
	move.b     d6,d1		/* set d1-d5 to all fill byte */
	lsl.w      #8,d1
	move.b     d6,d1
	move.w     d1,d2
	swap       d1
	move.w     d2,d1
	move.l     d1,d2
	move.l     d1,d3
	move.l     d1,d4
	move.l     d1,d5
	/* fill screen */
	movea.l    a3,a6
	move.w     #1600-1,d0
delta:
	movem.l    d1-d5,-(a6)
	dbf        d0,delta

	move.l     a5,d0		/* Offset */
	subq.w     #1,d0		/* runs */
	bmi.s      endmain
mainloop:
	clr.w      d1
	move.b     (a0)+,d1		/* get first byte */
	cmp.b      d7,d1
	beq.s      esccode

writeone:
	move.b     d1,(a2)		/* if no escape, write immediately */
	adda.l     a5,a2
	cmpa.l     a3,a2
	blt.s      mainloop
	addq.l     #1,a1
	movea.l    a1,a2
	dbf        d0,mainloop
endmain:
	movem.l    (a7),d2-d7/a2-a6
	lea        44(a7),a7
	rts

/* found escape byte */
esccode:
	move.b     (a0)+,d1		/* get 2nd byte */
	cmp.b      d7,d1		
	beq.s      writeone		/* write esc */

	tst.b      d1			/* COMP0 */
	bne.s      code1
	clr.w      d2			/* ESC 00 ANZAHL-1 BYTE */
	move.b     (a0)+,d2		/* Anzahl 3-255 ist bedeutet 4-256 */
	move.b     (a0)+,d1		/* gleiche Bytes */
loop0:
	move.b     d1,(a2)
	adda.l     a5,a2
	cmpa.l     a3,a2
	blt.s      drin0
	addq.l     #1,a1
	movea.l    a1,a2
	subq.w     #1,d0		/* Ueberschlag gemacht */
	bmi.s      endmain
drin0:
	dbf        d2,loop0
	bra.s      mainloop

code1:
	cmpi.b     #1,d1		/* COMP1 */
	bne.s      code2
	clr.w      d2			/* ESC 01 MULT REST-1 BYTE */
	clr.w      d3
	move.b     (a0)+,d3		/* Multiplikator */
	lsl.w      #8,d3
	move.b     (a0)+,d2		/* Anzahl 1-256 */
	add.w      d3,d2
	move.b     (a0)+,d1		/* komprimiertes Byte */
loop1:
	move.b     d1,(a2)
	adda.l     a5,a2
	cmpa.l     a3,a2
	blt.s      drin1
	addq.l     #1,a1
	movea.l    a1,a2
	subq.w     #1,d0		/* Ueberschlag gemacht */
	bmi.s      endmain
drin1:
	dbf        d2,loop1
	bra.s      mainloop

code2:
	cmpi.b     #2,d1		/* SAME */
	bne.s      multiple		/* Komprimiert 3<n<=256 */
	clr.w      d3
	move.b     (a0)+,d3		/* Multiplikator */
	beq.s      endmain		/* Abbruchcode ESC 02 00 */
	lsl.w      #8,d3
	clr.w      d2			/* ESC 02 MULT REST-1 */
	move.b     (a0)+,d2		/* Anzahl 1-256 */
	add.w      d3,d2
loop2:
	adda.l     a5,a2		/* DELTAs 'schreiben' */
	cmpa.l     a3,a2
	blt.s      drin2
	addq.l     #1,a1
	movea.l    a1,a2
	subq.w     #1,d0		/* Ueberschlag gemacht */
	bmi.s      endmain
drin2:
	dbf        d2,loop2
	bra        mainloop

multiple:
	clr.w      d2			/* ESC ANZAHL-1 BYTE */
	move.b     (a0)+,d2		/* Byte */
loop3:
	move.b     d2,(a2)
	adda.l     a5,a2
	cmpa.l     a3,a2
	blt.s      drin3
	addq.l     #1,a1
	movea.l    a1,a2
	subq.w     #1,d0		/* Ueberschlag gemacht */
	bmi        endmain
drin3:
	dbf        d1,loop3
	bra        mainloop
