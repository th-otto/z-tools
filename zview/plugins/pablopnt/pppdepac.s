/* Pablo Paint image decompression code */
/* code taken from Pablo Paint v1.1 -> pablo_c.prg */
/*   removed link/unlink commands */
/*   removed pallette and extra parameter */
/* dis-assembled and modified by Lonny Pursell */

/* note: after reading the first 2 ascii lines call this routine */
/*       thus source address would point to the resolution flag */

	.globl	depack

SRC	= (14*4)+4
DST	= (14*4)+8
/* OPT	= (14*4)+12 */

	.text

depack:
	movem.l	d0-d7/a0-a5,-(a7)
	movea.l	SRC(sp),a0
	moveq	#0,d7
	move.b	1(a0),d7		/* compression type */
	cmp.b	#30,d7
	bhi.s	depack_end

	moveq	#0,d2
	movea.l	a0,a1
	movea.l	DST(sp),a2
	move.w	2(a1),d2
	sub.w	#36,d2
	adda.l	#36,a1			/* skip 4+32 (header) */
	lea 	jmptable(pc),a3
	add.w 	d7,d7
	add.w	0(a3,d7.w),a3
	jmp		(a3)

jmptable:
	dc.w	type0-jmptable
	dc.w	type1-jmptable
	dc.w	type2-jmptable
	dc.w	type3-jmptable
	dc.w	type4-jmptable
	dc.w	type5-jmptable
	dc.w	type6-jmptable
	dc.w	type7-jmptable
	dc.w	type8-jmptable
	dc.w	type9-jmptable
	dc.w	type10-jmptable
	dc.w	type11-jmptable
	dc.w	type12-jmptable
	dc.w	type13-jmptable
	dc.w	type14-jmptable
	dc.w	type15-jmptable
	dc.w	type16-jmptable
	dc.w	type17-jmptable
	dc.w	type18-jmptable
	dc.w	type19-jmptable
	dc.w	type20-jmptable
	dc.w	type21-jmptable
	dc.w	type22-jmptable
	dc.w	type23-jmptable
	dc.w	type24-jmptable
	dc.w	type25-jmptable
	dc.w	type26-jmptable
	dc.w	type27-jmptable
	dc.w	type28-jmptable
	dc.w	type29-jmptable
	dc.w	type30-jmptable

type0:		/* 0 - copy src -> dst, uncompressed */
	move.w	#7999,d0
L2102:
	move.l	(a1)+,(a2)+
	dbf		d0,L2102
depack_end:
	movem.l	(a7)+,d0-d7/a0-a5
	rts

type1:			/* 1 */
	lsr.w	#1,d2
	movea.l	a1,a3
	adda.l	d2,a3
	subq.w	#1,d2
L2124:
	moveq	#0,d3
	move.b	(a3)+,d3
L2128:
	move.b	(a1),(a2)+
	dbf		d3,L2128
	addq.l	#1,a1
	dbf		d2,L2124
	bra		depack_end

type2:			/* 2 */
	divu	#3,d2
	lsl.w	#1,d2
	movea.l	a1,a3
	adda.l	d2,a3
	lsr.w	#1,d2
	subq.w	#1,d2
L2162:
	moveq	#0,d3
	move.b	(a3)+,d3
L2166:
	move.w	(a1),(a2)+
	dbf		d3,L2166
	addq.l	#2,a1
	dbf		d2,L2162
	bra		depack_end

type3:			/* 3 */
	divu	#5,d2
	lsl.w	#2,d2
	movea.l	a1,a3
	adda.l	d2,a3
	lsr.w	#2,d2
	subq.w	#1,d2
L2200:
	moveq	#0,d3
	move.b	(a3)+,d3
L2204:
	move.l	(a1),(a2)+
	dbf		d3,L2204
	addq.l	#4,a1
	dbf		d2,L2200
	bra		depack_end

type4:			/* 4 */
	move.l	#160,d4
L2230:
	lsr.w	#1,d2
	movea.l	a1,a3
	adda.l	d2,a3
	subq.w	#1,d2
	moveq	#0,d1
L2240:
	moveq	#0,d3
	move.b	(a3)+,d3
L2244:
	move.b	(a1),0(a2,d1.w)
	add.w	d4,d1
	cmp.w	#31999,d1
	ble.s	L2260
	moveq	#0,d1
	addq.l	#1,a2
L2260:
	dbf		d3,L2244
	addq.l	#1,a1
	dbf		d2,L2240
	bra		depack_end

type5:			/* 5 */
	move.l	#160,d4
L2284:
	divu	#3,d2
	lsl.l	#1,d2
	movea.l	a1,a3
	adda.l	d2,a3
	lsr.w	#1,d2
	subq.w	#1,d2
	moveq	#0,d1
L2300:
	moveq	#0,d3
	move.b	(a3)+,d3
L2304:
	move.w	(a1),0(a2,d1.w)
	add.w	d4,d1
	cmp.w	#32000,d1
	blt.s	L2320
	moveq	#0,d1
	addq.l	#2,a2
L2320:
	dbf		d3,L2304
	addq.l	#2,a1
	dbf		d2,L2300
	bra		depack_end

type6:			/* 6 */
	move.l	#160,d4
L2344:
	divu	#5,d2
	lsl.w	#2,d2
	movea.l	a1,a3
	adda.l	d2,a3
	lsr.w	#2,d2
	subq.w	#1,d2
	moveq	#0,d1
L2360:
	moveq	#0,d3
	move.b	(a3)+,d3
L2364:
	move.l	(a1),0(a2,d1.w)
	add.w	d4,d1
	cmp.w	#32000,d1
	blt.s	L2380
	moveq	#0,d1
	addq.l	#4,a2
L2380:
	dbf		d3,L2364
	addq.l	#4,a1
	dbf		d2,L2360
	bra		depack_end

type7:			/* 7 */
	moveq	#80,d4
	bra		L2230

type8:			/* 8 */
	moveq	#80,d4
	bra.s	L2284

type9:			/* 9 */
	moveq	#80,d4
	bra.s	L2344

type10:			/* 10 */
	lsr.w	#1,d2
	movea.l	a1,a3
	adda.l	d2,a3
	movea.l	a2,a0
	adda.l	#32000,a0
	moveq	#7,d0
	bra.s	L2434

L2430:
	tst.w	d2
	bpl.s	L2440
L2434:
	move.b	(a1)+,d1
	moveq	#0,d2
	move.b	(a3)+,d2
L2440:
	cmpa.l	a2,a0
	bls.s	L2456
	move.b	d1,(a2)
	addq.l	#8,a2
	dbf		d2,L2440
	cmpa.l	a2,a0
	bhi.s	L2430
L2456:
	suba.l	#31999,a2
	dbf		d0,L2430
	bra		depack_end

type11:			/* 11 */
	divu	#3,d2
	lsl.w	#1,d2
	movea.l	a1,a3
	adda.l	d2,a3
	movea.l	a2,a0
	adda.l	#32000,a0
	moveq	#3,d0
	bra.s	L2500

L2496:
	tst.w	d2
	bpl.s	L2506
L2500:
	move.w	(a1)+,d1
	moveq	#0,d2
	move.b	(a3)+,d2
L2506:
	cmpa.l	a2,a0
	bls.s	L2522
	move.w	d1,(a2)
	addq.l	#8,a2
	dbf		d2,L2506
	cmpa.l	a2,a0
	bhi.s	L2496
L2522:
	suba.l	#31998,a2
	dbf		d0,L2496
	bra		depack_end

type12:			/* 12 */
	divu	#5,d2
	lsl.w	#2,d2
	movea.l	a1,a3
	adda.l	d2,a3
	movea.l	a2,a0
	adda.l	#32000,a0
	moveq	#1,d0
	bra.s	L2566

L2562:
	tst.w	d2
	bpl.s	L2572
L2566:
	move.l	(a1)+,d1
	moveq	#0,d2
	move.b	(a3)+,d2
L2572:
	cmpa.l	a2,a0
	bls.s	L2588
	move.l	d1,(a2)
	addq.l	#8,a2
	dbf		d2,L2572
	cmpa.l	a2,a0
	bhi.s	L2562
L2588:
	suba.l	#31996,a2
	dbf		d0,L2562
	bra		depack_end

type13:			/* 13 */
	lsr.w	#1,d2
	movea.l	a1,a3
	adda.l	d2,a3
	moveq	#0,d7
	moveq	#7,d0
	bra.s	L2622

L2618:
	tst.w	d2
	bpl.s	L2628
L2622:
	move.b	(a1)+,d1
	moveq	#0,d2
	move.b	(a3)+,d2
L2628:
	cmp.w	#32000,d7
	bcc.s	L2652
	move.b	d1,0(a2,d7.w)
	add.w	#160,d7
	dbf		d2,L2628
	cmp.w	#32000,d7
	bcs.s	L2618
L2652:
	sub.w	#31992,d7
	cmp.w	#160,d7
	bcs.s	L2618
	sub.w	#159,d7
	dbf		d0,L2618
	bra		depack_end

type14:			/* 14 */
	divu	#3,d2
	lsl.l	#1,d2
	movea.l	a1,a3
	adda.l	d2,a3
	moveq	#0,d7
	moveq	#3,d0
	bra.s	L2698

L2694:
	tst.w	d2
	bpl.s	L2704
L2698:
	move.w	(a1)+,d1
	moveq	#0,d2
	move.b	(a3)+,d2
L2704:
	cmp.w	#32000,d7
	bcc.s	L2728
	move.w	d1,0(a2,d7.w)
	add.w	#160,d7
	dbf		d2,L2704
	cmp.w	#32000,d7
	bcs.s	L2694
L2728:
	sub.w	#31992,d7
	cmp.w	#160,d7
	bcs.s	L2694
	sub.w	#158,d7
	dbf		d0,L2694
	bra		depack_end

type15:			/* 15 */
	divu	#5,d2
	lsl.w	#2,d2
	movea.l	a1,a3
	adda.l	d2,a3
	moveq	#0,d7
	moveq	#1,d0
	bra.s	L2774

L2770:
	tst.w	d2
	bpl.s	L2780
L2774:
	move.l	(a1)+,d1
	moveq	#0,d2
	move.b	(a3)+,d2
L2780:
	cmp.w	#32000,d7
	bcc.s	L2804
	move.l	d1,0(a2,d7.w)
	add.w	#160,d7
	dbf		d2,L2780
	cmp.w	#32000,d7
	bcs.s	L2770
L2804:
	sub.w	#31992,d7
	cmp.w	#160,d7
	bcs.s	L2770
	sub.w	#156,d7
	dbf		d0,L2770
	bra		depack_end

type16:		/* 16 */
	lea 	L2840(pc),a5
	moveq	#1,d7
	bra		L3080

L2840:
	add.w	d7,d0
	rts

type17:		/* 17 */
	lea 	L2840(pc),a5
	moveq	#2,d7
	bra		L3132

type18:		/* 18 */
	lea 	L2840(pc),a5
	moveq	#4,d7
	bra		L3184

type19:		/* 19 */
	lea 	L2880(pc),a5
	move.w	#160,d6
	move.w	#31999,d7
	bra		L3080

L2880:
	add.w	d6,d0
	cmp.w	#32000,d0
	bcs.s	L2890
	sub.w	d7,d0
L2890:
	rts

type20:		/* 20 */
	lea 	L2880(pc),a5
	move.w	#160,d6
	move.w	#31998,d7
	bra		L3132

type21:		/* 21 */
	lea 	L2880(pc),a5
	move.w	#160,d6
	move.w	#31996,d7
	bra		L3184

type22:		/* 22 */
	lea 	L2880(pc),a5
	move.w	#80,d6
	move.w	#31999,d7
	bra		L3080

type23:		/* 23 */
	lea 	L2880(pc),a5
	move.w	#80,d6
	move.w	#31998,d7
	bra		L3132

type24:		/* 24 */
	lea 	L2880(pc),a5
	move.w	#80,d6
	move.w	#31996,d7
	bra		L3184

type25:		/* 25 */
	lea 	L2984(pc),a5
	move.w	#31999,d7
	bra		L3080

L2984:
	addq.w	#8,d0
	cmp.w	#32000,d0
	bcs.s	L2994
	sub.w	d7,d0
L2994:
	rts

type26:		/* 26 */
	lea 	L2984(pc),a5
	move.w	#31998,d7
	bra		L3132

type27:		/* 27 */
	lea 	L2984(pc),a5
	move.w	#31996,d7
	bra		L3184

type28:		/* 28 */
	lea 	L3032(pc),a5
	move.w	#159,d7
	bra		L3080

L3032:
	add.w	#160,d0
	cmp.w	#32000,d0
	bcs.s	L3054
	sub.w	#31992,d0
	cmp.w	#160,d0
	bcs.s	L3054
	sub.w	d7,d0
L3054:
	rts

type29:		/* 29 - depac ppp,pa3 */
	lea 	L3032(pc),a5
	move.w	#158,d7
	bra		L3132

type30:		/* 30 */
	lea 	L3032(pc),a5
	move.w	#156,d7
	bra		L3184

L3080:
	bsr		L3236
L3084:
	bsr		L3260
	tst.w	d1
	bge.s	L3114
	neg.w	d1
	bra.s	L3102

L3096:
	move.b	(a1)+,0(a2,d0.w)
	jsr		(a5)
L3102:
	dbf		d1,L3096
	bra.s	L3120

L3108:
	move.b	(a1),0(a2,d0.w)
	jsr		(a5)
L3114:
	dbf		d1,L3108
	addq.l	#1,a1
L3120:
	cmp.w	d3,d4
	bne.s	L3084
	bra		depack_end

L3132:
	bsr		L3236
L3136:
	bsr		L3260
	tst.w	d1
	bge.s	L3166
	neg.w	d1
	bra.s	L3154

L3148:
	move.w	(a1)+,0(a2,d0.w)
	jsr		(a5)
L3154:
	dbf		d1,L3148
	bra.s	L3172

L3160:
	move.w	(a1),0(a2,d0.w)
	jsr		(a5)
L3166:
	dbf		d1,L3160
	addq.l	#2,a1
L3172:
	cmp.w	d3,d4
	bne.s	L3136
	bra		depack_end

L3184:
	bsr		L3236
L3188:
	bsr		L3260
	tst.w	d1
	bge.s	L3218
	neg.w	d1
	bra.s	L3206

L3200:
	move.l	(a1)+,0(a2,d0.w)
	jsr		(a5)
L3206:
	dbf		d1,L3200
	bra.s	L3224

L3212:
	move.l	(a1),0(a2,d0.w)
	jsr		(a5)
L3218:
	dbf		d1,L3212
	addq.l	#4,a1
L3224:
	cmp.w	d3,d4
	bne.s	L3188
	bra		depack_end

L3236:
	subq.w	#2,d2
	move.b	0(a1,d2.w),d4
	lsl.w	#8,d4
	move.b	1(a1,d2.w),d4
	sub.w	d4,d2
	movea.l	a1,a3
	adda.l	d2,a3
	moveq	#0,d0
	moveq	#0,d3
	rts

L3260:
	moveq	#0,d1
	tst.b	0(a3,d3.w)
	beq.s	L3286
	cmpi.b	#1,0(a3,d3.w)
	beq.s	L3286
	move.b	0(a3,d3.w),d1
	ext.w	d1
	addq.w	#1,d3
	rts

L3286:
	move.b	0(a3,d3.w),d1
	move.b	1(a3,d3.w),d5
	lsl.w	#8,d5
	move.b	2(a3,d3.w),d5
	addq.w	#3,d3
	cmp.b	#1,d1
	bne.s	L3310
	neg.w	d5
L3310:
	move.w	d5,d1
	rts
