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


static PROC Func[] = 
{
	{ "plugin_init", 	"", init },
	{ "reader_init", 	"", reader_init },
	{ "reader_get_txt", "", reader_get_txt },
	{ "reader_read", 	"", reader_read },
	{ "reader_quit", 	"", reader_quit },
};


static LDGLIB plugin =
{
	0x102, 	/* Plugin version */
	sizeof(Func) / sizeof(Func[0]),					/* Number of plugin's functions */
	Func,				/* List of functions */
	"CEL\0",			/* File's type Handled */
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
