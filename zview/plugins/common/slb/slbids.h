/*
 * 3rd-party libraries the application knows about.
 * Codecs that need some of these libraries should
 * arrange to call back to the application in order
 * to load them, via p_slb_open/p_slb_close/p_slb_get
 * defined in the zview_plugin_funcs struct
 */
#define LIB_PNG      0
#define LIB_Z        1
#define LIB_JPEG     2
#define LIB_TIFF     3
#define LIB_LZMA     4
#define LIB_EXIF     5
#define LIB_BZIP2    6
#define LIB_FREETYPE 7
#define LIB_WEBP     8
#define LIB_ZSTD     9
#define LIB_ICONV   10

/*
 * callback functions used to load other shared libs like zlib etc.
 */
extern SLB *__CDECL (*p_slb_get)(long lib);
extern long __CDECL (*p_slb_open)(long lib, const char *slbpath);
extern void __CDECL (*p_slb_close)(long lib);
