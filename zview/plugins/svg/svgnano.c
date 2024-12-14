/* must include math.h here before atof etc. are redefined */
#include <math.h>

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

/* nanosvg taken from
   https://github.com/memononen/nanosvg/commit/93ce879dc4c04a3ef1758428ec80083c38610b1f
   2024/12/12
*/
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "nanosvg.h"
#include "nanorast.h"

#ifdef PLUGIN_SLB

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

/* some functions are not yet exported to SLB */
#include "libcmini.c"

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
	NSVGrasterizer *svg_raster;
	NSVGimage *svg_image;
	char *data;
	size_t image_size;
	int16_t fd;
	uint8_t magic[2];
	uint32_t file_size;
	
#ifdef ZLIB_SLB
	if (init_zlib_slb() < 0)
	{
		RETURN_ERROR(EC_InternalError);
	}
#endif
	
	fd = (int16_t)Fopen(name, FO_READ);
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

	svg_image = nsvgParse(data, "px", 96);
	if (svg_image == NULL)
	{
		free(data);
		RETURN_ERROR(EC_Malloc);
	}
	free(data);

	info->width = svg_image->width;
	info->height = svg_image->height;
	if (info->width == 0 || info->height == 0)
	{
		nsvgDelete(svg_image);
		RETURN_ERROR(EC_FileType);
	}
	image_size = (size_t)info->width * info->height * 4;
	bmap = (void *)malloc(image_size);
	if (bmap == NULL)
	{
		nsvgDelete(svg_image);
		RETURN_ERROR(EC_Malloc);
	}
	svg_raster = nsvgCreateRasterizer();
	if (svg_raster == NULL)
	{
		free(bmap);
		nsvgDelete(svg_image);
		RETURN_ERROR(EC_Malloc);
	}

	nsvgRasterize(svg_raster, svg_image, 0, 0, 1, bmap, info->width, info->height, (size_t)info->width * 4);
	nsvgDeleteRasterizer(svg_raster);
	nsvgDelete(svg_image);

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
	
	/* RGBA */
	do
	{
		*buffer++ = *bmap++;
		*buffer++ = *bmap++;
		*buffer++ = *bmap++;
		bmap++;
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
