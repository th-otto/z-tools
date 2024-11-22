/*
 * Code originally written by cholok
 * https://drive.google.com/file/d/1ejrHgC4ZeTjMRe4CYqaeftedBJY-6PjZ/view?usp%3Dsharing
 */
	.globl c2p

	.text

/* a0=chunky a1=plane[] d0=sw */
	
c2p:
	movem.l	d2-d7/a2-a6,-(sp)

	move.l	a0,a6
	move.l	a1,a4
	lea	(a0,d0.w),a5  /* end of chunky */

	move.l     #0x0F0F0F0F,d5 /* d5 = constant 0x0f0f0f0f */
	move.l     #0x55555555,d6 /* d6 = constant 0x55555555 */
	move.l     #0x3333CCCC,d7 /* d7 = constant 0x3333cccc */
c2p_1:
	move.l	(a6)+,d0	/* get next 4 chunky pixels in d0 */
	move.l	(a6)+,d1	/* get next 4 chunky pixels in d1 */

	move.l	d0,d2
	and.l	d5,d2		/* d5=0x0f0f0f0f */
	eor.l	d2,d0
	move.l	d1,d3
	and.l	d5,d3		/* d5=0x0f0f0f0f */
	eor.l	d3,d1
	lsl.l	#4,d2
	or.l	d3,d2
	lsr.l	#4,d1
	or.l	d1,d0
	move.l	d2,d3
	and.l	d7,d3		/* d7=0x3333cccc */
	move.w	d3,d1
	clr.w	d3
	lsl.l	#2,d3
	lsr.w	#2,d1
	or.w	d1,d3
	swap	d2
	and.l	d7,d2		/* d7=0x3333cccc */
	or.l	d2,d3
	move.l	d0,d1
	and.l	d7,d1		/* d7=0x3333cccc */
	move.w	d1,d2
	clr.w	d1
	lsl.l	#2,d1
	lsr.w	#2,d2
	or.w	d2,d1
	swap	d0
	and.l	d7,d0		/* d7=0x3333cccc */
	or.l	d0,d1
	move.l	d1,d2
	lsr.l	#7,d2
	move.l	d1,d0
	and.l	d6,d0		/* d6=0x55555555 */
	eor.l	d0,d1
	move.l	d2,d4
	and.l	d6,d4		/* d6=0x55555555 */
	eor.l	d4,d2
	or.l	d4,d1
	lsr.l	#1,d1

	movem.l	16(a4),a0-a3
	move.b	d1,(a3)+	/* plane 7 */
	swap	d1
	move.b	d1,(a1)+	/* plane 5 */
	or.l	d0,d2
	move.b	d2,(a2)+	/* plane 6 */
	swap	d2
	move.b	d2,(a0)+	/* plane 4 */
	movem.l	a0-a3,16(a4)

	move.l	d3,d2
	lsr.l	#7,d2
	move.l	d3,d0
	and.l	d6,d0		/* d6=0x55555555 */
	eor.l	d0,d3
	move.l	d2,d4
	and.l	d6,d4		/* d6=0x55555555 */
	eor.l	d4,d2
	or.l	d4,d3
	lsr.l	#1,d3

	movem.l	(a4),a0-a3
	move.b	d3,(a3)+	/* plane 3 */
	swap	d3
	move.b	d3,(a1)+	/* plane 1 */
	or.l	d0,d2
	move.b	d2,(a2)+	/* plane 2 */
	swap	d2
	move.b	d2,(a0)+	/* plane 0 */
	movem.l	a0-a3,(a4)

	/* test if finished */
	cmpa.l	a6,a5
	bgt.w	c2p_1

	movem.l	(sp)+,d2-d7/a2-a6
	rts
