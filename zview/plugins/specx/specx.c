#include "plugin.h"
#include "zvplugin.h"
#include "symbols.h"

#define VERSION		0x201
#define AUTHOR      "Thorsten Otto"
#define NAME        "Spectrum 512 Extended"
#define DATE        __DATE__ " " __TIME__
#define MISC_INFO   "Some code by Hans Wessels"

/*
Spectrum 512 Extended    *.SPX

SPX files are extended Spectrum files with more than 200 lines in the picture,
designed by Gizmo of Electronic Images. With a SPX creator like SPXCRT14.PRG
multiple spectrum pictures can be attached to become one big .SPX file. There
are two known versions of the format. Version 2 files of the format can be
packed as a whole (including the header) with either ICE 2.10 or Atomik 3.5.

3  bytes        file ID, "SPX"
1  byte         version (1 or 2)
1  byte         graphics compression flag, 0 = unpacked
1  byte         palette compression flag, 0 = unpacked
1  byte         number of 32000 byte screens
3  bytes        reserved
?? bytes        NULL terminated author string
?? bytes        NULL terminated description string
4  bytes        size of (packed) graphics data in bytes
4  bytes        size of (packed) palette data in bytes
?? bytes        (packed) graphics data
?? bytes        (packed) palette data

The number of lines in the picture can be calculated by dividing the size of
the unpacked graphics data by 160. The unpacked SPX picture can be treated as a
Spectrum screen with more than 200 lines.

In version 1 when a compression flag is set the data is packed with ICE 2.10.

In version 2 a special packer is used, graphics and palettes are packed in one
chunk.
4 bytes         unpacked size
4 bytes         packed size
Like the ICE packer data is packed from end of file to start. The depacker
starts to read data from packed data start+packed size and to write data to
depacked data start+depacked size. A long is read form packed data into a
bitbuffer, when the bitbuffer is empty it is refilled with a new long from the
packed file. With getbits(n) the n most significat bytes are read from the
bitbuffer.
while(bytes read<depacked size)
  getbits(1)
    0 : literals
        len = getbits(4*(getbits(2)+1))
        read len bytes with getbits(8) and write them to the destination
        if(len<>$ffff) do a ptr_len
    1 : ptr_len
        ptr = getbits(4*(getbits(2)+1))
        len = getbits(4*(getbits(2)+1))
        copy len+3 bytes from ptr
Note: after a literal there is always a ptr_len except when len was $ffff.

See also Spectrum 512 file format
*/


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("SPX\0");

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


static uint8_t const colortable[16] = {
	0x00,								/* 0000 */
	0x22,								/* 0001 */
	0x44,								/* 0010 */
	0x66,								/* 0011 */
	0x88,								/* 0100 */
	0xAA,								/* 0101 */
	0xCC,								/* 0110 */
	0xEE,								/* 0111 */
	0x11,								/* 1000 */
	0x33,								/* 1001 */
	0x55,								/* 1010 */
	0x77,								/* 1011 */
	0x99,								/* 1100 */
	0xBB,								/* 1101 */
	0xDD,								/* 1110 */
	0xFF								/* 1111 */
};

#define SCREEN_W 320
#define SCREEN_H 200
#define SCREEN_SIZE 32000L

typedef long coord_t;
typedef uint16_t stcolor_t;


/*
 *  Given an x-coordinate and a color index, returns the corresponding Spectrum palette index.
 *  by Steve Belczyk; placed in the public domain December, 1990.
 */
static int find_index(int x, int c)
{
	int x1 = 10 * c;

	if (1 & c)
	{									/* If c is odd */
		x1 = x1 - 5;
	} else
	{									/* If c is even */
		x1 = x1 + 1;
	}
	if (x >= x1 && x < (x1 + 160))
	{
		c = c + 16;
	} else if (x >= (x1 + 160))
	{
		c = c + 32;
	}
	return c;
}


static uint32_t get_long(const uint8_t *p)
{
	uint32_t res;

	res = *p++;
	res <<= 8;
	res += *p++;
	res <<= 8;
	res += *p++;
	res <<= 8;
	res += *p;
	return res;
}

/*
** Ice 2_10 depacker, universal C version
** function is reentrand and thread safe
** placed in public domain 2007 by Hans Wessels
**
** the function:
** ice_21_depack(uint8_t *src, uint8_t *dst);
** depacks Ice 2.1 packed data located at src to dst, it
** returns the size of the depacked data or -1 if no valid
** Ice 2.1 header was found. The memory at dst should be large
** enough to hold the depacked data.
**
** the function:
** int ice_21_header(uint8_t *src)
** returns 0 if no valid Ice 2.1 header was found at src
**
** the function:
** unsigned long ice_21_packedsize(uint8_t *src)
** returns the size of the ice 2.1 packed data located at src,
** the function does not check for a valid ice 2.1 header
**
** the function:
** unsigned long ice_21_origsize(uint8_t *src)
** returns the unpacked (original) size of the ice 2.1 packed 
** data located at src, the function does not check for a valid
** ice 2.1 header
**
** Ice 2.1 was a popular data packer for the Atari ST series
** Ice 2.1 packed data can be recognized by the characters "Ice!"
** at the first 4 positions in a file
*/

static uint32_t ice_get_long(uint8_t **p)
{
	uint8_t *q = *p;

	q -= 4;
	*p = q;
	return get_long(q);
}


static uint16_t ice_getbits(int len, uint32_t *cmdp, uint32_t *maskp, uint8_t **p)
{
	uint16_t tmp = 0;
	uint32_t cmd = *cmdp;
	uint32_t mask = *maskp;

	while (len > 0)
	{
		tmp += tmp;
		mask >>= 1;
		if (mask == 0)
		{
			cmd = ice_get_long(p);
			mask = 0x80000000UL;
		}
		if (cmd & mask)
		{
			tmp += 1;
		}
		len--;
	}
	*cmdp = cmd;
	*maskp = mask;
	return tmp;
}


/* returns 0 if no ice 21 header was found */
static int ice_21_header(uint8_t *src)
{
	return get_long(src) == 0x49636521UL;
}


/* returns packed size of ice packed data */
static int32_t ice_21_packedsize(uint8_t *src)
{
	return get_long(src + 4);
}


/* returns origiginal size of ice packed data */
static int32_t ice_21_origsize(uint8_t *src)
{
	return get_long(src + 8);
}


/* Ice! V 2.1 depacker */
static int32_t ice_21_depack(uint8_t *src, uint8_t *dst)
{
	uint8_t *p;
	uint32_t cmd;
	uint32_t mask = 0;
	int32_t orig_size;

	if (!ice_21_header(src))
	{									/* No 'Ice!' header */
		return -1;
	}
	orig_size = ice_21_origsize(src);	/* orig size */
	if (orig_size < 0 || ice_21_packedsize(src) < 0)
	{
		return -1;
	}
	p = dst + orig_size;
	src += ice_21_packedsize(src);		/* packed size */
	ice_getbits(1, &cmd, &mask, &src);	/* init cmd */
	/* Ice has an init problem, the LSB in cmd that is set is _NOT_ valid
	 * reaching this bit is a sign to reload the cmd data (4 bytes)
	 * as the msb bit is always set there are always 2 bits set in the cmd
	 * block
	 */
	{									/* fix reload */
		mask = 0x80000000UL;
		while (!(cmd & 1))
		{								/* dump all 0 bits */
			cmd >>= 1;
			mask >>= 1;
		}
		/* dump one 1 bit */
		cmd >>= 1;
	}
	for (;;)
	{
		if (ice_getbits(1, &cmd, &mask, &src))
		{								/* literal */
			const int lenbits[] = { 1, 2, 2, 3, 8, 15 };
			const long int maxlen[] = { 1, 3, 3, 7, 255, 32768L };
			const int offset[] = { 1, 2, 5, 8, 15, 270 };
			int tablepos = -1;
			long len;

			do
			{
				tablepos++;
				len = ice_getbits(lenbits[tablepos], &cmd, &mask, &src);
			} while (len == maxlen[tablepos]);
			len += offset[tablepos];
			if ((p - dst) < len)
			{
				len = (p - dst);
			}
			while (len > 0)
			{
				*--p = *--src;
				len--;
			}
			if (p <= dst)
			{
				return orig_size;
			}
		}
		/* no else here, always a sld after a literal */
		{								/* sld */
			uint8_t *q;
			int len;
			int pos = 0;

			{
				const int extra_bits[] = { 0, 0, 1, 2, 10 };
				const int offset[] = { 0, 1, 2, 4, 8 };
				int tablepos = 0;

				while (ice_getbits(1, &cmd, &mask, &src))
				{
					tablepos++;
					if (tablepos == 4)
					{
						break;
					}
				}
				len = offset[tablepos] + ice_getbits(extra_bits[tablepos], &cmd, &mask, &src);
			}
			if (len)
			{
				const int extra_bits[] = { 8, 5, 12 };
				const int offset[] = { 32, 0, 288 };
				int tablepos = 0;

				while (ice_getbits(1, &cmd, &mask, &src))
				{
					tablepos++;
					if (tablepos == 2)
					{
						break;
					}
				}
				pos = offset[tablepos] + ice_getbits(extra_bits[tablepos], &cmd, &mask, &src);
			} else
			{
				if (ice_getbits(1, &cmd, &mask, &src))
				{
					pos = 64 + ice_getbits(9, &cmd, &mask, &src);
				} else
				{
					pos = ice_getbits(6, &cmd, &mask, &src);
				}
			}
			len += 2;
			q = p + len + pos;
			if ((p - dst) < len)
			{
				len = (int) (p - dst);
			}
			while (len > 0)
			{
				*--p = *--q;
				len--;
			}
			if (p <= dst)
			{
				return orig_size;
			}
		}
	}
}


/* also used by SPX v2 depacker */
static uint32_t spx_v2_get_long(uint8_t **p)
{
	uint32_t res;
	uint8_t *q = *p;

	q -= 4;
	res = q[0];
	res <<= 8;
	res += q[1];
	res <<= 8;
	res += q[2];
	res <<= 8;
	res += q[3];
	*p = q;
	return res;
}


/* used by SPX v2 depacker */
static unsigned int spx_v2_getbits(int len, uint32_t *cmdp, uint32_t *maskp, uint8_t **p)
{
	int tmp = 0;
	uint32_t cmd = *cmdp;
	uint32_t mask = *maskp;

	while (len > 0)
	{
		tmp += tmp;
		mask >>= 1;
		if (mask == 0)
		{
			cmd = spx_v2_get_long(p);
			mask = 0x80000000UL;
		}
		if (cmd & mask)
		{
			tmp += 1;
		}
		len--;
	}
	*cmdp = cmd;
	*maskp = mask;
	return tmp;
}


static uint32_t spx_v2_getlen(uint32_t *cmdp, uint32_t *maskp, uint8_t **p)
{
	int bits = spx_v2_getbits(2, cmdp, maskp, p);

	bits++;
	bits *= 4;
	return spx_v2_getbits(bits, cmdp, maskp, p);
}


static long depack_spx_v2(uint8_t *dst, uint8_t *src)
{
	uint8_t *p;
	uint32_t cmd;
	uint32_t mask = 0;
	long orig_size;

	src += 8;
	cmd = spx_v2_get_long(&src);		/* packed size */
	orig_size = spx_v2_get_long(&src);	/* orig size */
	p = dst + orig_size;
	src += cmd + 8;						/* start at end */
	for (;;)
	{
		if (spx_v2_getbits(1, &cmd, &mask, &src) == 0)
		{
			/* literal */
			long int len = spx_v2_getlen(&cmd, &mask, &src);
			long int tmp = len;

			if (len > (p - dst))
			{
				len = (p - dst);
			}
			while (len > 0)
			{
				*--p = spx_v2_getbits(8, &cmd, &mask, &src);
				len--;
			}
			if (p <= dst)
			{
				return orig_size;
			}
			if (tmp == 0xffffL)
			{
				continue;
			}
		}
		{
			/* sld */
			uint8_t *q;
			long int pos = spx_v2_getlen(&cmd, &mask, &src);
			long int len = spx_v2_getlen(&cmd, &mask, &src);

			len += 3;
			q = p + pos;
			if (len > (p - dst))
			{
				len = (p - dst);
			}
			while (len > 0)
			{
				*--p = *--q;
				len--;
			}
			if (p <= dst)
			{
				return orig_size;
			}
		}
	}
}


static uint8_t *convert_spectrum(uint8_t *gfx, uint8_t *pal, long lines)
{
	uint8_t *dst;

	dst = malloc(SCREEN_W * 3 * lines);
	if (dst != NULL)
	{
		uint8_t *p = dst;
		coord_t y;

		for (y = 0; y < 960; y++)
		{								/* 1st line is black */
			*p++ = 0;
		}
		for (y = 1; y < lines; y++)
		{
			uint8_t *q = gfx + y * 160;
			uint8_t *cp = pal + (y - 1) * 96;
			int x;

			for (x = 0; x < 20; x++)
			{
				uint16_t plane0;
				uint16_t plane1;
				uint16_t plane2;
				uint16_t plane3;
				int i;

				plane0 = *q++;
				plane0 <<= 8;
				plane0 += *q++;
				plane1 = *q++;
				plane1 <<= 8;
				plane1 += *q++;
				plane2 = *q++;
				plane2 <<= 8;
				plane2 += *q++;
				plane3 = *q++;
				plane3 <<= 8;
				plane3 += *q++;
				for (i = 0; i < 16; i++)
				{
					uint8_t data;
					stcolor_t color;

					data = (uint8_t) ((plane3 >> 12) & 8);
					data += (uint8_t) ((plane2 >> 13) & 4);
					data += (uint8_t) ((plane1 >> 14) & 2);
					data += (uint8_t) ((plane0 >> 15) & 1);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
					color = find_index(x * 16 + i, data);
					color = (cp[color * 2] << 8) + cp[color * 2 + 1];
					*p++ = colortable[(color >> 8) & 0xf];
					*p++ = colortable[(color >> 4) & 0xf];
					*p++ = colortable[color & 0xf];
				}
			}
		}
	}
	return dst;
}




boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	uint8_t *file;
	int16_t handle;
	uint8_t *bmap;
	int version;
	coord_t height;

	if ((handle = (int16_t)Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file = malloc(2048L);
	if (file == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}

	if (Fread(handle, 2048, file) != 2048)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (file[0] != 'S' || file[1] != 'P' || file[2] != 'X')
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}

	/* found SPX header */
	{
		uint8_t *p = file;
		uint8_t *gfx;
		uint8_t *pal;
		int packed_gfx;
		int packed_pal;
		size_t screens;
		coord_t lines;
		size_t gfx_size;
		size_t pal_size;
		size_t memneed;
		size_t len;
		int headersize;

		file[2046] = 0;				/* make sure strlen will end in memblock */
		file[2047] = 0;
		p += 3;						/* skip header */
		version = *p++;
		if (version > 2)
		{
			free(file);
			Fclose(handle);
			RETURN_ERROR(EC_HeaderVersion);
		}
		packed_gfx = *p++;
		packed_pal = *p++;
		screens = *p++;
		p += 3;						/* reserved */
		p += strlen(p) + 1;			/* skip author */
		p += strlen(p) + 1;			/* skip description */
		if ((p - file) > 2040)
		{
			free(file);
			Fclose(handle);
			RETURN_ERROR(EC_HeaderLength);
		}
		gfx_size = get_long(p);
		p += 4;
		pal_size = get_long(p);
		p += 4;
		memneed = (p - file) + gfx_size + pal_size + screens * 52000L;
		headersize = (int) (p - file);
		free(file);
		file = malloc(memneed);
		if (file == NULL)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		len = Fseek(0, handle, SEEK_END);
		Fseek(0, handle, SEEK_SET);
		if (len > memneed)
		{
			free(file);
			Fclose(handle);
			RETURN_ERROR(EC_FileLength);
		}
		if ((size_t)Fread(handle, len, file) != len)
		{
			free(file);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		Fclose(handle);
		if (version == 2 && packed_gfx != 0)
		{							/* special v2 packed */
			size_t size;
			uint8_t *q;

			if (len < (headersize + gfx_size))
			{
				free(file);
				RETURN_ERROR(EC_FileLength);
			}
			p = file + headersize;
			q = p;
			size = get_long(q);
			q += 4;
			if (size > screens * (SCREEN_SIZE + SCREEN_H * 3L * 16L * 2L))
			{						/* incorrect origsize */
				free(file);
				RETURN_ERROR(EC_CompLength);
			}
			size = get_long(q);
			q += 4;
			if (size > gfx_size)
			{						/* incorrect packed size */
				free(file);
				RETURN_ERROR(EC_CompLength);
			}
			gfx = file + headersize + gfx_size;
			pal = gfx + pal_size - 96L;
			if (depack_spx_v2(gfx, p) < 0)
			{						/* ice depack error */
				free(file);
				RETURN_ERROR(EC_DecompError);
			}
			if ((pal_size % 160) == 0)
				lines = pal_size / 160;
			else
				lines = screens * (SCREEN_H - 1) + 1;
		} else
		{
			if (len < (headersize + gfx_size + pal_size))
			{
				free(file);
				RETURN_ERROR(EC_FileLength);
			}
			p = file + headersize;
			if (packed_gfx)
			{
				size_t size;

				size = ice_21_packedsize(p);
				if (size > gfx_size)
				{					/* incorrect packed size */
					free(file);
					RETURN_ERROR(EC_CompLength);
				}
				size = ice_21_origsize(p);
				if (size > screens * SCREEN_SIZE)
				{					/* incorrect origsize */
					free(file);
					RETURN_ERROR(EC_BitmapLength);
				}
				gfx = file + headersize + gfx_size + pal_size;
				if (ice_21_depack(p, gfx) < 0)
				{					/* ice depack error */
					free(file);
					RETURN_ERROR(EC_DecompError);
				}
				lines = size / 160;
			} else
			{
				if (gfx_size > screens * SCREEN_SIZE)
				{					/* incorrect gfxsize */
					free(file);
					RETURN_ERROR(EC_BitmapLength);
				}
				gfx = p;
				lines = gfx_size / 160;
			}
			p += gfx_size;
			if (packed_pal)
			{
				size_t size;

				size = ice_21_packedsize(p);
				if (size > pal_size)
				{					/* incorrect packed size */
					free(file);
					RETURN_ERROR(EC_CompLength);
				}
				size = ice_21_origsize(p);
				if (size > screens * (SCREEN_H * 48L * 2))
				{					/* incorrect origsize */
					free(file);
					RETURN_ERROR(EC_ColorMapLength);
				}
				pal = file + headersize + gfx_size + pal_size;
				if (packed_gfx)
				{
					pal += screens * SCREEN_SIZE;
				}
				if (ice_21_depack(p, pal) < 0)
				{					/* ice depack error */
					free(file);
					RETURN_ERROR(EC_DecompError);
				}
			} else
			{
				if (pal_size > screens * (SCREEN_H * 48L * 2))
				{					/* incorrect palsize */
					free(file);
					RETURN_ERROR(EC_ColorMapLength);
				}
				pal = p;
			}
		}
		height = lines;
		bmap = convert_spectrum(gfx, pal, lines);
		if (bmap == NULL)
		{
			free(file);
			RETURN_ERROR(EC_Malloc);
		}
		free(file);

		if (packed_gfx || packed_pal)
		{
			strcpy(info->compression, "ICE");
		} else
		{
			strcpy(info->compression, "None");
		}
	}
	
	strcpy(info->info, "Spectrum 512 Extended");
	if (version == 1)
		strcat(info->info, " (v1)");
	else if (version == 2)
		strcat(info->info, " (v2)");

	info->planes = 12;
	info->width = SCREEN_W;
	info->height = height;
	info->components = 3;
	info->indexed_color = FALSE;
	info->colors = 1L << 12;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */

	info->_priv_var = 0;
	info->_priv_ptr = bmap;

	RETURN_SUCCESS();
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	int x;
	uint8_t *bmap = info->_priv_ptr;
	
	bmap += info->_priv_var;
	x = info->width;
	info->_priv_var += x * 3;
	do
	{
		*buffer++ = *bmap++;
		*buffer++ = *bmap++;
		*buffer++ = *bmap++;
	} while (--x > 0);
	RETURN_SUCCESS();
}


void __CDECL reader_quit(IMGINFO info)
{
	free(info->_priv_ptr);
	info->_priv_ptr = NULL;
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
