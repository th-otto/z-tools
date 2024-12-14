/* must include math.h here before atof etc. are redefined */
#include <math.h>
/* must include C++ headers here, or our macros get undefined by cstdio */
#include <lunasvg/lunasvg.h>

#ifdef ZLIB_SLB
#include <slb/zlib.h>
#else
#include <zlib.h>
#endif
#include "plugin.h"
#include "zvplugin.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"
#define NF_DEBUG 0
#include "nfdebug.h"

using namespace lunasvg;

#ifdef PLUGIN_SLB

#ifdef __cplusplus
extern "C" {
#endif

/* maybe referenced by some math functions */
#undef errno
int errno;

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
	case OPTION_EXTENSIONS:
		return (long) (EXTENSIONS);

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
#ifdef MISC_INFO
	case INFO_MISC:
		return (long)MISC_INFO;
#endif
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}

#ifdef ZLIB_SLB
static long init_zlib_slb(void)
{
	struct _zview_plugin_funcs *funcs;
	long ret;

	funcs = get_slb_funcs();
	nf_debugprintf(("zvsvg: open zlib\n"));
	if ((ret = funcs->p_slb_open(LIB_Z, NULL)) < 0)
	{
		nf_debugprintf(("error loading " ZLIB_SHAREDLIB_NAME ": %ld\n", ret));
		return ret;
	}
	return 0;
}


static void quit_zlib_slb(void)
{
	struct _zview_plugin_funcs *funcs;

	funcs = get_slb_funcs();
	nf_debugprintf(("zvsvg: close zlib\n"));
	funcs->p_slb_close(LIB_Z);
}
#endif

/* some functions are not yet exported to SLB */
#include "libcmini.c"


/*
 * lunasvg has references to some functions
 */
void *(memcpy)(void *dest, const void *src, size_t len)
{
	return memcpy(dest, src, len);
}
void *(memmove)(void *dest, const void *src, size_t len) __attribute__((alias("memcpy")));


void *(memset)(void *dest, int c, size_t len)
{
	return memset(dest, c, len);
}

void *(malloc)(size_t size)
{
	return malloc(size);
}

void *(calloc)(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}

void *(realloc)(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void (free)(void *ptr)
{
	free(ptr);
}

size_t (strlen)(const char *s)
{
	return strlen(s);
}

int (strcmp)(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}

int (strncmp)(const char *s1, const char *s2, size_t n)
{
	return strncmp(s1, s2, n);
}

int (memcmp)(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

void (abort)(void)
{
	abort();
}

void *(memchr)(const void *s, int c, size_t n)
{
	return memchr(s, c, n);
}

FILE *(fopen)(const char *pathname, const char *mode)
{
	return fopen(pathname, mode);
}

size_t (fread)(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return fread(ptr, size, nmemb, stream);
}

size_t (fwrite)(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return fwrite(ptr, size, nmemb, stream);
}

int (fseek)(FILE *stream, long offset, int whence)
{
	return fseek(stream, offset, whence);
}

int (lseek)(int fd, off_t offset, int whence)
{
	return lseek(fd, offset, whence);
}

int (fclose)(FILE *stream)
{
	return fclose(stream);
}

long (ftell)(FILE *stream)
{
	return ftell(stream);
}

#undef sprintf
int sprintf(char *str, const char *format, ...)
{
	va_list ap;
	int ret;
	
	va_start(ap, format);
	ret = vsnprintf(str, 0x7ffffff0l, format, ap);
	va_end(ap);
	return ret;
}

int (vsnprintf)(char *str, size_t size, const char *format, va_list ap)
{
	return vsnprintf(str, size, format, ap);
}

void *(bsearch)(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
	return bsearch(key, base, nmemb, size, compar);
}

int (fputs)(const char *s, FILE *stream)
{
	return fwrite(s, 1, strlen(s), stream);
}

int (fflush)(FILE *stream)
{
	return fflush(stream);
}

char *(strerror)(int e)
{
	return strerror(e);
}

void (longjmp)(jmp_buf env, int val)
{
	longjmp(env, val);
}

int (sigsetjmp)(sigjmp_buf env, int savesigs)
{
	return sigsetjmp(env, savesigs);
}

ssize_t (write)(int fd, const void *buf, size_t count)
{
	return write(fd, buf, count);
}

ssize_t (read)(int fd, void *buf, size_t count)
{
	return read(fd, buf, count);
}

FILE *(fdopen)(int fd, const char *mode)
{
	return fdopen(fd, mode);
}

int (fstat)(int fd, struct stat *statbuf)
{
	return fstat(fd, statbuf);
}

/*
 * Remaining functions are only referenced in libstc++,
 * from functions which are (hopefully) not used here
 */

int (fileno)(FILE *stream)
{
	(void)stream;
	return -1;
}

char *setlocale(int category, const char *name)
{
	static char c_locale[] = "C";
	(void) category;
	(void) name;
	return c_locale;
}

int strcoll(const char *s1, const char *s2)
{
	(void)s1;
	(void)s2;
	return 0;
}

size_t (strxfrm)(char *dest, const char *src, size_t n)
{
	(void)dest;
	(void)src;
	(void)n;
	return 0;
}
                      
ssize_t (writev)(int fd, const struct iovec *iov, int iovcnt)
{
	(void)fd;
	(void)iov;
	(void)iovcnt;
	return -1;
}

int (atexit)(void (*function)(void))
{
	(void)function;
	return -1;
}

long double (strtold)(const char *nptr, char **endptr)
{
	(void)nptr;
	(void)endptr;
	return 0;
}

double (strtod)(const char *nptr, char **endptr)
{
	(void)nptr;
	(void)endptr;
	return 0;
}

float (strtof)(const char *nptr, char **endptr)
{
	(void)nptr;
	(void)endptr;
	return 0;
}

size_t (strftime)(char *s, size_t max, const char *format, const struct tm *tm)
{
	(void)s;
	(void)max;
	(void)format;
	(void)tm;
	return 0;
}

int (setvbuf)(FILE *stream, char *buf, int mode, size_t size)
{
	(void)stream;
	(void)buf;
	(void)mode;
	(void)size;
	return -1;
}

int (fputc)(int c, FILE *stream)
{
	(void)c;
	(void)stream;
	return -1;
}

int (ungetc)(int c, FILE *stream)
{
	(void)c;
	(void)stream;
	return -1;
}

int (ioctl)(int fildes, int request, ...)
{
	(void)fildes;
	(void)request;
	return -1;
}

int (fgetc)(FILE *stream)
{
	(void)stream;
	return -1;
}

char* (strerror_r)(int errnum, char* buf, size_t buflen)
{
	(void)errnum;
	(void)buf;
	(void)buflen;
	return buf;
}

char *getenv(const char *name)
{
	(void)name;
	return 0;
}

#undef open
int (open)(const char *path, int oflag, ...)
{
	(void)path;
	(void)oflag;
	return -1;
}

int (close)(int fd)
{
	(void)fd;
	return -1;
}

#include <poll.h>

int (poll)(struct pollfd *fds, nfds_t nfds, int timeout)
{
	(void)fds;
	(void)nfds;
	(void)timeout;
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif
void __main(void);
#ifdef __cplusplus
}
#endif

static uint32_t swap32(uint32_t l)
{
	return ((l >> 24) & 0xffL) | ((l << 8) & 0xff0000L) | ((l >> 8) & 0xff00L) | ((l << 24) & 0xff000000UL);
}


/*==================================================================================*
 * boolean __CDECL reader_init:														*
 *		Open the file "name", fit the "info" struct. ( see zview.h) and make others	*
 *		things needed by the decoder.												*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		name		->	The file to open.											*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	uint8_t *bmap;
	char *data;
	size_t image_size;
	uint32_t file_size;
	int fd;
	uint8_t magic[2];
	
	/* main() won't call __main() for global constructors, so do it here. */
	__main();

#ifdef ZLIB_SLB
	if (init_zlib_slb() < 0)
	{
		RETURN_ERROR(EC_InternalError);
	}
#endif
	
	fd = (int)Fopen(name, FO_READ);
	if (fd < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	/*
	 * check for gzip header
	 */
	if (Fread(fd, 2, magic) == 2 &&
		magic[0] == 31 && magic[1] == 139)
	{
		gzFile gzfile;

		/*
		 * uncompressed size is stored at the file end,
		 * in little-endian order
		 */
		Fseek(-4, fd, SEEK_END);
		if (Fread(fd, sizeof(file_size), &file_size) != sizeof(file_size))
		{
			Fclose(fd);
			RETURN_ERROR(EC_Fread);
		}
		
		file_size = swap32(file_size);;
		nf_debugprintf(("compressed file: %lu\n", (unsigned long)file_size));

		Fseek(0, fd, SEEK_SET);
		gzfile = gzdopen(fd, "r");
		if (gzfile == NULL)
		{
			Fclose(fd);
			RETURN_ERROR(EC_Malloc);
		}
		data = (char *) malloc(file_size + 1);
		if (data == NULL)
		{
			gzclose(gzfile);
			RETURN_ERROR(EC_Malloc);
		}

		/*
		 * Use gzfread rather than gzread, because parameter for gzread
		 * may be 16bit only
		 */
		if (gzfread(data, 1, file_size, gzfile) != file_size)
		{
#if NF_DEBUG
			z_int_t err;
			gzerror(gzfile, &err);
			nf_debugprintf(("decompress error: %ld\n", (long)err));
#endif
			free(data);
			gzclose(gzfile);
			RETURN_ERROR(EC_DecompError);
		}
		gzclose(gzfile);
	} else
	{
		nf_debugprintf(("uncompressed file\n"));
		file_size = Fseek(0, fd, SEEK_END);
		Fseek(0, fd, SEEK_SET);
		data = (char *) malloc(file_size + 1);
		if (data == NULL)
		{
			Fclose(fd);
			RETURN_ERROR(EC_Malloc);
		}
		if ((size_t)Fread(fd, file_size, data) != file_size)
		{
			free(data);
			Fclose(fd);
			RETURN_ERROR(EC_Fread);
		}
		Fclose(fd);
	}

	data[file_size] = '\0';					/* Must be null terminated. */

	auto svg_image = Document::loadFromData(data, file_size);
	if (svg_image == NULL)
	{
		free(data);
		RETURN_ERROR(EC_Malloc);
	}
	free(data);

	info->width = svg_image->width();
	info->height = svg_image->height();
	if (info->width <= 0 || info->height <= 0)
	{
		RETURN_ERROR(EC_FileType);
	}
	image_size = (size_t)info->width * info->height * 4;
	bmap = (uint8_t *)malloc(image_size);
	if (bmap == NULL)
	{
		RETURN_ERROR(EC_Malloc);
	}

	float xScale = 1.0f;
	float yScale = 1.0f;
	Matrix matrix(xScale, 0, 0, yScale, 0, 0);
	Bitmap bitmap(bmap, info->width, info->height, (size_t)info->width * 4);
	if (bitmap.isNull())
	{
		free(bmap);
		RETURN_ERROR(EC_Malloc);
	}

	bitmap.clear(0xffffffffUL);
	svg_image->render(bitmap, matrix);

	info->planes = 24;
	info->colors = 1L << 24;
	info->components = 3;
	info->indexed_color = FALSE;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;

	strcpy(info->info, "Scalable Vector Graphics");
	strcpy(info->compression, "None");

	info->_priv_ptr = bmap;
	info->_priv_var = 0;		/* y offset */

	RETURN_SUCCESS();
}


/*==================================================================================*
 * boolean __CDECL reader_read:														*
 *		This function fits the buffer with image data								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		buffer		->	The destination buffer.										*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	int x;
	
	bmap += info->_priv_var;
	x = info->width;
	info->_priv_var += (size_t)x * 4;
	
	/* ARGB */
	do
	{
		bmap++;
		*buffer++ = *bmap++;
		*buffer++ = *bmap++;
		*buffer++ = *bmap++;
	} while (--x > 0);

	RETURN_SUCCESS();
}


/*==================================================================================*
 * boolean __CDECL reader_quit:														*
 *		This function makes the last job like close the file handler and free the	*
 *		allocated memory.															*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_quit(IMGINFO info)
{
	void *p = info->_priv_ptr;
	info->_priv_ptr = NULL;
	free(p);
#ifdef ZLIB_SLB
	quit_zlib_slb();
#endif
}


/*==================================================================================*
 * boolean __CDECL reader_get_txt													*
 *		This function , like other function must be always present.					*
 *		It fills txtdata struct. with the text present in the picture ( if any).	*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		txtdata		->	The destination text buffer.								*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
