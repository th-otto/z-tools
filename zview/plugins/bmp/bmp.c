/*
  1.00 1st release, addded encoder, build info
       added 16-bit high color and 32-bit true color support, fixed encoder
  1.01 added os/2 v1 support, added rle4, rle8, and bf compression support
  1.02 added os/2 v2 support, fixed reader_quit() bugs, fixed mono colors
  1.03 removed printf() debug calls
  1.04 optimized intel i/o, firebee fixes (patched purec library)
  1.05 made array global, better error handling, fixed encoder conflict
  1.06 code cleanup
  1.07 Fix a bug in rle4 decoder
*/

#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"
#define NF_DEBUG 0
#include "nfdebug.h"

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
#ifdef MISC_INFO
	case INFO_MISC:
		return (long)MISC_INFO;
#endif
	case INFO_AUTHOR:
		return (long)AUTHOR;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif

#define fill4B(a)	((4 - ((a) & 3)) & 3)

#define	FILETYPE	0x4d42	/* 'BM' */
#define RGB16_555	32767uL	/* these give problems in hex! */
#define RGB16_565	65535uL
#define RGB32_ARGB	16777215uL
/*#define RGB32_101010	?uL*/

#define	HDR_LEN1	14
#define	HDR_LEN2	40

#ifdef debug
#define db(S,V)	printf("DB: %s %d: "S"\n", __FILE__, __LINE__, V)
#else
#define db(S,V)	/* nop */
#endif

#define BMP_RGB 0
#define BMP_RLE8 1
#define BMP_RLE4 2
#define BMP_BITFIELDS 3
#define BMP_JPEG 4
#define BMP_PNG 5
#define BMP_ALPHABITFIELDS 6

#define RLE_COMMAND 0
#define RLE_ENDOFLINE 0
#define RLE_ENDOFBITMAP 1
#define RLE_DELTA 2


typedef struct {
	/* file header */
	unsigned char magic[2];		/* BM */
	unsigned char filesize[4];
	unsigned char xHotSpot[2];     
	unsigned char yHotSpot[2];
	unsigned char offbits[4];		/* offset to data */

	/* info header */
	union {
		struct {
			unsigned char hsize[4];			/* size of info header (40) */
			unsigned char width[4];			/* width in pixels */
			unsigned char height[4];		/* height in pixels */
			unsigned char planes[2];		/* always 1 */
			unsigned char bitcount[2];		/* bits per pixel: 1, 4, 8, 24 or 32 */
			unsigned char compression[4];	/* compression method */
			unsigned char sizeImage[4];		/* size of data */
			unsigned char xPelsPerMeter[4];
			unsigned char yPelsPerMeter[4];
			unsigned char clrUsed[4];		/* # of colors used */
			unsigned char clrImportant[4];	/* # of important colors */
			unsigned char RedMask[4];
			unsigned char GreenMask[4];
			unsigned char BlueMask[4];
			unsigned char AlphaMask[4];
		} bitmapinfoheader;
		struct {
			unsigned char hsize[4];			/* size of info header (12) */
			unsigned char width[2];			/* width in pixels */
			unsigned char height[2];		/* height in pixels */
			unsigned char planes[2];		/* always 1 */
			unsigned char bitcount[2];		/* bits per pixel: 1, 4, 8, 24 or 32 */
		} bitmapcoreheader;
	} bmp_info_header;
} BMP_HEADER;





static void outw(uint8_t *p, uint16_t d)
{
	*p++ = d;
	d >>= 8;
	*p = d;
}

static void outl(uint8_t *p, uint32_t d)
{
	*p++ = d;
	d >>= 8;
	*p++ = d;
	d >>= 8;
	*p++ = d;
	d >>= 8;
	*p = d;
}


/*==================================================================================*
 * uint32_t ToL:																	*
 *		Transform a unsigned long from little to big endian.						*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		puffer		->	pointer to the little endian long to convert.				*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
static uint32_t ToL(const uint8_t *puffer)
{
	return (uint32_t)puffer[0] | ((uint32_t)puffer[1] << 8) | ((uint32_t)puffer[2] << 16) | ((uint32_t)puffer[3] << 24);
}


/*==================================================================================*
 * uint16_t ToS:																	*
 *		Transform a unsigned word from little to big endian.						*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		puffer		->	pointer to the little endian word to convert.				*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
static uint16_t ToS(const uint8_t *puffer)
{
	return (uint16_t)puffer[0] | ((uint16_t)puffer[1] << 8);
}


static void decode_rle8(int16_t handle, IMGINFO info, uint8_t *bmap, uint32_t line_size)
{
	uint8_t buff[2];
	uint32_t x = 0;
	uint32_t y = 0;
	uint16_t i;
	uint8_t status_byte;
	uint8_t command_byte;
	uint8_t second_byte;
	uint8_t input;
	uint32_t xmargin;

	xmargin = info->width;
	
	for (;;)
	{
		if (Fread(handle, 1, &status_byte) != 1)
			break;
		if (status_byte == RLE_COMMAND)
		{						/* get control byte */
			if (Fread(handle, 1, &command_byte) != 1)
			{
				/* corrupted */
				break;
			}
			if (command_byte == RLE_ENDOFLINE)
			{					/* end of row? */
				y++;
				x = 0;
			} else if (command_byte == RLE_ENDOFBITMAP)
			{					/* end of bitmap? */
				break;
			} else if (command_byte == RLE_DELTA)
			{					/* delta offset? */
				if (Fread(handle, 2, buff) != 2)
				{
					/* corrupted */
					break;
				}
				x += buff[0];
				y += buff[1];
			} else
			{					/* literal group? */
				if ((x + command_byte) > xmargin || y >= info->height)
				{
					/* corrupted */
					nf_debugprintf(("corrupted\n"));
					break;
				}
				for (i = 0; i < command_byte; i++)
				{
					if (Fread(handle, 1, &input) != 1)
					{
						/* corrupted */
						break;
					}
					bmap[(y * line_size) + x] = input;
					x++;
				}
				if (command_byte & 1)
				{				/* odd? */
					/* pad to word boundary */
					Fread(handle, 1, &input);
				}
			}
		} else
		{						/* repeating group? */
			if ((x + status_byte) > xmargin || y >= info->height)
			{
				/* corrupted */
				nf_debugprintf(("corrupted\n"));
				break;
			}
			if (Fread(handle, 1, &second_byte) != 1)
			{
				/* corrupted */
				break;
			}
			for (i = 0; i < status_byte; i++)
			{
				bmap[(y * line_size) + x] = second_byte;
				x++;
			}
		}
	}
}


static void decode_rle4(int16_t handle, IMGINFO info, uint8_t *bmap, uint32_t line_size)
{
	uint8_t buff[2];
	uint32_t x = 0;
	uint32_t y = 0;
	uint16_t i;
	uint8_t status_byte;
	uint8_t command_byte;
	uint8_t second_byte;
	uint32_t xmargin;
	
	xmargin = info->width + (info->width & 1);
	
	for (;;)
	{
		if (Fread(handle, 1, &status_byte) != 1)
			break;
		nf_debugprintf(("status_byte $%02x, x=%u\n", status_byte, x));
		if (status_byte == 0)
		{						/* get control byte */
			if (Fread(handle, 1, &command_byte) != 1)
			{
				/* corrupted */
				nf_debugprintf(("corrupted\n"));
				break;
			}
			if (command_byte == RLE_COMMAND)
			{					/* end of row? */
				nf_debugprintf(("end of row, x=%u, y=%u\n", x, y));
				y++;
				x = 0;
			} else if (command_byte == RLE_ENDOFBITMAP)
			{					/* end of bitmap? */
				nf_debugprintf(("end of bitmap\n"));
				break;
			} else if (command_byte == RLE_DELTA)
			{					/* delta offset? */
				if (Fread(handle, 2, buff) != 2)
				{
					/* corrupted */
					nf_debugprintf(("corrupted\n"));
					break;
				}
				x += buff[0];
				y += buff[1];
				nf_debugprintf(("delta x=%u y=%u -> x=%u, y=%u\n", buff[0], buff[1], x, y));
			} else
			{					/* literal group? */
				if ((x + command_byte) > xmargin || y >= info->height)
				{
					/* corrupted */
					nf_debugprintf(("corrupted\n"));
					break;
				}
				nf_debugprintf(("literal %u\n", command_byte));
				for (i = 0; i < (command_byte >> 1); i++)
				{
					if (Fread(handle, 1, &second_byte) != 1)
					{
						/* corrupted */
						nf_debugprintf(("corrupted\n"));
						break;
					}
					bmap[(y * line_size) + x] = second_byte >> 4;
					x++;
					bmap[(y * line_size) + x] = second_byte & 0x0f;
					x++;
				}
				if (command_byte & 1)
				{				/* odd? */
					if (Fread(handle, 1, &second_byte) != 1)
					{
						/* corrupted */
						nf_debugprintf(("corrupted\n"));
						break;
					}
					bmap[(y * line_size) + x] = second_byte >> 4;
					x++;
				}
				if ((command_byte & 3) == 1 || (command_byte & 3) == 2)
				{
					/* pad to word boundary */
					Fread(handle, 1, &second_byte);
				}
			}
		} else
		{						/* repeating group? */
			if ((x + status_byte) > xmargin || y >= info->height)
			{
				/* corrupted */
				nf_debugprintf(("corrupted\n"));
				break;
			}
			if (Fread(handle, 1, &second_byte) != 1)
			{
				/* corrupted */
				nf_debugprintf(("corrupted\n"));
				break;
			}
			nf_debugprintf(("repeat %u $%02x\n", status_byte, second_byte));
			for (i = 0; i < (status_byte >> 1); i++)
			{
				bmap[(y * line_size) + x] = second_byte >> 4;
				x++;
				bmap[(y * line_size) + x] = second_byte & 0x0f;
				x++;
			}
			if (status_byte & 1)
			{
				bmap[(y * line_size) + x] = second_byte >> 4;
				x++;
			}
		}
	}
}


static void decode_rle24(int16_t handle, IMGINFO info, uint8_t *bmap, uint32_t line_size)
{
	uint8_t buff[4];
	uint32_t x = 0;
	uint32_t y = 0;
	uint16_t i;
	uint8_t status_byte;
	uint8_t command_byte;
	uint8_t input;
	uint32_t xmargin;

	xmargin = info->width;
	
	for (;;)
	{
		if (Fread(handle, 1, &status_byte) != 1)
			break;
		if (status_byte == RLE_COMMAND)
		{						/* get control byte */
			if (Fread(handle, 1, &command_byte) != 1)
			{
				/* corrupted */
				break;
			}
			if (command_byte == RLE_ENDOFLINE)
			{					/* end of row? */
				y++;
				x = 0;
			} else if (command_byte == RLE_ENDOFBITMAP)
			{					/* end of bitmap? */
				break;
			} else if (command_byte == RLE_DELTA)
			{					/* delta offset? */
				if (Fread(handle, 2, buff) != 2)
				{
					/* corrupted */
					break;
				}
				x += buff[0];
				y += buff[1];
			} else
			{					/* literal group? */
				if ((x + command_byte) > xmargin || y >= info->height)
				{
					/* corrupted */
					nf_debugprintf(("corrupted\n"));
					break;
				}
				for (i = 0; i < command_byte; i++)
				{
					if (Fread(handle, 3, buff) != 3)
					{
						/* corrupted */
						break;
					}
					bmap[(y * line_size) + (x * 3) + 0] = buff[2];
					bmap[(y * line_size) + (x * 3) + 1] = buff[1];
					bmap[(y * line_size) + (x * 3) + 2] = buff[0];
					x++;
				}
				if ((command_byte & 1) != 0)
				{				/* odd? */
					/* pad to word boundary */
					Fread(handle, 1, &input);
				}
			}
		} else
		{						/* repeating group? */
			if ((x + status_byte) > xmargin || y >= info->height)
			{
				/* corrupted */
				nf_debugprintf(("corrupted\n"));
				break;
			}
			if (Fread(handle, 3, buff) != 3)
			{
				/* corrupted */
				break;
			}
			for (i = 0; i < status_byte; i++)
			{
				bmap[(y * line_size) + (x * 3) + 0] = buff[2];
				bmap[(y * line_size) + (x * 3) + 1] = buff[1];
				bmap[(y * line_size) + (x * 3) + 2] = buff[0];
				x++;
			}
		}
	}
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
	uint8_t buff[4];
	int16_t compressed;
	int32_t colors_used;
	uint32_t hdrsize;
	uint32_t rmask, gmask, bmask;
	uint32_t offbits;
	uint32_t datasize;
	int16_t handle;
	BMP_HEADER header;
	uint8_t *bmap;
	uint32_t line_size;
	uint32_t pixelmask;
	
	db("", 0);
	db("%s", name);

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}

	if (Fread(handle, sizeof(header), &header) < (14 + 12))
	{
		Fclose(handle);
		return FALSE;
	}

	/* 1st header - always 14 bytes */
	if (ToS(header.magic) != FILETYPE)
	{
		Fclose(handle);
		return FALSE;
	}
	offbits = ToL(header.offbits);				/* offset to image data */

	/* 2nd header - 12, 40, 64 bytes */
	hdrsize = ToL(header.bmp_info_header.bitmapcoreheader.hsize);				/* size of header in bytes */
	db("hdrsize=%lu", hdrsize);

	switch ((int)hdrsize)
	{
	case 12:									/* os/2 v1 */
		strcpy(info->info, "OS/2 BitMaP v1");
		info->width = ToS(header.bmp_info_header.bitmapcoreheader.width);
		info->height = ToS(header.bmp_info_header.bitmapcoreheader.height);
		info->planes = ToS(header.bmp_info_header.bitmapcoreheader.bitcount);
		compressed = BMP_RGB;			/* force */
		colors_used = 1L << info->planes;	/* force */
		break;
	case 16: /* OS/2 BITMAPINFOHEADER2 */
	case 40: /* BITMAPINFOHEADER */
	case 52: /* BITMAPV2INFOHEADER */
	case 56: /* BITMAPV3INFOHEADER */
	case 64: /* OS/2 BITMAPINFOHEADER2 */
	case 108: /* BITMAPV4HEADER */
	case 124: /* BITMAPV5HEADER */
		info->width = (uint16_t) ToL(header.bmp_info_header.bitmapinfoheader.width);
		info->height = (uint16_t) ToL(header.bmp_info_header.bitmapinfoheader.height);
		info->planes = ToS(header.bmp_info_header.bitmapinfoheader.bitcount);
		if (hdrsize > 16)
		{
			compressed = (int16_t)ToL(header.bmp_info_header.bitmapinfoheader.compression);
			colors_used = ToL(header.bmp_info_header.bitmapinfoheader.clrUsed);	/* colors used */
			if (colors_used > 256)
			{
				db("invalid number of colors", 0);
				Fclose(handle);
				return FALSE;
			}
		} else
		{
			compressed = BMP_RGB;
			colors_used = 0;
		}
		if (colors_used == 0 && info->planes <= 8)
			colors_used = 1L << info->planes;
		if (hdrsize == 64 || hdrsize == 16)
			strcpy(info->info, "OS/2 BitMaP v2");
		else if (hdrsize >= 124)
			strcpy(info->info, "Windows BitMaP v5");
		else if (hdrsize >= 108)
			strcpy(info->info, "Windows BitMaP v4");
		else if (hdrsize >= 56)
			strcpy(info->info, "Windows BitMaP v3");
		else if (hdrsize >= 52)
			strcpy(info->info, "Windows BitMaP v2");
		else
			strcpy(info->info, "Windows BitMaP v1");
		break;
	default:
		db("invalid header size", 0);
		Fclose(handle);
		return FALSE;
	}
	Fseek(hdrsize + 14 - sizeof(header), handle, SEEK_CUR);

	if (colors_used == 0 && info->planes <= 8)
	{
		colors_used = 1L << info->planes;
	}

	db("compressed=%i", compressed);
	db("colors_used=%ld", colors_used);

	switch (info->planes)
	{
	case 1:
		if (compressed != BMP_RGB)
		{
			/* unsupported */
			Fclose(handle);
			return FALSE;
		}
		line_size = (info->width >> 3) + ((info->width & 7) ? 1 : 0);
		info->indexed_color = FALSE;
		break;
	case 4:
		if (compressed == BMP_RLE4)
		{
			/* decompression will write 1 byte per pixel */
			line_size = info->width;
		} else if (compressed == BMP_RGB)
		{
			/* uncompressed: */
			line_size = (info->width + 1) >> 1;
		} else
		{
			/* unsupported */
			Fclose(handle);
			return FALSE;
		}
		info->indexed_color = TRUE;
		break;
	case 8:
		if (compressed != BMP_RGB && compressed != BMP_RLE8)
		{
			/* unsupported */
			Fclose(handle);
			return FALSE;
		}
		line_size = info->width;
		info->indexed_color = TRUE;
		break;
	case 16:
		if (compressed != BMP_RGB && compressed != BMP_BITFIELDS)
		{
			/* unsupported */
			Fclose(handle);
			return FALSE;
		}
		line_size = info->width * 2;
		info->indexed_color = FALSE;
		break;
	case 24:
		line_size = info->width * 3;
		info->indexed_color = FALSE;
		break;
	case 32:
		line_size = info->width * 4;
		info->indexed_color = FALSE;
		break;
	default:
		Fclose(handle);
		return FALSE;
	}

	info->orientation = DOWN_TO_UP;
	if ((int16_t)info->height < 0)
	{
		info->height = -info->height;
		info->orientation = UP_TO_DOWN;
	}

	nf_debugprintf(("line_size1: %u %u\n", line_size, compressed));
	line_size += fill4B(line_size);
	nf_debugprintf(("line_size2: %u\n", line_size));
	datasize = line_size * info->height;
	bmap = malloc(datasize + 256L);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}

	info->colors = 1L << MIN(info->planes, 24);
	info->real_width = info->width;
	info->real_height = info->height;
	info->components = info->planes == 1 ? 1 : 3;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->num_comments = 0;

	if (info->planes <= 8)
	{
		int16_t i;

		for (i = 0; i < (int16_t)colors_used; i++)
		{
			if (hdrsize == 12 || hdrsize == 64)
				Fread(handle, 3, buff);							/* os/2 v1 */
			else
				Fread(handle, 4, buff);
			info->palette[i].blue = buff[0];
			info->palette[i].green = buff[1];
			info->palette[i].red = buff[2];
		}
	}

	/* sane defaults */
	if (info->planes == 16)
	{
		/* default masks for RGB555 */
		rmask = 0x7c00;
		gmask = 0x03e0;
		bmask = 0x001f;
	} else if (info->planes == 32)
	{
		rmask = 0xff0000UL;
		gmask = 0x00ff00UL;
		bmask = 0x0000FFUL;
	} else
	{
		rmask = gmask = bmask = 0;
	}

	/* try to get masks from file */
	if (compressed == BMP_BITFIELDS)
	{
		/*
		 * Some files seems to have a header size of 40,
		 * but still contain the bitfield masks
		 */
		if ((info->planes == 16 || info->planes == 32) && (hdrsize >= 52 || (hdrsize >= 40 && offbits >= 66)))
		{
			rmask = ToL(header.bmp_info_header.bitmapinfoheader.RedMask);
			gmask = ToL(header.bmp_info_header.bitmapinfoheader.GreenMask);
			bmask = ToL(header.bmp_info_header.bitmapinfoheader.BlueMask);
		}
	}
	pixelmask = rmask | gmask | bmask;

	if (offbits != 0)
		Fseek(offbits, handle, SEEK_SET);

#if 0
	db("rmask=%lu",rmask);
	db("gmask=%lu",gmask);
	db("bmask=%lu",bmask);
	db("pixelmask=%lu",pixelmask);
#endif

	switch (compressed)
	{
	case BMP_RGB:					/* uncompressed */
		strcpy(info->compression, "None");
		if ((uint32_t) Fread(handle, datasize, bmap) != datasize)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		if (info->planes == 1)
		{									/* mono */
			uint16_t bc = info->palette[0].red + info->palette[0].green + info->palette[0].blue;
			uint16_t fc = info->palette[1].red + info->palette[1].green + info->palette[1].blue;
	
			if (bc < fc)
			{
				uint32_t s;
				uint32_t *p;
				
				p = (uint32_t *)bmap;
				s = datasize >> 2;
				do
				{
					*p = ~*p;
					p++;
				} while (--s > 0);
			}
		}
		break;

	case BMP_RLE8:					/* 8-bit rle (RLE8) */
		strcpy(info->compression, "RLE8");
		memset(bmap, 0, datasize);
		decode_rle8(handle, info, bmap, line_size);
		break;

	case BMP_RLE4:						/* 4-bit rle (RLE4) !! converted to 8 planes !! */
		strcpy(info->compression, "RLE4");
		memset(bmap, 0, datasize);
		decode_rle4(handle, info, bmap, line_size);
		break;

	case BMP_BITFIELDS:					/* huffman 1d/bf */
		if (info->planes == 1)
		{
			strcpy(info->compression, "H1D");
			free(bmap);
			Fclose(handle);
			return FALSE;
		} else
		{
			strcpy(info->compression, "BF");
			Fread(handle, datasize, bmap);
		}
		break;

	case BMP_JPEG:							/* 24-bit rle (RLE24) !! untested !! */
		strcpy(info->compression, "RLE");
		memset(bmap, 0, datasize);
		decode_rle24(handle, info, bmap, line_size);
		break;

	default:							/* compression not supported? */
		free(bmap);
		Fclose(handle);
		return FALSE;
	}

	Fclose(handle);

	info->_priv_ptr = bmap;
	info->_priv_var = compressed;
	info->_priv_var_more = 0;				/* y offset */
	info->_priv_ptr_more = (void *)line_size;
	info->__priv_ptr_more = (void *)pixelmask;

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
	int16_t i;
	int16_t x;
	uint8_t b;
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	uint32_t line_size = (uint32_t)info->_priv_ptr_more;
	uint32_t pos = info->_priv_var_more;
	uint32_t pixelmask = (uint32_t)info->__priv_ptr_more;

	info->_priv_var_more += line_size;

	switch (info->planes)
	{
	case 1:
		x = 0;
		do
		{
			b = bmap[pos++];
			for (i = 0; i < 8; i++)
			{
				if (b & 0x80)
				{
					buffer[x++] = 0xff;
				} else
				{
					buffer[x++] = 0x00;
				}
				b = b << 1;
			}
		} while (x < info->width);
		break;
	case 4:
		x = 0;
		if (info->_priv_var)
		{
			/* decompression already wrote 1 byte per pixel */
			do
			{
				b = bmap[pos++];
				buffer[x++] = b;
			} while (x < info->width);
		} else
		{
			do
			{
				b = bmap[pos++];
				buffer[x++] = b >> 4;
				buffer[x++] = b & 0xF;
			} while (x < info->width);
		}
		break;
	case 8:
		for (x = 0; x < info->width; x++)
		{
			buffer[x] = bmap[pos++];
		}
		break;
	case 16:
		i = 0;
		for (x = 0; x < info->width; x++)
		{
			uint16_t rgb = ((uint16_t) bmap[pos + 1] << 8) | (uint16_t) bmap[pos];

			if (pixelmask == 32767uL)
			{							/* xrrrrrgggggbbbbb */
				buffer[i++] = ((rgb >> 10) & 0x1F) << 3;
				buffer[i++] = ((rgb >> 5) & 0x1F) << 3;
				buffer[i++] = (rgb & 0x1F) << 3;
			} else
			{							/* rrrrrggggggbbbbb */
				buffer[i++] = ((rgb >> 11) & 0x1F) << 3;
				buffer[i++] = ((rgb >> 5) & 0x3F) << 2;
				buffer[i++] = (rgb & 0x1F) << 3;
			}
			pos += 2;
		}
		break;
	case 24:
		i = 0;
		for (x = 0; x < info->width; x++)
		{
			buffer[i++] = bmap[pos + 2];
			buffer[i++] = bmap[pos + 1];
			buffer[i++] = bmap[pos + 0];
			pos += 3;
		}
		break;
	case 32:
		i = 0;
		for (x = 0; x < info->width; x++)
		{
			if (pixelmask == 16777215uL)
			{							/* bgra */
				/* bbbbbbbb gggggggg rrrrrrrr xxxxxxxx */
				buffer[i++] = bmap[pos + 2];
				buffer[i++] = bmap[pos + 1];
				buffer[i++] = bmap[pos + 0];
			} else
			{
				/* xxrrrrrr rrrrgggg ggggggbb bbbbbbbb */
				uint32_t rgbx =
					((uint32_t) bmap[pos + 3] << 24) | ((uint32_t) bmap[pos + 2] << 16) | ((uint32_t) bmap[pos + 1] << 8) |
					(uint32_t) bmap[pos];
				buffer[i++] = ((rgbx >> 20) & 0x3FF) >> 2;
				buffer[i++] = ((rgbx >> 10) & 0x3FF) >> 2;
				buffer[i++] = (rgbx & 0x3FF) >> 2;
			}
			pos += 4;
		}
		break;
	}

	return TRUE;
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
	(void) info;
	(void) txtdata;
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


boolean __CDECL encoder_init(const char *name, IMGINFO info)
{
	uint32_t datasize;
	int16_t handle;
	BMP_HEADER header;
	uint8_t *bmap;
	uint32_t line_size;
	long ret;

	line_size = (int32_t) info->width * 3;
	line_size += fill4B(line_size);
	datasize = line_size * info->height;

	if ((handle = (int16_t) Fcreate(name, 0)) < 0)
	{
		return FALSE;
	}
	bmap = malloc(datasize);				/* entire image, needs flipped */
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	/* bmp version 3 (microsoft windows x.x) */
	outw(header.magic, FILETYPE);		/* id */
	outl(header.filesize, HDR_LEN1 + HDR_LEN2 + datasize);	/* files size */
	outw(header.xHotSpot, 0);					/* reserved1 */
	outw(header.yHotSpot, 0);					/* reserved2 */
	outl(header.offbits, HDR_LEN1 + HDR_LEN2);	/* offset to image data */
	/* header part 2 */
	outl(header.bmp_info_header.bitmapinfoheader.hsize, HDR_LEN2);				/* hdr size */
	outl(header.bmp_info_header.bitmapinfoheader.width, info->width);			/* pixel width */
	outl(header.bmp_info_header.bitmapinfoheader.height, info->height);			/* pixel height */
	outw(header.bmp_info_header.bitmapinfoheader.planes, 1);					/* color planes - always 1 */
	outw(header.bmp_info_header.bitmapinfoheader.bitcount, 24);					/* bits per pixel */
	outl(header.bmp_info_header.bitmapinfoheader.compression, BMP_RGB);			/* no compression */
	outl(header.bmp_info_header.bitmapinfoheader.sizeImage, datasize);			/* size of bitmap */
	outl(header.bmp_info_header.bitmapinfoheader.xPelsPerMeter, 0);				/* horz res */
	outl(header.bmp_info_header.bitmapinfoheader.yPelsPerMeter, 0);				/* vert res */
	outl(header.bmp_info_header.bitmapinfoheader.clrUsed, 0);					/* number of colors */
	outl(header.bmp_info_header.bitmapinfoheader.clrImportant, 0);				/* important colors */

	if ((ret = Fwrite(handle, HDR_LEN1 + HDR_LEN2, &header)) != HDR_LEN1 + HDR_LEN2)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	(void)ret;
	
	info->planes = 24;
	info->components = 3;
	info->colors = 1L << 24;
	info->orientation = DOWN_TO_UP;		/* no worky, flip it ourselves! */
	info->memory_alloc = TT_RAM;
	info->indexed_color = FALSE;
	info->page = 1;
	info->_priv_var = handle;
	info->_priv_var_more = line_size * (info->height - 1);
	info->_priv_ptr = bmap;
	info->_priv_ptr_more = (void *)line_size;

	return TRUE;
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	int16_t x;
	uint32_t line_size = (uint32_t)info->_priv_ptr_more;
	int32_t yy = info->_priv_var_more;		/* current offset */

	bmap += yy;
	info->_priv_var_more -= line_size;		/* dec current y */
	
	x = info->width;
	do
	{
		*bmap++ = buffer[2];	/* blue */
		*bmap++ = buffer[1];	/* green as is */
		*bmap++ = buffer[0];	/* red */
		buffer += 3;
	} while (--x > 0);

	return TRUE;
}


void __CDECL encoder_quit(IMGINFO info)
{
	int16_t handle = (int16_t)info->_priv_var;
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	uint32_t line_size = (uint32_t)info->_priv_ptr_more;

	/* write the entire bitmap in one go, unable to return an error code */
	if (handle > 0)
	{
		Fwrite(handle, line_size * info->height, bmap);
		Fclose(handle);
		info->_priv_var = 0;
	}

	free(bmap);
	info->_priv_ptr = 0;
}
