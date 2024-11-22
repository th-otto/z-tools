/*
 * Code originally written by cholok
 * https://drive.google.com/file/d/1ejrHgC4ZeTjMRe4CYqaeftedBJY-6PjZ/view?usp%3Dsharing
 */
	.globl p2c

	.text

/* a0=chunky a1=bitmap d0=line */
	
p2c:
	movem.l    d2-d7/a2-a5,-(a7)
	move.w     d0,d1
	move.w     (a1),d0  /* bytes per row */
	mulu.w     d0,d1	/* offset=line*bpr */
	asl.w      #3,d0	/* width=bpr*8 */
	moveq.l    #0,d2
	move.b     5(a1),d2	/* Depth */
	addq.w     #8,a1	/* offset to plane table */
	lea.l      p2coffs(pc),a2
	add.w      d2,d2
	adda.w     -2(a2,d2.w),a2
	jsr        (a2)
	movem.l    (a7)+,d2-d7/a2-a5
	rts


p2coffs:
	.dc.w p2c1-p2coffs
	.dc.w p2c2-p2coffs
	.dc.w p2c3-p2coffs
	.dc.w p2c4-p2coffs

/* --------------------------------------------------------------------------- */

/* a0=chunky a1=plane[] d1=sw */
p2c1:
	movea.l    a0,a4          /* chunky */
	lea.l      0(a4,d0.w),a5  /* end of chunky */
	movem.l    (a1),a0	      /* planes */
	adda.l     d1,a0
	move.l     #0x0F0F0F0F,d5 /* d5 = constant 0x0f0f0f0f */
	move.l     #0x55555555,d6 /* d6 = constant 0x55555555 */
	move.l     #0x3333CCCC,d7 /* d7 = constant 0x3333cccc */
p2c1_1:
	moveq.l    #0,d0 /* get next chunky pixel in d0 */
	move.b     (a0)+,d0
	ror.l      #8,d0

	move.l     d0,d2
	and.l      d5,d2
	eor.l      d2,d0
	lsr.l      #4,d0
	move.l     d2,d3
	and.l      d7,d3
	swap       d2
	and.l      d7,d2
	lsr.w      #2,d2
	lsl.l      #2,d2
	lsr.w      #2,d2
	or.l       d2,d3
	move.l     d0,d1
	and.l      d7,d1
	swap       d0
	and.l      d7,d0
	lsr.w      #2,d0
	lsl.l      #2,d0
	lsr.w      #2,d0
	or.l       d0,d1
	move.l     d1,d2
	lsr.l      #8,d2
	move.l     d1,d0
	and.l      d6,d0
	eor.l      d0,d1
	move.l     d2,d4
	and.l      d6,d4
	eor.l      d4,d2
	lsr.l      #1,d2
	or.l       d2,d1
	lsl.l      #1,d0
	or.l       d4,d0
	move.b     d1,(a4)+
	move.b     d0,(a4)+
	swap       d0
	swap       d1
	move.b     d1,(a4)+
	move.b     d0,(a4)+
	move.l     d3,d2
	lsr.l      #8,d2
	move.l     d3,d0
	and.l      d6,d0
	eor.l      d0,d3
	move.l     d2,d4
	and.l      d6,d4
	eor.l      d4,d2
	lsr.l      #1,d2
	or.l       d2,d3
	lsl.l      #1,d0
	or.l       d4,d0
	move.b     d3,(a4)+
	move.b     d0,(a4)+
	swap       d0
	swap       d3
	move.b     d3,(a4)+
	move.b     d0,(a4)+
	cmpa.l     a4,a5
	bgt.w      p2c1_1
	rts

/* --------------------------------------------------------------------------- */

/* a0=chunky a1=plane[] d1=sw */
p2c2:
	movea.l    a0,a4
	lea.l      0(a4,d0.w),a5
	movem.l    (a1),a0-a1	      /* planes */
	adda.l     d1,a0
	adda.l     d1,a1
	move.l     #0x0F0F0F0F,d5 /* d5 = constant 0x0f0f0f0f */
	move.l     #0x55555555,d6 /* d6 = constant 0x55555555 */
	move.l     #0x3333CCCC,d7 /* d7 = constant 0x3333cccc */
p2c2_1:
	moveq.l    #0,d0 /* get next 2 chunky pixels in d0 */
	move.b     (a0)+,d0
	lsl.w      #8,d0
	move.b     (a1)+,d0
	swap       d0

	move.l     d0,d2
	and.l      d5,d2
	eor.l      d2,d0
	lsr.l      #4,d0
	move.l     d2,d3
	and.l      d7,d3
	swap       d2
	and.l      d7,d2
	lsr.w      #2,d2
	lsl.l      #2,d2
	lsr.w      #2,d2
	or.l       d2,d3
	move.l     d0,d1
	and.l      d7,d1
	swap       d0
	and.l      d7,d0
	lsr.w      #2,d0
	lsl.l      #2,d0
	lsr.w      #2,d0
	or.l       d0,d1
	move.l     d1,d2
	lsr.l      #8,d2
	move.l     d1,d0
	and.l      d6,d0
	eor.l      d0,d1
	move.l     d2,d4
	and.l      d6,d4
	eor.l      d4,d2
	lsr.l      #1,d2
	or.l       d2,d1
	lsl.l      #1,d0
	or.l       d4,d0
	move.b     d1,(a4)+
	move.b     d0,(a4)+
	swap       d0
	swap       d1
	move.b     d1,(a4)+
	move.b     d0,(a4)+
	move.l     d3,d2
	lsr.l      #8,d2
	move.l     d3,d0
	and.l      d6,d0
	eor.l      d0,d3
	move.l     d2,d4
	and.l      d6,d4
	eor.l      d4,d2
	lsr.l      #1,d2
	or.l       d2,d3
	lsl.l      #1,d0
	or.l       d4,d0
	move.b     d3,(a4)+
	move.b     d0,(a4)+
	swap       d0
	swap       d3
	move.b     d3,(a4)+
	move.b     d0,(a4)+
	cmpa.l     a4,a5
	bgt.w      p2c2_1
	rts

/* --------------------------------------------------------------------------- */

/* a0=chunky a1=plane[] d1=sw */
p2c3:
	movea.l    a0,a4
	lea.l      0(a4,d0.w),a5
	movem.l    (a1),a0-a2	      /* planes */
	adda.l     d1,a0
	adda.l     d1,a1
	adda.l     d1,a2
	move.l     #0x0F0F0F0F,d5 /* d5 = constant 0x0f0f0f0f */
	move.l     #0x55555555,d6 /* d6 = constant 0x55555555 */
	move.l     #0x3333CCCC,d7 /* d7 = constant 0x3333cccc */
p2c3_1:
	moveq.l    #0,d0 /* get next 3 chunky pixels in d0 */
	move.b     (a0)+,d0
	lsl.w      #8,d0
	move.b     (a1)+,d0
	swap       d0
	move.b     (a2)+,d0
	lsl.w      #8,d0

	move.l     d0,d2
	and.l      d5,d2
	eor.l      d2,d0
	lsr.l      #4,d0
	move.l     d2,d3
	and.l      d7,d3
	swap       d2
	and.l      d7,d2
	lsr.w      #2,d2
	lsl.l      #2,d2
	lsr.w      #2,d2
	or.l       d2,d3
	move.l     d0,d1
	and.l      d7,d1
	swap       d0
	and.l      d7,d0
	lsr.w      #2,d0
	lsl.l      #2,d0
	lsr.w      #2,d0
	or.l       d0,d1
	move.l     d1,d2
	lsr.l      #8,d2
	move.l     d1,d0
	and.l      d6,d0
	eor.l      d0,d1
	move.l     d2,d4
	and.l      d6,d4
	eor.l      d4,d2
	lsr.l      #1,d2
	or.l       d2,d1
	lsl.l      #1,d0
	or.l       d4,d0
	move.b     d1,(a4)+
	move.b     d0,(a4)+
	swap       d0
	swap       d1
	move.b     d1,(a4)+
	move.b     d0,(a4)+
	move.l     d3,d2
	lsr.l      #8,d2
	move.l     d3,d0
	and.l      d6,d0
	eor.l      d0,d3
	move.l     d2,d4
	and.l      d6,d4
	eor.l      d4,d2
	lsr.l      #1,d2
	or.l       d2,d3
	lsl.l      #1,d0
	or.l       d4,d0
	move.b     d3,(a4)+
	move.b     d0,(a4)+
	swap       d0
	swap       d3
	move.b     d3,(a4)+
	move.b     d0,(a4)+
	cmpa.l     a4,a5
	bgt        p2c3_1
	rts

/* --------------------------------------------------------------------------- */

/* a0=chunky a1=plane[] d1=sw */
p2c4:
	movea.l    a0,a4
	lea.l      0(a4,d0.w),a5
	movem.l    (a1),a0-a3	      /* planes */
	adda.l     d1,a0
	adda.l     d1,a1
	adda.l     d1,a2
	adda.l     d1,a3
	move.l     #0x0F0F0F0F,d5 /* d5 = constant 0x0f0f0f0f */
	move.l     #0x55555555,d6 /* d6 = constant 0x55555555 */
	move.l     #0x3333CCCC,d7 /* d7 = constant 0x3333cccc */
p2c4_1:
	move.b     (a0)+,d0 /* get next 4 chunky pixels in d0 */
	lsl.w      #8,d0
	move.b     (a1)+,d0
	swap       d0
	move.b     (a2)+,d0
	lsl.w      #8,d0
	move.b     (a3)+,d0
	move.l     d0,d2
	and.l      d5,d2
	eor.l      d2,d0
	lsr.l      #4,d0
	move.l     d2,d3
	and.l      d7,d3
	swap       d2
	and.l      d7,d2
	lsr.w      #2,d2
	lsl.l      #2,d2
	lsr.w      #2,d2
	or.l       d2,d3
	move.l     d0,d1
	and.l      d7,d1
	swap       d0
	and.l      d7,d0
	lsr.w      #2,d0
	lsl.l      #2,d0
	lsr.w      #2,d0
	or.l       d0,d1
	move.l     d1,d2
	lsr.l      #8,d2
	move.l     d1,d0
	and.l      d6,d0
	eor.l      d0,d1
	move.l     d2,d4
	and.l      d6,d4
	eor.l      d4,d2
	lsr.l      #1,d2
	or.l       d2,d1
	lsl.l      #1,d0
	or.l       d4,d0
	move.b     d1,(a4)+
	move.b     d0,(a4)+
	swap       d0
	swap       d1
	move.b     d1,(a4)+
	move.b     d0,(a4)+
	move.l     d3,d2
	lsr.l      #8,d2
	move.l     d3,d0
	and.l      d6,d0
	eor.l      d0,d3
	move.l     d2,d4
	and.l      d6,d4
	eor.l      d4,d2
	lsr.l      #1,d2
	or.l       d2,d3
	lsl.l      #1,d0
	or.l       d4,d0
	move.b     d3,(a4)+
	move.b     d0,(a4)+
	swap       d0
	swap       d3
	move.b     d3,(a4)+
	move.b     d0,(a4)+
	cmpa.l     a4,a5
	bgt        p2c4_1
	rts
