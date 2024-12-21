/*
 * exports.h - internal header with definitions of all exported functions
 *
 * Copyright (C) 2024 Thorsten Otto
 *
 * For conditions of distribution and use, see copyright file.
 */

#define	VERSION	    0x207
#define NAME        "EggPaint/Spooky Sprites"
#define EXTENSIONS  "TRP\0"
#define DATE        __DATE__ " " __TIME__
#define AUTHOR      "Lonny Pursell"
#define MISC_INFO   "zView module by Thorsten Otto"

#define SHAREDLIB "zveggpnt.slb"

#ifndef LIBFUNC
#define LIBFUNC(_fn, name, _nargs)
#endif
#ifndef LIBFUNC2
#define LIBFUNC2(_fn, name, _nargs) LIBFUNC(_fn, name, _nargs)
#endif
#ifndef NOFUNC
#define NOFUNC
#endif

/*   0 */ LIBFUNC(0, slb_control, 2)
/*   1 */ LIBFUNC(1, reader_init, 2)
/*   2 */ LIBFUNC(2, reader_read, 2)
/*   3 */ LIBFUNC(3, reader_get_txt, 2)
/*   4 */ LIBFUNC(4, reader_quit, 1)
/*   5 */ LIBFUNC(5, encoder_init, 2)
/*   6 */ LIBFUNC(6, encoder_write, 2)
/*   7 */ LIBFUNC(7, encoder_quit, 1)
/*   8 */ LIBFUNC(8, get_option, 1)
/*   9 */ NOFUNC

#undef LIBFUNC
#undef LIBFUNC2
#undef NOFUNC
