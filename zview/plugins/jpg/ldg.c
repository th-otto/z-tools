#include <stdlib.h>
#include <stdio.h>
#define boolean jpg_boolean
#include <jpeglib.h>
#undef EXTERN
#undef LOCAL
#undef GLOBAL
#undef boolean
#define boolean zv_boolean
#include "plugin.h"
#include "zvplugin.h"
#include <gem.h> /* for MFDB */
#undef EXTERN
#undef LOCAL
#undef GLOBAL
#include "jpgdh.h"
#ifdef __PUREC__
#include <libexif/exifdata.h>
#include <libexif/exifutil.h>
#else
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#endif
#include "zvjpg.h"
#include "ldglib/ldg.h"
#undef VERSION
#undef NAME
#include "exports.h"

/*==================================================================================*
 * boolean set_jpg_option:															*
 *		This function set some encoder's options							 		*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
static void __CDECL set_jpg_option(int32_t set_quality, int32_t set_color_space, int32_t set_progressive)
{
	quality = (int) set_quality;
	color_space = (J_COLOR_SPACE)set_color_space;
	progressive = (boolean)set_progressive;
}


/*==================================================================================*
 * boolean CDECL init:																*
 *		First function called from zview, in this one, you can make some internal	*
 *		initialisation.																*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		--																			*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
static void __CDECL init( void)
{
    jpg_init();
}


#ifndef MISC_INFO
#define MISC_INFO ""
#endif

static PROC LibFunc[] = 
{ 
	{ "plugin_init", "Codec: " NAME, init },
	{ "reader_init", "Author: " AUTHOR, reader_init },
	{ "reader_read", "Date: " __DATE__, reader_read },
	{ "reader_quit", "Time: " __TIME__, reader_quit },
	{ "reader_get_txt", MISC_INFO, reader_get_txt },
	{ "encoder_init", 	"", encoder_init },
	{ "encoder_write",	"", encoder_write },
	{ "encoder_quit", 	"", encoder_quit },
	{ "set_jpg_option",	"", set_jpg_option }
};


static LDGLIB plugin =
{
	VERSION, 			/* Plugin version */
	sizeof(LibFunc) / sizeof(LibFunc[0]),					/* Number of plugin's functions */
	LibFunc,			/* List of functions */
	EXTENSIONS,		/* File's type Handled */
	LDG_NOT_SHARED, 	/* The flags NOT_SHARED is used here.. */
	0,
	0					/* Howmany file type are supported by this plugin (0 = double-zero terminated) */
};

/*==================================================================================*
 * int main:																		*
 *		Main function, his job is to call ldg_init function.						*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		--																			*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      0																			*
 *==================================================================================*/
int main( void)
{
	ldg_init( &plugin);
	return( 0);
}
