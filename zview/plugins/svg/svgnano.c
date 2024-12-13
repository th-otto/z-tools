/* must include math.h here before atof etc. are redefined */
#include <math.h>

#include "plugin.h"
#include "zvplugin.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"

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

#endif

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
	size_t file_size;
	int fd;
	
	fd = (int)Fopen(name, FO_READ);
	if (fd < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
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
	data[file_size] = '\0';					/* Must be null terminated. */
	Fclose(fd);

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
