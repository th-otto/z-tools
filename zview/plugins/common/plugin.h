/*
 * plugin_struct.h - internal header file.
 * List of functions that are imported from the application
 *
 * Copyright (C) 2018 Thorsten Otto
 *
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * This file should only be used by the codecs itself,
 * and by the interface code in the application that loads them.
 * Its purpose is to provide some common library functions to the
 * codecs, and to redirect calls to them.
 */

#ifndef __ZVIEW_PLUGIN_STRUCT_H__
#define __ZVIEW_PLUGIN_STRUCT_H__ 1

#include <stdarg.h>
#include <stdio.h>
#if defined(__PUREC__) && !defined(_COMPILER_H)
#define __CDECL cdecl
#endif
#ifndef __SLB_H
#include <mint/slb.h>
#endif
#include <slb/slbids.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#include <setjmp.h>
#include <time.h>
#if defined(__PUREC__) && !defined(_COMPILER_H)
#include <ext.h>
#else
#include <sys/stat.h>
#endif
#include <stdio.h>
#ifdef __MINT__
#include <fcntl.h>
#endif
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#if defined(__PUREC__) && !defined(_COMPILER_H)
#include <tos.h>
#define BASEPAGE BASPAG
#else
#include <osbind.h>
#include <mintbind.h>
#endif

#include "imginfo.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef K_ALT
#define K_RSHIFT        0x0001
#define K_LSHIFT        0x0002
#define K_CTRL          0x0004
#define K_ALT           0x0008
#endif


#ifndef MIN
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


#include "plugver.h"

struct _zview_plugin_funcs {
	/*
	 * sizeof(this struct), as
	 * used by the application.
	 */
	size_t struct_size;
	/*
	 * sizeof of an int of the caller.
	 * As for now, must match the one which
	 * was used to compile the library (32 bit)
	 */
	size_t int_size;
	/*
	 * version of zview.h the caller used.
	 * As for now, should match the version that was
	 * used to compile the library.
	 */
	long interface_version;

	long __CDECL (*p_slb_open)(long lib, const char *path);
	void __CDECL (*p_slb_close)(long lib);
	SLB *__CDECL (*p_slb_get)(long lib);
	
	void *__CDECL (*p_memset)(void *, zv_int_t, size_t);
	void *__CDECL (*p_memcpy)(void *, const void *, size_t);
	void *__CDECL (*p_memchr)(const void *, zv_int_t, size_t);
	zv_int_t __CDECL (*p_memcmp)(const void *, const void *, size_t);

	size_t __CDECL (*p_strlen)(const char *);
	char *__CDECL (*p_strcpy)(char *, const char *);
	char *__CDECL (*p_strncpy)(char *, const char *, size_t);
	char *__CDECL (*p_strcat)(char *, const char *);
	char *__CDECL (*p_strncat)(char *, const char *, size_t);
	zv_int_t __CDECL (*p_strcmp)(const char *, const char *);
	zv_int_t __CDECL (*p_strncmp)(const char *, const char *, size_t);

	void *__CDECL (*p_malloc)(size_t);
	void *__CDECL (*p_calloc)(size_t, size_t);
	void *__CDECL (*p_realloc)(void *ptr, size_t size);
	void __CDECL (*p_free)(void *);

	zv_int_t __CDECL (*p_get_errno)(void);
	char *__CDECL (*p_strerror)(zv_int_t);
	__attribute__((__noreturn__)) void __CDECL (*p_abort)(void);
	FILE *stderr_location;

	zv_int_t __CDECL (*p_remove)(const char *);

	zv_int_t __CDECL (*p_open)(const char *, zv_int_t, ...);
	zv_int_t __CDECL (*p_close)(zv_int_t);
	ssize_t __CDECL (*p_read)(zv_int_t, void *, size_t);
	ssize_t __CDECL (*p_write)(zv_int_t, const void *, size_t);
	off_t __CDECL (*p_lseek)(zv_int_t, off_t, zv_int_t);

	FILE *__CDECL (*p_fopen)(const char *, const char *);
	FILE *__CDECL (*p_fdopen)(zv_int_t, const char *);
	zv_int_t __CDECL (*p_fclose)(FILE *);
	zv_int_t __CDECL (*p_fseek)(FILE *, long, zv_int_t);
	zv_int_t __CDECL (*p_fseeko)(FILE *, off_t, zv_int_t);
	long __CDECL (*p_ftell)(FILE *);
	off_t __CDECL (*p_ftello)(FILE *);
	zv_int_t __CDECL (*p_printf)(const char *, ...);
	zv_int_t __CDECL (*p_sprintf)(char *, const char *, ...);
	zv_int_t __CDECL (*p_vsnprintf)(char *, size_t, const char *, va_list);
	zv_int_t __CDECL (*p_vfprintf)(FILE *, const char *, va_list);
	size_t __CDECL (*p_fread)(void *, size_t, size_t, FILE *);
	size_t __CDECL (*p_fwrite)(const void *, size_t, size_t, FILE *);
	zv_int_t __CDECL (*p_ferror)(FILE *);
	zv_int_t __CDECL (*p_fflush)(FILE *);

	zv_int_t __CDECL (*p_rand)(void);
	void __CDECL (*p_srand)(zv_uint_t seed);

	void __CDECL (*p_qsort)(void *base, size_t nmemb, size_t size, zv_int_t __CDECL (*compar)(const void *, const void *));
	void *__CDECL (*p_bsearch)(const void *key, const void *base, size_t nmemb, size_t size, zv_int_t __CDECL (*compar)(const void *, const void *));
                     
	time_t __CDECL (*p_time)(time_t *tloc);
	struct tm *__CDECL (*p_localtime)(const time_t *timep);
	struct tm *__CDECL (*p_gmtime)(const time_t *);

	zv_int_t __CDECL (*p_fstat)(zv_int_t fd, struct stat *s);
	
	zv_int_t __CDECL (*p_sigsetjmp)(jmp_buf, zv_int_t);
	__attribute__((__noreturn__)) void __CDECL (*p_longjmp)(jmp_buf, zv_int_t);
	
	double __CDECL (*p_atof)(const char *);

	/*
	 * new in 1.0.3:
	 * functions to convert meta-information in UTF-8 encoding
	 * to atarist
	 */
	unsigned short *__CDECL (*p_utf8_to_ucs16)(const char *s, size_t len);
	char *__CDECL (*p_ucs16_to_latin1)(const unsigned short *u, size_t len);
	void __CDECL (*p_latin1_to_atari)(char *text);

	/* room for later extensions */
	void *unused[28];
};

#ifdef PLUGIN_SLB

#undef memset
#undef memcpy
#undef memmove
#undef memchr
#undef memcmp

#undef strlen
#undef strcpy
#undef strncpy
#undef strcat
#undef strncat
#undef strcmp
#undef strncmp

#undef malloc
#undef calloc
#undef realloc
#undef free

#undef errno
#undef strerror
#undef abort
#undef stderr

#undef remove

#undef open
#undef close
#undef read
#undef write
#undef lseek

#undef fopen
#undef fdopen
#undef fclose
#undef fseek
#undef fseeko
#undef ftell
#undef ftello
#undef printf
#undef sprintf
#undef vsnprintf
#undef vfprintf
#undef fread
#undef fwrite
#undef ferror
#undef fflush

#undef rand
#undef srand

#undef qsort
#undef bsearch

#undef time
#undef localtime
#undef gmtime

#undef fstat

#undef sigsetjmp
#undef setjmp
#undef longjmp

#undef atof

#undef utf8_to_ucs16
#undef ucs16_to_latin1
#undef latin1_to_atari

struct _zview_plugin_funcs *get_slb_funcs(void);

#define memset(d, c, l) get_slb_funcs()->p_memset(d, c, l)
#define memcpy(d, s, l) get_slb_funcs()->p_memcpy(d, s, l)
#define memmove(d, s, l) get_slb_funcs()->p_memcpy(d, s, l)
#define memchr(d, c, l) get_slb_funcs()->p_memchr(d, c, l)
#define memcmp(d, s, l) get_slb_funcs()->p_memcmp(d, s, l)

#define strlen(s) get_slb_funcs()->p_strlen(s)
#define strcpy(d, s) get_slb_funcs()->p_strcpy(d, s)
#define strncpy(d, s, n) get_slb_funcs()->p_strncpy(d, s, n)
#define strcat(d, s) get_slb_funcs()->p_strcat(d, s)
#define strncat(d, s, n) get_slb_funcs()->p_strncat(d, s, n)
#define strcmp(s1, s2) get_slb_funcs()->p_strcmp(s1, s2)
#define strncmp(s1, s2, n) get_slb_funcs()->p_strncmp(s1, s2, n)

#define malloc(s) get_slb_funcs()->p_malloc(s)
#define calloc(n, s) get_slb_funcs()->p_calloc(n, s)
#define realloc(p, s) get_slb_funcs()->p_realloc(p, s)
#define free(p) get_slb_funcs()->p_free(p)

#define errno (get_slb_funcs()->p_get_errno())
#define strerror(e) get_slb_funcs()->p_strerror(e)
#define abort() get_slb_funcs()->p_abort()
#define stderr (get_slb_funcs()->stderr_location)

#define remove(p) get_slb_funcs()->p_remove(p)

#define open get_slb_funcs()->p_open
#define close(f) get_slb_funcs()->p_close(f)
#define read(fildes, buf, nbyte) get_slb_funcs()->p_read(fildes, buf, nbyte)
#define write(fildes, buf, nbyte) get_slb_funcs()->p_write(fildes, buf, nbyte)
#define lseek(fd, offset, whence) get_slb_funcs()->p_lseek(fd, offset, whence)

#define fopen(f, m) get_slb_funcs()->p_fopen(f, m)
#define fdopen(fd, m) get_slb_funcs()->p_fdopen(fd, m)
#define fclose(f) get_slb_funcs()->p_fclose(f)
#define fseek(f, offset, whence) get_slb_funcs()->p_fseek(f, offset, whence)
#define fseeko(f, offset, whence) get_slb_funcs()->p_fseeko(f, offset, whence)
#define ftell(f) get_slb_funcs()->p_ftell(f)
#define ftello(f) get_slb_funcs()->p_ftello(f)
#define fread(ptr, size, nmemb, f) get_slb_funcs()->p_fread(ptr, size, nmemb, f)
#define fwrite(ptr, size, nmemb, f) get_slb_funcs()->p_fwrite(ptr, size, nmemb, f)
#define ferror(f) get_slb_funcs()->p_ferror(f)
#define fflush(f) get_slb_funcs()->p_fflush(f)

#define printf get_slb_funcs()->p_printf
#define sprintf get_slb_funcs()->p_sprintf
#define vsnprintf(str, size, format, ap) get_slb_funcs()->p_vsnprintf(str, size, format, ap)
#define vfprintf(f, format, ap) get_slb_funcs()->p_vfprintf(f, format, ap)

#if 0
#undef vsnprintf
#define vsnprintf my_vsnprintf
int vsnprintf(char *str, size_t size, const char *fmt, va_list va);
void nf_debugprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#endif

#define rand() get_slb_funcs()->p_rand()
#define srand(seed) get_slb_funcs()->p_srand(seed)

#define qsort(base, nmemb, size, compar) get_slb_funcs()->p_qsort(base, nmemb, size, compar)
#define bsearch(key, base, nmemb, size, compar) get_slb_funcs()->p_bsearch(key, base, nmemb, size, compar)

#define time(t) get_slb_funcs()->p_time(t)
#define localtime(t) get_slb_funcs()->p_localtime(t)
#define gmtime(tm) get_slb_funcs()->p_gmtime(tm)

#define fstat(fd, s) get_slb_funcs()->p_fstat(fd, s)

#define sigsetjmp(env, val) get_slb_funcs()->p_sigsetjmp(env, val)
#define setjmp(j) get_slb_funcs()->p_sigsetjmp(j, 1)
#define longjmp(env, val) get_slb_funcs()->p_longjmp(env, val)

#define atof(x) get_slb_funcs()->p_atof(x)

#define utf8_to_ucs16 get_slb_funcs()->p_utf8_to_ucs16
#define ucs16_to_latin1 get_slb_funcs()->p_ucs16_to_latin1
#define latin1_to_atari get_slb_funcs()->p_latin1_to_atari

#ifdef __PUREC__
/* not implemented yet; needs format conversion */
#undef atof
#define atof(x) function_not_implemented
#undef fstat
#define fstat(fd, s) function_not_implemented
#endif

#endif /* PLUGIN_SLB */

#if !defined(__MINT__)
/*
 * be sure to get the TOS error codes here,
 * not any pseudo UNIX-style error numbers
 */
#undef ENOSYS
#define ENOSYS 32
#undef ENOENT
#define ENOENT 33
#undef EINVAL
#define EINVAL 25
#undef ENOMEM
#define ENOMEM 39
#undef EBADARG
#define EBADARG 64
#undef ERANGE
#define ERANGE 88
#endif

#ifdef __cplusplus
}
#endif

#endif
