#include "plugin.h"
#include "zvplugin.h"
#include "ldglib/ldg.h"
#include "exports.h"

extern char const misc_info[];

static void __CDECL init(void)
{
}


static PROC Func[] = {
	{ "plugin_init", "Codec: " NAME, init },
	{ "reader_init", "Author: " AUTHOR, reader_init },
	{ "reader_read", "Date: " __DATE__, reader_read },
	{ "reader_quit", "Time: " __TIME__, reader_quit },
#ifdef MISC_INFO
	{ "reader_get_txt", MISC_INFO, reader_get_txt }
#else
	{ "reader_get_txt", "", reader_get_txt }
#endif
};

static LDGLIB plugin = {
	VERSION,							/* plugin version */
	sizeof(Func) / sizeof(Func[0]),		/* Number of plugin's functions */
	Func,								/* list of functions */
	EXTENSIONS,							/* file types handled */
	LDG_NOT_SHARED,						/* use this flag, don't know if ldg.prg is installed */
	0,									/* function called when the plugin is unloaded */
	0									/* Howmany file type are supported by this plugin (0 = double-zero terminated) */
};


int main(void)
{
	ldg_init(&plugin);
	return 0;
}
