#include "plugin.h"
#include "zvplugin.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"
#include "fastlz.h"

/*
  ================================
  = The SGX Graphics File Format =  - Version 4.2, 06.04.2009
  ================================

  File Format Information
  ~~~~~~~~~~~~~~~~~~~~~~~
  Pictures in the "SGX Graphics File Format" consist of two parts:
  a header and an attached packed or unpacked data section.

  Both sections are sequentially arranged within the same file.

  Construction (all fields in Motorola BYTE order):

  00  ID              UBYTE[18]       "SVG Graphics File" + 0-Byte
                                     OR "SGX Graphics File" + 0-Byte
  18  Version         UWORD           currently 1
  20  GfxDataOffset   ULONG           header length (depends on version)
  24  LeftEdge        ULONG           as with e.g. ILBM
  28  TopEdge         ULONG           ...
  32  Width           ULONG           ...
  36  Height          ULONG           ...
  40  ColorDepth      ULONG           used colors as x of 2^x
  44  ViewMode32      ULONG           32 Bit Amiga AGA 8 Bit ViewMode
  48  PixelBits       UBYTE           1, 8, 24, 32, 48, 64
  49  PixelPlanes     UBYTE           # of planes with PixelBits
  50  BytesPerLine    ULONG           bpl of a PixelPlane
  54  ColorMap        UBYTE [256][3]  unused, if > 256 Colors (zero-ed)

  Note: On specific platforms certain alignment restrictions may
        prevent you from putting this into exactly one C structure.

  For SVG:
  --------
  After that either follows XPK compressed data or uncompressed
  data, which can be detected by the leading chars "XPK" or "PP20"
  for packed data at GfxDataOffset (relative to beginning of the file).
  Structure of packed data is as follows:

  0x00  ID              UBYTE[3]/[4]    XPK / PP20
  0x04  Data            UBYTE[]         compressed data (upto EOF)

  => Note, that XPK compression is deprecated. Please use SGX/LZ77 instead.

  For SGX:
  --------
  After that either follows ZIP compressed data or uncompressed
  data, which can be detected by the leading chars "LZ77" for
  packed data at GfxDataOffset (relative to beginning of the file).
  Structure of packed data is as follows:

  0x00  ID              UBYTE[4]        LZ77
  0x04  Length          ULONG           size of uncompressed data
  0x08  Data            UBYTE[]         compressed data (upto EOF)

  Please note, that with upto 256 colors (ColorDepth <= 8) it has
  to be checked, whether the graphics actually is EHB / HAM.
  Use the Viewmode32 field for checking this when reading,
  and eventually binary-OR the field with HAM_KEY / EHB_KEY when saving.

  Note:
  -----
  "PixelBits" and "PixelPlanes" do allow for a lot of combinations, obviously.
  However not all of them are legal.

  Actually used and supported by current software are only the
  following:

     Bits   Planes  Depth  Content
     ---------------------------------------------------------------------
     1      1..8    1..8   (unaligned Bitmaps with 2..256 colors)
     8      1       1..8   (chunky Bitmaps with 2..256 colors)
     24     1       24     (24 Bit RGB Bitmaps with 8:8:8 RGB)
     32     1       32     (32 Bit RGBA Bitmaps with 8:8:8:8 RGBA)
     48     1       48     (48 Bit RGB Bitmaps with 16:16:16 RGB)
     64     1       64     (64 Bit RGBA Bitmaps with 16:16:16:16 RGBA)

  So >24 Bit Data should not be saved planewise, but as 24/32/48/64 Bit RGB(A).

  If you ever should save any other data, please avoid any planar
  configurations and respect the following rules for RGB data chunks:

     Bits   Planes  Depth  Content
     ---------------------------------------------------------------------
     16     1       15/16  (15/16 Bit Bitmap with 5:5:5:1 RGB0/A)
     32     1       24/32  (24/32 Bit RGB Bitmaps with 8:8:8:8 RGB0/A)
     48     1       48     (48    Bit RGB Bitmaps with 16:16:16 RGB)
     64     1       48/64  (48/64 Bit RGB Bitmaps with 16:16:16:16 RGB0/A)
     ... etc ...

 Note, that an alpha channel can only be correctly identified, when
 "ColorDepth" is handled as an indicator, whether there actually is one,
 or not. Programs not supporting alpha channels should simply ignore
 the color depth and interpret "Bits=16 and Planes=1" as 5:5:5:0 RGB
 and "Bits=32 and Planes=1" as 8:8:8:0 RGB and so on...

 Planar configuration actually only is intended for 2..256 color
 bitmaps - so please adhered to the convention to use only values
 in the range "1..8" for Planes with Bits=1 (and Depth==Planes).

 Note: Storing of "masks" as known from IFF-ILBM could be achieved
       using different values for Planes and Depth, in theory.
       This is not recommended, better stick to IFF-ILBM then.

 Alpha Channel Remarks
 ---------------------
 For 8 Bit alpha channel values, the following interpreation is
 mandatory:

  0x00  fully transparent (100% transparent)
  ...
  0xFF  fully opaque      (0% transparent)

 Any value X inbetween accordingly therefore has a transparency
 of (255-X)/255 percent.

 1 bit transparency translates into boolean transparency being
 mapped to the aformentioned values (0% resp. 100%)

 In case of 16 bit or greater transparency values the scheme is
 similar, so use 0x0000 and 0xFFFF (instead of 0x00 / 0xFF).

 This BTW matches the PNG specification.

 If you want to scale down a 16 bit alpha channel to 8 bit,
 use the following approach:

        0xhhll -> 0xhh

 If you want to scale up a 8 bit alpha channel to 16 bit,
 do it as follows:

        0xll   -> 0xllll

 Otherwise it would not be guaranteed that a maximum
 transparency of 0xFF / 0xFFFF is equal in both notations.

*/


#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
	case OPTION_EXTENSIONS:
		return (long) (EXTENSIONS);

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
#ifdef MISC_INFO
	case INFO_MISC:
		return (long)MISC_INFO;
#endif
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif

struct file_header {
	uint8_t magic[18];
	uint16_t version;
	uint32_t data_offset;
	uint32_t left_edge;
	uint32_t top_edge;
	uint32_t width;
	uint32_t height;
	uint32_t color_depth;
	uint32_t viewmode;
	uint8_t pixel_bits;
	uint8_t pixel_planes;
	uint32_t bytes_per_line;
	uint8_t colormap[256][3];
};


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
	uint8_t *bmap;
	int16_t handle;
	size_t bytes_per_line;
	struct file_header header;
	size_t file_size;
	
	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (memcmp(header.magic, "SVG Graphics File", 17) != 0 &&
		memcmp(header.magic, "SGX Graphics File", 17) != 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	if (header.version != 1)
	{
		Fclose(handle);
		RETURN_ERROR(EC_HeaderVersion);
	}
	if (header.viewmode != 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_ImageType);
	}

	if (header.pixel_planes != 1)
	{
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}
	info->planes = header.pixel_bits;
	if (header.pixel_bits <= 8)
	{
		int i;
		
		info->indexed_color = TRUE;
		info->components = 1;
		info->colors = 1L << info->planes;
		for (i = 0; i < (int)info->colors; i++)
		{
			info->palette[i].red = header.colormap[i][0];
			info->palette[i].green = header.colormap[i][1];
			info->palette[i].blue = header.colormap[i][2];
		}
	} else
	{
		info->indexed_color = FALSE;
		info->components = 3;
		info->colors = 1L << 24;
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}

	file_size -= sizeof(header);
	bmap = (void *)Malloc(file_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, file_size, bmap) != file_size)
	{
		Mfree(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);
	bytes_per_line = header.bytes_per_line;

	if (bmap[0] == 'X' && bmap[1] == 'P' && bmap[2] == 'K')
	{
		strcpy(info->compression, "XPK");
		Mfree(bmap);
		/* not yet supported */
		RETURN_ERROR(EC_CompType);
	} else if (bmap[0] == 'P' && bmap[1] == 'P' && bmap[2] == '2' && bmap[3] == '0')
	{
		strcpy(info->compression, "PP20");
		Mfree(bmap);
		/* not yet supported */
		RETURN_ERROR(EC_CompType);
	} else if (bmap[0] == 'L' && bmap[1] == 'Z' && bmap[2] == '7' && bmap[3] == '7')
	{
		size_t image_size;
		uint8_t *temp;
		
		strcpy(info->compression, "LZ77");
		image_size = bytes_per_line * header.height;
		temp = (void *)Malloc(image_size);
		if (temp == NULL)
		{
			Mfree(bmap);
			RETURN_ERROR(EC_Malloc);
		}
		if (fastlz_decompress(bmap + 4, file_size - 4, temp, image_size) == 0)
		{
			Mfree(temp);
			Mfree(bmap);
			RETURN_ERROR(EC_DecompError);
		}
		Mfree(bmap);
		bmap = temp;
	} else
	{
		strcpy(info->compression, "None");
	}

	info->width = header.width;
	info->height = header.height;
	info->planes = header.pixel_bits;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;

	strcpy(info->info, "SuperView Graphics");

	info->_priv_ptr = bmap;
	info->_priv_var = 0;		/* y offset */
	info->_priv_var_more = bytes_per_line;

	RETURN_SUCCESS();
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
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	size_t bytes_per_row = info->_priv_var_more;
	
	bmap += info->_priv_var;
	info->_priv_var += bytes_per_row;
	
	switch (info->planes)
	{
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		memcpy(buffer, bmap, info->width);
		break;
	}
	RETURN_SUCCESS();
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
	Mfree(info->_priv_ptr);
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
