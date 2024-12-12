#include "plugin.h"
#include "zvplugin.h"
#include "ldglib/ldg.h"

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


static PROC POFOFunc[] = 
{
	{ "plugin_init", 	"", init },
	{ "reader_init", 	"", reader_init },
	{ "reader_get_txt", "", reader_get_txt },
	{ "reader_read", 	"", reader_read },
	{ "reader_quit", 	"", reader_quit },
};


static LDGLIB pofo_plugin =
{
	0x201, 	/* Plugin version */
	sizeof(POFOFunc) / sizeof(POFOFunc[0]),					/* Number of plugin's functions */
	POFOFunc,			/* List of functions */
	"PGFPGCPGX\0",			/* File's type Handled */
	LDG_NOT_SHARED, 	/* The flags NOT_SHARED is used here.. even if zview plugins are reentrant 
					   	   and are shareable, we must use this flags because we don't know if the 
					   	   user has ldg.prg deamon installed on his computer */
	0,					/* Function called when the plugin is unloaded */
	3					/* Howmany file type are supported by this plugin */
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
	ldg_init(&pofo_plugin);
	return 0;
}
