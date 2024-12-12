#include "plugin.h"
#include "zvplugin.h"
#include "ldglib/ldg.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"


static void __CDECL init(void)
{
}


static PROC DATAFunc[] = {
	{ "plugin_init", "Codec: " NAME, init },
	{ "reader_init", "Author: " AUTHOR, reader_init },
	{ "reader_read", "Date: " __DATE__, reader_read },
	{ "reader_quit", "Time: " __TIME__, reader_quit },
	{ "reader_get_txt", "", reader_get_txt }
};

static LDGLIB data_plugin = {
	VERSION,							/* plugin version */
	5,									/* number of plugin functions */
	DATAFunc,							/* list of functions */
	EXTENSIONS,							/* file types handled */
	LDG_NOT_SHARED,						/* use this flag, don't know if ldg.prg is installed */
	0,									/* function called when the plugin is unloaded */
	0									/* Howmany file type are supported by this plugin (0 = double-zero terminated) */
};



int main(void)
{
	ldg_init(&data_plugin);
	return 0;
}
