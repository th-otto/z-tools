/* Pablo Paint image decompression code */
/* code taken from Pablo Paint v1.1 -> pablo_c.prg */
/*   removed link/unlink commands */
/*   removed pallette and extra parameter */
/* dis-assembled and modified by Lonny Pursell */
/* Converted for GCC by Thorsten Otto */

/* note: after reading the first 2 ascii lines call this routine */
/*       thus source address would point to the resolution flag */

static void depack(void *src, void *dst)
{
	__asm__ __volatile__(

	" movea.l	%0,%%a0\n"
	" moveq	#0,%%d7\n"
	" move.b	1(%%a0),%%d7\n"		/* compression type */
	" cmp.b	#30,%%d7\n"
	" jbhi	depack_end\n"

	" moveq	#0,%%d2\n"
	" movea.l	%%a0,%%a1\n"
	" movea.l	%1,%%a2\n"
	" move.w	2(%%a1),%%d2\n"
	" sub.w	#36,%%d2\n"
	" adda.l	#36,%%a1\n"			/* skip 4+32 (header) */
	" lea 	X3314(%%pc),%%a3\n"
	" lsl.w	#2,%%d7\n"
	" move.l	0(%%a3,%%d7.w),%%d1\n"
	" jmp	0(%%a3,%%d1.w)\n"

"X3314:\n"
	" dc.l	J1-X3314\n"	/* 98 */
	" dc.l	J2-X3314\n"	/* 116 */
	" dc.l	J3-X3314\n"	/* 148 */
	" dc.l	J4-X3314\n"	/* 186 */
	" dc.l	J5-X3314\n"	/* 224 */
	" dc.l	J6-X3314\n"	/* 278 */
	" dc.l	J7-X3314\n"	/* 338 */
	" dc.l	J8-X3314\n"	/* 398 */
	" dc.l	J9-X3314\n"	/* 404 */
	" dc.l	J10-X3314\n"	/* 408 */
	" dc.l	J11-X3314\n"	/* 412 */
	" dc.l	J12-X3314\n"	/* 474 */
	" dc.l	J13-X3314\n"	/* 540 */
	" dc.l	J14-X3314\n"	/* 606 */
	" dc.l	J15-X3314\n"	/* 678 */
	" dc.l	J16-X3314\n"	/* 754 */
	" dc.l	J17-X3314\n"	/* 830 */
	" dc.l	J18-X3314\n"	/* 844 */
	" dc.l	J19-X3314\n"	/* 854 */
	" dc.l	J20-X3314\n"	/* 864 */
	" dc.l	J21-X3314\n"	/* 892 */
	" dc.l	J22-X3314\n"	/* 908 */
	" dc.l	J23-X3314\n"	/* 924 */
	" dc.l	J24-X3314\n"	/* 940 */
	" dc.l	J25-X3314\n"	/* 956 */
	" dc.l	J26-X3314\n"	/* 972 */
	" dc.l	J27-X3314\n"	/* 996 */
	" dc.l	J28-X3314\n"	/* 1008 */
	" dc.l	J29-X3314\n"	/* 1020 */
	" dc.l	J30-X3314\n"	/* 1056 */
	" dc.l	J31-X3314\n"	/* 1068 */

"J1:\n"		/* 0 - copy src -> dst, uncompressed */
	" move.w	#7999,%%d0\n"
"L2102:\n"
	" move.l	(%%a1)+,(%%a2)+\n"
	" dbf	%%d0,L2102\n"
	" jbra    depack_end\n"

"J2:\n"			/* 1 */
	" lsr.w	#1,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" subq.w	#1,%%d2\n"
"L2124:\n"
	" moveq	#0,%%d3\n"
	" move.b	(%%a3)+,%%d3\n"
"L2128:\n"
	" move.b	(%%a1),(%%a2)+\n"
	" dbf	%%d3,L2128\n"
	" addq.l	#1,%%a1\n"
	" dbf	%%d2,L2124\n"
	" jbra    depack_end\n"

"J3:\n"			/* 2 */
	" divu	#3,%%d2\n"
	" lsl.w	#1,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" lsr.w	#1,%%d2\n"
	" subq.w	#1,%%d2\n"
"L2162:\n"
	" moveq	#0,%%d3\n"
	" move.b	(%%a3)+,%%d3\n"
"L2166:\n"
	" move.w	(%%a1),(%%a2)+\n"
	" dbf	%%d3,L2166\n"
	" addq.l	#2,%%a1\n"
	" dbf	%%d2,L2162\n"
	" jbra    depack_end\n"

"J4:\n"			/* 3 */
	" divu	#5,%%d2\n"
	" lsl.w	#2,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" lsr.w	#2,%%d2\n"
	" subq.w	#1,%%d2\n"
"L2200:\n"
	" moveq	#0,%%d3\n"
	" move.b	(%%a3)+,%%d3\n"
"L2204:\n"
	" move.l	(%%a1),(%%a2)+\n"
	" dbf	%%d3,L2204\n"
	" addq.l	#4,%%a1\n"
	" dbf	%%d2,L2200\n"
	" jbra    depack_end\n"

"J5:\n"			/* 4 */
	" move.l	#160,%%d4\n"
"L2230:\n"
	" lsr.w	#1,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" subq.w	#1,%%d2\n"
	" moveq	#0,%%d1\n"
"L2240:\n"
	" moveq	#0,%%d3\n"
	" move.b	(%%a3)+,%%d3\n"
"L2244:\n"
	" move.b	(%%a1),0(%%a2,%%d1.w)\n"
	" add.w	%%d4,%%d1\n"
	" cmp.w	#31999,%%d1\n"
	" ble.s	L2260\n"
	" moveq	#0,%%d1\n"
	" addq.l	#1,%%a2\n"
"L2260:\n"
	" dbf	%%d3,L2244\n"
	" addq.l	#1,%%a1\n"
	" dbf	%%d2,L2240\n"
	" jbra    depack_end\n"

"J6:\n"			/* 5 */
	" move.l	#160,%%d4\n"
"L2284:\n"
	" divu	#3,%%d2\n"
	" lsl.l	#1,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" lsr.w	#1,%%d2\n"
	" subq.w	#1,%%d2\n"
	" moveq	#0,%%d1\n"
"L2300:\n"
	" moveq	#0,%%d3\n"
	" move.b	(%%a3)+,%%d3\n"
"L2304:\n"
	" move.w	(%%a1),0(%%a2,%%d1.w)\n"
	" add.w	%%d4,%%d1\n"
	" cmp.w	#32000,%%d1\n"
	" blt.s	L2320\n"
	" moveq	#0,%%d1\n"
	" addq.l	#2,%%a2\n"
"L2320:\n"
	" dbf	%%d3,L2304\n"
	" addq.l	#2,%%a1\n"
	" dbf	%%d2,L2300\n"
	" jbra    depack_end\n"

"J7:\n"			/* 6 */
	" move.l	#160,%%d4\n"
"L2344:\n"
	" divu	#5,%%d2\n"
	" lsl.w	#2,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" lsr.w	#2,%%d2\n"
	" subq.w	#1,%%d2\n"
	" moveq	#0,%%d1\n"
"L2360:\n"
	" moveq	#0,%%d3\n"
	" move.b	(%%a3)+,%%d3\n"
"L2364:\n"
	" move.l	(%%a1),0(%%a2,%%d1.w)\n"
	" add.w	%%d4,%%d1\n"
	" cmp.w	#32000,%%d1\n"
	" blt.s	L2380\n"
	" moveq	#0,%%d1\n"
	" addq.l	#4,%%a2\n"
"L2380:\n"
	" dbf	%%d3,L2364\n"
	" addq.l	#4,%%a1\n"
	" dbf	%%d2,L2360\n"
	" jbra    depack_end\n"

"J8:\n"			/* 7 */
	" moveq	#80,%%d4\n"
	" bra	L2230\n"

"J9:\n"			/* 8 */
	" moveq	#80,%%d4\n"
	" bra.s	L2284\n"

"J10:\n"			/* 9 */
	" moveq	#80,%%d4\n"
	" bra.s	L2344\n"

"J11:\n"			/* 10 */
	" lsr.w	#1,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" movea.l	%%a2,%%a0\n"
	" adda.l	#32000,%%a0\n"
	" moveq	#7,%%d0\n"
	" bra.s	L2434\n"

"L2430:\n"
	" tst.w	%%d2\n"
	" bpl.s	L2440\n"
"L2434:\n"
	" move.b	(%%a1)+,%%d1\n"
	" moveq	#0,%%d2\n"
	" move.b	(%%a3)+,%%d2\n"
"L2440:\n"
	" cmpa.l	%%a2,%%a0\n"
	" bls.s	L2456\n"
	" move.b	%%d1,(%%a2)\n"
	" addq.l	#8,%%a2\n"
	" dbf	%%d2,L2440\n"
	" cmpa.l	%%a2,%%a0\n"
	" bhi.s	L2430\n"
"L2456:\n"
	" suba.l	#31999,%%a2\n"
	" dbf	%%d0,L2430\n"
	" jbra    depack_end\n"

"J12:\n"			/* 11 */
	" divu	#3,%%d2\n"
	" lsl.w	#1,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" movea.l	%%a2,%%a0\n"
	" adda.l	#32000,%%a0\n"
	" moveq	#3,%%d0\n"
	" bra.s	L2500\n"

"L2496:\n"
	" tst.w	%%d2\n"
	" bpl.s	L2506\n"
"L2500:\n"
	" move.w	(%%a1)+,%%d1\n"
	" moveq	#0,%%d2\n"
	" move.b	(%%a3)+,%%d2\n"
"L2506:\n"
	" cmpa.l	%%a2,%%a0\n"
	" bls.s	L2522\n"
	" move.w	%%d1,(%%a2)\n"
	" addq.l	#8,%%a2\n"
	" dbf	%%d2,L2506\n"
	" cmpa.l	%%a2,%%a0\n"
	" bhi.s	L2496\n"
"L2522:\n"
	" suba.l	#31998,%%a2\n"
	" dbf	%%d0,L2496\n"
	" jbra    depack_end\n"

"J13:\n"			/* 12 */
	" divu	#5,%%d2\n"
	" lsl.w	#2,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" movea.l	%%a2,%%a0\n"
	" adda.l	#32000,%%a0\n"
	" moveq	#1,%%d0\n"
	" bra.s	L2566\n"

"L2562:\n"
	" tst.w	%%d2\n"
	" bpl.s	L2572\n"
"L2566:\n"
	" move.l	(%%a1)+,%%d1\n"
	" moveq	#0,%%d2\n"
	" move.b	(%%a3)+,%%d2\n"
"L2572:\n"
	" cmpa.l	%%a2,%%a0\n"
	" bls.s	L2588\n"
	" move.l	%%d1,(%%a2)\n"
	" addq.l	#8,%%a2\n"
	" dbf	%%d2,L2572\n"
	" cmpa.l	%%a2,%%a0\n"
	" bhi.s	L2562\n"
"L2588:\n"
	" suba.l	#31996,%%a2\n"
	" dbf	%%d0,L2562\n"
	" jbra    depack_end\n"

"J14:\n"			/* 13 */
	" lsr.w	#1,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" moveq	#0,%%d7\n"
	" moveq	#7,%%d0\n"
	" bra.s	L2622\n"

"L2618:\n"
	" tst.w	%%d2\n"
	" bpl.s	L2628\n"
"L2622:\n"
	" move.b	(%%a1)+,%%d1\n"
	" moveq	#0,%%d2\n"
	" move.b	(%%a3)+,%%d2\n"
"L2628:\n"
	" cmp.w	#32000,%%d7\n"
	" bcc.s	L2652\n"
	" move.b	%%d1,0(%%a2,%%d7.w)\n"
	" add.w	#160,%%d7\n"
	" dbf	%%d2,L2628\n"
	" cmp.w	#32000,%%d7\n"
	" bcs.s	L2618\n"
"L2652:\n"
	" sub.w	#31992,%%d7\n"
	" cmp.w	#160,%%d7\n"
	" bcs.s	L2618\n"
	" sub.w	#159,%%d7\n"
	" dbf	%%d0,L2618\n"
	" jbra    depack_end\n"

"J15:\n"			/* 14 */
	" divu	#3,%%d2\n"
	" lsl.l	#1,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" moveq	#0,%%d7\n"
	" moveq	#3,%%d0\n"
	" bra.s	L2698\n"

"L2694:\n"
	" tst.w	%%d2\n"
	" bpl.s	L2704\n"
"L2698:\n"
	" move.w	(%%a1)+,%%d1\n"
	" moveq	#0,%%d2\n"
	" move.b	(%%a3)+,%%d2\n"
"L2704:\n"
	" cmp.w	#32000,%%d7\n"
	" bcc.s	L2728\n"
	" move.w	%%d1,0(%%a2,%%d7.w)\n"
	" add.w	#160,%%d7\n"
	" dbf	%%d2,L2704\n"
	" cmp.w	#32000,%%d7\n"
	" bcs.s	L2694\n"
"L2728:\n"
	" sub.w	#31992,%%d7\n"
	" cmp.w	#160,%%d7\n"
	" bcs.s	L2694\n"
	" sub.w	#158,%%d7\n"
	" dbf	%%d0,L2694\n"
	" jbra    depack_end\n"

"J16:\n"			/* 15 */
	" divu	#5,%%d2\n"
	" lsl.w	#2,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" moveq	#0,%%d7\n"
	" moveq	#1,%%d0\n"
	" bra.s	L2774\n"

"L2770:\n"
	" tst.w	%%d2\n"
	" bpl.s	L2780\n"
"L2774:\n"
	" move.l	(%%a1)+,%%d1\n"
	" moveq	#0,%%d2\n"
	" move.b	(%%a3)+,%%d2\n"
"L2780:\n"
	" cmp.w	#32000,%%d7\n"
	" bcc.s	L2804\n"
	" move.l	%%d1,0(%%a2,%%d7.w)\n"
	" add.w	#160,%%d7\n"
	" dbf	%%d2,L2780\n"
	" cmp.w	#32000,%%d7\n"
	" bcs.s	L2770\n"
"L2804:\n"
	" sub.w	#31992,%%d7\n"
	" cmp.w	#160,%%d7\n"
	" bcs.s	L2770\n"
	" sub.w	#156,%%d7\n"
	" dbf	%%d0,L2770\n"
	" jbra    depack_end\n"

"J17:\n"		/* 16 */
	" lea 	L2840(%%pc),%%a5\n"
	" moveq	#1,%%d7\n"
	" bra	L3080\n"

"L2840:\n"
	" add.w	%%d7,%%d0\n"
	" rts\n"

"J18:\n"		/* 17 */
	" lea 	L2840(%%pc),%%a5\n"
	" moveq	#2,%%d7\n"
	" bra	L3132\n"

"J19:\n"		/* 18 */
	" lea 	L2840(%%pc),%%a5\n"
	" moveq	#4,%%d7\n"
	" bra	L3184\n"

"J20:\n"		/* 19 */
	" lea 	L2880(%%pc),%%a5\n"
	" move.w	#160,%%d6\n"
	" move.w	#31999,%%d7\n"
	" bra	L3080\n"

"L2880:\n"
	" add.w	%%d6,%%d0\n"
	" cmp.w	#32000,%%d0\n"
	" bcs.s	L2890\n"
	" sub.w	%%d7,%%d0\n"
"L2890:\n"
	" rts\n"

"J21:\n"		/* 20 */
	" lea 	L2880(%%pc),%%a5\n"
	" move.w	#160,%%d6\n"
	" move.w	#31998,%%d7\n"
	" bra	L3132\n"

"J22:\n"		/* 21 */
	" lea 	L2880(%%pc),%%a5\n"
	" move.w	#160,%%d6\n"
	" move.w	#31996,%%d7\n"
	" bra	L3184\n"

"J23:\n"		/* 22 */
	" lea 	L2880(%%pc),%%a5\n"
	" move.w	#80,%%d6\n"
	" move.w	#31999,%%d7\n"
	" bra	L3080\n"

"J24:\n"		/* 23 */
	" lea 	L2880(%%pc),%%a5\n"
	" move.w	#80,%%d6\n"
	" move.w	#31998,%%d7\n"
	" bra	L3132\n"

"J25:\n"		/* 24 */
	" lea 	L2880(%%pc),%%a5\n"
	" move.w	#80,%%d6\n"
	" move.w	#31996,%%d7\n"
	" bra	L3184\n"

"J26:\n"		/* 25 */
	" lea 	L2984(%%pc),%%a5\n"
	" move.w	#31999,%%d7\n"
	" bra	L3080\n"

"L2984:\n"
	" addq.w	#8,%%d0\n"
	" cmp.w	#32000,%%d0\n"
	" bcs.s	L2994\n"
	" sub.w	%%d7,%%d0\n"
"L2994:\n"
	" rts\n"

"J27:\n"		/* 26 */
	" lea 	L2984(%%pc),%%a5\n"
	" move.w	#31998,%%d7\n"
	" bra	L3132\n"

"J28:\n"		/* 27 */
	" lea 	L2984(%%pc),%%a5\n"
	" move.w	#31996,%%d7\n"
	" bra	L3184\n"

"J29:\n"		/* 28 */
	" lea 	L3032(%%pc),%%a5\n"
	" move.w	#159,%%d7\n"
	" bra	L3080\n"

"L3032:\n"
	" add.w	#160,%%d0\n"
	" cmp.w	#32000,%%d0\n"
	" bcs.s	L3054\n"
	" sub.w	#31992,%%d0\n"
	" cmp.w	#160,%%d0\n"
	" bcs.s	L3054\n"
	" sub.w	%%d7,%%d0\n"
"L3054:\n"
	" rts\n"

"J30:\n"		/* 29 - depac ppp,p%%a3 */
	" lea 	L3032(%%pc),%%a5\n"
	" move.w	#158,%%d7\n"
	" bra	L3132\n"

"J31:\n"		/* 30 */
	" lea 	L3032(%%pc),%%a5\n"
	" move.w	#156,%%d7\n"
	" bra	L3184\n"

"L3080:\n"
	" bsr	L3236\n"
"L3084:\n"
	" bsr	L3260\n"
	" tst.w	%%d1\n"
	" bge.s	L3114\n"
	" neg.w	%%d1\n"
	" bra.s	L3102\n"

"L3096:\n"
	" move.b	(%%a1)+,0(%%a2,%%d0.w)\n"
	" jsr	(%%a5)\n"
"L3102:\n"
	" dbf	%%d1,L3096\n"
	" bra.s	L3120\n"

"L3108:\n"
	" move.b	(%%a1),0(%%a2,%%d0.w)\n"
	" jsr	(%%a5)\n"
"L3114:\n"
	" dbf	%%d1,L3108\n"
	" addq.w	#1,%%a1\n"
"L3120:\n"
	" cmp.w	%%d3,%%d4\n"
	" bne.s	L3084\n"
	" jbra    depack_end\n"

"L3132:\n"
	" bsr	L3236\n"
"L3136:\n"
	" bsr	L3260\n"
	" tst.w	%%d1\n"
	" bge.s	L3166\n"
	" neg.w	%%d1\n"
	" bra.s	L3154\n"

"L3148:\n"
	" move.w	(%%a1)+,0(%%a2,%%d0.w)\n"
	" jsr	(%%a5)\n"
"L3154:\n"
	" dbf	%%d1,L3148\n"
	" bra.s	L3172\n"

"L3160:\n"
	" move.w	(%%a1),0(%%a2,%%d0.w)\n"
	" jsr	(%%a5)\n"
"L3166:\n"
	" dbf	%%d1,L3160\n"
	" addq.w	#2,%%a1\n"
"L3172:\n"
	" cmp.w	%%d3,%%d4\n"
	" bne.s	L3136\n"
	" jbra    depack_end\n"

"L3184:\n"
	" bsr	L3236\n"
"L3188:\n"
	" bsr	L3260\n"
	" tst.w	%%d1\n"
	" bge.s	L3218\n"
	" neg.w	%%d1\n"
	" bra.s	L3206\n"

"L3200:\n"
	" move.l	(%%a1)+,0(%%a2,%%d0.w)\n"
	" jsr	(%%a5)\n"
"L3206:\n"
	" dbf	%%d1,L3200\n"
	" bra.s	L3224\n"

"L3212:\n"
	" move.l	(%%a1),0(%%a2,%%d0.w)\n"
	" jsr	(%%a5)\n"
"L3218:\n"
	" dbf	%%d1,L3212\n"
	" addq.w	#4,%%a1\n"
"L3224:\n"
	" cmp.w	%%d3,%%d4\n"
	" bne.s	L3188\n"
	" jbra    depack_end\n"

"L3236:\n"
	" subq.w	#2,%%d2\n"
	" move.b	0(%%a1,%%d2.w),%%d4\n"
	" lsl.w	#8,%%d4\n"
	" move.b	1(%%a1,%%d2.w),%%d4\n"
	" sub.w	%%d4,%%d2\n"
	" movea.l	%%a1,%%a3\n"
	" adda.l	%%d2,%%a3\n"
	" moveq	#0,%%d0\n"
	" moveq	#0,%%d3\n"
	" rts\n"

"L3260:\n"
	" moveq	#0,%%d1\n"
	" tst.b	0(%%a3,%%d3.w)\n"
	" beq.s	L3286\n"
	" cmpi.b	#1,0(%%a3,%%d3.w)\n"
	" beq.s	L3286\n"
	" move.b	0(%%a3,%%d3.w),%%d1\n"
	" ext.w	%%d1\n"
	" addq.w	#1,%%d3\n"
	" rts\n"

"L3286:\n"
	" move.b	0(%%a3,%%d3.w),%%d1\n"
	" move.b	1(%%a3,%%d3.w),%%d5\n"
	" lsl.w	#8,%%d5\n"
	" move.b	2(%%a3,%%d3.w),%%d5\n"
	" addq.w	#3,%%d3\n"
	" cmp.b	#1,%%d1\n"
	" bne.s	L3310\n"
	" neg.w	%%d5\n"
"L3310:\n"
	" move.w	%%d5,%%d1\n"
	" rts\n"

"depack_end:\n"

	:
	: "g"(src), "g"(dst)
	: "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "a2", "a3", "a5", "cc" AND_MEMORY);
}

