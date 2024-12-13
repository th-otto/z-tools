/*
 * main.c
 *
 *  Created on: 29.05.2013
 *      Author: mfro
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mintbind.h>

extern int main (void);

long _stksize;

#define MINFREE	(8L * 1024L)		/* free at least this much mem on top */
#define MINKEEP (64L * 1024L)		/* keep at least this much mem on stack */

/*
 * replacement for getenv(), because the startup routines
 * does not setup the environ[] array.
 */
char *getenv(const char *str)
{
	BASEPAGE *bp;
	char *env;
	size_t len;
	
	bp = _base;
	env = bp->p_env;
	if (env == 0)
		return NULL;
	len = strlen(str);
	while (*env)
	{
		if (strncmp(env, str, len) == 0 && env[len] == '=')
			return env + len + 1;
		env += strlen(env) + 1;
	}
	return NULL;
}



static void _main (void)
{
	Pterm(main());
	__builtin_unreachable();
}

/* jumped to from startup code */
void _crtinit_noargs(void);

#ifndef __ELF__
/* dummy function for constructors that we don't need */
void __main(void);
void __main(void)
{
}
#endif


void _crtinit_noargs(void)
{
	BASEPAGE *bp;
	long m;
	long freemem;

	bp = _base;

	/* make m the total number of bytes required by program sans stack/heap */
	m = (bp->p_tlen + bp->p_dlen + bp->p_blen + sizeof(BASEPAGE));
	m = (m + 3L) & (~3L);

	/* freemem the amount of free mem accounting for MINFREE at top */
	if ((freemem = (long)bp->p_hitpa - (long)bp - MINFREE - m) <= 0L)
	    goto notenough;

	if (_stksize == 0)			/* free all but MINKEEP */
		_stksize = MINKEEP;

	/* make m the total number of bytes including stack */
	_stksize = _stksize & (~3L);
	m += _stksize;

	/* make sure there's enough room for the stack */
	if (((long)bp + m) > ((long)bp->p_hitpa - MINFREE))
	    goto notenough;

	/*
	 * shrink the TPA,
	 * and set up the new stack to bp + m.
	 * Instead of calling Mshrink() and _setstack, this is done inline here,
	 * because we cannot access the bp parameter after changing the stack anymore.
	 */
	__asm__ __volatile__(
		"\tmovel    %0,%%a0\n"
		"\taddl     %1,%%a0\n"
		"\tlea      -64(%%a0),%%sp\n" /* push some unused space for buggy OS */
		"\tmove.l   %1,-(%%sp)\n"
		"\tmove.l   %0,-(%%sp)\n"
		"\tclr.w    -(%%sp)\n"
		"\tmove.w   #0x4a,-(%%sp)\n" /* Mshrink */
		"\ttrap     #1\n"
		"\tlea      12(%%sp),%%sp\n"
		: /* no outputs */
		: "r"(bp), "r"(m)
		: "d0", "d1", "d2", "a0", "a1", "a2", "cc" AND_MEMORY
	);

	/* local variables must not be accessed after this point,
	   because we just changed the stack */

	_main();
	/* not reached normally */

notenough:
	Pterm(-1);
	__builtin_unreachable();
}
