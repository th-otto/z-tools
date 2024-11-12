		.globl bst_decompress
		
		.text

/* void bst_decompress(uint32_t *source, uint32_t *screen, int16_t *my_control); */

bst_decompress:
	movem.l    d1-d7/a0-a6,-(a7)
	clr.w      d5
	movea.l    60(a7),a5
	cmpi.w     #20,(a5)
	blt.s      bst_decompress_1
	bra        bst_decompress_2
bst_decompress_1:
	move.w     2(a5),d2			/* get number of control words  */
	move.w     d2,d4			/* off = (cntrl_max - 2) / 2 + 1; */
	subq.w     #2,d4
	ext.l      d4
	divs.w     #2,d4
	addq.w     #1,d4
	move.w     #2,d7			/* for (k = 2; k < cntrl_max; k++) */
	bra.s      bst_decompress_3
bst_decompress_4:
	move.l     a5,d0			/* my_control[k - 2] = tempi[k]; */
	move.w     d7,d1
	asl.w      #1,d1
	ext.l      d1
	add.l      d1,d0
	movea.l    d0,a0
	move.w     (a0),d0
	move.w     d7,d1
	subq.w     #2,d1
	asl.w      #1,d1
	movea.l    68(a7),a0
	move.w     d0,0(a0,d1.w)
	addq.w     #1,d7
bst_decompress_3:
	move.w     d7,d0
	cmp.w      d2,d0
	blt.s      bst_decompress_4

	clr.w      d7				/* for (k = 0; k < cntrl_max - 2; k++) */
	bra        bst_decompress_5
bst_decompress_12:
	move.w     d7,d0			/* if (my_control[k] < 0) */
	asl.w      #1,d0
	movea.l    68(a7),a0
	tst.w      0(a0,d0.w)
	bge.s      bst_decompress_6
	/* deal with compressed stuff */
	clr.w      d6				/* for (m = 0; m < -1 * my_control[k]; m++) */
	bra.s      bst_decompress_7
bst_decompress_8:
	move.w     d4,d0			/* screen[j++] = source[off]; */
	ext.l      d0
	asl.l      #2,d0
	movea.l    60(a7),a0
	move.l     0(a0,d0.l),d0
	move.w     d5,d1
	addq.w     #1,d5
	ext.l      d1
	asl.l      #2,d1
	movea.l    64(a7),a0
	move.l     d0,0(a0,d1.l)
	addq.w     #1,d6
bst_decompress_7:
	move.w     d7,d0
	asl.w      #1,d0
	movea.l    68(a7),a0
	move.w     0(a0,d0.w),d0
	move.w     #0xFFFF,d1
	muls.w     d0,d1
	move.w     d6,d0
	cmp.w      d1,d0
	blt.s      bst_decompress_8
	addq.w     #1,d4			/* off++ */
	bra.s      bst_decompress_9
bst_decompress_6:
	/* deal with raw stuff */
	clr.w      d6				/* for (m = 0; m < my_control[k]; m++) */
	bra.s      bst_decompress_10
bst_decompress_11:
	move.w     d4,d0			/* screen[j++] = source[off++]; */
	addq.w     #1,d4
	ext.l      d0
	asl.l      #2,d0
	movea.l    60(a7),a0
	move.l     0(a0,d0.l),d0
	move.w     d5,d1
	addq.w     #1,d5
	ext.l      d1
	asl.l      #2,d1
	movea.l    64(a7),a0
	move.l     d0,0(a0,d1.l)
	addq.w     #1,d6
bst_decompress_10:
	move.w     d7,d1
	asl.w      #1,d1
	movea.l    68(a7),a0
	move.w     0(a0,d1.w),d1
	move.w     d6,d0
	cmp.w      d1,d0
	blt.s      bst_decompress_11
bst_decompress_9:
	addq.w     #1,d7
bst_decompress_5:
	move.w     d7,d0
	move.w     d2,d1
	subq.w     #2,d1
	cmp.w      d1,d0
	blt        bst_decompress_12
bst_decompress_2:
	movem.l    (a7)+,d1-d7/a0-a6
	rts
