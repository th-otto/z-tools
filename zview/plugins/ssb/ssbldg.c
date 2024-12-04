#include "plugin.h"
#include "zvplugin.h"
#include "ldglib/ldg.h"


static void __CDECL init(void)
{
}


static PROC DATAFunc[] = {
	{ "plugin_init", "", init },
	{ "reader_init", "", reader_init },
	{ "reader_read", "", reader_read },
	{ "reader_quit", "", reader_quit },
	{ "reader_get_txt", "", reader_get_txt }
};

static LDGLIB data_plugin = {
	0x202,								/* plugin version */
	5,									/* number of plugin functions */
	DATAFunc,							/* list of functions */
	"SSB\0",							/* file types handled */
	LDG_NOT_SHARED,						/* use this flag, don't know if ldg.prg is installed */
	0,									/* function called when the plugin is unloaded */
	0									/* how many file types are supported by this plugin */
	0									/* Howmany file type are supported by this plugin (0 = double-zero terminated) */
};



int main(void)
{
	ldg_init(&data_plugin);
	return 0;
}
