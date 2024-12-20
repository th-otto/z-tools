#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"
#define NF_DEBUG 0
#include "nfdebug.h"
#include "cvtdctv.c"

/*
Interchange File Format    *.IFF

4 bytes         "FORM" (FORM chunk ID)
1 long          length of file that follows
4 bytes         "ILBM" (InterLeaved BitMap file ID)

4 bytes         "BMHD" (BitMap HeaDer chunk ID)
1 long          length of chunk [20]
20 bytes        1 word = image width in pixels
                1 word = image height in lines
                1 word = image x-offset [usually 0]
                1 word = image y-offset [usually 0]
                1 byte = # bitplanes
                1 byte = mask (0=no, 1=impl., 2=transparent, 3=lasso)
                1 byte = uncompressed [0], packbits [1], vertical RLE [2]
                1 byte = unused [0]
                1 word = transparent color (for mask=2)
                1 byte = x-aspect [5=640x200, 10=320x200/640x400, 20=320x400]
                1 byte = y-aspect [11]
                1 word = page width (usually the same as image width)
                1 word = page height (usually the same as image height)

4 bytes         "CMAP" (ColorMAP chunk ID)
1 long          length of chunk [3*n where n is the # colors]
3n bytes        3 bytes per RGB color.  Each color value is a byte and the
                actual color value is left-justified in the byte such that the
                most significant bit of the value is the MSB of the byte.
                (ie. a color value of 15 ($0F) i s stored as $F0) The bytes are
                stored in R,G,B order.

4 bytes         "CRNG" (Color RaNGe chunk ID)
1 long          length of chunk [8]
8 bytes         1 word = reserved [0]
                1 word = animation speed (16384 = 60 steps per second)
                1 word = active [1] or inactive [0]
                1 byte = left/lower color animation limit
                1 byte = right/upper color animation limit

4 bytes         "CAMG" (Commodore Amiga viewport mode chunk ID)
1 long          length of chunk [4]
1 long          viewport mode bits:
                  bit  2 = interlaced
                  bit  7 = half-bright
                  bit 11 = HAM
                  bit 15 = high res

4 bytes         "BODY" (BODY chunk ID)
1 long          length of chunk [# bytes of image data that follow]
? bytes         actual image data

NOTES: Some of these chunks may not be present in every IFF file, and may not
be in this order. You should always look for the ID bytes to find a certain
chunk. All chunk IDs are followed by a long value that tells the size of the
chunk (note that "ILBM" is not a chunk ID). This is the number of bytes that
FOLLOW the 4 ID bytes and size longword. The exception to this is the FORM
chunk. The size longword that follows the FORM ID is the size of the remainder
of the file. The FORM chunk must always be the first chunk in an IFF file.

The R,G,B ranges of Amiga and ST are different (Amiga 0...15, ST 0...7),
as is the maximum number of bitplanes (Amiga: 5, ST: 4).

Format of body data

An expanded picture is simply a bitmap. The most common packing method is
PackBits (see below), and is identical to MacPaint and DEGAS Elite compressed.

The (decompressed) body data appears in the following order:

        line 1 plane 0 ... line 1 plane 1 ... ... line 1 plane m
        [line 1 mask (if appropriate)]
        line 2 plane 0 ... line 2 plane 1 ... ... line 2 plane m
        [line 2 mask (if appropriate)]
        ...
        line x plane 0 ... line x plane 1 ... ... line x plane m
        [line x mask (if appropriate)]

The FORM chunk identifies the type of data:

        "ILBM" = interleaved bit map
        "8SVX" = 8-bit sample voice
        "SMUS" = simple music score
        "FTXT" = formatted text (Amiga)

The ST version of DPAINT always uses the vertical RLE packing format in the
body data. Within the BODY chunk there is one VDAT chunk per bit plane (four in
total). Every VDAT chunk is laid out in columns, similar to the Tiny format.
Only the compression scheme is slightly different, command 0, and 1 are
flipped, and extra count words store in data list.
4 bytes          'VDAT' vertical bitplane data
4 bytes          length of cunk
?? bytes         compressed data

Data compression format:
2 bytes          cmd_cnt command bytes count - 2
? bytes          cmd_cnt - 2 command bytes.
? words          data words

Note that the number of commands is 2 more than actual commands, and that the
data words may start on an odd address if the number of command bytes are odd.

while(cmd_cnt>2)
  1 byte cmd     0: read one data word as count
                    output count literal words from data words.
                 1: read one command word as count, then one data word as data
                    output data count times
                <0: -cmd is count, output count words from data words
                >2: cmd is count, read one data word as data
                    output data count times

See also PackBits Compression Algorithm
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
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
	case INFO_MISC:
		return (long)MISC_INFO;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif

/* FIXME: statics */
static char anno[256 + 1];
static struct BitMap bitmap;
static char info_txt[] = "DCTVx - Interchange File Format";

#include "../degas/packbits.c"


#if NF_DEBUG
static void bail(const char *msg)
#else
static void bail(void)
#define bail(msg) bail()
#endif
{
	int i;
	
	nf_debugprintf((msg));
	for (i = 0; i < 8; i++)
	{
		if (bitmap.Planes[i] != NULL)
		{
			free(bitmap.Planes[i]);
			bitmap.Planes[i] = NULL;
		}
	}
}


#define MAKE_ID(c1, c2, c3, c4) \
	((((uint32_t)(c1)) << 24) | \
	 (((uint32_t)(c2)) << 16) | \
	 (((uint32_t)(c3)) <<  8) | \
	 (((uint32_t)(c4))      ))


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
	unsigned int i;
	size_t image_size;
	uint8_t dummy;
	uint32_t chunk_id;
	uint32_t form_id;
	uint32_t chunk_size;
	int handle;
	struct DCTVCvtHandle *cvt;
	uint8_t *body;
	uint32_t src_offset;
	uint32_t dst_offset;
	uint16_t y;
	uint32_t camg;
	struct {
		uint16_t width;
		uint16_t height;
		uint16_t x_offset;
		uint16_t y_offset;
		uint8_t planes;
		uint8_t mask;
		uint8_t compressed;
		uint8_t unused;
		uint16_t transparent;
		uint8_t x_aspect;
		uint8_t y_aspect;
		int16_t page_width;
		int16_t page_height;
	} bmhd;
	uint16_t palette[16];
	
	nf_debugprintf(("reader_init: %s\n", name));

	anno[0] = '\0';
	for (i = 0; i < 8; i++)
	{
		bitmap.Planes[i] = NULL;
	}
	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		bail("fopen failed\n");
		return FALSE;
	}
	if (Fread(handle, sizeof(form_id), &form_id) != sizeof(form_id))
	{
		bail("fread failed\n");
		Fclose(handle);
		return FALSE;
	}
	if (form_id == MAKE_ID('F', 'O', 'R', 'M'))
	{
		Fseek(0, handle, SEEK_SET);
	} else
	{
		bail("abort - FORM chunk not found\n");
		return FALSE;
	}
	
	image_size = 0;
	camg = 0;
	body = NULL;
	for (;;)
	{
		if (Fread(handle, sizeof(chunk_id), &chunk_id) !=sizeof(chunk_id) ||
			Fread(handle, sizeof(chunk_size), &chunk_size) != sizeof(chunk_size))
		{
			bail("fread failed\n");
			Fclose(handle);
			return FALSE;
		}
		if (chunk_id == MAKE_ID('F', 'O', 'R', 'M'))
		{
			uint32_t form_type;

			if (Fread(handle, sizeof(form_type), &form_type) != sizeof(form_type) ||
				form_type != MAKE_ID('I', 'L', 'B', 'M'))
			{
				bail("fread failed\n");
				Fclose(handle);
				return FALSE;
			}
		} else if (chunk_id == MAKE_ID('B', 'M', 'H', 'D'))
		{
			nf_debugprintf(("load bmhd\n"));
			if (Fread(handle, sizeof(bmhd), &bmhd) != sizeof(bmhd))
			{
				bail("fread failed\n");
				Fclose(handle);
				return FALSE;
			}
			if (bmhd.planes > 4)
			{
				bail("abort - too many planes\n");
				Fclose(handle);
				return FALSE;
			}
			if (bmhd.mask == 1 || bmhd.mask == 4)
			{
				bail("abort - alpha/mask not allowed\n");
				Fclose(handle);
				return FALSE;
			}
			image_size = (((size_t)bmhd.width + 15) >> 4) * 2 * (size_t)bmhd.planes * bmhd.height;
		} else if (chunk_id == MAKE_ID('C', 'A', 'M', 'G'))
		{
			uint32_t val;

			nf_debugprintf(("load camg\n"));
			if (Fread(handle, sizeof(val), &val) != sizeof(val))
			{
				bail("fread failed\n");
				Fclose(handle);
				return FALSE;
			}
			camg = val & 0x8024L;
		} else if (chunk_id == MAKE_ID('A', 'N', 'N', 'O'))
		{
			nf_debugprintf(("load anno\n"));
			if (chunk_size <= sizeof(anno) - 1)
			{
				if ((size_t)Fread(handle, chunk_size, anno) != chunk_size)
				{
					bail("fread failed\n");
					Fclose(handle);
					return FALSE;
				}
				anno[chunk_size] = '\0';
			} else
			{
				nf_debugprintf(("skipped - anno to long\n"));
				Fseek(chunk_size, handle, SEEK_CUR);
			}
		} else if (chunk_id == MAKE_ID('C', 'M', 'A', 'P'))
		{
			nf_debugprintf(("load cmap\n"));
			if (chunk_size <= 16 * 3)
			{
				unsigned int colors;
				uint8_t rgb[3];
				
				colors = (uint16_t)chunk_size / 3;
				for (i = 0; i < colors; i++)
				{
					if (Fread(handle, 3, rgb) != 3)
					{
						bail("fread failed\n");
						Fclose(handle);
						return FALSE;
					}
					palette[i] = ((rgb[0] & 0xf0) << 4) | (rgb[1] & 0xf0) | ((rgb[2] & 0xf0u) >> 4);
				}
			} else
			{
				bail("abort - too many colors\n");
				Fclose(handle);
				return FALSE;
			}
		} else if (chunk_id == MAKE_ID('B', 'O', 'D', 'Y'))
		{
			nf_debugprintf(("load body\n"));
			if (image_size == 0)
			{
				bail("abort - BODY without bitmap header\n");
				Fclose(handle);
				return FALSE;
			}
			body = malloc(image_size);
			if (body == NULL)
			{
				bail("abort - malloc(bmap) failed\n");
				Fclose(handle);
				return FALSE;
			}
			if (bmhd.compressed == 0)
			{
				if ((size_t) Fread(handle, chunk_size, body) != chunk_size)
				{
					bail("fread failed\n");
					free(body);
					Fclose(handle);
					return FALSE;
				}
			} else if (bmhd.compressed == 1)
			{
				uint8_t *temp;

				temp = malloc(chunk_size);
				if (temp == NULL)
				{
					bail("abort - malloc(temp) failed\n");
					Fclose(handle);
					return FALSE;
				}
				if ((size_t)Fread(handle, chunk_size, temp) != chunk_size)
				{
					bail("fread failed\n");
					free(temp);
					free(body);
					Fclose(handle);
					return FALSE;
				}
				nf_debugprintf(("decode packbits\n"));
				decode_packbits(body, temp, image_size);
				free(temp);
			} else
			{
				bail("abort - unsupported compression type\n");
				Fclose(handle);
				return FALSE;
			}
			break;
		} else
		{
			if (chunk_size != 0)
			{
				nf_debugprintf(("skipped a chunk\n"));
				Fseek(chunk_size, handle, SEEK_CUR);
			}
		}
		if (chunk_size & 1)
		{
			if (Fread(handle, 1, &dummy) != 1)
			{
				bail("fread failed\n");
				Fclose(handle);
				return FALSE;
			}
			if (dummy != 0)
				Fseek(-1, handle, SEEK_CUR);
		}
	}
	nf_debugprintf(("fclose iff\n"));
	Fclose(handle);
	
	bitmap.BytesPerRow = ((((uint16_t)bmhd.width + 15) >> 4) << 4) / 8;
	bitmap.Rows = bmhd.height;
	bitmap.Depth = bmhd.planes;
	
	if (camg == 0)
	{
		nf_debugprintf(("fixed missing camg\n"));
		if (bmhd.page_width >= 640)
			camg = 0x8000;
		if (bmhd.page_height >= 400)
			camg |= 4;
	}
	
	nf_debugprintf(("reorder planes\n"));
	for (i = 0; i < bmhd.planes; i++)
	{
		uint8_t *plane;

		plane = malloc((size_t)bitmap.BytesPerRow * bitmap.Rows);
		if (plane == NULL)
		{
			bail("abort - malloc(addr) failed\n");
			free(body);
			return FALSE;
		}
		bitmap.Planes[i] = plane;
	}
	src_offset = 0;
	dst_offset = 0;
	for (y = 0; y < bitmap.Rows; y++)
	{
		for (i = 0; i < bmhd.planes; i++)
		{
			uint8_t *plane;

			plane = bitmap.Planes[i];
			memcpy(&plane[dst_offset], &body[src_offset], bitmap.BytesPerRow);
			src_offset += bitmap.BytesPerRow;
		}
		dst_offset += bitmap.BytesPerRow;
	}
	free(body);

	nf_debugprintf(("check sig\n"));
	if (CheckDCTV(&bitmap) == FALSE)
	{
		bail("sig not found\n");
		return FALSE;
	}
	nf_debugprintf(("sig ok\n"));
	
	nf_debugprintf(("init cvt structure\n"));
	cvt = AllocDCTV(&bitmap, camg & 4 ? 1 : 0); 
	if (cvt == NULL)
	{
		bail("abort - malloc(cvt) failed\n");
		return FALSE;
	}
	
	nf_debugprintf(("cvt colors\n"));
	SetmapDCTV(cvt, palette);
	
	nf_debugprintf(("info->structure\n"));
	
	info->planes = 24;
	info->indexed_color = FALSE;
	info->components = 3;
	info->width = bmhd.width;
	info->height = bmhd.height;
	info->colors = 1L << 24;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = cvt;
	info->max_comments_length = 0;
	if (anno[0] != '\0')
	{
		info->num_comments = 1;
		info->max_comments_length = strlen(anno);
	}

	info_txt[4] = bmhd.planes + '0';
	strcpy(info->info, info_txt);
	strcpy(info->compression, "YUV");
	
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
	int x;
	size_t w;
	uint8_t *rbuf;
	uint8_t *gbuf;
	uint8_t *bbuf;
	struct DCTVCvtHandle *cvt;

	cvt = info->_priv_ptr;
	ConvertDCTVLine(cvt);
	rbuf = cvt->Red;
	gbuf = cvt->Green;
	bbuf = cvt->Blue;
	
	w = 0;
	for (x = 0; x < info->width; x++)
	{
		buffer[w++] = rbuf[x];
		buffer[w++] = gbuf[x];
		buffer[w++] = bbuf[x];
	}
	
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
	bail("reader_quit(cleanup)\n");
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
	size_t i;
	
	if (info->num_comments != 0)
	{
		/* FIXME: ANNO chunk in examples does not seem to be ascii */
		for (i = 0; anno[i] != '\0'; i++)
		{
			if ((int8_t)anno[i] < ' ')
				anno[i] = ' ';
		}
		strcpy(txtdata->txt[0], anno);
	}
}
