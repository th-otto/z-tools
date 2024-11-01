#include "zview.h"
#include <mint/slb.h>
#include <mint/mintbind.h>
#include <errno.h>
#include "symbols.h"
#include <slb/png.h>
#include <slb/zlib.h>
#undef INT16
#include <slb/jpeg.h>
#include <slb/tiff.h>
#include <slb/lzma.h>
#include <slb/exif.h>
#include <slb/bzip2.h>
#include <slb/freetype.h>
#include "zvplugin.h"
#include "plugin.h"
#include "plugver.h"
#include "slbload.h"
#include "../../unicodemap.h"
#define NF_DEBUG 1
#if NF_DEBUG
#include <mint/arch/nf_ops.h>
#else
#define nf_debugprintf(format, ...)
#endif


/*
 * redirect all management of dependant libraries to us
 */
SLB *__CDECL (*p_slb_get)(long lib) = plugin_slb_get;
long __CDECL (*p_slb_open)(long lib, const char *slbpath) = plugin_slb_open;
void __CDECL (*p_slb_close)(long lib) = plugin_slb_close;

static struct _zview_plugin_funcs zview_plugin_funcs;

#undef SLB_NWORDS
#undef SLB_NARGS
#if defined(__MSHORT__) || defined(__PUREC__)
#define SLB_NARGS(_nargs) 0, _nargs
#else
#define SLB_NARGS(_nargs) _nargs
#endif

#undef errno

typedef struct {
	SLB slb;
	unsigned int refcount;
} ZVSLB;

static zv_int_t __CDECL get_errno(void)
{
	return errno;
}


#if 0
#pragma GCC optimize "-fno-defer-pop"

static long __inline __attribute__((__always_inline__)) get_sp(void)
{
	long r;
	asm volatile("move.l %%sp,%0" : "=r"(r) : : "cc");
	return r;
}
#endif


long __CDECL plugin_slb_control(SLB *slb, long fn, void *arg)
{
	return slb->exec(slb->handle, 0, SLB_NARGS(2), fn, arg);
}

	
boolean __CDECL plugin_reader_init(SLB *slb, const char *name, IMGINFO info)
{
	return slb->exec(slb->handle, 1, SLB_NARGS(2), name, info) > 0;
}


boolean __CDECL plugin_reader_read(SLB *slb, IMGINFO info, uint8_t *buffer)
{
	return slb->exec(slb->handle, 2, SLB_NARGS(2), info, buffer) > 0;
}


void __CDECL plugin_reader_get_txt(SLB *slb, IMGINFO info, txt_data *txtdata)
{
	slb->exec(slb->handle, 3, SLB_NARGS(2), info, txtdata);
}


void __CDECL plugin_reader_quit(SLB *slb, IMGINFO info)
{
	slb->exec(slb->handle, 4, SLB_NARGS(1), info);
}


boolean __CDECL plugin_encoder_init(SLB *slb, const char *name, IMGINFO info)
{
	return slb->exec(slb->handle, 5, SLB_NARGS(2), name, info) > 0;
}


boolean __CDECL plugin_encoder_write(SLB *slb, IMGINFO info, uint8_t *buffer)
{
	return slb->exec(slb->handle, 6, SLB_NARGS(2), info, buffer) > 0;
}


void __CDECL plugin_encoder_quit(SLB *slb, IMGINFO info)
{
	slb->exec(slb->handle, 7, SLB_NARGS(1), info);
}


long __CDECL plugin_get_option(SLB *slb, zv_int_t which)
{
	return slb->exec(slb->handle, 8, SLB_NARGS(1), which);
}


long __CDECL plugin_set_option(SLB *slb, zv_int_t which, zv_int_t value)
{
	return slb->exec(slb->handle, 9, SLB_NARGS(2), which, value);
}


static long __CDECL slb_not_loaded(SLB_HANDLE slb, long fn, short nwords, ...)
{
	(void)slb;
	(void)fn;
	(void)nwords;
	(void) Cconws("a shared lib was not loaded\r\n");
	Pterm(1);
	return -32;
}


static long __CDECL slb_unloaded(SLB_HANDLE slb, long fn, short nwords, ...)
{
	(void)slb;
	(void)fn;
	(void)nwords;
	(void) Cconws("a shared lib was already unloaded\r\n");
	Pterm(1);
	return -32;
}


static void slb_unref(ZVSLB *lib, const char *which)
{
	if (!lib->slb.handle)
	{
		nf_debugprintf("slb_unref(%s): not loaded\n", which);
		return;
	}
	if (lib->refcount == 0)
	{
		nf_debugprintf("slb_unref(%s): underflow\n", which);
		return;
	}
	if (--lib->refcount == 0)
	{
		nf_debugprintf("slb_unref(%s): unloading\n", which);
		slb_unload(lib->slb.handle);
		lib->slb.handle = 0;
		lib->slb.exec = slb_unloaded;
	}
}



/*
 * ZLIB is always needed, since it is used by PNGLIB.
 * It might also be used by TIFF, ZSTD & FREETYPE
 */
static ZVSLB zlib = { { 0, slb_not_loaded }, 0 };

SLB *slb_zlib_get(void)
{
	return &zlib.slb;
}


void slb_zlib_close(void)
{
	slb_unref(&zlib, "zlib");
}


/*
 * PNGLIB is always needed, since it is used by the png plugin.
 * It might also be used by XPDF
 */
static ZVSLB pnglib = { { 0, slb_not_loaded }, 0 };

SLB *slb_pnglib_get(void)
{
	return &pnglib.slb;
}


void slb_pnglib_close(void)
{
#if 0
	slb_zlib_close();
#endif
	slb_unref(&pnglib, "pnglib");
}


/*
 * JPEG is always needed, since it is used by the jpeg plugin.
 * It might also be used by TIFF
 */
static ZVSLB jpeglib = { { 0, slb_not_loaded }, 0 };

SLB *slb_jpeglib_get(void)
{
	return &jpeglib.slb;
}


void slb_jpeglib_close(void)
{
	slb_unref(&jpeglib, "jpeg");
}


/*
 * EXIF is always needed, since it is used by the jpeg plugin.
 */
static ZVSLB exif = { { 0, slb_not_loaded }, 0 };

SLB *slb_exif_get(void)
{
	return &exif.slb;
}


void slb_exif_close(void)
{
	slb_unref(&exif, "exif");
}


/*
 * TIFF is always needed, since it is used by the tiff plugin.
 */
static ZVSLB tiff = { { 0, slb_not_loaded }, 0 };

SLB *slb_tiff_get(void)
{
	return &tiff.slb;
}


void slb_tiff_close(void)
{
	slb_unref(&tiff, "tiff");
}


/*
 * currently, only the TIFF library might use LZMA compression,
 * and tiffconf.h defines LZMA_SUPPORT in this case
 */
#if defined(LZMA_SUPPORT) && defined(LZMA_SLB)
static ZVSLB lzma = { { 0, slb_not_loaded }, 0 };

SLB *slb_lzma_get(void)
{
	return &lzma.slb;
}


void slb_lzma_close(void)
{
	slb_unref(&lzma, "lzma");
}
#endif


/*
 * BZIP2 is always needed, since it is used by XPDF.
 */
static ZVSLB bzip2 = { { 0, slb_not_loaded }, 0 };

SLB *slb_bzip2_get(void)
{
	return &bzip2.slb;
}


void slb_bzip2_close(void)
{
	slb_unref(&bzip2, "bzip2");
}


/*
 * FREETYPE is always needed, since it is used by XPDF.
 */
static ZVSLB freetype = { { 0, slb_not_loaded }, 0 };

SLB *slb_freetype_get(void)
{
	return &freetype.slb;
}


void slb_freetype_close(void)
{
	slb_unref(&freetype, "freetype");
}


/*
 * WEBP_SLB is currently not used, since the only
 * library using it is the webp plugin, and there is
 * no shared lib for it yet
 */
#ifdef WEBP_SLB
static ZVSLB webp = { { 0, slb_not_loaded }, 0 };

SLB *slb_webp_get(void)
{
	return &webp.slb;
}


void slb_webp_close(void)
{
	slb_unref(&webp, "webp");
}
#endif


/*
 * currently, only the TIFF library might use ZSTD compression,
 * and tiffconf.h defines ZSTD_SUPPORT in this case
 */
#if defined(ZSTD_SUPPORT) && defined(ZSTD_SLB)
static ZVSLB zstd = { { 0, slb_not_loaded }, 0 };

SLB *slb_zstd_get(void)
{
	return &zstd.slb;
}


void slb_zstd_close(void)
{
	slb_unref(&zstd, "zstd");
}
#endif


static long slb_ref(ZVSLB *lib, long (*openit)(const char *path), const char *path, const char *which)
{
	long ret;

	if (lib->refcount == 0)
	{
		/*
		 * the loading function may call slb_unload on failure.
		 * prevent underflow of the refcount
		 */
		++lib->refcount;
		ret = openit(path);
		if (ret < 0)
		{
			lib->refcount = 0;
			return ret;
		}
		lib->refcount = 0;
		nf_debugprintf("slb_ref(%s): loaded\n", which);
	}
	if (++lib->refcount == 0)
	{
		nf_debugprintf("slb_ref(%s): overflow\n", which);
		lib->refcount = -1;
	}
	nf_debugprintf("slb_ref(%s): refcount=%u\n", which, lib->refcount);
	return 0;
}


#if 0
/*
 * when loading pnglib, and zlib was not loaded before,
 * it will open zlib on its own. That would however
 * not use our reference counter, so load it here.
 */
static long slb_open_zlib_and_pnglib(const char *path)
{
	long ret;
	
	ret = slb_ref(&zlib, slb_zlib_open, path, "zlib");
	if (ret >= 0)
	{
		ret = slb_pnglib_open(path);
		if (ret < 0)
			slb_zlib_close();
	}
	return ret;
}
#endif


static long __CDECL plugin_slb_try_open(long lib, const char *path)
{
	switch ((int) lib)
	{
	case LIB_PNG:
#if 0
		return slb_ref(&pnglib, slb_open_zlib_and_pnglib, path, "pnglib");
#else
		return slb_ref(&pnglib, slb_pnglib_open, path, "pnglib");
#endif
	case LIB_Z:
		return slb_ref(&zlib, slb_zlib_open, path, "zlib");
	case LIB_JPEG:
		return slb_ref(&jpeglib, slb_jpeglib_open, path, "jpeg");
	case LIB_TIFF:
		return slb_ref(&tiff, slb_tiff_open, path, "tiff");
#if defined(LZMA_SUPPORT) && defined(LZMA_SLB)
	case LIB_LZMA:
		return slb_ref(&lzma, slb_lzma_open, path, "lzma");
#endif
	case LIB_EXIF:
		return slb_ref(&exif, slb_exif_open, path, "exif");
	case LIB_BZIP2:
		return slb_ref(&bzip2, slb_bzip2_open, path, "bzip");
	case LIB_FREETYPE:
		return slb_ref(&freetype, slb_freetype_open, path, "freetype");
#ifdef WEBP_SLB
	case LIB_WEBP:
		return slb_ref(&webp, slb_webp_open, path, "webp");
#endif
#if defined(ZSTD_SUPPORT) && defined(ZSTD_SLB)
	case LIB_ZSTD:
		return slb_ref(&zszd, slb_zstd_open, path, "zstd");
#endif
	}
	return -ENOENT;
}


long __CDECL plugin_slb_open(long lib, const char *path)
{
	long ret;
	
	(void)path;
	/*
	 * try CPU dependant directory first <path-to-zview>/slb/000
	 */
	ret = plugin_slb_try_open(lib, zview_slb_dir);
	if (ret < 0)
	{
		/*
		 * try zview directory next <zview>/slb
		 */
		char c = *zview_slb_dir_end;
		*zview_slb_dir_end = '\0';
		ret = plugin_slb_try_open(lib, zview_slb_dir);
		*zview_slb_dir_end = c;
	}
	if (ret < 0)
	{
		/*
		 * try mints directory last ($SLBPATH)
		 */
		ret = plugin_slb_try_open(lib, NULL);
	}
	return ret;
}


void __CDECL plugin_slb_close(long lib)
{
	switch ((int) lib)
	{
	case LIB_PNG:
		slb_pnglib_close();
		break;
	case LIB_Z:
		slb_zlib_close();
		break;
	case LIB_JPEG:
		slb_jpeglib_close();
		break;
	case LIB_TIFF:
		slb_tiff_close();
		break;
#if defined(LZMA_SUPPORT) && defined(LZMA_SLB)
	case LIB_LZMA:
		slb_lzma_close();
		break;
#endif
	case LIB_EXIF:
		slb_exif_close();
		break;
	case LIB_BZIP2:
		slb_bzip2_close();
		break;
	case LIB_FREETYPE:
		slb_freetype_close();
		break;
#ifdef WEBP_SLB
	case LIB_WEBP:
		slb_webp_close();
		break;
#endif
#if defined(ZSTD_SUPPORT) && defined(ZSTD_SLB)
	case LIB_ZSTD:
		slb_zstd_close();
		break;
#endif
	}
}


SLB *__CDECL plugin_slb_get(long lib)
{
	switch ((int) lib)
	{
	case LIB_PNG:
		return slb_pnglib_get();
	case LIB_Z:
		return slb_zlib_get();
	case LIB_JPEG:
		return slb_jpeglib_get();
	case LIB_TIFF:
		return slb_tiff_get();
#if defined(LZMA_SUPPORT) && defined(LZMA_SLB)
	case LIB_LZMA:
		return slb_lzma_get();
#endif
	case LIB_EXIF:
		return slb_exif_get();
	case LIB_BZIP2:
		return slb_bzip2_get();
	case LIB_FREETYPE:
		return slb_freetype_get();
#ifdef WEBP_SLB
	case LIB_WEBP:
		return slb_webp_get();
#endif
#if defined(ZSTD_SUPPORT) && defined(ZSTD_SLB)
	case LIB_ZSTD:
		return slb_zstd_get();
#endif
	}
	return NULL;
}


long plugin_open(const char *name, const char *path, SLB *slb)
{
	long ret;
	unsigned long flags;
	long cpu = 0;

	if (!slb)
		return -EBADF;
	if (slb->handle)
		return 0;

	zview_plugin_funcs.struct_size = sizeof(zview_plugin_funcs);
	zview_plugin_funcs.int_size = sizeof(zv_int_t);
	if (zview_plugin_funcs.int_size != sizeof(long))
		return -EINVAL;
	zview_plugin_funcs.interface_version = PLUGIN_INTERFACE_VERSION;
	zview_plugin_funcs.p_slb_open = plugin_slb_open;
	zview_plugin_funcs.p_slb_close = plugin_slb_close;
	zview_plugin_funcs.p_slb_get = plugin_slb_get;

#define S(x) zview_plugin_funcs.p_##x = x

	S(memset);
	S(memcpy);
	S(memchr);
	S(memcmp);

	S(strlen);
	S(strcpy);
	S(strncpy);
	S(strcat);
	S(strncat);
	S(strcmp);
	S(strncmp);

	S(malloc);
	S(calloc);
	S(realloc);
	S(free);

	S(get_errno);
	S(strerror);
	S(abort);
	zview_plugin_funcs.stderr_location = stderr;
	
	S(remove);

	S(open);
	S(close);
	S(read);
	S(write);
	S(lseek);

	S(fopen);
	S(fdopen);
	S(fclose);
	S(fseek);
	S(fseeko);
	S(ftell);
	S(ftello);
	S(printf);
	S(sprintf);
	S(vsnprintf);
	S(vfprintf);
	S(fread);
	S(fwrite);
	S(ferror);
	S(fflush);

	S(rand);
	S(srand);

	S(qsort);
	S(bsearch);

	S(time);
	S(localtime);
	S(gmtime);

	S(fstat);

	S(sigsetjmp);
	S(longjmp);

	S(atof);
	
	S(utf8_to_ucs16);
	S(ucs16_to_latin1);
	S(latin1_to_atari);
#undef S

	ret = slb_load(name, path, PLUGIN_INTERFACE_VERSION, &slb->handle, &slb->exec);
	if (ret < 0)
	{
		slb->handle = 0;
		return ret;
	}

	/*
	 * check compile flags; that function should be as simple as to just return a constant
	 * and we can hopefully call it even on mismatched configurations
	 */
	flags = plugin_compile_flags(slb);
	Getcookie(C__CPU, &cpu);
	if (cpu >= 20)
	{
		/* should be able to use a 000 library, anyways */
	} else
	{
		if (flags & SLB_M68020)
		{
			/* cpu is not 020+, but library was compiled for it */
			plugin_close(slb, FALSE);
			return -EINVAL;
		}
	}
#if defined(__mcoldfire__)
	/* if cpu is cf, but library was not compiled for it... */
	if (!(flags & SLB_COLDFIRE))
#else
	/* if cpu is not cf, but library was compiled for it... */
	if (flags & SLB_COLDFIRE)
#endif
	{
		plugin_close(slb, FALSE);
		return -EINVAL;
	}
	
	ret = plugin_set_imports(slb, &zview_plugin_funcs);
	if (ret < 0)
	{
		plugin_close(slb, FALSE);
		return ret;
	}
	
	return ret;
}


void plugin_close(SLB *slb, boolean waitpid)
{
	if (!slb || !slb->handle)
		return;
	slb_unload(slb->handle);
	slb->handle = 0;
	slb->exec = 0;
	if (waitpid)
	{
		/*
		 * MiNT has a bug, leaving the unloaded library behind as a zombie
		 */
		Syield();
		Pwaitpid(-1, 3, NULL);
	}
}
