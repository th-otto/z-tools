	.globl convert_high_resolution
		
	.text

/* void __CDECL convert_high_resolution(const uint8_t *src, uint8_t *dst); */
convert_high_resolution:
	movem.l    d0-d7/a0-a6,-(a7)
	movea.l    64(a7),a5
	movea.l    68(a7),a4
	move.w     #640-1,d6
flip_orient_3:
	move.w     #200-1,d7
	moveq.l    #0,d3
flip_orient_1:
	move.b     (a5)+,d0
	move.b     d0,0(a4,d3.l)
	add.l      #1280,d3
	dbf        d7,flip_orient_1

	move.w     #200-1,d7
	move.l     #640,d3
flip_orient_2:
	move.b     (a5)+,d0
	move.b     d0,0(a4,d3.l)
	add.l      #1280,d3
	dbf        d7,flip_orient_2
	addq.l     #1,a4
	dbf        d6,flip_orient_3
	movem.l    (a7)+,d0-d7/a0-a6
	rts
