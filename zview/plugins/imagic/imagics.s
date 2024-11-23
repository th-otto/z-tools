/*
 * Imagic image decompressoion code for PureC
 * code taken from Imagic demo -> denisdem.prg
 * size of unpacked data is always 32000 bytes (st resolutions)
 * dis-assembled and modified by Thorsten Otto
 * note: after processing the 64 byte header, call this subroutine
 *     thus source address would be start of file + 64
 */


	.globl depack_imagic

	.text
	
/* void cdecl depack_imagic(void *src, void *dst); */
depack_imagic:
	movem.l    d0-d7/a0-a6,-(a7)
	movea.l    64(a7),a0	/* src_addr */
	movea.l    68(a7),a1	/* dst_addr */
	move.l     #32000,d6
	move.l     a1,start_adr
	movea.l    a1,a3
	adda.w     d6,a3
	move.l     a3,end_adr
	moveq.l    #0,d1
	moveq.l    #0,d2
	move.b     (a0)+,d1  /* column size */
	move.b     (a0)+,d2  /* number of colums */
	move.b     (a0)+,d7  /* escape byte */
	mulu.w     #80,d2
	cmpi.b     #0xFF,d1
	bne.s      depack_imagic_1
	move.w     d6,d1
	move.w     #1,d2
depack_imagic_1:
	movea.w    d2,a4
	move.w     d2,d6
	subq.w     #1,d6
	move.w     d1,d5
	subq.w     #1,d5
	movea.w    d5,a3
	neg.w      d1
	muls.w     d2,d1
	addq.l     #1,d1
	movea.l    d1,a5
	muls.w     d5,d2
	movea.l    d2,a6
	moveq.l    #1,d1
	moveq.l    #3,d2
	moveq.l    #2,d4
	moveq.l    #0,d0
depack_imagic_4:
	move.b     (a0)+,d0
	cmp.b      d0,d7
	beq.s      depack_imagic_2
depack_imagic_5:
	cmpa.l     start_adr,a1
	bmi        depack_imagic_3
	cmpa.l     end_adr,a1
	bpl        depack_imagic_3
	move.b     d0,(a1)
	adda.l     a4,a1
	dbf        d5,depack_imagic_4
	move.w     a3,d5
	adda.l     a5,a1
	dbf        d6,depack_imagic_4
	move.w     a4,d6
	subq.w     #1,d6
	adda.l     a6,a1
	bra.s      depack_imagic_4
depack_imagic_2:
	move.b     (a0)+,d0
	cmp.b      d0,d7
	beq.s      depack_imagic_5
	moveq.l    #0,d3
	cmp.w      d2,d0
	bpl.s      depack_imagic_6
	cmp.b      d4,d0
	bne.s      depack_imagic_7
	move.b     (a0)+,d0
	beq.s      depack_imagic_3
	cmp.w      d2,d0
	bpl.s      depack_imagic_8
	cmp.b      d4,d0
	beq.s      depack_imagic_9
depack_imagic_11:
	cmp.b      d1,d0
	bne.s      depack_imagic_10
	addi.w     #256,d3
	move.b     (a0)+,d0
	bra.s      depack_imagic_11
depack_imagic_10:
	move.b     (a0)+,d0
depack_imagic_8:
	add.w      d0,d3
depack_imagic_13:
	adda.l     a4,a1
	dbf        d5,depack_imagic_12
	move.w     a3,d5
	adda.l     a5,a1
	dbf        d6,depack_imagic_12
	move.w     a4,d6
	subq.w     #1,d6
	adda.l     a6,a1
depack_imagic_12:
	dbf        d3,depack_imagic_13
	bra.s      depack_imagic_4
depack_imagic_7:
	cmp.b      d1,d0
	bne.s      depack_imagic_14
	addi.w     #256,d3
	move.b     (a0)+,d0
	bra.s      depack_imagic_7
depack_imagic_14:
	move.b     (a0)+,d0
depack_imagic_6:
	add.w      d0,d3
	move.b     (a0)+,d0
depack_imagic_16:
	cmpa.l     start_adr,a1
	bmi.s      depack_imagic_3
	cmpa.l     end_adr,a1
	bpl.s      depack_imagic_3
	move.b     d0,(a1)
	adda.l     a4,a1
	dbf        d5,depack_imagic_15
	move.w     a3,d5
	adda.l     a5,a1
	dbf        d6,depack_imagic_15
	move.w     a4,d6
	subq.w     #1,d6
	adda.l     a6,a1
depack_imagic_15:
	dbf        d3,depack_imagic_16
	bra        depack_imagic_4
depack_imagic_9:
	move.b     (a0)+,d0
	beq        depack_imagic_4
	bra.s      depack_imagic_9
depack_imagic_3:
	movem.l    (a7)+,d0-d7/a0-a6
	rts

start_adr: dc.l 0
end_adr: dc.l 0
