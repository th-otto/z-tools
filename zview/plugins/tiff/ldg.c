#include <tiffio.h>
#define HAVE_INTS_DEFINED
#include "plugin.h"
#include "zvplugin.h"
#include "ldglib/ldg.h"
#include "zvtiff.h"
#include "exports.h"

/*==================================================================================*
 * boolean set_tiff_option:															*
 *		This function set some encoder's options							 		*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
static void __CDECL set_tiff_option( int16_t set_quality, uint16_t set_encode_compression)
{
	encode_compression 	= set_encode_compression;
	quality = set_quality;
}


/*==================================================================================*
 * boolean __CDECL init:															*
 *		First function called from zview, in this one, you can make some internal	*
 *		initialisation.																*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		--																			*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
static void __CDECL init(void)
{
}


#ifndef MISC_INFO
#define MISC_INFO ""
#endif

static PROC Func[] =
{
	{ "plugin_init", "Codec: " NAME, init },
	{ "reader_init", "Author: " AUTHOR, reader_init },
	{ "reader_read", "Date: " __DATE__, reader_read },
	{ "reader_quit", "Time: " __TIME__, reader_quit },
	{ "reader_get_txt", MISC_INFO, reader_get_txt },
	{ "encoder_init", 	"", encoder_init},
	{ "encoder_write",	"", encoder_write},
	{ "encoder_quit", 	"", encoder_quit},
	{ "set_tiff_option","", set_tiff_option}
};


static LDGLIB plugin =
{
	VERSION, 	/* Plugin version */
	sizeof(Func) / sizeof(Func[0]),					/* Number of plugin's functions */
	Func,				/* List of functions */
	EXTENSIONS,			/* File types handled */
	LDG_NOT_SHARED, 	/* The flags NOT_SHARED is used here.. even if zview plugins are reentrant 
					   	   and are shareable, we must use this flags because we don't know if the 
					   	   user has ldg.prg deamon installed on his computer */
	0,					/* Function called when the plugin is unloaded */
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
int main(void)
{
	ldg_init(&plugin);
	return 0;
}
