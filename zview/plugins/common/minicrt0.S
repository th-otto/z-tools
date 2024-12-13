/*
 * Initialization code; this is common to both 16 and 32 bit libraries,
 * so be careful!
 *
 * This is is *very* minimalistic version that does not do any argument
 * parsing. Suitable for ACCs, and auto-folder programs.
 *
 * No __FASTCALL__ modifications required.
 */

#include "asmdefs.h"

	.xref C_SYMBOL_NAME(_crtinit_noargs)
	.xref C_SYMBOL_NAME(_base)
	.globl	_start

	.text
_start:
	subl	a6,a6		/* clear a6 for debuggers */
	movel	4(sp),a0	/* get basepage */
	movel	a0,C_SYMBOL_NAME(_base)	/* save it */
	movel	4(a0),d0	/* get _base->p_hitpa */
	bclr	#0,d0		/* round off */
	movel	d0,sp		/* set stack (temporarily) */
	lea		-64(sp),sp	/* see note in mintlib/setstack.S */
	jmp	C_SYMBOL_NAME(_crtinit_noargs)	/* in minimain.c */
