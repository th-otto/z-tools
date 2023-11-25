/*
 * resource set indices for zview
 *
 * created by ORCS 2.18
 */

/*
 * Number of Strings:        369
 * Number of Bitblks:        2
 * Number of Iconblks:       0
 * Number of Color Iconblks: 9
 * Number of Color Icons:    15
 * Number of Tedinfos:       53
 * Number of Free Strings:   47
 * Number of Free Images:    0
 * Number of Objects:        257
 * Number of Trees:          14
 * Number of Userblks:       0
 * Number of Images:         2
 * Total file size:          19690
 */

#ifdef RSC_NAME
#undef RSC_NAME
#endif
#ifndef __ALCYON__
#define RSC_NAME "zview"
#endif
#ifdef RSC_ID
#undef RSC_ID
#endif
#ifdef zview
#define RSC_ID zview
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 369
#define NUM_FRSTR 47
#define NUM_UD 0
#define NUM_IMAGES 2
#define NUM_BB 2
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 9
#define NUM_TI 53
#define NUM_OBS 257
#define NUM_TREE 14
#endif



#define MENU_BAR                           0 /* menu */
#define MENU_BAR_FILE                      4 /* TITLE in tree MENU_BAR */
#define MENU_BAR_EDIT                      5 /* TITLE in tree MENU_BAR */
#define MENU_BAR_VIEW                      6 /* TITLE in tree MENU_BAR */
#define MENU_BAR_HELP                      7 /* TITLE in tree MENU_BAR */
#define MENU_BAR_ABOUT                    10 /* STRING in tree MENU_BAR */
#define MENU_BAR_BROWSER                  19 /* STRING in tree MENU_BAR */
#define MENU_BAR_OPEN                     20 /* STRING in tree MENU_BAR */
#define MENU_BAR_INFORMATION              22 /* STRING in tree MENU_BAR */
#define MENU_BAR_SAVE                     24 /* STRING in tree MENU_BAR */
#define MENU_BAR_PRINT                    26 /* STRING in tree MENU_BAR */
#define MENU_BAR_CLOSE                    28 /* STRING in tree MENU_BAR */
#define MENU_BAR_QUIT                     30 /* STRING in tree MENU_BAR */
#define MENU_BAR_DELETE                   32 /* STRING in tree MENU_BAR */
#define MENU_BAR_SELECT_ALL               34 /* STRING in tree MENU_BAR */
#define MENU_BAR_PREFERENCE               36 /* STRING in tree MENU_BAR */
#define MENU_BAR_BY_NAME                  38 /* STRING in tree MENU_BAR */
#define MENU_BAR_BY_SIZE                  39 /* STRING in tree MENU_BAR */
#define MENU_BAR_BY_DATE                  40 /* STRING in tree MENU_BAR */
#define MENU_BAR_SHOW_BROWSER             42 /* STRING in tree MENU_BAR */
#define MENU_BAR_SHOW_ONLY_IMAGES         44 /* STRING in tree MENU_BAR */
#define MENU_BAR_LARGE_THUMB              46 /* STRING in tree MENU_BAR */
#define MENU_BAR_LONG_THUMB               47 /* STRING in tree MENU_BAR */
#define MENU_BAR_SHOW_FULLSCREEN          49 /* STRING in tree MENU_BAR */
#define MENU_BAR_GUIDE                    51 /* STRING in tree MENU_BAR */
#define MENU_BAR_HISTORY                  52 /* STRING in tree MENU_BAR */

#define TOOLBAR                            1 /* form/dialog */
#define TOOLBAR_BACK                       0 /* BOX in tree TOOLBAR */
#define TOOLBAR_UP                         1 /* BUTTON in tree TOOLBAR */
#define TOOLBAR_RELOAD                     2 /* BUTTON in tree TOOLBAR */
#define TOOLBAR_BIG                        3 /* BUTTON in tree TOOLBAR */
#define TOOLBAR_PRINT                      4 /* BUTTON in tree TOOLBAR */
#define TOOLBAR_INFO                       5 /* BUTTON in tree TOOLBAR */
#define TOOLBAR_SMALL                      6 /* BUTTON in tree TOOLBAR */
#define TOOLBAR_DELETE                     7 /* BUTTON in tree TOOLBAR */
#define TOOLBAR_OPEN                       8 /* BUTTON in tree TOOLBAR */
#define TOOLBAR_SAVE                       9 /* BUTTON in tree TOOLBAR */

#define SINGLE_PROGRESS                    2 /* form/dialog */
#define SINGLE_PROGRESS_CONTAINER          1 /* BOX in tree SINGLE_PROGRESS */
#define SINGLE_PROGRESS_TXT                2 /* TEXT in tree SINGLE_PROGRESS */

#define MINI_ICON                          3 /* form/dialog */
#define MINI_ICON_SNIPE                    1 /* CICON in tree MINI_ICON */

#define OPERATION_DIAL                     4 /* form/dialog */
#define OPERATION_DIAL_BAR                 1 /* BOX in tree OPERATION_DIAL */
#define OPERATION_DIAL_OK                  2 /* BUTTON in tree OPERATION_DIAL */
#define OPERATION_DIAL_CANCEL              3 /* BUTTON in tree OPERATION_DIAL */
#define OPERATION_DIAL_INFO                4 /* TEXT in tree OPERATION_DIAL */
#define OPERATION_DIAL_PROGRESS            5 /* TEXT in tree OPERATION_DIAL */

#define SLIDERS                            5 /* form/dialog */
#define SLIDERS_BOX                        0 /* BOX in tree SLIDERS */
#define SLIDERS_UP                         1 /* CICON in tree SLIDERS */
#define SLIDERS_DOWN                       2 /* CICON in tree SLIDERS */
#define SLIDERS_BACK                       3 /* BOX in tree SLIDERS */
#define SLIDERS_MOVER                      4 /* BOX in tree SLIDERS */

#define ABOUT                              6 /* form/dialog */
#define ABOUT_S                            0 /* BOX in tree ABOUT */
#define ABOUT_VERSION                      1 /* BUTTON in tree ABOUT */
#define ABOUT_DATE                         3 /* TEXT in tree ABOUT */

#define SAVE_DIAL                          7 /* form/dialog */
#define SAVE_DIAL_SAVE                     1 /* BUTTON in tree SAVE_DIAL */
#define SAVE_DIAL_ABORT                    2 /* BUTTON in tree SAVE_DIAL */
#define SAVE_DIAL_FORMAT                   6 /* BUTTON in tree SAVE_DIAL */
#define SAVE_DIAL_OPTIONS                  7 /* BUTTON in tree SAVE_DIAL */

#define FILE_INFO                          8 /* form/dialog */
#define FILE_INFO_PANEL3                   1 /* BUTTON in tree FILE_INFO */
#define FILE_INFO_COMMENT                  2 /* BOX in tree FILE_INFO */
#define FILE_INFO_BOX                      4 /* BOX in tree FILE_INFO */
#define FILE_INFO_UP                       5 /* CICON in tree FILE_INFO */
#define FILE_INFO_DOWN                     6 /* CICON in tree FILE_INFO */
#define FILE_INFO_VBACK                    7 /* BOX in tree FILE_INFO */
#define FILE_INFO_VMOVER                   8 /* BOX in tree FILE_INFO */
#define FILE_INFO_PANEL2                   9 /* BUTTON in tree FILE_INFO */
#define FILE_INFO_INFO                    20 /* TEXT in tree FILE_INFO */
#define FILE_INFO_WIDTH                   21 /* TEXT in tree FILE_INFO */
#define FILE_INFO_HEIGHT                  22 /* TEXT in tree FILE_INFO */
#define FILE_INFO_PLANES                  23 /* TEXT in tree FILE_INFO */
#define FILE_INFO_COMPRESSION             24 /* TEXT in tree FILE_INFO */
#define FILE_INFO_COLOR                   25 /* TEXT in tree FILE_INFO */
#define FILE_INFO_RATIO                   26 /* TEXT in tree FILE_INFO */
#define FILE_INFO_DEC_TIME                27 /* TEXT in tree FILE_INFO */
#define FILE_INFO_MEM                     28 /* TEXT in tree FILE_INFO */
#define FILE_INFO_PAGE                    29 /* TEXT in tree FILE_INFO */
#define FILE_INFO_PANEL1                  30 /* BUTTON in tree FILE_INFO */
#define FILE_INFO_TIME                    31 /* TEXT in tree FILE_INFO */
#define FILE_INFO_DATE                    32 /* TEXT in tree FILE_INFO */
#define FILE_INFO_SIZE                    33 /* TEXT in tree FILE_INFO */
#define FILE_INFO_NAME                    34 /* TEXT in tree FILE_INFO */
#define FILE_INFO_FILE                    39 /* BUTTON in tree FILE_INFO */
#define FILE_INFO_IMAGE                   40 /* BUTTON in tree FILE_INFO */
#define FILE_INFO_EXIF                    41 /* BUTTON in tree FILE_INFO */
#define FILE_INFO_CODEC                   42 /* BUTTON in tree FILE_INFO */
#define FILE_INFO_PANEL4                  43 /* BUTTON in tree FILE_INFO */
#define FILE_CODEC_FILENAME               44 /* TEXT in tree FILE_INFO */
#define FILE_CODEC_NAME_FIRST             46 /* TEXT in tree FILE_INFO */
#define FILE_CODEC_NAME_LAST              47 /* TEXT in tree FILE_INFO */
#define FILE_CODEC_EXTENSIONS             48 /* TEXT in tree FILE_INFO */
#define FILE_CODEC_VERSION                50 /* TEXT in tree FILE_INFO */
#define FILE_CODEC_DATE                   52 /* TEXT in tree FILE_INFO */
#define FILE_CODEC_AUTHOR                 54 /* TEXT in tree FILE_INFO */
#define FILE_CODEC_INFO_FIRST             56 /* TEXT in tree FILE_INFO */
#define FILE_CODEC_INFO_LAST              62 /* TEXT in tree FILE_INFO */
#define FILE_CODEC_COMPILER               63 /* TEXT in tree FILE_INFO */

#define HSLIDERS                           9 /* form/dialog */
#define HSLIDERS_LEFT                      2 /* CICON in tree HSLIDERS */
#define HSLIDERS_RIGHT                     3 /* CICON in tree HSLIDERS */
#define HSLIDERS_HBACK                     4 /* BOX in tree HSLIDERS */
#define HSLIDERS_HMOVER                    5 /* BOX in tree HSLIDERS */

#define SAVEPREF                          10 /* form/dialog */
#define PREF_PERCENT                       2 /* BOXTEXT in tree SAVEPREF */
#define PREF_BG                            4 /* IBOX in tree SAVEPREF */
#define PREF_LF                            5 /* BOXCHAR in tree SAVEPREF */
#define PREF_RT                            6 /* BOXCHAR in tree SAVEPREF */
#define PREF_PG                            7 /* BOX in tree SAVEPREF */
#define PREF_SL                            8 /* BOX in tree SAVEPREF */
#define PREF_COLORSPACE                   12 /* BUTTON in tree SAVEPREF */
#define PREF_PROGRESSIVE                  13 /* BUTTON in tree SAVEPREF */
#define PREF_COMPRESSION_BOX              14 /* BUTTON in tree SAVEPREF */
#define TIFFPREF_NONE                     15 /* BUTTON in tree SAVEPREF */
#define TIFFPREF_RLE                      16 /* BUTTON in tree SAVEPREF */
#define TIFFPREF_LZW                      17 /* BUTTON in tree SAVEPREF */
#define TIFFPREF_JPEG                     18 /* BUTTON in tree SAVEPREF */
#define TIFFPREF_DEFLATE                  19 /* BUTTON in tree SAVEPREF */
#define PREF_OK                           20 /* BUTTON in tree SAVEPREF */

#define VIEWTOOLBAR                       11 /* form/dialog */
#define VIEWTOOLBAR_BACK                   0 /* BOX in tree VIEWTOOLBAR */
#define VIEWTOOLBAR_BIG                    1 /* BUTTON in tree VIEWTOOLBAR */
#define VIEWTOOLBAR_SMALL                  2 /* BUTTON in tree VIEWTOOLBAR */
#define VIEWTOOLBAR_OPEN                   3 /* BUTTON in tree VIEWTOOLBAR */
#define VIEWTOOLBAR_SAVE                   4 /* BUTTON in tree VIEWTOOLBAR */
#define VIEWTOOLBAR_INFO                   5 /* BUTTON in tree VIEWTOOLBAR */
#define VIEWTOOLBAR_FULLSCREEN             6 /* BUTTON in tree VIEWTOOLBAR */
#define VIEWTOOLBAR_PRINT                  7 /* BUTTON in tree VIEWTOOLBAR */

#define PDFTOOLBAR                        12 /* form/dialog */
#define PDFTOOLBAR_BACK                    0 /* BOX in tree PDFTOOLBAR */
#define PDFTOOLBAR_BIG                     1 /* BUTTON in tree PDFTOOLBAR */
#define PDFTOOLBAR_SMALL                   2 /* BUTTON in tree PDFTOOLBAR */
#define PDFTOOLBAR_PREVIOUS                3 /* BUTTON in tree PDFTOOLBAR */
#define PDFTOOLBAR_NEXT                    4 /* BUTTON in tree PDFTOOLBAR */
#define PDFTOOLBAR_OPEN                    5 /* BUTTON in tree PDFTOOLBAR */
#define PDFTOOLBAR_PRINT                   6 /* BUTTON in tree PDFTOOLBAR */
#define PDFTOOLBAR_FIND                    7 /* BUTTON in tree PDFTOOLBAR */
#define PDFTOOLBAR_PAGE                    8 /* BOXTEXT in tree PDFTOOLBAR */
#define PDFTOOLBAR_INFO                    9 /* BUTTON in tree PDFTOOLBAR */
#define PDFTOOLBAR_PERCENT                10 /* BOXTEXT in tree PDFTOOLBAR */

#define PREFS                             13 /* form/dialog */
#define PREFS_PANEL3                       1 /* BUTTON in tree PREFS */
#define PREFS_PDF_ANTIALIAS                2 /* BUTTON in tree PREFS */
#define PREFS_PDF_FIT_TO_WINDOW            3 /* BUTTON in tree PREFS */
#define PREFS_PANEL2                       4 /* BUTTON in tree PREFS */
#define PREFS_SHOW_SIZE                    5 /* BUTTON in tree PREFS */
#define PREFS_SHOW_HIDDEN                  6 /* BUTTON in tree PREFS */
#define PREFS_SHOW_UNSUPPORTED_FILES       7 /* BUTTON in tree PREFS */
#define PREFS_SMOOTH_METHOD                9 /* BUTTON in tree PREFS */
#define PREFS_A                           12 /* BUTTON in tree PREFS */
#define PREFS_B                           13 /* BUTTON in tree PREFS */
#define PREFS_C                           14 /* BUTTON in tree PREFS */
#define PREFS_D                           15 /* BUTTON in tree PREFS */
#define PREFS_E                           16 /* BUTTON in tree PREFS */
#define PREFS_F                           17 /* BUTTON in tree PREFS */
#define PREFS_G                           18 /* BUTTON in tree PREFS */
#define PREFS_H                           19 /* BUTTON in tree PREFS */
#define PREFS_I                           20 /* BUTTON in tree PREFS */
#define PREFS_J                           21 /* BUTTON in tree PREFS */
#define PREFS_K                           22 /* BUTTON in tree PREFS */
#define PREFS_L                           23 /* BUTTON in tree PREFS */
#define PREFS_M                           24 /* BUTTON in tree PREFS */
#define PREFS_N                           25 /* BUTTON in tree PREFS */
#define PREFS_O                           26 /* BUTTON in tree PREFS */
#define PREFS_P                           27 /* BUTTON in tree PREFS */
#define PREFS_Q                           28 /* BUTTON in tree PREFS */
#define PREFS_R                           29 /* BUTTON in tree PREFS */
#define PREFS_S                           30 /* BUTTON in tree PREFS */
#define PREFS_T                           31 /* BUTTON in tree PREFS */
#define PREFS_U                           32 /* BUTTON in tree PREFS */
#define PREFS_V                           33 /* BUTTON in tree PREFS */
#define PREFS_W                           34 /* BUTTON in tree PREFS */
#define PREFS_X                           35 /* BUTTON in tree PREFS */
#define PREFS_Y                           36 /* BUTTON in tree PREFS */
#define PREFS_Z                           37 /* BUTTON in tree PREFS */
#define PREFS_PANEL1                      39 /* BUTTON in tree PREFS */
#define PREFS_SHOW_READ_PROGRESS          40 /* BUTTON in tree PREFS */
#define PREFS_SHOW_WRITE_PROGRESS         41 /* BUTTON in tree PREFS */
#define PREFS_GENERAL                     42 /* BUTTON in tree PREFS */
#define PREFS_BROWSER                     43 /* BUTTON in tree PREFS */
#define PREFS_PDF                         44 /* BUTTON in tree PREFS */
#define PREFS_OK                          45 /* BUTTON in tree PREFS */
#define PREFS_CANCEL                      46 /* BUTTON in tree PREFS */

#define NOMEM                              0 /* Alert string */

#define ALERT_WINDOW                       1 /* Alert string */

#define GENERIC                            2 /* Alert string */

#define ASK_READONLY                       3 /* Alert string */

#define NOENT                              4 /* Alert string */

#define NAMETOOLONG                        5 /* Alert string */

#define ACCES                              6 /* Alert string */

#define LOOP                               7 /* Alert string */

#define NOTDIR                             8 /* Alert string */

#define ISDIR                              9 /* Alert string */

#define XDEV                              10 /* Alert string */

#define NOSPC                             11 /* Alert string */

#define IO                                12 /* Alert string */

#define ROFS                              13 /* Alert string */

#define FAULT                             14 /* Alert string */

#define INVAL                             15 /* Alert string */

#define BADF                              16 /* Alert string */

#define NAMEALREADYUSED                   17 /* Alert string */

#define LIB_FULLED                        18 /* Alert string */

#define APP_FULLED                        19 /* Alert string */

#define ERR_EXEC                          20 /* Alert string */

#define BAD_LFORMAT                       21 /* Alert string */

#define LIB_LOCKED                        22 /* Alert string */

#define NOT_FOUND                         23 /* Alert string */

#define NO_MEMORY                         24 /* Alert string */

#define NOFUNC                            25 /* Alert string */

#define PLUGIN_DECODER_ABSENT             26 /* Alert string */

#define PLUGIN_ENCODER_ABSENT             27 /* Alert string */

#define CANT_LOAD_IMG                     28 /* Alert string */

#define CANT_SAVE_IMG                     29 /* Alert string */

#define IMG_NO_VALID                      30 /* Alert string */

#define SOURCE_TARGET_SAME                31 /* Alert string */

#define SAVE_TITLE                        32 /* Free string */

#define LOAD_TITLE                        33 /* Free string */

#define DELETE_TITLE                      34 /* Free string */

#define DELETE_INFO                       35 /* Free string */

#define CALCUL                            36 /* Free string */

#define PROGRESS_TXT                      37 /* Free string */

#define ABOUT_TITLE                       38 /* Free string */

#define DELETE_ASK                        39 /* Free string */

#define SAVE_OPTION_TITLE                 40 /* Free string */

#define NOZCODECS                         41 /* Alert string */

#define NOICONS                           42 /* Alert string */

#define NO_EDDI                           43 /* Alert string */

#define PLEASE_WAIT                       44 /* Free string */

#define AL_DUPLICATE                      45 /* Alert string */

#define PLUGIN_MISMATCH                   46 /* Alert string */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD zview_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD zview_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD zview_rsc_free(void);
#endif
