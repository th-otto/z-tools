	.globl gfa_depac

	.text

gfa_depac:
	movem.l    d2-d7/a2-a6,-(a7)
	movea.l    48(a7),a5
	movea.l    52(a7),a6
	sub.l      d2,d2
	move.w     56(a7),d2
	move.w     (a5)+,d4
	moveq.l    #15,d3
x10192_1:
	bsr.w      x101b8
	move.w     d5,(a6)+
	subq.l     #2,d2
	bne.s      x10192_1
	movem.l    (a7)+,d2-d7/a2-a6
	rts

x101b8:
	btst       #15,d4
	bne.s      x101b8_1
	clr.w      d5
	clr.w      d6
	bra.s      x101e0
x101b8_1:
	btst       #14,d4
	bne.s      x101b8_3
	move.w     #-1,d5
	move.w     #1,d6
	bra.s      x101e0
x101b8_3:
	move.w     #1,d6
	bsr.s      x101e0
	move.w     d4,d5
	move.w     #15,d6

x101e0:
	move.w     (a5),d0
x101b8_5:
	roxl.w     #1,d0
	roxl.w     #1,d4
	dbf        d3,x101b8_4
	move.w     #15,d3
	addq.l     #2,a5
	move.w     (a5),d0
x101b8_4:
	dbf        d6,x101b8_5
	move.w     d0,(a5)
	clr.l      d0
	rts
