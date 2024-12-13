/*
 * LDG : Gem Dynamical Libraries
 * Copyright (c) 1997-2004 Olivier Landemarre, Dominique Bereziat & Arnaud Bercegeay
 *
 * Header file of LDG devel library, should be included after the AES header
 *
 * Current version is 2.31
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: ldg.h 102 2012-10-04 22:31:16Z vriviere $
 */

#ifndef __LDG__
#define __LDG__


#ifdef __PUREC__
#define CDECL cdecl
#else
#include <compiler.h>
#define CDECL __CDECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GEMLIB__
#define ldg_global	aes_global
#else
#ifdef __PUREC__	/* For Pure C users using PCGEMLIB */
#define ldg_global	_GemParBlk.global
#endif
#endif

#if (!defined(__PUREC__) || !defined(__MSHORT__) || defined(__GEMLIB__)) && !defined(__USE_GEMLIB)
#define __USE_GEMLIB 1
#endif
#ifndef _WORD
# if defined(__GEMLIB__) || defined(__USE_GEMLIB)
#  define _WORD short
# else
#  define _WORD int
# endif
#endif

#define LDG_QUIT       	0x4C4A 		/* ldg->client : a lib discharged */
#define LDG_LOST_LIB	0x4C4D		/* ldg->client : a lib lost */
#define LDG_COOKIE     	0x4C44474DL	/* 'LDGM' */
#define PATHLEN			128

/*
 * Data structures
 */

typedef struct entrie {
	const char *name;  	/* Function name */
	const char *info;  	/* Describe the prototype of the function */
	void *func;  	/* Function address */
} PROC;

/* 
 * The LDG-library descriptor (private)
 */

typedef struct ldg {
	long magic;				/* magic number ('LDGM')   */
	short vers;				/* library version 	*/
	short id;				/* AES-id of launcher */
	short num;				/* count of available functions */
	const PROC *list;		/* pointer to the functions list */
	const char *infos;		/* describe the library */
	void *baspag;			/* basepage of library */
/* from version 0.99 */
	unsigned short flags;	/* library flags ( shared, resident, locked) */
/* from version 0.99c */
	void (*close)(void);	/* function launched by ldg_term() */
/* from version 1.00 */
	short vers_ldg;			/* LDG-protocol version */
	char path[PATHLEN];		/* real path of the lib */
	long user_ext;			/* own library extension */
	long addr_ext;			/* for future extension */
} LDG;

/*
 * The LDG cookie
 */

typedef struct {
	short version;			/* The cookie number version */
	char path[PATHLEN];		/* Path of LDG-libraries */
	short garbage;			/* The garbage collector time */
	short idle;				/* Obsolet : for backward compatibility */
	
	LDG*  CDECL (*ldg_open)	( const char *lib, _WORD *gl);
							/* Open a library */
	short CDECL (*ldg_close) ( LDG *ldg, _WORD *gl);  
							/* Close a library */
	void* CDECL (*ldg_find)	( const char *fnc, LDG *ldg);
							/* Find a function in a library */
	LDG*  CDECL (*ldg_open_ev)( const char *lib, _WORD *gl, void (*)(_WORD *));
							/* Obsolet : for backward compatibility */
	short  error;			/* Last error occuring */
	void  CDECL (*ldg_garbage)( _WORD *gl);
							/* Release unused libs */
	void  CDECL	(*ldg_reset)( _WORD *gl);
							/* Release all libs */
	void* internal;			/* Reserved */
	short CDECL (*ldg_libpath)( char *, _WORD *global);
							/* Find the path of a library */
} LDG_INFOS;

/*
 * This structure is used by ldg_init() to initiate the
 * the LDG-protocol inside a LDG-library.
 */

typedef struct ldglib {
	short vers;   			/* library version */
	short num;    			/* count of available functions */
	const PROC *list;		/* pointer to the functions list */
	const char *infos; 		/* description of the lib */
	unsigned short flags;	/* library flags (shared, locked, resident) */
	void (*close)(void);	/* function executed by ldg_term()*/
	long user_ext;			/* own library extension */
} LDGLIB;

/* value of the 'flag' field */
#define LDG_NOT_SHARED	0x1		/* the library is unshareable */
#define LDG_LOCKED		0x2		/* the library is locked */
#define LDG_RESIDENT	0x4 	/* the library is memory resident */
#define LDG_STDCALL		0x100	/* a2/d2 are not scratch register - private */

/* Errors returned by ldg_error() */
#define LDG_LIB_FULLED	-1
#define LDG_APP_FULLED	-2
#define LDG_ERR_EXEC	-3
#define LDG_BAD_FORMAT	-4
#define LDG_LIB_LOCKED	-6
#define LDG_NO_MANAGER	-7
#define LDG_NOT_FOUND	-8
#define LDG_BAD_LIB		LDG_BAD_FORMAT
#define LDG_NO_MEMORY	-9
#define LDG_TIME_IDLE	-10
#define LDG_NO_TSR		-11
#define LDG_BAD_TSR		-12
#define LDG_NO_FUNC		-13

/* Client functions */
LDG*	ldg_open	( const char *name, _WORD *gl);
short	ldg_close	( LDG *lib, _WORD *gl);
short	ldg_error	( void);
void*	ldg_find	( const char *name, LDG *ldg);
short 	ldg_libpath	( char *path, _WORD *gl);

/* Server functions */
int		ldg_init	( const LDGLIB *libldg);
const char   *ldg_getpath	( void);

/* Diverse functions */
int 	ldg_cookie	( long, long *);
void 	ldg_debug	( const char *, ...);
void*	ldg_Malloc	( long size);
void*	ldg_Calloc	( long count,long size);
int 	ldg_Free	( void *memory);
void*	ldg_Realloc	( void *oldblk, long oldsize, long newsize);
void	ldg_cpush 	( void);

long CDECL ldg_callback( void *f, ...);

/* For backward compatibility */
#define ldg_exec(a,b)	ldg_open( b, ldg_global)
#define ldg_exec_evnt(a,b,c)	ldg_open( b, ldg_global)
#define ldg_term(a,b)	ldg_close( b, ldg_global)
#define ldg_libexec_evnt(a,b,c)	ldg_open( a, b)
#define ldg_libexec(a,b)	ldg_open( a, b)
#define ldg_libterm(a,b)	ldg_close( a,b)

/* C-library version (currently 1.20) */
struct _ldg_version {
	const char *name;
	short  num;
};

extern struct _ldg_version  ldg_version;

#ifdef __cplusplus
}
#endif

#endif /* __LDG__ */
