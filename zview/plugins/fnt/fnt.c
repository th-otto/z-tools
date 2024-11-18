#define	VERSION	     0x108
#define NAME        "GDOS, Degas Font, Warp9 Font"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

/*
DEGAS Font    *.FNT

The font format defines characters 0-127.
Each character is stored as 16 bytes, one byte for each row in the character.
Characters are rendered as 8x16 pixels, thus 16 * 128 = 2048 bytes.

2048 bytes    character data
1 word        half height flag [0 = not allowed, 1 = allowed]
----------
2050 bytes    total for file

Example character:
Byte 1  -> 00000000
Byte 2  -> 00001000
Byte 3  -> 00011100
.          00111110
.          01110111
.          01100011
.          01100011
.          01100011
.          01111111
.          01111111
.          01100011
.          01100011
.          01100011
.          01100011
.          01100011
Byte 16 -> 01100011

Character data:
16 bytes of character 0
16 bytes of character 1
16 bytes of character 2
.
.
16 bytes of character 127

Note: Some files are 2048 bytes because the 'half height flag' is missing.


Warp9 Font    *.FNT

The font format defines characters 0-255.
Each character is stored as 16 bytes, one byte for each row in the character.
Characters are rendered as 8x16 pixels, thus 16 * 256 = 4096 bytes.

4096 bytes    character data
1 word        half height flag [0 = not allowed, 1 = allowed]
----------
4098 bytes    total for file

Example character:
Byte 1  -> 00000000
Byte 2  -> 00001000
Byte 3  -> 00011100
.          00111110
.          01110111
.          01100011
.          01100011
.          01100011
.          01111111
.          01111111
.          01100011
.          01100011
.          01100011
.          01100011
.          01100011
Byte 16 -> 01100011

Character data:
16 bytes of character 0
16 bytes of character 1
16 bytes of character 2
.
.
16 bytes of character 255

Some files are 4096 bytes because the 'half height flag' is missing.


Filenames ending with the extension '.FNT' represent bitmap font files. These
files may be utilized by loading them through any version of GDOS. FNT files
are composed of a file header, font data, a character offset table, and
(optionally) a horizontal offset table.

The FNT Header:

Font files begin with a header 88 BYTEs long. WORD and LONG format entries in
the header must be byte-swapped as they appear in Intel ('Little Endian')
format (see FONT_HDR structure).

The font header is formatted as follows:

BYTE(s)    Contents                                       Related VDI Call
 0 -  1    Face ID (must be unique).                      vqt_name()
 2 -  3    Face size (in points).                         vst_point()
 4 - 35    Face name.                                     vqt_name()
36 - 37    Lowest character index in face (usually 32     vqt_fontinfo()
           for disk-loaded fonts).
38 - 39    Highest character index in face.               vqt_fontinfo()
40 - 41    Top line distance expressed as a positive      vqt_fontinfo()
           offset from baseline.
42 - 43    Ascent line distance expressed as a positive   vqt_fontinfo()
           offset from baseline.
44 - 45    Half line distance expressed as a positive     vqt_fontinfo()
           offset from baseline.
46 - 47    Descent line distance expressed as a positive  vqt_fontinfo()
           offset from baseline.
48 - 49    Bottom line distance expressed as a positive   vqt_fontinfo()
           offset from baseline.
50 - 51    Width of the widest character.                 N/A
52 - 53    Width of the widest character cell.            vqt_fontinfo()
54 - 55    Left offset.                                   vqt_fontinfo()
56 - 57    Right offset.                                  vqt_fontinfo()
58 - 59    Thickening size (in pixels).                   vqt_fontinfo()
60 - 61    Underline size (in pixels).                    vqt_fontinfo()
62 - 63    Lightening mask (used to eliminate pixels,     N/A
           usually 0x5555).
64 - 65    Skewing mask (rotated to determine when to     N/A
           perform additional rotation on a character
           when skewing, usually 0x5555).
66 - 67    Font flags as follows:                         N/A
             Bit  Meaning (if Set)
              0   Contains System Font
              1   Horizontal Offset Tables
                  should be used.
              2   Font data need not be byte-swapped.
              3   Font is mono-spaced.
68 - 71    Offset from start of file to horizontal        vqt_width()
           offset table.
72 - 75    Offset from start of file to character offset  vqt_width()
           table.
76 - 79    Offset from start of file to font data.        N/A
80 - 81    Form width (in bytes).                         N/A
82 - 83    Form height (in scanlines).                    N/A
84 - 87    Pointer to the next font (set by GDOS after    N/A
           loading).

Font Data:

The binary font data is arranged on a single raster form. The raster's height
is the same as the font's height. The raster's width is the sum of the
character width's padded to end on a WORD boundary.

There is no padding between characters. Each character may overlap BYTE
boundaries. Only the last character in a font is padded to make the width of
the form end on an even WORD boundary.

If bit #2 of the font flags header item is cleared, each WORD in the font data
must be byte-swapped.

Character Offset Table:

The Character Offset Table is an array of WORDs which specifies the distance
(in pixels) from the previous character to the next. The first entry is the
distance from the start of the raster form to the left side of the first
character. One succeeding entry follows for each character in the font yielding
(number of characters + 1) entries in the table. Each entry must be
byte-swapped as it appears in Intel ('Little Endian') format.

Horizontal Offset Table:

The Horizontal Offset Table is an optional array of positive or negative WORD
values which when added to the values in the character offset table yield the
true spacing information for each character. One entry appears in the table for
each character. This table is not often used.

*/

#include "plugin.h"
#include "zvplugin.h"

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("FNT\0");

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif


typedef struct FONT_HDR
{
/*  0 */       short font_id;
/*  2 */       short point;
/*  4 */       char name[32];
/* 36 */       unsigned short first_ade;
/* 38 */       unsigned short last_ade;
/* 40 */       unsigned short top;
/* 42 */       unsigned short ascent;
/* 44 */       unsigned short half;
/* 46 */       unsigned short descent;
/* 48 */       unsigned short bottom;
/* 50 */       unsigned short max_char_width;
/* 52 */       unsigned short max_cell_width;
/* 54 */       unsigned short left_offset;          /* amount character slants left when skewed */
/* 56 */       unsigned short right_offset;         /* amount character slants right */
/* 58 */       unsigned short thicken;              /* number of pixels to smear when bolding */
/* 60 */       unsigned short ul_size;              /* height of the underline */
/* 62 */       unsigned short lighten;              /* mask for lightening  */
/* 64 */       unsigned short skew;                 /* mask for skewing */
/* 66 */       unsigned short flags;                /* see below */
/* 68 */       uint32_t hor_table;                  /* horizontal offsets */
/* 72 */       uint32_t off_table;                  /* character offsets  */
/* 76 */       uint32_t dat_table;                  /* character definitions (raster data) */
/* 80 */       unsigned short form_width;           /* width of raster in bytes */
/* 82 */       unsigned short form_height;          /* height of raster in lines */
/* 84 */       uint32_t next_font;                  /* pointer to next font */
/* 88 */
} FONT_HDR;

/* definitions for flags */
#define FONTF_SYSTEM     0x0001            /* Default system font */
#define FONTF_HORTABLE   0x0002            /* Use horizontal offsets table */
#define FONTF_BIGENDIAN  0x0004            /* Font image is in byteswapped format */
#define FONTF_MONOSPACED 0x0008            /* Font is monospaced */
#define FONTF_EXTENDED   0x0020            /* Extended font header */
#define FONTF_COMPRESSED FONTF_EXTENDED
#define FONTF_FULLID     0x2000            /* Use 'full font ID' */


static uint16_t swap16(uint16_t w)
{
	return (w >> 8) | (w << 8);
}


static uint32_t swap32(uint32_t l)
{
	return ((l >> 24) & 0xffL) | ((l << 8) & 0xff0000L) | ((l >> 8) & 0xff00L) | ((l << 24) & 0xff000000UL);
}


/*==================================================================================*
 * boolean __CDECL reader_init:														*
 *		Open the file "name", fit the "info" struct. ( see zview.h) and make others	*
 *		things needed by the decoder.												*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		name		->	The file to open.											*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	size_t file_size;
	int16_t handle;
	uint8_t *bmap;
	FONT_HDR font_header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	bmap = malloc(file_size);
	if (bmap == NULL)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}

	if (file_size == 2048 || file_size == 2050)
	{
		uint8_t *temp;
		int y;
		int x;
		int pos;
		int w;
		int dst;

		temp = malloc(file_size + 2);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		info->width = 256;
		info->height = 64;
		temp[2049] = 0;
		if ((size_t)Fread(handle, file_size, temp) != file_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		dst = 0;
		for (y = 0; y < 4; y++)
		{
			for (x = 0; x < 16; x++)
			{
				pos = y * 512 + x;
				for (w = 0; w < 32; w++)
				{
					bmap[dst++] = temp[pos];
					pos += 16;
				}
			}
		}
		strcpy(info->info, "Degas Font");
		if (temp[2049])
			strcat(info->info, " (Scalable)");
		free(temp);
	} else if (file_size == 4096 || file_size == 4098)
	{
		uint8_t *temp;
		int y;
		int x;
		int pos;
		int w;
		int dst;
		
		temp = malloc(file_size + 2);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		info->width = 256;
		info->height = 128;
		temp[4097] = 0;
		if ((size_t)Fread(handle, file_size, temp) != file_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		dst = 0;
		for (y = 0; y < 8; y++)
		{
			for (x = 0; x < 16; x++)
			{
				pos = y * 512 + x;
				for (w = 0; w < 32; w++)
				{
					bmap[dst++] = temp[pos];
					pos += 16;
				}
			}
		}
		strcpy(info->info, "Warp9 Font");
		if (temp[4097])
			strcat(info->info, " (Scalable)");
		free(temp);
	} else
	{
		char fontname[32 + 1];
		int bigendian = FALSE;

		if (Fread(handle, sizeof(font_header), &font_header) != sizeof(font_header) ||
			font_header.lighten != 0x5555)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		if ((font_header.flags >> 8) != 0)
			font_header.flags = swap16(font_header.flags);
		if (font_header.flags & FONTF_BIGENDIAN)
			bigendian = TRUE;
		if (!bigendian)
		{
			font_header.form_width = swap16(font_header.form_width);
			font_header.form_height = swap16(font_header.form_height);
			font_header.dat_table = swap32(font_header.dat_table);
		}
		info->width = font_header.form_width * 8;
		info->height = font_header.form_height;
		Fseek(font_header.dat_table, handle, SEEK_SET);
		file_size = (size_t)font_header.form_width * font_header.form_height;
		if ((size_t)Fread(handle, file_size, bmap) != file_size)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		memcpy(fontname, font_header.name, 32);
		fontname[32] = '\0';
		strcat(strcat(strcpy(info->info, "GDOS ("), fontname), ")");
	}
	
	Fclose(handle);
	
	info->planes = 1;
	info->indexed_color = FALSE;
	info->components = 1;
	info->colors = 1L << 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->compression, "None");
	
	return TRUE;
}


/*==================================================================================*
 * boolean __CDECL reader_read:														*
 *		This function fits the buffer with image data								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		buffer		->	The destination buffer.										*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	size_t pos = info->_priv_var;
	uint8_t *bmap = info->_priv_ptr;
	int width = info->width;
	uint8_t *end = buffer + width;
	uint8_t byte;
	
	do
	{
		byte = bmap[pos++];
		*buffer++ = (byte >> 7) & 1;
		*buffer++ = (byte >> 6) & 1;
		*buffer++ = (byte >> 5) & 1;
		*buffer++ = (byte >> 4) & 1;
		*buffer++ = (byte >> 3) & 1;
		*buffer++ = (byte >> 2) & 1;
		*buffer++ = (byte >> 1) & 1;
		*buffer++ = (byte >> 0) & 1;
	} while (buffer < end);
	
	info->_priv_var = pos;
	return TRUE;
}


/*==================================================================================*
 * boolean __CDECL reader_quit:														*
 *		This function makes the last job like close the file handler and free the	*
 *		allocated memory.															*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_quit(IMGINFO info)
{
	free(info->_priv_ptr);
	info->_priv_ptr = 0;
}


/*==================================================================================*
 * boolean __CDECL reader_get_txt													*
 *		This function , like other function must be always present.					*
 *		It fills txtdata struct. with the text present in the picture ( if any).	*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		txtdata		->	The destination text buffer.								*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
