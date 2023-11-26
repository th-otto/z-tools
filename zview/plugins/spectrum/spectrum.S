	.globl decode_spectrum1
	.globl decode_spectrum2

#ifdef __mcoldfire__
#  define W l
#else
#  define W w
#endif

/* void decode_spectrum1(uint8_t *src, uint8_t *dst, uint16_t *palettes); */
decode_spectrum1:
#ifdef __mcoldfire__
	lea        -44(a7),a7
	movem.l    d2-d7/a2-a6,(a7)
#else
	movem.l    d2-d7/a2-a6,-(a7)
#endif
	movea.l    48(a7),a6
	move.w     (a6),d0
	cmp.w      #0x5350,d0
	bne        decode1_12
	movea.l    52(a7),a0
	moveq.l    #39,d0
decode1_1:
	clr.l      (a0)+
#ifdef __mcoldfire__
	subq.l	   #1,d0
	bpl.s      decode1_1
#else
	dbf        d0,decode1_1
#endif
	lea.l      12(a6),a0
	movea.l    52(a7),a1
	moveq      #1,d1
	moveq      #7,d7
	lea.l      32000(a1),a2
	lea.l      8(a2),a3
decode1_2:
	lea.l      -31840(a2),a1
decode1_3:
	move.b     (a0)+,d6
	ext.w      d6
#ifdef __mcoldfire__
	ext.l      d6
#endif
	bmi.s      decode1_5
decode1_4:
	move.b     (a0)+,(a1)
	adda.W     d1,a1
#ifdef __mcoldfire__
	move.l     d1,d4
	move.l     d7,d1
	move.l     d4,d7
	subq.l     #1,d6
	bpl        decode1_4
#else
	exg        d1,d7
	dbf        d6,decode1_4
#endif
	bra.s      decode1_7
decode1_5:
	neg.W      d6
	addq.W     #1,d6
	move.b     (a0)+,d0
decode1_6:
	move.b     d0,(a1)
	adda.W     d1,a1
#ifdef __mcoldfire__
	move.l     d1,d4
	move.l     d7,d1
	move.l     d4,d7
	subq.l	   #1,d6
	bpl        decode1_6
#else
	exg        d1,d7
	dbf        d6,decode1_6
#endif
decode1_7:
	cmpa.l     a2,a1
	bcs.s      decode1_3
	bne.s      decode1_12
	addq.l     #2,a2
	cmpa.l     a3,a2
	bcs.s      decode1_2
	move.l     a0,d0
	addq.l     #1,d0
	bclr       #0,d0
	subi.l     #12,d0
	sub.l      a6,d0
	cmp.l      4(a6),d0
	bne.s      decode1_12
	lea.l      12(a6),a0
	adda.l     4(a6),a0
	movea.l    56(a7),a1
	move.W     #596,d7
	clr.w      d0
decode1_8:
	moveq      #13,d6
	move.w     (a0)+,d1
	lsr.W      #1,d1
	move.w     d0,(a1)+
decode1_9:
	lsr.W      #1,d1
	bcc.s      decode1_10
	move.w     (a0)+,(a1)+
#ifdef __mcoldfire__
	subq.l     #1,d6
	bpl        decode1_9
#else
	dbf        d6,decode1_9
#endif
	bra.s      decode1_11
decode1_10:
	move.w     d0,(a1)+
#ifdef __mcoldfire__
	subq.l     #1,d6
	bpl        decode1_9
#else
	dbf        d6,decode1_9
#endif
decode1_11:
	move.w     d0,(a1)+
#ifdef __mcoldfire__
	subq.l     #1,d7
	bpl        decode1_8
#else
	dbf        d7,decode1_8
#endif
	lea.l      -12(a0),a0
	suba.l     a6,a0
	suba.l     4(a6),a0
	cmpa.l     8(a6),a0
decode1_12:
#ifdef __mcoldfire__
	movem.l    (a7),d2-d7/a2-a6
	lea        44(a7),a7
#else
	movem.l    (a7)+,d2-d7/a2-a6
#endif
	rts


/* void decode_spectrum2(uint8_t *src, uint8_t *dst, uint16_t *palettes); */
decode_spectrum2:
#ifdef __mcoldfire__
	lea        -44(a7),a7
	movem.l    d2-d7/a2-a6,(a7)
#else
	movem.l    d2-d7/a2-a6,-(a7)
#endif
	movea.l    48(a7),a0
	move.w     (a0),d0
	cmp.w      #0x5350,d0
	bne        decode2_19
	movea.l    52(a7),a1
	lea.l      12(a0),a0
	lea.l      160(a1),a1
	lea.l      31840(a1),a2
	lea.l      8(a1),a3
	moveq.l    #1,d0
	moveq.l    #7,d1
	movea.l    a0,a4
	adda.l     -8(a0),a4
	adda.l     -4(a0),a4
	btst       #0,-1(a4)
	beq.s      decode2_7
decode2_1:
	clr.w      d2
	move.b     (a0)+,d2
	btst       #7,d2
	beq.s      decode2_4
	andi.W     #0x007F,d2
decode2_2:
	move.b     (a0)+,(a1)
	adda.l     d0,a1
#ifdef __mcoldfire__
	move.l     d0,d4
	move.l     d1,d0
	move.l     d4,d1
#else
	exg        d0,d1
#endif
	cmpa.l     a2,a1
	blt.s      decode2_3
	lea.l      -31838(a1),a1
	cmpa.l     a3,a1
	bge        decode2_14
decode2_3:
#ifdef __mcoldfire__
	subq.l	   #1,d2
	bpl        decode2_2
#else
	dbf        d2,decode2_2
#endif
	bra.s      decode2_1
decode2_4:
	addq.W     #2,d2
	move.b     (a0)+,d3
decode2_5:
	move.b     d3,(a1)
	adda.l     d0,a1
#ifdef __mcoldfire__
	move.l     d0,d4
	move.l     d1,d0
	move.l     d4,d1
#else
	exg        d0,d1
#endif
	cmpa.l     a2,a1
	blt.s      decode2_6
	lea.l      -31838(a1),a1
	cmpa.l     a3,a1
	bge.s      decode2_14
decode2_6:
#ifdef __mcoldfire__
	subq.l	   #1,d2
	bpl        decode2_5
#else
	dbf        d2,decode2_5
#endif
	bra.s      decode2_1
decode2_7:
	lea.l      160(a1),a4
decode2_8:
	clr.W      d2
	move.b     (a0)+,d2
	btst       #7,d2
	beq.s      decode2_11
	andi.W     #0x007F,d2
decode2_9:
	move.b     (a0)+,(a1)
	lea.l      160(a1),a1
	cmpa.l     a2,a1
	blt.s      decode2_10
	lea.l      -31840(a1),a1
	adda.l     d0,a1
#ifdef __mcoldfire__
	move.l     d0,d4
	move.l     d1,d0
	move.l     d4,d1
#else
	exg        d0,d1
#endif
	cmpa.l     a4,a1
	blt.s      decode2_10
	lea.l      -158(a1),a1
	cmpa.l     a3,a1
	bge.s      decode2_14
decode2_10:
#ifdef __mcoldfire__
	subq.l     #1,d2
	bpl        decode2_9
#else
	dbf        d2,decode2_9
#endif
	bra.s      decode2_8
decode2_11:
	addq.W     #2,d2
	move.b     (a0)+,d3
decode2_12:
	move.b     d3,(a1)
	lea.l      160(a1),a1
	cmpa.l     a2,a1
	blt.s      decode2_13
	lea.l      -31840(a1),a1
	adda.l     d0,a1
#ifdef __mcoldfire__
	move.l     d0,d4
	move.l     d1,d0
	move.l     d4,d1
#else
	exg        d0,d1
#endif
	cmpa.l     a4,a1
	blt.s      decode2_13
	lea.l      -158(a1),a1
	cmpa.l     a3,a1
	bge.s      decode2_14
decode2_13:
#ifdef __mcoldfire__
	subq.l     #1,d2
	bpl        decode2_12
#else
	dbf        d2,decode2_12
#endif
	bra.s      decode2_8
decode2_14:
	movea.l    56(a7),a1
#ifdef __mcoldfire__
	move.l     #596,d0
#else
	move.w     #596,d0
#endif
	suba.l     a2,a2
	moveq      #3,d1
	moveq      #8,d2
decode2_15:
	move.l     a2,d3
	lsr.l      d1,d3
	move.b     0(a0,d3.W),d4
	lsl.W      d2,d4
	move.b     1(a0,d3.W),d4
	lsl.l      d2,d4
	move.b     2(a0,d3.W),d4
	move.l     a2,d3
	andi.W     #7,d3
	moveq      #10,d5
	sub.W      d3,d5
	lsr.l      d5,d4
	lea.l      14(a2),a2
	clr.w      (a1)+
	moveq      #13,d6
decode2_16:
	btst       d6,d4
	bne.s      decode2_17
	clr.w      (a1)+
	bra.s      decode2_18
decode2_17:
	move.l     a2,d3
	lsr.l      d1,d3
	move.b     0(a0,d3.W),d7
	lsl.W      d2,d7
	move.b     1(a0,d3.W),d7
	move.l     a2,d3
	andi.W     #7,d3
	moveq      #7,d5
	sub.W      d3,d5
	lsr.l      d5,d7
	lea.l      9(a2),a2
	move.W     d7,d5
	andi.W     #0x01C0,d5
	lsl.W      #2,d5
	move.W     d5,d3
	move.W     d7,d5
	andi.W     #0x0038,d5
	add.W      d5,d5
	or.W       d5,d3
	andi.W     #7,d7
	or.W       d7,d3
	move.w     d3,(a1)+
decode2_18:
#ifdef __mcoldfire__
	subq.l     #1,d6
	bpl        decode2_16
#else
	dbf        d6,decode2_16
#endif
	clr.w      (a1)+
#ifdef __mcoldfire__
	subq.l     #1,d0
	bpl        decode2_15
#else
	dbf        d0,decode2_15
#endif
decode2_19:
#ifdef __mcoldfire__
	movem.l    (a7),d2-d7/a2-a6
	lea        44(a7),a7
#else
	movem.l    (a7)+,d2-d7/a2-a6
#endif
	rts
