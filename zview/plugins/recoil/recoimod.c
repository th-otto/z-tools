#include "plugin.h"
#include "zvplugin.h"
#include "recoil.h"

#define VERSION		0x0204
#define AUTHOR      "Thorsten Otto"
#define NAME        "RECOIL - Retro Computer Image Library"
#define DATE        __DATE__ " " __TIME__
#define MISC_INFO   "RECOIL v" RECOIL_VERSION " Retro Computer Image Library (C) " RECOIL_YEARS " Piotr Fusik"

#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("*\0");

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
	case INFO_MISC:
		return (long)MISC_INFO;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif


/*
 * RECOIL uses some large arrays on the stack in some functions
 */
#define PLUGIN_STACKSIZE 524288

#ifdef PLUGIN_STACKSIZE
static char plugin_stack[PLUGIN_STACKSIZE];

static boolean __CDECL reader_init_on_stack(const char *name, IMGINFO info);

boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	register boolean ret asm("d0");
	__asm__ __volatile__(
		" move.l %%a5,-(%%sp)\n"   /* save a5 */
		" move.l %%sp,%%a5\n"      /* save original stack pointer */
		" move.l %4,%%sp\n"        /* set new stack */
		" move.l %2,-(%%sp)\n"     /* push arguments */
		" move.l %1,-(%%sp)\n"
		" jbsr %3\n"               /* call actual function */
		" move.l %%a5,%%sp\n"      /* restore stack pointer */
		" move.l (%%sp)+,%%a5\n"   /* restore a5 */
	: "=d"(ret)
	: "r"(name), "r"(info), "m"(reader_init_on_stack), "i"(plugin_stack + PLUGIN_STACKSIZE - 32)
	: "a5", "cc" AND_MEMORY
	);
	return ret;
#define reader_init reader_init_on_stack
}

#endif


typedef struct {
	int (*readFile)(const RECOIL *self, const char *filename, uint8_t *content, int contentLength);
} RECOILVtbl;

static int RECOILStdio_ReadFile(const RECOIL *self, const char *filename, uint8_t *content, int contentLength)
{
	int fd;
	
	(void)self;
	fd = Fopen(filename, FO_READ);
	if (fd < 0)
		return -1;
	contentLength = Fread(fd, contentLength, content);
	Fclose(fd);
	return contentLength;
}

RECOIL *RECOILStdio_New(void)
{
	RECOIL *self = RECOIL_New();
	if (self != NULL)
	{
		static const RECOILVtbl vtbl = { RECOILStdio_ReadFile };
		*(const RECOILVtbl **) self = &vtbl;
	}
	return self;
}

boolean RECOILStdio_Load(RECOIL *self, const char *filename)
{
	int fd;
	long contentLength;
	uint8_t *content;
	boolean ok;
	
	fd = Fopen(filename, FO_READ);
	if (fd < 0)
		return FALSE;
	contentLength = Fseek(0, fd, SEEK_END);
	Fseek(0, fd, SEEK_SET);
	content = (uint8_t *) malloc(contentLength);
	if (content == NULL)
	{
		Fclose(fd);
		return FALSE;
	}
	contentLength = Fread(fd, contentLength, content);
	Fclose(fd);
	ok = RECOIL_Decode(self, filename, content, (int) contentLength);
	free(content);
	return ok;
}


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	RECOIL *recoil;
	const char *format;
	
	if (!RECOIL_IsOurFile(name))
		return FALSE;
	
	recoil = RECOIL_New();
	if (recoil == NULL)
		return FALSE;

	if (RECOILStdio_Load(recoil, name) == FALSE)
	{
		RECOIL_Delete(recoil);
		return FALSE;
	}

	info->width = RECOIL_GetWidth(recoil);
	info->height = RECOIL_GetHeight(recoil);
	info->planes = 24;
	info->components = 3;
	info->indexed_color = FALSE;
	info->colors = 1L << 24;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = recoil;
	format = RECOIL_GetPlatform(recoil);
	strncpy(info->info, format, sizeof(info->info) - 1);
	if (strlen(info->info) + 9 < sizeof(info->info))
	{
		strcat(info->info, " (RECOIL)");
	}
	/* Attention: compression field is only 5 chars */
	strcpy(info->compression, "Unkn");
	
	return TRUE;
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	RECOIL *recoil = (RECOIL *) info->_priv_ptr;
	uint8_t *pixels;
	int16_t x;
	uint32_t pos;

	pos = info->_priv_var;
	pixels = (uint8_t *)RECOIL_GetPixels(recoil);
	pixels += pos;
	pos += info->width * 4;
	info->_priv_var = pos;
	
	for (x = info->width; --x >= 0; )
	{
		pixels++;
		*buffer++ = *pixels++;
		*buffer++ = *pixels++;
		*buffer++ = *pixels++;
	}

	return TRUE;
}


void __CDECL reader_quit(IMGINFO info)
{
	RECOIL *recoil = (RECOIL *) info->_priv_ptr;

	if (recoil)
	{
		RECOIL_Delete(recoil);
	}
	info->_priv_ptr = NULL;
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
