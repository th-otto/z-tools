/*
 * functions that are called by the application
 */
#ifndef __SLB_H
#include <mint/slb.h>
#endif
#include "imginfo.h"
#if defined(__PUREC__) && !defined(_COMPILER_H)
#define __CDECL cdecl
#endif

extern char *zview_slb_dir;
extern char *zview_slb_dir_end;

long plugin_open(const char *name, const char *path, SLB *slb);
void plugin_close(SLB *slb, boolean waitpid);

boolean __CDECL plugin_reader_init(SLB *slb, const char *name, IMGINFO info);
boolean __CDECL plugin_reader_read(SLB *slb, IMGINFO info, uint8_t *buffer);
void __CDECL plugin_reader_get_txt(SLB *slb, IMGINFO info, txt_data *txtdata);
void __CDECL plugin_reader_quit(SLB *slb, IMGINFO info);
boolean __CDECL plugin_encoder_init(SLB *slb, const char *name, IMGINFO info);
boolean __CDECL plugin_encoder_write(SLB *slb, IMGINFO info, uint8_t *buffer);
void __CDECL plugin_encoder_quit(SLB *slb, IMGINFO info);

/*
 * Parameters to get_option()/set_option()
 * OPTION_CAPABILITIES and OPTION_EXTENSIONS are
 * mandatory for get_option, others are optional
 */
#define OPTION_CAPABILITIES 0
#define OPTION_EXTENSIONS   1
#define OPTION_QUALITY      2
#define OPTION_COLOR_SPACE  3
#define OPTION_PROGRESSIVE  4
#define OPTION_COMPRESSION  5 /* set compression method: LZW, deflate etc. */
#define OPTION_COMPRLEVEL  11 /* set compression level: for deflate, zstd etc. */

/*
 * below are only used to display codec information
 */
#define INFO_NAME           6	/* a single string */
#define INFO_VERSION        7	/* should be a BCD encoded version, like 0x102 */
#define INFO_AUTHOR         8	/* a single string */
#define INFO_DATETIME       9	/* a single string, in the format produced by the __DATE__ macro, optionally also with time */
#define INFO_MISC          10	/* other information, can be multiple lines separated by '\n' */
#define INFO_COMPILER      12   /* compiler used */


/*
 * Flags for get_option(OPTION_CAPABILITIES)
 */
#define CAN_DECODE 0x01
#define CAN_ENCODE 0x02


/*
 * bitmasks returned by plugin_compile_flags()
 */
#define SLB_M68020   (1L << 16)  /* was compiled for m68020 or better */
#define SLB_COLDFIRE (1L << 17)  /* was compiled for ColdFire */


long __CDECL plugin_get_option(SLB *slb, zv_int_t which);
long __CDECL plugin_set_option(SLB *slb, zv_int_t which, zv_int_t value);

/*
 * internal functions used while loading libraries
 */
long plugin_slb_control(SLB *slb, long fn, void *arg);

#define plugin_compile_flags(slb) plugin_slb_control(slb, 0, 0)
#define plugin_set_imports(slb, f) plugin_slb_control(slb, 1, f)
#define plugin_get_basepage(slb) ((BASEPAGE *)plugin_slb_control(slb, 2, 0))
#define plugin_get_header(slb) plugin_slb_control(slb, 3, 0)
#define plugin_get_libpath(slb) ((const char *)plugin_slb_control(slb, 4, 0))
#define plugin_required_libs(slb) ((const char *)plugin_slb_control(slb, 5, 0))

/*
 * callback functions used to load other shared libs like zlib etc.
 */
long __CDECL plugin_slb_open(long lib, const char *path);
void __CDECL plugin_slb_close(long lib);
SLB *__CDECL plugin_slb_get(long lib);


/*
 * entry points in the plugin
 */
boolean __CDECL reader_init(const char *name, IMGINFO info);
boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer);
void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata);
void __CDECL reader_quit(IMGINFO info);
boolean __CDECL encoder_init(const char *name, IMGINFO info);
boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer);
void __CDECL encoder_quit(IMGINFO info);
long __CDECL get_option(zv_int_t which);
long __CDECL set_option(zv_int_t which, zv_int_t value);

#define stringify1(x) #x
#define stringify(x) stringify1(x)

#if (defined(__INTEL_CLANG_COMPILER) || defined(__INTEL_LLVM_COMPILER) || defined(_ICC)) && defined(__VERSION__)
#define COMPILER_VERSION_STRING __VERSION__ bitvers
#elif defined(__clang_version__)
#define COMPILER_VERSION_STRING "Clang version %s (%s) " __clang_version__ bitvers
#elif defined(__GNUC__)
#define COMPILER_VERSION_STRING "GNU-C version " stringify(__GNUC__) "." stringify(__GNUC_MINOR__) "." stringify(__GNUC_PATCHLEVEL__) bitvers
#elif defined(__AHCC__)
#define bitvers " (16-bit)"
#define COMPILER_VERSION_STRING "AHCC version " stringify(__AHCC__) bitvers
#elif defined(__PUREC__)
#define bitvers " (16-bit)"
#define COMPILER_VERSION_STRING "Pure-C version " stringify(__PUREC__) bitvers
#else
#define COMPILER_VERSION_STRING "Unknown" bitvers
#endif
#ifndef bitvers
# if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__) >= 16
#   define bitvers " (128-bit)"
# elif defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__) >= 8
#   define bitvers " (64-bit)"
# elif defined(__SIZEOF_INT__) && (__SIZEOF_INT__) >= 4
#   define bitvers " (32-bit)"
# elif defined(__SIZEOF_INT__) && (__SIZEOF_INT__) >= 2
#   define bitvers " (16-bit)"
# else
#   define bitvers ""
# endif
#endif

/*
 * helper function to get the actual basepage start,
 * if executable has some extended header,
 * like MiNT a.out/ELF headers
 */
static inline const BASEPAGE *get_bp_address(const char *header)
{
	const BASEPAGE *bp;
	const long *exec_longs;

	bp = (const BASEPAGE *)header - 1;
	exec_longs = (const long *)((const char *)bp + 28);
	if ((exec_longs[0] == 0x283a001aL && exec_longs[1] == 0x4efb48faL) ||	/* Original binutils */
		(exec_longs[0] == 0x203a001aL && exec_longs[1] == 0x4efb08faL))     /* binutils >= 2.18-mint-20080209 */
	{
		bp = (const BASEPAGE *)((const char *)bp - 228);
	} else if ((exec_longs[0] & 0xffffff00L) == 0x203a0000L &&              /* binutils >= 2.41-mintelf */
		exec_longs[1] == 0x4efb08faUL &&
		/*
		 * 40 = (minimum) offset of elf header from start of file
		 * 24 = offset of e_entry in common header
		 * 30 = branch offset (sizeof(GEMDOS header) + 2)
		 */
		(exec_longs[0] & 0xff) >= (40 + 24 - 30))
	{
		long elf_offset;
		long e_entry;

		elf_offset = (exec_longs[0] & 0xff);
		e_entry = *((long *)((char *)bp + elf_offset + 2));
		bp = (const BASEPAGE *) ((const char *) bp - e_entry);
	}
	return bp;
}
