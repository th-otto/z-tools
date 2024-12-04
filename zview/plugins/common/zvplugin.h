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
 * one day decoders should return error codes instead of FALSE/TRUE
 */
#define RETURN_ERROR(err) return ((err) - (err))
#define RETURN_SUCCESS() return TRUE

/* standard gemdos error codes, these are just here for my reference	*/
/* codecs return gemdos error codes where it makes sense				*/
#define	EC_Fwrite			-10	/* fwrite failed, size in/out didn't match, or negative error code	*/
#define	EC_Fread			-11	/* fread failed, size in/out didn't match, or negative error code	*/
#define	EC_Fopen			-33	/* fopen failed, image file not found	*/
#define	EC_Fcreate			-36	/* fcreate failed						*/
#define	EC_Fclose			-37	/* fclose failed						*/
#define EC_Malloc			-39	/* malloc failed						*/
#define EC_Mfree			-40	/* mfree failed							*/
#define	EC_Fseek			-64	/* fseek failed, truncated file or eof	*/

#define	EC_Ok				0	/* no error */

/* error codes related to file content									*/
/* these will be used in multiple plugins and new ones in the future	*/
#define EC_CompType			10	/* header -> unsupported compression type		*/
#define	EC_DecompError		11	/* error during decompression phase				*/
#define EC_ResolutionType	12	/* header -> unsupported mode code neo/degas	*/
#define EC_ImageType		13  /* header -> unsupported image type				*/
#define	EC_PixelDepth		14	/* header -> unsupported pixel depth			*/
#define	EC_ColorMapDepth	15	/* header -> unsupported color map depth		*/
#define	EC_ColorMapType		16	/* header -> unsupported color map type			*/
#define	EC_FileLength		17	/* incorrect file length						*/
#define	EC_FileId			18	/* header -> unknown file identifier			*/
#define	EC_HeaderLength		19	/* header -> unsupported header length			*/
#define EC_WidthNegative	20	/* header -> image width < 0					*/
#define	EC_HeightNegative	21	/* header -> image height < 0					*/
#define	EC_InternalError	22	/* non-specific, subfunction failure: png/gif	*/
#define	EC_ColorSpace		23	/* header -> unsupported color space			*/
#define	EC_ColorMapLength	24	/* header -> unsupported color map length		*/
#define	EC_MaskType			25	/* header -> unsupported mask type				*/
#define	EC_ChunkId			26	/* unsupported chunk identifier: iff			*/
#define	EC_FileType			27	/* received wrong file type						*/
#define	EC_FrameCount		28	/* frame count exceeds limit					*/
#define	EC_ColorCount		29	/* header -> unsupported color count			*/
#define	EC_BitmapLength		30	/* header -> calc'd length doesn't match		*/
#define	EC_HeaderVersion	31	/* header -> unsupported version				*/
#define	EC_HeightSmall		32	/* header -> unsupported height, to small: fnt	*/
#define	EC_CompLength		33	/* header -> incorrect compressed size: spx 	*/
#define	EC_FrameType		34	/* header -> unsupported frame type: seq 		*/
#define	EC_RequiresNVDI		35	/* NVDI not installed 							*/
#define	EC_FuncNotDefined	36	/* function not implemented						*/
/* new as of 12/18/2021 */
#define	EC_StructLength		37	/* incorrect structure length			*/
#define EC_RequiresJPD		38	/* JPEGD DSP decoder not installed		*/
#define	EC_OpenDriverJPD	39	/* JPEGD OpenDriver() failed			*/
#define	EC_GetImageInfoJPD	40	/* JPEGD GetImageInfo() failed			*/
#define	EC_GetImageSizeJPD	41	/* JPEGD GetImageSize() failed			*/
#define	EC_DecodeImageJPD	42	/* JPEGD DecodeImage() failed			*/
#define EC_OrientationBad	43	/* orientation unsupported				*/
#define EC_BadHandleVDI		44  /* Invalid VDI handle					*/
#define EC_v_open_bmERR		45	/* Failed at v_open_bm()				*/
#define	EC_WordWidth		46	/* Pixel width not divisible by 16		*/
#define	EC_BufferOverrun	47	/* data could exceed buffer, aborted	*/
#define	EC_DecoderOptions	48	/* decoder option out of range			*/
#define	EC_EncoderOptions	49	/* encoder option out of range			*/
#define	EC_WidthOver4K		50	/* width exceeds 4096 pixels			*/
/* new error codes will be added to the end of this list as needed */

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

#ifdef __PUREC__
#define inline
#endif

/*
 * small optimization for Pure-C:
 * perform 16x16 -> 32bit multiplication
 */
#ifdef __PUREC__
static unsigned long ulmul(unsigned short a, unsigned short b) 0xc0c1; /* mulu.w d1,d0 */
#else
static inline unsigned long ulmul(unsigned short a, unsigned short b)
{
	return (unsigned long)a * b;
}
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
