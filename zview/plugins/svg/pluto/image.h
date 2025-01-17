/* stb_image - v2.30 - public domain image loader - 
                                  no warranty implied; use at your own risk

   Do this:
      #define STB_IMAGE_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.

   i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define STB_IMAGE_IMPLEMENTATION
   #include "stb_image.h"

   You can #define STBI_ASSERT(x) before the #include to avoid using assert.h.
   And #define STBI_MALLOC, STBI_REALLOC, and STBI_FREE to avoid using malloc,realloc,free


   QUICK NOTES:
      Primarily of interest to game developers and other people who can
          avoid problematic images and only need the trivial interface

      JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
      PNG 1/2/4/8/16-bit-per-channel

      BMP non-1bpp, non-RLE
      PSD (composited view only, no extra channels, 8/16 bit-per-channel)

      GIF (*comp always reports as 4-channel)
      HDR (radiance rgbE format)
      PIC (Softimage PIC)
      PNM (PPM and PGM binary only)

      Animated GIF still needs a proper API, but here's one way to do it:
          http://gist.github.com/urraka/685d9a6340b26b830d49

      - decode from memory or through FILE (define STBI_NO_STDIO to remove code)
      - decode from arbitrary I/O callbacks
      - SIMD acceleration on x86/x64 (SSE2) and ARM (NEON)

   Full documentation under "DOCUMENTATION" below.


LICENSE

  See end of file for license information.

RECENT REVISION HISTORY:

      2.30  (2024-05-31) avoid erroneous gcc warning
      2.29  (2023-05-xx) optimizations
      2.28  (2023-01-29) many error fixes, security errors, just tons of stuff
      2.27  (2021-07-11) document stbi_info better, 16-bit PNM support, bug fixes
      2.26  (2020-07-13) many minor fixes
      2.25  (2020-02-02) fix warnings
      2.24  (2020-02-02) fix warnings; thread-local failure_reason and flip_vertically
      2.23  (2019-08-11) fix clang static analysis warning
      2.22  (2019-03-04) gif fixes, fix warnings
      2.21  (2019-02-25) fix typo in comment
      2.20  (2019-02-07) support utf8 filenames in Windows; fix warnings and platform ifdefs
      2.19  (2018-02-11) fix warning
      2.18  (2018-01-30) fix warnings
      2.17  (2018-01-29) bugfix, 1-bit BMP, 16-bitness query, fix warnings
      2.16  (2017-07-23) all functions have 16-bit variants; optimizations; bugfixes
      2.15  (2017-03-18) fix png-1,2,4; all Imagenet JPGs; no runtime SSE detection on GCC
      2.14  (2017-03-03) remove deprecated STBI_JPEG_OLD; fixes for Imagenet JPGs
      2.13  (2016-12-04) experimental 16-bit API, only for PNG so far; fixes
      2.12  (2016-04-02) fix typo in 2.11 PSD fix that caused crashes
      2.11  (2016-04-02) 16-bit PNGS; enable SSE2 in non-gcc x64
                         RGB-format JPEG; remove white matting in PSD;
                         allocate large structures on the stack;
                         correct channel count for PNG & BMP
      2.10  (2016-01-22) avoid warning introduced in 2.09
      2.09  (2016-01-16) 16-bit TGA; comments in PNM files; STBI_REALLOC_SIZED

   See end of file for full revision history.


 ============================    Contributors    =========================

 Image formats                          Extensions, features
    Sean Barrett (jpeg, png, bmp)          Jetro Lauha (stbi_info)
    Nicolas Schulz (hdr, psd)              Martin "SpartanJ" Golini (stbi_info)
    Jonathan Dummer (tga)                  James "moose2000" Brown (iPhone PNG)
    Jean-Marc Lienher (gif)                Ben "Disch" Wenger (io callbacks)
    Tom Seddon (pic)                       Omar Cornut (1/2/4-bit PNG)
    Thatcher Ulrich (psd)                  Nicolas Guillemot (vertical flip)
    Ken Miller (pgm, ppm)                  Richard Mitton (16-bit PSD)
    github:urraka (animated gif)           Junggon Kim (PNM comments)
    Christopher Forseth (animated gif)     Daniel Gibson (16-bit TGA)
                                           socks-the-fox (16-bit PNG)
                                           Jeremy Sawicki (handle all ImageNet JPGs)
 Optimizations & bugfixes                  Mikhail Morozov (1-bit BMP)
    Fabian "ryg" Giesen                    Anael Seghezzi (is-16-bit query)
    Arseny Kapoulkine                      Simon Breuss (16-bit PNM)
    John-Mark Allen
    Carmelo J Fdez-Aguera

 Bug & warning fixes
    Marc LeBlanc            David Woo          Guillaume George     Martins Mozeiko
    Christpher Lloyd        Jerry Jansson      Joseph Thomson       Blazej Dariusz Roszkowski
    Phil Jordan                                Dave Moore           Roy Eltham
    Hayaki Saito            Nathan Reed        Won Chun
    Luke Graham             Johan Duparc       Nick Verigakis       the Horde3D community
    Thomas Ruf              Ronny Chevalier                         github:rlyeh
    Janez Zemva             John Bartholomew   Michal Cichon        github:romigrou
    Jonathan Blow           Ken Hamada         Tero Hanninen        github:svdijk
    Eugene Golushkov        Laurent Gomila     Cort Stratton        github:snagar
    Aruelien Pocheville     Sergio Gonzalez    Thibault Reuille     github:Zelex
    Cass Everitt            Ryamond Barbiero                        github:grim210
    Paul Du Bois            Engin Manap        Aldo Culquicondor    github:sammyhw
    Philipp Wiesemann       Dale Weiler        Oriol Ferrer Mesia   github:phprus
    Josh Tobin              Neil Bickford      Matthew Gregan       github:poppolopoppo
    Julian Raschke          Gregory Mullen     Christian Floisand   github:darealshinji
    Baldur Karlsson         Kevin Schmidt      JR Smith             github:Michaelangel007
                            Brad Weinberger    Matvey Cherevko      github:mosra
    Luca Sas                Alexander Veselov  Zack Middleton       [reserved]
    Ryan C. Gordon          [reserved]                              [reserved]
                     DO NOT ADD YOUR NAME HERE

                     Jacko Dirks

  To add your name to the credits, pick a random blank space in the middle and fill it.
  80% of merge conflicts on stb PRs are due to people adding their name at the end
  of the credits.
*/

#ifndef PLUTOVG_STB_IMAGE_H
#define PLUTOVG_STB_IMAGE_H

/*
 * DOCUMENTATION
 *
 * Limitations:
 *    - no 12-bit-per-channel JPEG
 *    - no JPEGs with arithmetic coding
 *    - GIF always returns *comp=4
 *
 * Basic usage (see HDR discussion below for HDR usage):
 *    int x,y,n;
 *    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
 *     * ... process data if not NULL ...
 *     * ... x = width, y = height, n = # 8-bit components per pixel ...
 *     * ... replace '0' with '1'..'4' to force that many components per pixel
 *     * ... but 'n' will always be the number that it would have been if you said 0
 *    stbi_image_free(data);
 *
 * Standard parameters:
 *    int *x                 -- outputs image width in pixels
 *    int *y                 -- outputs image height in pixels
 *    int *channels_in_file  -- outputs # of image components in image file
 *    int desired_channels   -- if non-zero, # of image components requested in result
 *
 * The return value from an image loader is an 'unsigned char *' which points
 * to the pixel data, or NULL on an allocation failure or if the image is
 * corrupt or invalid. The pixel data consists of *y scanlines of *x pixels,
 * with each pixel consisting of N interleaved 8-bit components; the first
 * pixel pointed to is top-left-most in the image. There is no padding between
 * image scanlines or between pixels, regardless of format. The number of
 * components N is 'desired_channels' if desired_channels is non-zero, or
 * *channels_in_file otherwise. If desired_channels is non-zero,
 * *channels_in_file has the number of components that _would_ have been
 * output otherwise. E.g. if you set desired_channels to 4, you will always
 * get RGBA output, but you can check *channels_in_file to see if it's trivially
 * opaque because e.g. there were only 3 channels in the source image.
 *
 * An output image with N components has the following components interleaved
 * in this order in each pixel:
 *
 *     N=#comp     components
 *       1           grey
 *       2           grey, alpha
 *       3           red, green, blue
 *       4           red, green, blue, alpha
 *
 * If image loading fails for any reason, the return value will be NULL,
 * and *x, *y, *channels_in_file will be unchanged. The function
 * stbi_failure_reason() can be queried for an extremely brief, end-user
 * unfriendly explanation of why the load failed. Define STBI_NO_FAILURE_STRINGS
 * to avoid compiling these strings at all, and STBI_FAILURE_USERMSG to get slightly
 * more user-friendly ones.
 *
 * Paletted PNG, BMP, GIF, and PIC images are automatically depalettized.
 *
 * To query the width, height and component count of an image without having to
 * decode the full file, you can use the stbi_info family of functions:
 *
 *   int x,y,n,ok;
 *   ok = stbi_info(filename, &x, &y, &n);
 *    * returns ok=1 and sets x, y, n if image is a supported format,
 *    * 0 otherwise.
 *
 * Note that stb_image pervasively uses ints in its public API for sizes,
 * including sizes of memory buffers. This is now part of the API and thus
 * hard to change without causing breakage. As a result, the various image
 * loaders all have certain limits on image size; these differ somewhat
 * by format but generally boil down to either just under 2GB or just under
 * 1GB. When the decoded image would be larger than this, stb_image decoding
 * will fail.
 *
 * Additionally, stb_image will reject image files that have any of their
 * dimensions set to a larger value than the configurable STBI_MAX_DIMENSIONS,
 * which defaults to 2**24 = 16777216 pixels. Due to the above memory limit,
 * the only way to have an image with such dimensions load correctly
 * is for it to have a rather extreme aspect ratio. Either way, the
 * assumption here is that such larger images are likely to be malformed
 * or malicious. If you do need to load an image with individual dimensions
 * larger than that, and it still fits in the overall size limit, you can
 * #define STBI_MAX_DIMENSIONS on your own to be something larger.
 *
 * ===========================================================================
 *
 * UNICODE:
 *
 *   If compiling for Windows and you wish to use Unicode filenames, compile
 *   with
 *       #define STBI_WINDOWS_UTF8
 *   and pass utf8-encoded filenames. Call stbi_convert_wchar_to_utf8 to convert
 *   Windows wchar_t filenames to utf8.
 *
 * ===========================================================================
 *
 * Philosophy
 *
 * stb libraries are designed with the following priorities:
 *
 *    1. easy to use
 *    2. easy to maintain
 *    3. good performance
 *
 * Sometimes I let "good performance" creep up in priority over "easy to maintain",
 * and for best performance I may provide less-easy-to-use APIs that give higher
 * performance, in addition to the easy-to-use ones. Nevertheless, it's important
 * to keep in mind that from the standpoint of you, a client of this library,
 * all you care about is #1 and #3, and stb libraries DO NOT emphasize #3 above all.
 *
 * Some secondary priorities arise directly from the first two, some of which
 * provide more explicit reasons why performance can't be emphasized.
 *
 *    - Portable ("ease of use")
 *    - Small source code footprint ("easy to maintain")
 *    - No dependencies ("ease of use")
 *
 * ===========================================================================
 *
 * I/O callbacks
 *
 * I/O callbacks allow you to read from arbitrary sources, like packaged
 * files or some other source. Data read from callbacks are processed
 * through a small internal buffer (currently 128 bytes) to try to reduce
 * overhead.
 *
 * The three functions you must define are "read" (reads some bytes of data),
 * "skip" (skips some bytes of data), "eof" (reports if the stream is at the end).
 *
 * ===========================================================================
 *
 * iPhone PNG support:
 *
 * We optionally support converting iPhone-formatted PNGs (which store
 * premultiplied BGRA) back to RGB, even though they're internally encoded
 * differently. To enable this conversion, call
 * stbi_convert_iphone_png_to_rgb(1).
 *
 * Call stbi_set_unpremultiply_on_load(1) as well to force a divide per
 * pixel to remove any premultiplied alpha *only* if the image file explicitly
 * says there's premultiplied data (currently only happens in iPhone images,
 * and only if iPhone convert-to-rgb processing is on).
 *
 * ===========================================================================
 *
 * ADDITIONAL CONFIGURATION
 *
 *  - You can suppress implementation of any of the decoders to reduce
 *    your code footprint by #defining one or more of the following
 *    symbols before creating the implementation.
 *
 *        STBI_NO_PNG
 *
 *   - If you use STBI_NO_PNG (or _ONLY_ without PNG), and you still
 *     want the zlib decoder to be available, #define STBI_SUPPORT_ZLIB
 *
 *  - If you define STBI_MAX_DIMENSIONS, stb_image will reject images greater
 *    than that size (in either width or height) without further processing.
 *    This is to let programs in the wild set an upper bound to prevent
 *    denial-of-service attacks on untrusted data, as one could generate a
 *    valid image of gigantic dimensions and force stb_image to allocate a
 *    huge block of memory and spend disproportionate time decoding it. By
 *    default this is set to (1 << 24), which is 16777216, but that's still
 *    very big.
 */

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

#include <stdint.h>
typedef uint16_t stbi__uint16;
typedef int16_t stbi__int16;
typedef uint32_t stbi__uint32;
typedef int32_t stbi__int32;

#define STBI_VERSION 1

enum
{
	STBI_default = 0,					 /* only used for desired_channels */

	STBI_grey = 1,
	STBI_grey_alpha = 2,
	STBI_rgb = 3,
	STBI_rgb_alpha = 4
};

#include <stdlib.h>
typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STBIDEF
#ifdef STB_IMAGE_STATIC
#define STBIDEF static
#else
#define STBIDEF extern
#endif
#endif

/* **************************************************************************** */
/*
 * PRIMARY API - works on images of any type
 */

/*
 * load image by filename, open file, or memory buffer
 */

typedef struct
{
	plutovg_int_t (*read)(void *user, char *data, plutovg_int_t size);	/* fill 'data' with 'size' bytes.  return number of bytes actually read */
	void (*skip)(void *user, plutovg_int_t n);	/* skip the next 'n' bytes, or 'unget' the last -n bytes if negative */
	int (*eof)(void *user);			/* returns nonzero if we are at end of file/data */
} stbi_io_callbacks;

/* **************************************************************************** */
/*
 * 8-bits-per-channel interface
 */

STBIDEF stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, plutovg_int_t len, plutovg_int_t *x, plutovg_int_t *y, int *channels_in_file,
	int desired_channels);
STBIDEF stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, plutovg_int_t *x, plutovg_int_t *y,
	int *channels_in_file, int desired_channels);

#ifndef STBI_NO_STDIO
STBIDEF stbi_uc *stbi_load(char const *filename, plutovg_int_t *x, plutovg_int_t *y, int *channels_in_file, int desired_channels);
STBIDEF stbi_uc *stbi_load_from_file(FILE * f, plutovg_int_t *x, plutovg_int_t *y, int *channels_in_file, int desired_channels);
/* for stbi_load_from_file, file pointer is left pointing immediately after image */
#endif

#ifdef STBI_WINDOWS_UTF8
STBIDEF int stbi_convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t *input);
#endif

/* **************************************************************************** */
/*
 * float-per-channel interface
 */
/* get a VERY brief reason for failure */
/* on most compilers (and ALL modern mainstream compilers) this is threadsafe */
STBIDEF const char *stbi_failure_reason(void);

/* free the loaded image -- this is just free() */
STBIDEF void stbi_image_free(void *retval_from_stbi_load);

#ifndef STBI_NO_STDIO
STBIDEF int stbi_info(char const *filename, plutovg_int_t *x, plutovg_int_t *y, int *comp);
STBIDEF int stbi_info_from_file(FILE * f, plutovg_int_t *x, plutovg_int_t *y, int *comp);
#endif



/*
 * for image formats that explicitly notate that they have premultiplied alpha.
 * we just return the colors as stored in the file. set this flag to force 
 * unpremultiplication. results are undefined if the unpremultiply overflow.
 */
STBIDEF void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply);

/*
 * indicate whether we should process iphone images back to canonical format,
 * or just pass them through "as-is"
 */
STBIDEF void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert);

/*
 * as above, but only applies to images loaded on the thread that calls the function
 * this function is only available if your compiler supports thread-local variables;
 * calling it will fail to link if your compiler doesn't
 */
STBIDEF void stbi_set_unpremultiply_on_load_thread(int flag_true_if_should_unpremultiply);
STBIDEF void stbi_convert_iphone_png_to_rgb_thread(int flag_true_if_should_convert);

/* ZLIB client - used by PNG, available for other purposes */

STBIDEF char *stbi_zlib_decode_malloc_guesssize(const char *buffer, plutovg_int_t len, plutovg_int_t initial_size, stbi__uint32 *outlen);
STBIDEF char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, plutovg_int_t len, stbi__uint32 initial_size, stbi__uint32 *outlen, int parse_header);
STBIDEF char *stbi_zlib_decode_malloc(const char *buffer, plutovg_int_t len, stbi__uint32 *outlen);
STBIDEF plutovg_int_t stbi_zlib_decode_buffer(char *obuffer, plutovg_int_t olen, const char *ibuffer, plutovg_int_t ilen);

STBIDEF char *stbi_zlib_decode_noheader_malloc(const char *buffer, plutovg_int_t len, stbi__uint32 *outlen);
STBIDEF plutovg_int_t stbi_zlib_decode_noheader_buffer(char *obuffer, plutovg_int_t olen, const char *ibuffer, plutovg_int_t ilen);


#ifdef __cplusplus
}
#endif

/*   end header file */
#endif									/* PLUTOVG_STB_IMAGE_H */

#ifdef STB_IMAGE_IMPLEMENTATION

#if defined(STBI_NO_PNG) && !defined(STBI_SUPPORT_ZLIB) && !defined(STBI_NO_ZLIB)
#define STBI_NO_ZLIB
#endif


#include <stdarg.h>
#include <stddef.h>						/* ptrdiff_t on osx */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>						/* ldexp, pow */

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

#ifndef STBI_ASSERT
#include <assert.h>
#define STBI_ASSERT(x) assert(x)
#endif

#ifdef __cplusplus
#define STBI_EXTERN extern "C"
#else
#define STBI_EXTERN extern
#endif


#ifndef _MSC_VER
#ifdef __cplusplus
#define stbi_inline inline
#else
#define stbi_inline
#endif
#else
#define stbi_inline __forceinline
#endif

#ifndef STBI_NO_THREAD_LOCALS
#if defined(__MINT__) || defined(__PUREC__)
#define STBI_THREAD_LOCAL
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define STBI_THREAD_LOCAL       thread_local
#elif defined(__GNUC__) && __GNUC__ < 5
#define STBI_THREAD_LOCAL       __thread
#elif defined(_MSC_VER)
#define STBI_THREAD_LOCAL       __declspec(thread)
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define STBI_THREAD_LOCAL       _Thread_local
#endif

#ifndef STBI_THREAD_LOCAL
#if defined(__GNUC__)
#define STBI_THREAD_LOCAL       __thread
#endif
#endif
#endif

/* should produce compiler error if size is wrong */
typedef unsigned char validate_uint32[sizeof(stbi__uint32) == 4 ? 1 : -1];

#define STBI_NOTUSED(v)  (void)(v)

#if defined(STBI_MALLOC) && defined(STBI_FREE) && (defined(STBI_REALLOC) || defined(STBI_REALLOC_SIZED))
/* ok */
#elif !defined(STBI_MALLOC) && !defined(STBI_FREE) && !defined(STBI_REALLOC) && !defined(STBI_REALLOC_SIZED)
/* ok */
#else
#error "Must define all or none of STBI_MALLOC, STBI_FREE, and STBI_REALLOC (or STBI_REALLOC_SIZED)."
#endif

#ifndef STBI_MALLOC
#define STBI_MALLOC(sz)           malloc(sz)
#define STBI_REALLOC(p,newsz)     realloc(p,newsz)
#define STBI_FREE(p)              free(p)
#endif

#ifndef STBI_REALLOC_SIZED
#define STBI_REALLOC_SIZED(p,oldsz,newsz) STBI_REALLOC(p,newsz)
#endif

/* stbi__context structure is our basic context used by all images, so it */
/* contains all the IO context, plus some basic image information */
typedef struct
{
	stbi__uint32 img_x, img_y;
	plutovg_int_t img_n;
	plutovg_int_t img_out_n;

	stbi_io_callbacks io;
	void *io_user_data;

	int read_from_callbacks;
	plutovg_int_t buflen;
	stbi_uc buffer_start[128];
	plutovg_int_t callback_already_read;

	stbi_uc *img_buffer;
	stbi_uc *img_buffer_end;
	stbi_uc *img_buffer_original;
	stbi_uc *img_buffer_original_end;
} stbi__context;


static void stbi__refill_buffer(stbi__context *s);

/* initialize a memory-decode context */
static void stbi__start_mem(stbi__context *s, stbi_uc const *buffer, plutovg_int_t len)
{
	s->io.read = NULL;
	s->read_from_callbacks = 0;
	s->callback_already_read = 0;
	s->img_buffer = s->img_buffer_original = (stbi_uc *) buffer;
	s->img_buffer_end = s->img_buffer_original_end = (stbi_uc *) buffer + len;
}

/* initialize a callback-based context */
static void stbi__start_callbacks(stbi__context *s, stbi_io_callbacks *c, void *user)
{
	s->io = *c;
	s->io_user_data = user;
	s->buflen = sizeof(s->buffer_start);
	s->read_from_callbacks = 1;
	s->callback_already_read = 0;
	s->img_buffer = s->img_buffer_original = s->buffer_start;
	stbi__refill_buffer(s);
	s->img_buffer_original_end = s->img_buffer_end;
}

#ifndef STBI_NO_STDIO

static plutovg_int_t stbi__stdio_read(void *user, char *data, plutovg_int_t size)
{
	return (plutovg_int_t) fread(data, 1, size, (FILE *) user);
}

static void stbi__stdio_skip(void *user, plutovg_int_t n)
{
	int ch;

	fseek((FILE *) user, n, SEEK_CUR);
	ch = fgetc((FILE *) user);			/* have to read a byte to reset feof()'s flag */
	if (ch != EOF)
	{
		ungetc(ch, (FILE *) user);		/* push byte back onto stream if valid. */
	}
}

static int stbi__stdio_eof(void *user)
{
	return feof((FILE *) user) || ferror((FILE *) user);
}

static stbi_io_callbacks stbi__stdio_callbacks = {
	stbi__stdio_read,
	stbi__stdio_skip,
	stbi__stdio_eof,
};

static void stbi__start_file(stbi__context *s, FILE *f)
{
	stbi__start_callbacks(s, &stbi__stdio_callbacks, (void *) f);
}

/*static void stop_file(stbi__context *s) { } */

#endif /* !STBI_NO_STDIO */

static void stbi__rewind(stbi__context *s)
{
	/* conceptually rewind SHOULD rewind to the beginning of the stream, */
	/* but we just rewind to the beginning of the initial buffer, because */
	/* we only use it after doing 'test', which only ever looks at at most 92 bytes */
	s->img_buffer = s->img_buffer_original;
	s->img_buffer_end = s->img_buffer_original_end;
}

enum
{
	STBI_ORDER_RGB,
	STBI_ORDER_BGR
};

typedef struct
{
	int bits_per_channel;
	int num_channels;
	int channel_order;
} stbi__result_info;

#ifndef STBI_NO_PNG
static int stbi__png_test(stbi__context *s);
static void *stbi__png_load(stbi__context *s, plutovg_int_t *x, plutovg_int_t *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__png_info(stbi__context *s, plutovg_int_t *x, plutovg_int_t *y, int *comp);
#endif

static
#ifdef STBI_THREAD_LOCAL
 STBI_THREAD_LOCAL
#endif
const char *stbi__g_failure_reason;

STBIDEF const char *stbi_failure_reason(void)
{
	return stbi__g_failure_reason;
}

#ifndef STBI_NO_FAILURE_STRINGS
static int stbi__err(const char *str)
{
	stbi__g_failure_reason = str;
	return 0;
}
#endif

static void *stbi__malloc(size_t size)
{
	return STBI_MALLOC(size);
}

/*
 * stb_image uses ints pervasively, including for offset calculations.
 * therefore the largest decoded image size we can support with the
 * current code, even on 64-bit targets, is INT_MAX. this is not a
 * significant limitation for the intended use case.
 *
 * we do, however, need to make sure our size calculations don't
 * overflow. hence a few helper functions for size calculations that
 * multiply integers together, making sure that they're non-negative
 * and no overflow occurs.
 */

/* return 1 if the sum is valid, 0 on overflow. */
/* negative terms are considered invalid. */
static plutovg_int_t stbi__addsizes_valid(plutovg_int_t a, plutovg_int_t b)
{
	if (b < 0)
		return 0;
	/* now 0 <= b <= INT_MAX, hence also */
	/* 0 <= INT_MAX - b <= INTMAX. */
	/* And "a + b <= INT_MAX" (which might overflow) is the */
	/* same as a <= INT_MAX - b (no overflow) */
	return a <= DIM_MAX - b;
}

/* returns 1 if the product is valid, 0 on overflow. */
/* negative factors are considered invalid. */
static plutovg_int_t stbi__mul2sizes_valid(plutovg_int_t a, plutovg_int_t b)
{
	if (a < 0 || b < 0)
		return 0;
	if (b == 0)
		return 1;						/* mul-by-0 is always safe */
	/* portable way to check for no overflows in a*b */
	return a <= DIM_MAX / b;
}

#if !defined(STBI_NO_PNG)
/* returns 1 if "a*b + add" has no negative terms/factors and doesn't overflow */
static plutovg_int_t stbi__mad2sizes_valid(plutovg_int_t a, plutovg_int_t b, plutovg_int_t add)
{
	return stbi__mul2sizes_valid(a, b) && stbi__addsizes_valid(a * b, add);
}
#endif

/* returns 1 if "a*b*c + add" has no negative terms/factors and doesn't overflow */
static plutovg_int_t stbi__mad3sizes_valid(plutovg_int_t a, plutovg_int_t b, plutovg_int_t c, plutovg_int_t add)
{
	return stbi__mul2sizes_valid(a, b) && stbi__mul2sizes_valid(a * b, c) && stbi__addsizes_valid(a * b * c, add);
}

#if !defined(STBI_NO_PNG)
/* mallocs with size overflow checking */
static void *stbi__malloc_mad2(plutovg_int_t a, plutovg_int_t b, plutovg_int_t add)
{
	if (!stbi__mad2sizes_valid(a, b, add))
		return NULL;
	return stbi__malloc(a * b + add);
}
#endif

static void *stbi__malloc_mad3(plutovg_int_t a, plutovg_int_t b, plutovg_int_t c, plutovg_int_t add)
{
	if (!stbi__mad3sizes_valid(a, b, c, add))
		return NULL;
	return stbi__malloc(a * b * c + add);
}

/* stbi__err - error */
/* stbi__errpf - error returning pointer to float */
/* stbi__errpuc - error returning pointer to unsigned char */

#ifdef STBI_NO_FAILURE_STRINGS
#define stbi__err(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
#define stbi__err(x,y)  stbi__err(y)
#else
#define stbi__err(x,y)  stbi__err(x)
#endif

#define stbi__errpf(x,y)   ((float *)(size_t) (stbi__err(x,y)?NULL:NULL))
#define stbi__errpuc(x,y)  ((unsigned char *)(size_t) (stbi__err(x,y)?NULL:NULL))

STBIDEF void stbi_image_free(void *retval_from_stbi_load)
{
	STBI_FREE(retval_from_stbi_load);
}

static void *stbi__load_main(stbi__context *s, plutovg_int_t *x, plutovg_int_t *y, int *comp, int req_comp, stbi__result_info *ri, int bpc)
{
	memset(ri, 0, sizeof(*ri));			/* make sure it's initialized if we add new fields */
	ri->bits_per_channel = 8;			/* default is 8 so most paths don't have to be changed */
	ri->channel_order = STBI_ORDER_RGB;	/* all current input & output are this, but this is here so we can add BGR order */
	ri->num_channels = 0;

	/* test the formats with a very explicit header first (at least a FOURCC */
	/* or distinctive magic number first) */
#ifndef STBI_NO_PNG
	if (stbi__png_test(s))
		return stbi__png_load(s, x, y, comp, req_comp, ri);
#endif
	STBI_NOTUSED(bpc);

	return stbi__errpuc("unknown image type", "Image not of any known type, or corrupt");
}

static stbi_uc *stbi__convert_16_to_8(stbi__uint16 *orig, plutovg_int_t w, plutovg_int_t h, int channels)
{
	plutovg_int_t i;
	plutovg_int_t img_len = w * h * channels;
	stbi_uc *reduced;

	reduced = (stbi_uc *) stbi__malloc(img_len);
	if (reduced == NULL)
		return stbi__errpuc("outofmem", "Out of memory");

	for (i = 0; i < img_len; ++i)
		reduced[i] = (stbi_uc) ((orig[i] >> 8) & 0xFF);	/* top half of each byte is sufficient approx of 16->8 bit scaling */

	STBI_FREE(orig);
	return reduced;
}

static unsigned char *stbi__load_and_postprocess_8bit(stbi__context *s, plutovg_int_t *x, plutovg_int_t *y, int *comp, int req_comp)
{
	stbi__result_info ri;
	void *result = stbi__load_main(s, x, y, comp, req_comp, &ri, 8);

	if (result == NULL)
		return NULL;

	/* it is the responsibility of the loaders to make sure we get either 8 or 16 bit. */
	STBI_ASSERT(ri.bits_per_channel == 8 || ri.bits_per_channel == 16);

	if (ri.bits_per_channel != 8)
	{
		result = stbi__convert_16_to_8((stbi__uint16 *) result, *x, *y, req_comp == 0 ? *comp : req_comp);
		ri.bits_per_channel = 8;
	}

	return (unsigned char *) result;
}

#ifndef STBI_NO_STDIO

#if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)
STBI_EXTERN __declspec(dllimport)
int __stdcall MultiByteToWideChar(unsigned int cp, unsigned long flags, const char *str, int cbmb, wchar_t *widestr,
	int cchwide);
STBI_EXTERN __declspec(dllimport)
int __stdcall WideCharToMultiByte(unsigned int cp, unsigned long flags, const wchar_t *widestr, int cchwide, char *str,
	int cbmb, const char *defchar, int *used_default);
#endif

#if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)
STBIDEF int stbi_convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t *input)
{
	return WideCharToMultiByte(65001 /* UTF8 */ , 0, input, -1, buffer, bufferlen, NULL, NULL);
}
#endif

static FILE *stbi__fopen(char const *filename, char const *mode)
{
	FILE *f;

#if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)
	wchar_t wMode[64];
	wchar_t wFilename[1024];

	if (0 == MultiByteToWideChar(65001 /* UTF8 */ , 0, filename, -1, wFilename, sizeof(wFilename) / sizeof(*wFilename)))
		return 0;

	if (0 == MultiByteToWideChar(65001 /* UTF8 */ , 0, mode, -1, wMode, sizeof(wMode) / sizeof(*wMode)))
		return 0;

#if defined(_MSC_VER) && _MSC_VER >= 1400
	if (0 != _wfopen_s(&f, wFilename, wMode))
		f = 0;
#else
	f = _wfopen(wFilename, wMode);
#endif

#elif defined(_MSC_VER) && _MSC_VER >= 1400
	if (0 != fopen_s(&f, filename, mode))
		f = 0;
#else
	f = fopen(filename, mode);
#endif
	return f;
}


STBIDEF stbi_uc *stbi_load(char const *filename, plutovg_int_t *x, plutovg_int_t *y, int *comp, int req_comp)
{
	FILE *f = stbi__fopen(filename, "rb");
	unsigned char *result;

	if (!f)
		return stbi__errpuc("can't fopen", "Unable to open file");
	result = stbi_load_from_file(f, x, y, comp, req_comp);
	fclose(f);
	return result;
}

STBIDEF stbi_uc *stbi_load_from_file(FILE *f, plutovg_int_t *x, plutovg_int_t *y, int *comp, int req_comp)
{
	unsigned char *result;
	stbi__context s;

	stbi__start_file(&s, f);
	result = stbi__load_and_postprocess_8bit(&s, x, y, comp, req_comp);
	if (result)
	{
		/* need to 'unget' all the characters in the IO buffer */
		fseek(f, -(plutovg_int_t) (s.img_buffer_end - s.img_buffer), SEEK_CUR);
	}
	return result;
}

#endif /*!STBI_NO_STDIO */

STBIDEF stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, plutovg_int_t len, plutovg_int_t *x, plutovg_int_t *y, int *comp, int req_comp)
{
	stbi__context s;

	stbi__start_mem(&s, buffer, len);
	return stbi__load_and_postprocess_8bit(&s, x, y, comp, req_comp);
}


STBIDEF stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, plutovg_int_t *x, plutovg_int_t *y, int *comp, int req_comp)
{
	stbi__context s;

	stbi__start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
	return stbi__load_and_postprocess_8bit(&s, x, y, comp, req_comp);
}

/* **************************************************************************** */
/*
 * Common code used by all image loaders
 */

enum
{
	STBI__SCAN_load = 0,
	STBI__SCAN_type,
	STBI__SCAN_header
};

static void stbi__refill_buffer(stbi__context *s)
{
	plutovg_int_t n = (s->io.read) (s->io_user_data, (char *) s->buffer_start, s->buflen);

	s->callback_already_read += (plutovg_int_t) (s->img_buffer - s->img_buffer_original);
	if (n == 0)
	{
		/* at end of file, treat same as if from memory, but need to handle case */
		/* where s->img_buffer isn't pointing to safe memory, e.g. 0-byte file */
		s->read_from_callbacks = 0;
		s->img_buffer = s->buffer_start;
		s->img_buffer_end = s->buffer_start + 1;
		*s->img_buffer = 0;
	} else
	{
		s->img_buffer = s->buffer_start;
		s->img_buffer_end = s->buffer_start + n;
	}
}

stbi_inline static stbi_uc stbi__get8(stbi__context *s)
{
	if (s->img_buffer < s->img_buffer_end)
		return *s->img_buffer++;
	if (s->read_from_callbacks)
	{
		stbi__refill_buffer(s);
		return *s->img_buffer++;
	}
	return 0;
}

#if defined(STBI_NO_PNG)
/* nothing */
#else
static void stbi__skip(stbi__context *s, plutovg_int_t n)
{
	if (n == 0)
		return;							/* already there! */
	if (n < 0)
	{
		s->img_buffer = s->img_buffer_end;
		return;
	}
	if (s->io.read)
	{
		plutovg_int_t blen = (plutovg_int_t) (s->img_buffer_end - s->img_buffer);

		if (blen < n)
		{
			s->img_buffer = s->img_buffer_end;
			(s->io.skip) (s->io_user_data, n - blen);
			return;
		}
	}
	s->img_buffer += n;
}
#endif

#if defined(STBI_NO_PNG)
/* nothing */
#else
static plutovg_int_t stbi__getn(stbi__context *s, stbi_uc *buffer, plutovg_int_t n)
{
	if (s->io.read)
	{
		plutovg_int_t blen = (plutovg_int_t) (s->img_buffer_end - s->img_buffer);

		if (blen < n)
		{
			plutovg_int_t res, count;

			memcpy(buffer, s->img_buffer, blen);

			count = (s->io.read) (s->io_user_data, (char *) buffer + blen, n - blen);
			res = (count == (n - blen));
			s->img_buffer = s->img_buffer_end;
			return res;
		}
	}

	if (s->img_buffer + n <= s->img_buffer_end)
	{
		memcpy(buffer, s->img_buffer, n);
		s->img_buffer += n;
		return 1;
	} else
		return 0;
}
#endif

#if defined(STBI_NO_PNG)
/* nothing */
#else
static plutovg_uint_t stbi__get16be(stbi__context *s)
{
	plutovg_uint_t z = stbi__get8(s);

	return (z << 8) + stbi__get8(s);
}
#endif

#if defined(STBI_NO_PNG)
/* nothing */
#else
static stbi__uint32 stbi__get32be(stbi__context *s)
{
	stbi__uint32 z = stbi__get16be(s);

	return (z << 16) + stbi__get16be(s);
}
#endif

#define STBI__BYTECAST(x)  ((stbi_uc) ((x) & 255))	/* truncate int to byte without warnings */

#if defined(STBI_NO_PNG)
/* nothing */
#else
/* **************************************************************************** */
/*
 *  generic converter from built-in img_n to req_comp
 *    individual types do this automatically as much as possible (e.g. jpeg
 *    does all cases internally since it needs to colorspace convert anyway,
 *    and it never has alpha, so very few cases ). png can automatically
 *    interleave an alpha=255 channel, but falls back to this for other cases
 *
 *  assume data buffer is malloced, so malloc a new one and free that one
 *  only failure mode is malloc failing
 */

static stbi_uc stbi__compute_y(int r, int g, int b)
{
	return (stbi_uc) (((r * 77) + (g * 150) + (29 * b)) >> 8);
}
#endif

#if defined(STBI_NO_PNG)
/* nothing */
#else
static unsigned char *stbi__convert_format(unsigned char *data, plutovg_int_t img_n, int req_comp, plutovg_uint_t x, plutovg_uint_t y)
{
	plutovg_int_t i, j;
	unsigned char *good;

	if (req_comp == img_n)
		return data;
	STBI_ASSERT(req_comp >= 1 && req_comp <= 4);

	good = (unsigned char *) stbi__malloc_mad3(req_comp, x, y, 0);
	if (good == NULL)
	{
		STBI_FREE(data);
		return stbi__errpuc("outofmem", "Out of memory");
	}

	for (j = 0; j < (plutovg_int_t) y; ++j)
	{
		unsigned char *src = data + j * x * img_n;
		unsigned char *dest = good + j * x * req_comp;

#define STBI__COMBO(a,b)  ((int)(a)*8+(b))
#define STBI__CASE(a,b)   case STBI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
		/* convert source image with img_n components to one with req_comp components; */
		/* avoid switch per pixel, so use switch per scanline and massive macros */
		switch (STBI__COMBO(img_n, req_comp))
		{
			STBI__CASE(1, 2)
			{
				dest[0] = src[0];
				dest[1] = 255;
			}
			break;
			STBI__CASE(1, 3)
			{
				dest[0] = dest[1] = dest[2] = src[0];
			}
			break;
			STBI__CASE(1, 4)
			{
				dest[0] = dest[1] = dest[2] = src[0];
				dest[3] = 255;
			}
			break;
			STBI__CASE(2, 1)
			{
				dest[0] = src[0];
			}
			break;
			STBI__CASE(2, 3)
			{
				dest[0] = dest[1] = dest[2] = src[0];
			}
			break;
			STBI__CASE(2, 4)
			{
				dest[0] = dest[1] = dest[2] = src[0];
				dest[3] = src[1];
			}
			break;
			STBI__CASE(3, 4)
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
				dest[3] = 255;
			}
			break;
			STBI__CASE(3, 1)
			{
				dest[0] = stbi__compute_y(src[0], src[1], src[2]);
			}
			break;
			STBI__CASE(3, 2)
			{
				dest[0] = stbi__compute_y(src[0], src[1], src[2]);
				dest[1] = 255;
			}
			break;
			STBI__CASE(4, 1)
			{
				dest[0] = stbi__compute_y(src[0], src[1], src[2]);
			}
			break;
			STBI__CASE(4, 2)
			{
				dest[0] = stbi__compute_y(src[0], src[1], src[2]);
				dest[1] = src[3];
			}
			break;
			STBI__CASE(4, 3)
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
			}
			break;
		default:
			STBI_ASSERT(0);
			STBI_FREE(data);
			STBI_FREE(good);
			return stbi__errpuc("unsupported", "Unsupported format conversion");
		}
#undef STBI__COMBO
#undef STBI__CASE
	}

	STBI_FREE(data);
	return good;
}
#endif

#if defined(STBI_NO_PNG)
/* nothing */
#else
static stbi__uint16 stbi__compute_y_16(int r, int g, int b)
{
	return (stbi__uint16) (((r * 77) + (g * 150) + (29 * b)) >> 8);
}
#endif

#if defined(STBI_NO_PNG)
/* nothing */
#else
static stbi__uint16 *stbi__convert_format16(stbi__uint16 *data, plutovg_int_t img_n, int req_comp, plutovg_uint_t x, plutovg_uint_t y)
{
	plutovg_int_t i, j;
	stbi__uint16 *good;

	if (req_comp == img_n)
		return data;
	STBI_ASSERT(req_comp >= 1 && req_comp <= 4);

	good = (stbi__uint16 *) stbi__malloc(req_comp * x * y * 2);
	if (good == NULL)
	{
		STBI_FREE(data);
		return (stbi__uint16 *) stbi__errpuc("outofmem", "Out of memory");
	}

	for (j = 0; j < (plutovg_int_t) y; ++j)
	{
		stbi__uint16 *src = data + j * x * img_n;
		stbi__uint16 *dest = good + j * x * req_comp;

#define STBI__COMBO(a,b)  ((int)(a)*8+(b))
#define STBI__CASE(a,b)   case STBI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
		/* convert source image with img_n components to one with req_comp components; */
		/* avoid switch per pixel, so use switch per scanline and massive macros */
		switch (STBI__COMBO(img_n, req_comp))
		{
			STBI__CASE(1, 2)
			{
				dest[0] = src[0];
				dest[1] = 0xffff;
			}
			break;
			STBI__CASE(1, 3)
			{
				dest[0] = dest[1] = dest[2] = src[0];
			}
			break;
			STBI__CASE(1, 4)
			{
				dest[0] = dest[1] = dest[2] = src[0];
				dest[3] = 0xffff;
			}
			break;
			STBI__CASE(2, 1)
			{
				dest[0] = src[0];
			}
			break;
			STBI__CASE(2, 3)
			{
				dest[0] = dest[1] = dest[2] = src[0];
			}
			break;
			STBI__CASE(2, 4)
			{
				dest[0] = dest[1] = dest[2] = src[0];
				dest[3] = src[1];
			}
			break;
			STBI__CASE(3, 4)
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
				dest[3] = 0xffff;
			}
			break;
			STBI__CASE(3, 1)
			{
				dest[0] = stbi__compute_y_16(src[0], src[1], src[2]);
			}
			break;
			STBI__CASE(3, 2)
			{
				dest[0] = stbi__compute_y_16(src[0], src[1], src[2]);
				dest[1] = 0xffff;
			}
			break;
			STBI__CASE(4, 1)
			{
				dest[0] = stbi__compute_y_16(src[0], src[1], src[2]);
			}
			break;
			STBI__CASE(4, 2)
			{
				dest[0] = stbi__compute_y_16(src[0], src[1], src[2]);
				dest[1] = src[3];
			}
			break;
			STBI__CASE(4, 3)
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
			}
			break;
		default:
			STBI_ASSERT(0);
			STBI_FREE(data);
			STBI_FREE(good);
			return (stbi__uint16 *) stbi__errpuc("unsupported", "Unsupported format conversion");
		}
#undef STBI__COMBO
#undef STBI__CASE
	}

	STBI_FREE(data);
	return good;
}
#endif

/* public domain zlib decode    v0.2  Sean Barrett 2006-11-18 */
/*    simple implementation */
/*      - all input must be provided in an upfront buffer */
/*      - all output is written to a single output buffer (can malloc/realloc) */
/*    performance */
/*      - fast huffman */

#ifndef STBI_NO_ZLIB

/* fast-way is faster to check than jpeg huffman, but slow way is slower */
#define STBI__ZFAST_BITS  9				/* accelerate all cases in default tables */
#define STBI__ZFAST_MASK  (((stbi__uint32)1 << STBI__ZFAST_BITS) - 1)
#define STBI__ZNSYMS 288				/* number of symbols in literal/length alphabet */

/* zlib-style huffman encoding */
/* (jpegs packs from left, zlib from right, so can't share code) */
typedef struct
{
	stbi__uint16 fast[1 << STBI__ZFAST_BITS];
	stbi__uint16 firstcode[16];
	int32_t maxcode[17];
	stbi__uint16 firstsymbol[16];
	stbi_uc size[STBI__ZNSYMS];
	stbi__uint16 value[STBI__ZNSYMS];
} stbi__zhuffman;

stbi_inline static uint32_t stbi__bitreverse16(uint32_t n)
{
	n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
	n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
	n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
	n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
	return n;
}

stbi_inline static uint32_t stbi__bit_reverse(uint32_t v, int bits)
{
	STBI_ASSERT(bits <= 16);
	/* to bit reverse n bits, reverse 16 and shift */
	/* e.g. 11 bits, bit reverse and shift away 5 */
	return stbi__bitreverse16(v) >> (16 - bits);
}

static int stbi__zbuild_huffman(stbi__zhuffman *z, const stbi_uc *sizelist, int num)
{
	int i;
	int32_t k = 0;
	int32_t code;
	int32_t next_code[16];
	int32_t sizes[17];

	/* DEFLATE spec for generating codes */
	memset(sizes, 0, sizeof(sizes));
	memset(z->fast, 0, sizeof(z->fast));
	for (i = 0; i < num; ++i)
		++sizes[sizelist[i]];
	sizes[0] = 0;
	for (i = 1; i < 16; ++i)
		if (sizes[i] > ((int32_t)1 << i))
			return stbi__err("bad sizes", "Corrupt PNG");
	code = 0;
	for (i = 1; i < 16; ++i)
	{
		next_code[i] = code;
		z->firstcode[i] = (stbi__uint16) code;
		z->firstsymbol[i] = (stbi__uint16) k;
		code = (code + sizes[i]);
		if (sizes[i])
			if (code - 1 >= ((int32_t)1 << i))
				return stbi__err("bad codelengths", "Corrupt PNG");
		z->maxcode[i] = code << (16 - i);	/* preshift for inner loop */
		code <<= 1;
		k += sizes[i];
	}
	z->maxcode[16] = 0x10000UL;			/* sentinel */
	for (i = 0; i < num; ++i)
	{
		int s = sizelist[i];

		if (s)
		{
			int c = (int)(next_code[s] - z->firstcode[s] + z->firstsymbol[s]);
			stbi__uint16 fastv = (stbi__uint16) ((s << 9) | i);

			z->size[c] = (stbi_uc) s;
			z->value[c] = (stbi__uint16) i;
			if (s <= STBI__ZFAST_BITS)
			{
				int j = (int)stbi__bit_reverse(next_code[s], s);

				while (j < ((int32_t)1 << STBI__ZFAST_BITS))
				{
					z->fast[j] = fastv;
					j += (1 << s);
				}
			}
			++next_code[s];
		}
	}
	return 1;
}

/* zlib-from-memory implementation for PNG reading */
/*    because PNG allows splitting the zlib stream arbitrarily, */
/*    and it's annoying structurally to have PNG call ZLIB call PNG, */
/*    we require PNG read all the IDATs and combine them into a single */
/*    memory buffer */

typedef struct
{
	stbi_uc *zbuffer;
	stbi_uc *zbuffer_end;
	int num_bits;
	int hit_zeof_once;
	stbi__uint32 code_buffer;

	char *zout;
	char *zout_start;
	char *zout_end;
	int z_expandable;

	stbi__zhuffman z_length;
	stbi__zhuffman z_distance;
} stbi__zbuf;

stbi_inline static int stbi__zeof(stbi__zbuf *z)
{
	return (z->zbuffer >= z->zbuffer_end);
}

stbi_inline static stbi_uc stbi__zget8(stbi__zbuf *z)
{
	return stbi__zeof(z) ? 0 : *z->zbuffer++;
}

static void stbi__fill_bits(stbi__zbuf *z)
{
	do
	{
		if (z->code_buffer >= ((stbi__uint32)1U << z->num_bits))
		{
			z->zbuffer = z->zbuffer_end;	/* treat this as EOF so we fail. */
			return;
		}
		z->code_buffer |= (stbi__uint32) stbi__zget8(z) << z->num_bits;
		z->num_bits += 8;
	} while (z->num_bits <= 24);
}

stbi_inline static uint32_t stbi__zreceive(stbi__zbuf *z, int n)
{
	uint32_t k;

	if (z->num_bits < n)
		stbi__fill_bits(z);
	k = (uint32_t)(z->code_buffer & (((uint32_t)1 << n) - 1));
	z->code_buffer >>= n;
	z->num_bits -= n;
	return k;
}

static int stbi__zhuffman_decode_slowpath(stbi__zbuf *a, stbi__zhuffman *z)
{
	int b, s;
	int32_t k;

	/* not resolved by fast table, so compute it the slow way */
	/* use jpeg approach, which requires MSbits at top */
	k = stbi__bit_reverse(a->code_buffer, 16);
	for (s = STBI__ZFAST_BITS + 1;; ++s)
		if (k < z->maxcode[s])
			break;
	if (s >= 16)
		return -1;						/* invalid code! */
	/* code size is s, so: */
	b = (int)((k >> (16 - s)) - z->firstcode[s] + z->firstsymbol[s]);
	if (b >= STBI__ZNSYMS)
		return -1;						/* some data was corrupt somewhere! */
	if (z->size[b] != s)
		return -1;						/* was originally an assert, but report failure instead. */
	a->code_buffer >>= s;
	a->num_bits -= s;
	return z->value[b];
}

stbi_inline static int stbi__zhuffman_decode(stbi__zbuf *a, stbi__zhuffman *z)
{
	int b, s;

	if (a->num_bits < 16)
	{
		if (stbi__zeof(a))
		{
			if (!a->hit_zeof_once)
			{
				/* This is the first time we hit eof, insert 16 extra padding btis */
				/* to allow us to keep going; if we actually consume any of them */
				/* though, that is invalid data. This is caught later. */
				a->hit_zeof_once = 1;
				a->num_bits += 16;		/* add 16 implicit zero bits */
			} else
			{
				/* We already inserted our extra 16 padding bits and are again */
				/* out, this stream is actually prematurely terminated. */
				return -1;
			}
		} else
		{
			stbi__fill_bits(a);
		}
	}
	b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
	if (b)
	{
		s = b >> 9;
		a->code_buffer >>= s;
		a->num_bits -= s;
		return b & 511;
	}
	return stbi__zhuffman_decode_slowpath(a, z);
}

static int stbi__zexpand(stbi__zbuf *z, char *zout, plutovg_int_t n)	/* need to make room for n bytes */
{
	char *q;
	size_t cur;
	size_t limit;
	size_t old_limit;

	z->zout = zout;
	if (!z->z_expandable)
		return stbi__err("output buffer limit", "Corrupt PNG");
	cur = (size_t) (z->zout - z->zout_start);
	limit = old_limit = (unsigned) (z->zout_end - z->zout_start);
	if (SIZE_MAX - cur < (size_t) n)
		return stbi__err("outofmem", "Out of memory");
	while (cur + n > limit)
	{
		if (limit > SIZE_MAX / 2)
			return stbi__err("outofmem", "Out of memory");
		limit *= 2;
	}
	q = (char *) STBI_REALLOC_SIZED(z->zout_start, old_limit, limit);
	STBI_NOTUSED(old_limit);
	if (q == NULL)
		return stbi__err("outofmem", "Out of memory");
	z->zout_start = q;
	z->zout = q + cur;
	z->zout_end = q + limit;
	return 1;
}

static const int stbi__zlength_base[31] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13,
	15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
	67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
};

static const int stbi__zlength_extra[31] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0 };

static const int stbi__zdist_base[32] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0, 0
};

static const int stbi__zdist_extra[32] =
	{ 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

static int stbi__parse_huffman_block(stbi__zbuf *a)
{
	char *zout = a->zout;

	for (;;)
	{
		int z = stbi__zhuffman_decode(a, &a->z_length);

		if (z < 256)
		{
			if (z < 0)
				return stbi__err("bad huffman code", "Corrupt PNG");	/* error in huffman codes */
			if (zout >= a->zout_end)
			{
				if (!stbi__zexpand(a, zout, 1))
					return 0;
				zout = a->zout;
			}
			*zout++ = (char) z;
		} else
		{
			stbi_uc *p;
			plutovg_int_t len, dist;

			if (z == 256)
			{
				a->zout = zout;
				if (a->hit_zeof_once && a->num_bits < 16)
				{
					/* The first time we hit zeof, we inserted 16 extra zero bits into our bit */
					/* buffer so the decoder can just do its speculative decoding. But if we */
					/* actually consumed any of those bits (which is the case when num_bits < 16), */
					/* the stream actually read past the end so it is malformed. */
					return stbi__err("unexpected end", "Corrupt PNG");
				}
				return 1;
			}
			if (z >= 286)
				return stbi__err("bad huffman code", "Corrupt PNG");	/* per DEFLATE, length codes 286 and 287 must not appear in compressed data */
			z -= 257;
			len = stbi__zlength_base[z];
			if (stbi__zlength_extra[z])
				len += stbi__zreceive(a, stbi__zlength_extra[z]);
			z = stbi__zhuffman_decode(a, &a->z_distance);
			if (z < 0 || z >= 30)
				return stbi__err("bad huffman code", "Corrupt PNG");	/* per DEFLATE, distance codes 30 and 31 must not appear in compressed data */
			dist = stbi__zdist_base[z];
			if (stbi__zdist_extra[z])
				dist += stbi__zreceive(a, stbi__zdist_extra[z]);
			if (zout - a->zout_start < dist)
				return stbi__err("bad dist", "Corrupt PNG");
			if (len > a->zout_end - zout)
			{
				if (!stbi__zexpand(a, zout, len))
					return 0;
				zout = a->zout;
			}
			p = (stbi_uc *) (zout - dist);
			if (dist == 1)
			{							/* run of one byte; common in images. */
				stbi_uc v = *p;

				if (len)
				{
					do
						*zout++ = v;
					while (--len);
				}
			} else
			{
				if (len)
				{
					do
						*zout++ = *p++;
					while (--len);
				}
			}
		}
	}
}

static int stbi__compute_huffman_codes(stbi__zbuf *a)
{
	static const stbi_uc length_dezigzag[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
	stbi__zhuffman z_codelength;
	stbi_uc lencodes[286 + 32 + 137];	/*padding for maximum single op */
	stbi_uc codelength_sizes[19];
	int i, n;

	int hlit = (int)stbi__zreceive(a, 5) + 257;
	int hdist = (int)stbi__zreceive(a, 5) + 1;
	int hclen = (int)stbi__zreceive(a, 4) + 4;
	int ntot = hlit + hdist;

	memset(codelength_sizes, 0, sizeof(codelength_sizes));
	for (i = 0; i < hclen; ++i)
	{
		int s = (int)stbi__zreceive(a, 3);

		codelength_sizes[length_dezigzag[i]] = (stbi_uc) s;
	}
	if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19))
		return 0;

	n = 0;
	while (n < ntot)
	{
		int c = stbi__zhuffman_decode(a, &z_codelength);

		if (c < 0 || c >= 19)
			return stbi__err("bad codelengths", "Corrupt PNG");
		if (c < 16)
			lencodes[n++] = (stbi_uc) c;
		else
		{
			stbi_uc fill = 0;

			if (c == 16)
			{
				c = (int)stbi__zreceive(a, 2) + 3;
				if (n == 0)
					return stbi__err("bad codelengths", "Corrupt PNG");
				fill = lencodes[n - 1];
			} else if (c == 17)
			{
				c = (int)stbi__zreceive(a, 3) + 3;
			} else if (c == 18)
			{
				c = (int)stbi__zreceive(a, 7) + 11;
			} else
			{
				return stbi__err("bad codelengths", "Corrupt PNG");
			}
			if (ntot - n < c)
				return stbi__err("bad codelengths", "Corrupt PNG");
			memset(lencodes + n, fill, c);
			n += c;
		}
	}
	if (n != ntot)
		return stbi__err("bad codelengths", "Corrupt PNG");
	if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit))
		return 0;
	if (!stbi__zbuild_huffman(&a->z_distance, lencodes + hlit, hdist))
		return 0;
	return 1;
}

static int stbi__parse_uncompressed_block(stbi__zbuf *a)
{
	stbi_uc header[4];
	plutovg_int_t len, nlen, k;

	if (a->num_bits & 7)
		stbi__zreceive(a, a->num_bits & 7);	/* discard */
	/* drain the bit-packed data into header */
	k = 0;
	while (a->num_bits > 0)
	{
		header[k++] = (stbi_uc) (a->code_buffer & 255);	/* suppress MSVC run-time check */
		a->code_buffer >>= 8;
		a->num_bits -= 8;
	}
	if (a->num_bits < 0)
		return stbi__err("zlib corrupt", "Corrupt PNG");
	/* now fill header the normal way */
	while (k < 4)
		header[k++] = stbi__zget8(a);
	len = header[1] * 256 + header[0];
	nlen = header[3] * 256 + header[2];
	if (nlen != (len ^ 0xffff))
		return stbi__err("zlib corrupt", "Corrupt PNG");
	if (a->zbuffer + len > a->zbuffer_end)
		return stbi__err("read past buffer", "Corrupt PNG");
	if (a->zout + len > a->zout_end)
		if (!stbi__zexpand(a, a->zout, len))
			return 0;
	memcpy(a->zout, a->zbuffer, len);
	a->zbuffer += len;
	a->zout += len;
	return 1;
}

static int stbi__parse_zlib_header(stbi__zbuf *a)
{
	int cmf = stbi__zget8(a);
	int cm = cmf & 15;

	/* int cinfo = cmf >> 4; */
	int flg = stbi__zget8(a);

	if (stbi__zeof(a))
		return stbi__err("bad zlib header", "Corrupt PNG");	/* zlib spec */
	if ((cmf * 256 + flg) % 31 != 0)
		return stbi__err("bad zlib header", "Corrupt PNG");	/* zlib spec */
	if (flg & 32)
		return stbi__err("no preset dict", "Corrupt PNG");	/* preset dictionary not allowed in png */
	if (cm != 8)
		return stbi__err("bad compression", "Corrupt PNG");	/* DEFLATE required for png */
	/* window = 1 << (8 + cinfo)... but who cares, we fully buffer output */
	return 1;
}

static const stbi_uc stbi__zdefault_length[STBI__ZNSYMS] = {
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8
};

static const stbi_uc stbi__zdefault_distance[32] = {
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

#if 0
Init algorithm:
{
   int i;   /* use <= to match clearly with spec */
   for (i=0; i <= 143; ++i)     stbi__zdefault_length[i]   = 8;
   for (   ; i <= 255; ++i)     stbi__zdefault_length[i]   = 9;
   for (   ; i <= 279; ++i)     stbi__zdefault_length[i]   = 7;
   for (   ; i <= 287; ++i)     stbi__zdefault_length[i]   = 8;

   for (i=0; i <=  31; ++i)     stbi__zdefault_distance[i] = 5;
}
#endif


static int stbi__parse_zlib(stbi__zbuf *a, int parse_header)
{
	int final, type;

	if (parse_header)
		if (!stbi__parse_zlib_header(a))
			return 0;
	a->num_bits = 0;
	a->code_buffer = 0;
	a->hit_zeof_once = 0;
	do
	{
		final = (int)stbi__zreceive(a, 1);
		type = (int)stbi__zreceive(a, 2);
		if (type == 0)
		{
			if (!stbi__parse_uncompressed_block(a))
				return 0;
		} else if (type == 3)
		{
			return 0;
		} else
		{
			if (type == 1)
			{
				/* use fixed code lengths */
				if (!stbi__zbuild_huffman(&a->z_length, stbi__zdefault_length, STBI__ZNSYMS))
					return 0;
				if (!stbi__zbuild_huffman(&a->z_distance, stbi__zdefault_distance, 32))
					return 0;
			} else
			{
				if (!stbi__compute_huffman_codes(a))
					return 0;
			}
			if (!stbi__parse_huffman_block(a))
				return 0;
		}
	} while (!final);
	return 1;
}

static int stbi__do_zlib(stbi__zbuf *a, char *obuf, plutovg_int_t olen, int exp, int parse_header)
{
	a->zout_start = obuf;
	a->zout = obuf;
	a->zout_end = obuf + olen;
	a->z_expandable = exp;

	return stbi__parse_zlib(a, parse_header);
}

STBIDEF char *stbi_zlib_decode_malloc_guesssize(const char *buffer, plutovg_int_t len, plutovg_int_t initial_size, stbi__uint32 *outlen)
{
	stbi__zbuf a;
	char *p = (char *) stbi__malloc(initial_size);

	if (p == NULL)
		return NULL;
	a.zbuffer = (stbi_uc *) buffer;
	a.zbuffer_end = (stbi_uc *) buffer + len;
	if (stbi__do_zlib(&a, p, initial_size, 1, 1))
	{
		if (outlen)
			*outlen = (stbi__uint32) (a.zout - a.zout_start);
		return a.zout_start;
	} else
	{
		STBI_FREE(a.zout_start);
		return NULL;
	}
}

STBIDEF char *stbi_zlib_decode_malloc(char const *buffer, plutovg_int_t len, stbi__uint32 *outlen)
{
	return stbi_zlib_decode_malloc_guesssize(buffer, len, 16384, outlen);
}

STBIDEF char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, plutovg_int_t len, stbi__uint32 initial_size, stbi__uint32 *outlen, int parse_header)
{
	stbi__zbuf a;
	char *p = (char *) stbi__malloc(initial_size);

	if (p == NULL)
		return NULL;
	a.zbuffer = (stbi_uc *) buffer;
	a.zbuffer_end = (stbi_uc *) buffer + len;
	if (stbi__do_zlib(&a, p, initial_size, 1, parse_header))
	{
		if (outlen)
			*outlen = (stbi__uint32) (a.zout - a.zout_start);
		return a.zout_start;
	} else
	{
		STBI_FREE(a.zout_start);
		return NULL;
	}
}

STBIDEF plutovg_int_t stbi_zlib_decode_buffer(char *obuffer, plutovg_int_t olen, char const *ibuffer, plutovg_int_t ilen)
{
	stbi__zbuf a;

	a.zbuffer = (stbi_uc *) ibuffer;
	a.zbuffer_end = (stbi_uc *) ibuffer + ilen;
	if (stbi__do_zlib(&a, obuffer, olen, 0, 1))
		return (plutovg_int_t) (a.zout - a.zout_start);
	else
		return -1;
}

STBIDEF char *stbi_zlib_decode_noheader_malloc(char const *buffer, plutovg_int_t len, stbi__uint32 *outlen)
{
	stbi__zbuf a;
	char *p = (char *) stbi__malloc(16384);

	if (p == NULL)
		return NULL;
	a.zbuffer = (stbi_uc *) buffer;
	a.zbuffer_end = (stbi_uc *) buffer + len;
	if (stbi__do_zlib(&a, p, 16384, 1, 0))
	{
		if (outlen)
			*outlen = (stbi__uint32) (a.zout - a.zout_start);
		return a.zout_start;
	} else
	{
		STBI_FREE(a.zout_start);
		return NULL;
	}
}

STBIDEF plutovg_int_t stbi_zlib_decode_noheader_buffer(char *obuffer, plutovg_int_t olen, const char *ibuffer, plutovg_int_t ilen)
{
	stbi__zbuf a;

	a.zbuffer = (stbi_uc *) ibuffer;
	a.zbuffer_end = (stbi_uc *) ibuffer + ilen;
	if (stbi__do_zlib(&a, obuffer, olen, 0, 0))
		return (plutovg_int_t) (a.zout - a.zout_start);
	else
		return -1;
}
#endif

/* public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18 */
/*    simple implementation */
/*      - only 8-bit samples */
/*      - no CRC checking */
/*      - allocates lots of intermediate memory */
/*        - avoids problem of streaming data between subsystems */
/*        - avoids explicit window management */
/*    performance */
/*      - uses stb_zlib, a PD zlib implementation with fast huffman decoding */

#ifndef STBI_NO_PNG
typedef struct
{
	stbi__uint32 length;
	stbi__uint32 type;
} stbi__pngchunk;

static void stbi__get_chunk_header(stbi__context *s, stbi__pngchunk *c)
{
	c->length = stbi__get32be(s);
	c->type = stbi__get32be(s);
}

static int stbi__check_png_header(stbi__context *s)
{
	static const stbi_uc png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	int i;

	for (i = 0; i < 8; ++i)
		if (stbi__get8(s) != png_sig[i])
			return stbi__err("bad png sig", "Not a PNG");
	return 1;
}

typedef struct
{
	stbi__context *s;
	stbi_uc *idata;
	stbi_uc *expanded;
	stbi_uc *out;
	int depth;
} stbi__png;


enum
{
	STBI__F_none = 0,
	STBI__F_sub = 1,
	STBI__F_up = 2,
	STBI__F_avg = 3,
	STBI__F_paeth = 4,
	/* synthetic filter used for first scanline to avoid needing a dummy row of 0s */
	STBI__F_avg_first
};

static stbi_uc first_row_filter[5] = {
	STBI__F_none,
	STBI__F_sub,
	STBI__F_none,
	STBI__F_avg_first,
	STBI__F_sub							/* Paeth with b=c=0 turns out to be equivalent to sub */
};

static int stbi__paeth(int a, int b, int c)
{
	/* This formulation looks very different from the reference in the PNG spec, but is */
	/* actually equivalent and has favorable data dependencies and admits straightforward */
	/* generation of branch-free code, which helps performance significantly. */
	int thresh = c * 3 - (a + b);
	int lo = a < b ? a : b;
	int hi = a < b ? b : a;
	int t0 = (hi <= thresh) ? lo : c;
	int t1 = (thresh <= lo) ? hi : t0;

	return t1;
}

static const stbi_uc stbi__depth_scale_table[9] = { 0, 0xff, 0x55, 0, 0x11, 0, 0, 0, 0x01 };

/* adds an extra all-255 alpha channel */
/* dest == src is legal */
/* img_n must be 1 or 3 */
static void stbi__create_png_alpha_expand8(stbi_uc *dest, stbi_uc *src, stbi__uint32 x, plutovg_int_t img_n)
{
	plutovg_int_t i;

	/* must process data backwards since we allow dest==src */
	if (img_n == 1)
	{
		for (i = x - 1; i >= 0; --i)
		{
			dest[i * 2 + 1] = 255;
			dest[i * 2 + 0] = src[i];
		}
	} else
	{
		STBI_ASSERT(img_n == 3);
		for (i = x - 1; i >= 0; --i)
		{
			dest[i * 4 + 3] = 255;
			dest[i * 4 + 2] = src[i * 3 + 2];
			dest[i * 4 + 1] = src[i * 3 + 1];
			dest[i * 4 + 0] = src[i * 3 + 0];
		}
	}
}

/* create the png data from post-deflated data */
static int stbi__create_png_image_raw(stbi__png *a, stbi_uc *raw, stbi__uint32 raw_len, plutovg_int_t out_n, stbi__uint32 x, stbi__uint32 y, int depth, int color)
{
	plutovg_int_t bytes = (depth == 16 ? 2 : 1);
	stbi__context *s = a->s;
	stbi__uint32 i, j;
	stbi__uint32 stride = x * out_n * bytes;
	stbi__uint32 img_len;
	stbi__uint32 img_width_bytes;
	stbi_uc *filter_buf;
	int all_ok = 1;
	plutovg_int_t k;
	plutovg_int_t img_n = s->img_n;				/* copy it into a local for later */

	plutovg_int_t output_bytes = out_n * bytes;
	plutovg_int_t filter_bytes = img_n * bytes;
	plutovg_int_t width = x;

	STBI_ASSERT(out_n == s->img_n || out_n == s->img_n + 1);
	a->out = (stbi_uc *) stbi__malloc_mad3(x, y, output_bytes, 0);	/* extra bytes to write off the end into */
	if (!a->out)
		return stbi__err("outofmem", "Out of memory");

	/* note: error exits here don't need to clean up a->out individually, */
	/* stbi__do_png always does on error. */
	if (!stbi__mad3sizes_valid(img_n, x, depth, 7))
		return stbi__err("too large", "Corrupt PNG");
	img_width_bytes = (((img_n * x * depth) + 7) >> 3);
	if (!stbi__mad2sizes_valid(img_width_bytes, y, img_width_bytes))
		return stbi__err("too large", "Corrupt PNG");
	img_len = (img_width_bytes + 1) * y;

	/* we used to check for exact match between raw_len and img_len on non-interlaced PNGs, */
	/* but issue #276 reported a PNG in the wild that had extra data at the end (all zeros), */
	/* so just check for raw_len < img_len always. */
	if (raw_len < img_len)
		return stbi__err("not enough pixels", "Corrupt PNG");

	/* Allocate two scan lines worth of filter workspace buffer. */
	filter_buf = (stbi_uc *) stbi__malloc_mad2(img_width_bytes, 2, 0);
	if (!filter_buf)
		return stbi__err("outofmem", "Out of memory");

	/* Filtering for low-bit-depth images */
	if (depth < 8)
	{
		filter_bytes = 1;
		width = img_width_bytes;
	}

	for (j = 0; j < y; ++j)
	{
		/* cur/prior filter buffers alternate */
		stbi_uc *cur = filter_buf + (j & 1) * img_width_bytes;
		stbi_uc *prior = filter_buf + (~j & 1) * img_width_bytes;
		stbi_uc *dest = a->out + stride * j;
		plutovg_int_t nk = width * filter_bytes;
		int filter = *raw++;

		/* check filter type */
		if (filter > 4)
		{
			all_ok = stbi__err("invalid filter", "Corrupt PNG");
			break;
		}

		/* if first row, use special filter that doesn't sample previous row */
		if (j == 0)
			filter = first_row_filter[filter];

		/* perform actual filtering */
		switch (filter)
		{
		case STBI__F_none:
			memcpy(cur, raw, nk);
			break;
		case STBI__F_sub:
			memcpy(cur, raw, filter_bytes);
			for (k = filter_bytes; k < nk; ++k)
				cur[k] = STBI__BYTECAST(raw[k] + cur[k - filter_bytes]);
			break;
		case STBI__F_up:
			for (k = 0; k < nk; ++k)
				cur[k] = STBI__BYTECAST(raw[k] + prior[k]);
			break;
		case STBI__F_avg:
			for (k = 0; k < filter_bytes; ++k)
				cur[k] = STBI__BYTECAST(raw[k] + (prior[k] >> 1));
			for (k = filter_bytes; k < nk; ++k)
				cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k - filter_bytes]) >> 1));
			break;
		case STBI__F_paeth:
			for (k = 0; k < filter_bytes; ++k)
				cur[k] = STBI__BYTECAST(raw[k] + prior[k]);	/* prior[k] == stbi__paeth(0,prior[k],0) */
			for (k = filter_bytes; k < nk; ++k)
				cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - filter_bytes], prior[k], prior[k - filter_bytes]));
			break;
		case STBI__F_avg_first:
			memcpy(cur, raw, filter_bytes);
			for (k = filter_bytes; k < nk; ++k)
				cur[k] = STBI__BYTECAST(raw[k] + (cur[k - filter_bytes] >> 1));
			break;
		}

		raw += nk;

		/* expand decoded bits in cur to dest, also adding an extra alpha channel if desired */
		if (depth < 8)
		{
			stbi_uc scale = (color == 0) ? stbi__depth_scale_table[depth] : 1;	/* scale grayscale values to 0..255 range */
			stbi_uc *in = cur;
			stbi_uc *out = dest;
			stbi_uc inb = 0;
			stbi__uint32 nsmp = x * img_n;

			/* expand bits to bytes first */
			if (depth == 4)
			{
				for (i = 0; i < nsmp; ++i)
				{
					if ((i & 1) == 0)
						inb = *in++;
					*out++ = scale * (inb >> 4);
					inb <<= 4;
				}
			} else if (depth == 2)
			{
				for (i = 0; i < nsmp; ++i)
				{
					if ((i & 3) == 0)
						inb = *in++;
					*out++ = scale * (inb >> 6);
					inb <<= 2;
				}
			} else
			{
				STBI_ASSERT(depth == 1);
				for (i = 0; i < nsmp; ++i)
				{
					if ((i & 7) == 0)
						inb = *in++;
					*out++ = scale * (inb >> 7);
					inb <<= 1;
				}
			}

			/* insert alpha=255 values if desired */
			if (img_n != out_n)
				stbi__create_png_alpha_expand8(dest, dest, x, img_n);
		} else if (depth == 8)
		{
			if (img_n == out_n)
				memcpy(dest, cur, x * img_n);
			else
				stbi__create_png_alpha_expand8(dest, cur, x, img_n);
		} else if (depth == 16)
		{
			/* convert the image data from big-endian to platform-native */
			stbi__uint16 *dest16 = (stbi__uint16 *) dest;
			stbi__uint32 nsmp = x * img_n;

			if (img_n == out_n)
			{
				for (i = 0; i < nsmp; ++i, ++dest16, cur += 2)
					*dest16 = (cur[0] << 8) | cur[1];
			} else
			{
				STBI_ASSERT(img_n + 1 == out_n);
				if (img_n == 1)
				{
					for (i = 0; i < x; ++i, dest16 += 2, cur += 2)
					{
						dest16[0] = (cur[0] << 8) | cur[1];
						dest16[1] = 0xffff;
					}
				} else
				{
					STBI_ASSERT(img_n == 3);
					for (i = 0; i < x; ++i, dest16 += 4, cur += 6)
					{
						dest16[0] = (cur[0] << 8) | cur[1];
						dest16[1] = (cur[2] << 8) | cur[3];
						dest16[2] = (cur[4] << 8) | cur[5];
						dest16[3] = 0xffff;
					}
				}
			}
		}
	}

	STBI_FREE(filter_buf);
	if (!all_ok)
		return 0;

	return 1;
}

static int stbi__create_png_image(stbi__png *a, stbi_uc *image_data, stbi__uint32 image_data_len, plutovg_int_t out_n, int depth,
	int color, int interlaced)
{
	plutovg_int_t bytes = (depth == 16 ? 2 : 1);
	plutovg_int_t out_bytes = out_n * bytes;
	stbi_uc *final;
	int p;

	if (!interlaced)
		return stbi__create_png_image_raw(a, image_data, image_data_len, out_n, a->s->img_x, a->s->img_y, depth, color);

	/* de-interlacing */
	final = (stbi_uc *) stbi__malloc_mad3(a->s->img_x, a->s->img_y, out_bytes, 0);
	if (!final)
		return stbi__err("outofmem", "Out of memory");
	for (p = 0; p < 7; ++p)
	{
		static int const xorig[] = { 0, 4, 0, 2, 0, 1, 0 };
		static int const yorig[] = { 0, 0, 4, 0, 2, 0, 1 };
		static int const xspc[] = { 8, 8, 4, 4, 2, 2, 1 };
		static int const yspc[] = { 8, 8, 8, 4, 4, 2, 2 };
		plutovg_int_t i, j, x, y;

		/* pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1 */
		x = (a->s->img_x - xorig[p] + xspc[p] - 1) / xspc[p];
		y = (a->s->img_y - yorig[p] + yspc[p] - 1) / yspc[p];
		if (x && y)
		{
			stbi__uint32 img_len = ((((a->s->img_n * x * depth) + 7) >> 3) + 1) * y;

			if (!stbi__create_png_image_raw(a, image_data, image_data_len, out_n, x, y, depth, color))
			{
				STBI_FREE(final);
				return 0;
			}
			for (j = 0; j < y; ++j)
			{
				for (i = 0; i < x; ++i)
				{
					plutovg_int_t out_y = j * yspc[p] + yorig[p];
					plutovg_int_t out_x = i * xspc[p] + xorig[p];

					memcpy(final + out_y * a->s->img_x * out_bytes + out_x * out_bytes, a->out + (j * x + i) * out_bytes, out_bytes);
				}
			}
			STBI_FREE(a->out);
			image_data += img_len;
			image_data_len -= img_len;
		}
	}
	a->out = final;

	return 1;
}

static int stbi__compute_transparency(stbi__png *z, stbi_uc tc[3], plutovg_int_t out_n)
{
	stbi__context *s = z->s;
	stbi__uint32 i;
	stbi__uint32 pixel_count = s->img_x * s->img_y;
	stbi_uc *p = z->out;

	/* compute color-based transparency, assuming we've */
	/* already got 255 as the alpha value in the output */
	STBI_ASSERT(out_n == 2 || out_n == 4);

	if (out_n == 2)
	{
		for (i = 0; i < pixel_count; ++i)
		{
			p[1] = (p[0] == tc[0] ? 0 : 255);
			p += 2;
		}
	} else
	{
		for (i = 0; i < pixel_count; ++i)
		{
			if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
				p[3] = 0;
			p += 4;
		}
	}
	return 1;
}

static int stbi__compute_transparency16(stbi__png *z, stbi__uint16 tc[3], plutovg_int_t out_n)
{
	stbi__context *s = z->s;
	stbi__uint32 i;
	stbi__uint32 pixel_count = s->img_x * s->img_y;
	stbi__uint16 *p = (stbi__uint16 *) z->out;

	/* compute color-based transparency, assuming we've */
	/* already got 65535 as the alpha value in the output */
	STBI_ASSERT(out_n == 2 || out_n == 4);

	if (out_n == 2)
	{
		for (i = 0; i < pixel_count; ++i)
		{
			p[1] = (p[0] == tc[0] ? 0 : 65535U);
			p += 2;
		}
	} else
	{
		for (i = 0; i < pixel_count; ++i)
		{
			if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
				p[3] = 0;
			p += 4;
		}
	}
	return 1;
}

static int stbi__expand_png_palette(stbi__png *a, stbi_uc *palette, plutovg_int_t len, plutovg_int_t pal_img_n)
{
	stbi__uint32 i;
	stbi__uint32 pixel_count = a->s->img_x * a->s->img_y;
	stbi_uc *p;
	stbi_uc *temp_out;
	stbi_uc *orig = a->out;

	p = (stbi_uc *) stbi__malloc_mad2(pixel_count, pal_img_n, 0);
	if (p == NULL)
		return stbi__err("outofmem", "Out of memory");

	/* between here and free(out) below, exitting would leak */
	temp_out = p;

	if (pal_img_n == 3)
	{
		for (i = 0; i < pixel_count; ++i)
		{
			int n = orig[i] * 4;

			p[0] = palette[n];
			p[1] = palette[n + 1];
			p[2] = palette[n + 2];
			p += 3;
		}
	} else
	{
		for (i = 0; i < pixel_count; ++i)
		{
			int n = orig[i] * 4;

			p[0] = palette[n];
			p[1] = palette[n + 1];
			p[2] = palette[n + 2];
			p[3] = palette[n + 3];
			p += 4;
		}
	}
	STBI_FREE(a->out);
	a->out = temp_out;

	STBI_NOTUSED(len);

	return 1;
}

static int stbi__unpremultiply_on_load_global = 0;
static int stbi__de_iphone_flag_global = 0;

STBIDEF void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply)
{
	stbi__unpremultiply_on_load_global = flag_true_if_should_unpremultiply;
}

STBIDEF void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert)
{
	stbi__de_iphone_flag_global = flag_true_if_should_convert;
}

#ifndef STBI_THREAD_LOCAL
#define stbi__unpremultiply_on_load stbi__unpremultiply_on_load_global
#define stbi__de_iphone_flag stbi__de_iphone_flag_global
#else
static STBI_THREAD_LOCAL int stbi__unpremultiply_on_load_local;
static STBI_THREAD_LOCAL int stbi__unpremultiply_on_load_set;
static STBI_THREAD_LOCAL int stbi__de_iphone_flag_local;
static STBI_THREAD_LOCAL int stbi__de_iphone_flag_set;

STBIDEF void stbi_set_unpremultiply_on_load_thread(int flag_true_if_should_unpremultiply)
{
	stbi__unpremultiply_on_load_local = flag_true_if_should_unpremultiply;
	stbi__unpremultiply_on_load_set = 1;
}

STBIDEF void stbi_convert_iphone_png_to_rgb_thread(int flag_true_if_should_convert)
{
	stbi__de_iphone_flag_local = flag_true_if_should_convert;
	stbi__de_iphone_flag_set = 1;
}

#define stbi__unpremultiply_on_load  (stbi__unpremultiply_on_load_set           \
                                       ? stbi__unpremultiply_on_load_local      \
                                       : stbi__unpremultiply_on_load_global)
#define stbi__de_iphone_flag  (stbi__de_iphone_flag_set                         \
                                ? stbi__de_iphone_flag_local                    \
                                : stbi__de_iphone_flag_global)
#endif /* STBI_THREAD_LOCAL */

static void stbi__de_iphone(stbi__png *z)
{
	stbi__context *s = z->s;
	stbi__uint32 i;
	stbi__uint32 pixel_count = s->img_x * s->img_y;
	stbi_uc *p = z->out;

	if (s->img_out_n == 3)
	{									/* convert bgr to rgb */
		for (i = 0; i < pixel_count; ++i)
		{
			stbi_uc t = p[0];

			p[0] = p[2];
			p[2] = t;
			p += 3;
		}
	} else
	{
		STBI_ASSERT(s->img_out_n == 4);
		if (stbi__unpremultiply_on_load)
		{
			/* convert bgr to rgb and unpremultiply */
			for (i = 0; i < pixel_count; ++i)
			{
				stbi_uc a = p[3];
				stbi_uc t = p[0];

				if (a)
				{
					stbi_uc half = a / 2;

					p[0] = (p[2] * 255 + half) / a;
					p[1] = (p[1] * 255 + half) / a;
					p[2] = (t * 255 + half) / a;
				} else
				{
					p[0] = p[2];
					p[2] = t;
				}
				p += 4;
			}
		} else
		{
			/* convert bgr to rgb */
			for (i = 0; i < pixel_count; ++i)
			{
				stbi_uc t = p[0];

				p[0] = p[2];
				p[2] = t;
				p += 4;
			}
		}
	}
}

#define STBI__PNG_TYPE(a,b,c,d)  (((stbi__uint32) (a) << 24) + ((stbi__uint32) (b) << 16) + ((stbi__uint32) (c) << 8) + (stbi__uint32) (d))

static int stbi__parse_png_file(stbi__png *z, int scan, int req_comp)
{
	stbi_uc palette[1024];
	stbi_uc pal_img_n = 0;
	stbi_uc has_trans = 0;
	stbi_uc tc[3] = { 0 };
	stbi__uint16 tc16[3];
	stbi__uint32 ioff = 0;
	stbi__uint32 idata_limit = 0;
	stbi__uint32 i;
	stbi__uint32 pal_len = 0;
	int first = 1;
	int k;
	int interlace = 0;
	int color = 0;
	int is_iphone = 0;
	stbi__context *s = z->s;

	z->expanded = NULL;
	z->idata = NULL;
	z->out = NULL;

	if (!stbi__check_png_header(s))
		return 0;

	if (scan == STBI__SCAN_type)
		return 1;

	for (;;)
	{
		stbi__pngchunk c;

		stbi__get_chunk_header(s, &c);
		if (c.type == STBI__PNG_TYPE('C', 'g', 'B', 'I'))
		{
			is_iphone = 1;
			stbi__skip(s, c.length);
		} else if (c.type == STBI__PNG_TYPE('I', 'H', 'D', 'R'))
		{
			int comp, filter;

			if (!first)
				return stbi__err("multiple IHDR", "Corrupt PNG");
			first = 0;
			if (c.length != 13)
				return stbi__err("bad IHDR len", "Corrupt PNG");
			s->img_x = stbi__get32be(s);
			s->img_y = stbi__get32be(s);
			if (s->img_y > STBI_MAX_DIMENSIONS)
				return stbi__err("too large", "Very large image (corrupt?)");
			if (s->img_x > STBI_MAX_DIMENSIONS)
				return stbi__err("too large", "Very large image (corrupt?)");
			z->depth = stbi__get8(s);
			if (z->depth != 1 && z->depth != 2 && z->depth != 4 && z->depth != 8 && z->depth != 16)
				return stbi__err("1/2/4/8/16-bit only", "PNG not supported: 1/2/4/8/16-bit only");
			color = stbi__get8(s);
			if (color > 6)
				return stbi__err("bad ctype", "Corrupt PNG");
			if (color == 3 && z->depth == 16)
				return stbi__err("bad ctype", "Corrupt PNG");
			if (color == 3)
				pal_img_n = 3;
			else if (color & 1)
				return stbi__err("bad ctype", "Corrupt PNG");
			comp = stbi__get8(s);
			if (comp)
				return stbi__err("bad comp method", "Corrupt PNG");
			filter = stbi__get8(s);
			if (filter)
				return stbi__err("bad filter method", "Corrupt PNG");
			interlace = stbi__get8(s);
			if (interlace > 1)
				return stbi__err("bad interlace method", "Corrupt PNG");
			if (!s->img_x || !s->img_y)
				return stbi__err("0-pixel image", "Corrupt PNG");
			if (!pal_img_n)
			{
				s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
				if (((int32_t)1 << 30) / s->img_x / s->img_n < s->img_y)
					return stbi__err("too large", "Image too large to decode");
			} else
			{
				/* if paletted, then pal_n is our final components, and */
				/* img_n is # components to decompress/filter. */
				s->img_n = 1;
				if (((int32_t)1 << 30) / s->img_x / 4 < s->img_y)
					return stbi__err("too large", "Corrupt PNG");
			}
			/* even with SCAN_header, have to scan to see if we have a tRNS */
		} else if (c.type == STBI__PNG_TYPE('P', 'L', 'T', 'E'))
		{
			if (first)
				return stbi__err("first not IHDR", "Corrupt PNG");
			if (c.length > 256 * 3)
				return stbi__err("invalid PLTE", "Corrupt PNG");
			pal_len = c.length / 3;
			if (pal_len * 3 != c.length)
				return stbi__err("invalid PLTE", "Corrupt PNG");
			for (i = 0; i < pal_len; ++i)
			{
				palette[i * 4 + 0] = stbi__get8(s);
				palette[i * 4 + 1] = stbi__get8(s);
				palette[i * 4 + 2] = stbi__get8(s);
				palette[i * 4 + 3] = 255;
			}
		} else if (c.type == STBI__PNG_TYPE('t', 'R', 'N', 'S'))
		{
			if (first)
				return stbi__err("first not IHDR", "Corrupt PNG");
			if (z->idata)
				return stbi__err("tRNS after IDAT", "Corrupt PNG");
			if (pal_img_n)
			{
				if (scan == STBI__SCAN_header)
				{
					s->img_n = 4;
					return 1;
				}
				if (pal_len == 0)
					return stbi__err("tRNS before PLTE", "Corrupt PNG");
				if (c.length > pal_len)
					return stbi__err("bad tRNS len", "Corrupt PNG");
				pal_img_n = 4;
				for (i = 0; i < c.length; ++i)
					palette[i * 4 + 3] = stbi__get8(s);
			} else
			{
				if (!(s->img_n & 1))
					return stbi__err("tRNS with alpha", "Corrupt PNG");
				if (c.length != (stbi__uint32) s->img_n * 2)
					return stbi__err("bad tRNS len", "Corrupt PNG");
				has_trans = 1;
				/* non-paletted with tRNS = constant alpha. if header-scanning, we can stop now. */
				if (scan == STBI__SCAN_header)
				{
					++s->img_n;
					return 1;
				}
				if (z->depth == 16)
				{
					for (k = 0; k < s->img_n && k < 3; ++k)	/* extra loop test to suppress false GCC warning */
						tc16[k] = (stbi__uint16) stbi__get16be(s);	/* copy the values as-is */
				} else
				{
					for (k = 0; k < s->img_n && k < 3; ++k)
						tc[k] = (stbi_uc) (stbi__get16be(s) & 255) * stbi__depth_scale_table[z->depth];	/* non 8-bit images will be larger */
				}
			}
		} else if (c.type == STBI__PNG_TYPE('I', 'D', 'A', 'T'))
		{
			if (first)
				return stbi__err("first not IHDR", "Corrupt PNG");
			if (pal_img_n && !pal_len)
				return stbi__err("no PLTE", "Corrupt PNG");
			if (scan == STBI__SCAN_header)
			{
				/* header scan definitely stops at first IDAT */
				if (pal_img_n)
					s->img_n = pal_img_n;
				return 1;
			}
			if (c.length > ((stbi__uint32)1u << 30))
				return stbi__err("IDAT size limit", "IDAT section larger than 2^30 bytes");
			if ((plutovg_int_t) (ioff + c.length) < (plutovg_int_t) ioff)
				return 0;
			if (ioff + c.length > idata_limit)
			{
				stbi__uint32 idata_limit_old = idata_limit;
				stbi_uc *p;

				if (idata_limit == 0)
					idata_limit = c.length > 4096 ? c.length : 4096;
				while (ioff + c.length > idata_limit)
					idata_limit *= 2;
				STBI_NOTUSED(idata_limit_old);
				p = (stbi_uc *) STBI_REALLOC_SIZED(z->idata, idata_limit_old, idata_limit);
				if (p == NULL)
					return stbi__err("outofmem", "Out of memory");
				z->idata = p;
			}
			if (!stbi__getn(s, z->idata + ioff, c.length))
				return stbi__err("outofdata", "Corrupt PNG");
			ioff += c.length;
		} else if (c.type == STBI__PNG_TYPE('I', 'E', 'N', 'D'))
		{
			stbi__uint32 raw_len;
			stbi__uint32 bpl;

			if (first)
				return stbi__err("first not IHDR", "Corrupt PNG");
			if (scan != STBI__SCAN_load)
				return 1;
			if (z->idata == NULL)
				return stbi__err("no IDAT", "Corrupt PNG");
			/* initial guess for decoded data size to avoid unnecessary reallocs */
			bpl = (s->img_x * z->depth + 7) / 8;	/* bytes per line, per component */
			raw_len = bpl * s->img_y * s->img_n /* pixels */  + s->img_y /* filter mode per row */ ;
			z->expanded =
				(stbi_uc *) stbi_zlib_decode_malloc_guesssize_headerflag((char *) z->idata, ioff, raw_len,
																		 &raw_len, !is_iphone);
			if (z->expanded == NULL)
				return 0;			/* zlib should set error */
			STBI_FREE(z->idata);
			z->idata = NULL;
			if ((req_comp == s->img_n + 1 && req_comp != 3 && !pal_img_n) || has_trans)
				s->img_out_n = s->img_n + 1;
			else
				s->img_out_n = s->img_n;
			if (!stbi__create_png_image(z, z->expanded, raw_len, s->img_out_n, z->depth, color, interlace))
				return 0;
			if (has_trans)
			{
				if (z->depth == 16)
				{
					if (!stbi__compute_transparency16(z, tc16, s->img_out_n))
						return 0;
				} else
				{
					if (!stbi__compute_transparency(z, tc, s->img_out_n))
						return 0;
				}
			}
			if (is_iphone && stbi__de_iphone_flag && s->img_out_n > 2)
				stbi__de_iphone(z);
			if (pal_img_n)
			{
				/* pal_img_n == 3 or 4 */
				s->img_n = pal_img_n;	/* record the actual colors we had */
				s->img_out_n = pal_img_n;
				if (req_comp >= 3)
					s->img_out_n = req_comp;
				if (!stbi__expand_png_palette(z, palette, pal_len, s->img_out_n))
					return 0;
			} else if (has_trans)
			{
				/* non-paletted image with tRNS -> source image has (constant) alpha */
				++s->img_n;
			}
			STBI_FREE(z->expanded);
			z->expanded = NULL;
			/* end of PNG chunk, read and skip CRC */
			stbi__get32be(s);
			return 1;
		} else
		{
			/* if critical, fail */
			if (first)
				return stbi__err("first not IHDR", "Corrupt PNG");
			if ((c.type & ((uint32_t)1 << 29)) == 0)
			{
#ifndef STBI_NO_FAILURE_STRINGS
				/* not threadsafe */
				static char invalid_chunk[] = "XXXX PNG chunk not known";

				invalid_chunk[0] = STBI__BYTECAST(c.type >> 24);
				invalid_chunk[1] = STBI__BYTECAST(c.type >> 16);
				invalid_chunk[2] = STBI__BYTECAST(c.type >> 8);
				invalid_chunk[3] = STBI__BYTECAST(c.type >> 0);
#endif
				return stbi__err(invalid_chunk, "PNG not supported: unknown PNG chunk type");
			}
			stbi__skip(s, c.length);
		}
		/* end of PNG chunk, read and skip CRC */
		stbi__get32be(s);
	}
}

static void *stbi__do_png(stbi__png *p, plutovg_int_t *x, plutovg_int_t *y, int *n, int req_comp, stbi__result_info *ri)
{
	void *result = NULL;

	if (req_comp < 0 || req_comp > 4)
		return stbi__errpuc("bad req_comp", "Internal error");
	if (stbi__parse_png_file(p, STBI__SCAN_load, req_comp))
	{
		if (p->depth <= 8)
			ri->bits_per_channel = 8;
		else if (p->depth == 16)
			ri->bits_per_channel = 16;
		else
			return stbi__errpuc("bad bits_per_channel", "PNG not supported: unsupported color depth");
		result = p->out;
		p->out = NULL;
		if (req_comp && req_comp != p->s->img_out_n)
		{
			if (ri->bits_per_channel == 8)
				result =
					stbi__convert_format((unsigned char *) result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
			else
				result =
					stbi__convert_format16((stbi__uint16 *) result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
			p->s->img_out_n = req_comp;
			if (result == NULL)
				return result;
		}
		*x = p->s->img_x;
		*y = p->s->img_y;
		if (n)
			*n = (int)p->s->img_n;
	}
	STBI_FREE(p->out);
	p->out = NULL;
	STBI_FREE(p->expanded);
	p->expanded = NULL;
	STBI_FREE(p->idata);
	p->idata = NULL;

	return result;
}

static void *stbi__png_load(stbi__context *s, plutovg_int_t *x, plutovg_int_t *y, int *comp, int req_comp, stbi__result_info *ri)
{
	stbi__png p;

	p.s = s;
	return stbi__do_png(&p, x, y, comp, req_comp, ri);
}

static int stbi__png_test(stbi__context *s)
{
	int r;

	r = stbi__check_png_header(s);
	stbi__rewind(s);
	return r;
}

static int stbi__png_info_raw(stbi__png *p, plutovg_int_t *x, plutovg_int_t *y, int *comp)
{
	if (!stbi__parse_png_file(p, STBI__SCAN_header, 0))
	{
		stbi__rewind(p->s);
		return 0;
	}
	if (x)
		*x = p->s->img_x;
	if (y)
		*y = p->s->img_y;
	if (comp)
		*comp = (int)p->s->img_n;
	return 1;
}

static int stbi__png_info(stbi__context *s, plutovg_int_t *x, plutovg_int_t *y, int *comp)
{
	stbi__png p;

	p.s = s;
	return stbi__png_info_raw(&p, x, y, comp);
}
#endif

static int stbi__info_main(stbi__context *s, plutovg_int_t *x, plutovg_int_t *y, int *comp)
{
#ifndef STBI_NO_PNG
	if (stbi__png_info(s, x, y, comp))
		return 1;
#endif

	return stbi__err("unknown image type", "Image not of any known type, or corrupt");
}

#ifndef STBI_NO_STDIO
STBIDEF int stbi_info(char const *filename, plutovg_int_t *x, plutovg_int_t *y, int *comp)
{
	FILE *f = stbi__fopen(filename, "rb");
	int result;

	if (!f)
		return stbi__err("can't fopen", "Unable to open file");
	result = stbi_info_from_file(f, x, y, comp);
	fclose(f);
	return result;
}

STBIDEF int stbi_info_from_file(FILE *f, plutovg_int_t *x, plutovg_int_t *y, int *comp)
{
	int r;
	stbi__context s;
	long pos = ftell(f);

	stbi__start_file(&s, f);
	r = stbi__info_main(&s, x, y, comp);
	fseek(f, pos, SEEK_SET);
	return r;
}

#endif /* !STBI_NO_STDIO */

#endif /* STB_IMAGE_IMPLEMENTATION */
/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
