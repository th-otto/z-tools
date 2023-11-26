#include "plugin.h"
#include "zvplugin.h"
#include "symbols.h"

#define VERSION		0x0116
#define AUTHOR      "Lonny Pursell, Thorsten Otto"
#define NAME        "Spectrum 512"
#define DATE        __DATE__ " " __TIME__

#define SP_ID   0x5350
#define SS_ID   0x5353

#define DATASIZE 32000L
#define PALETTESIZE (199 * 16 * 3 * 2L)
#define FILESIZE (DATASIZE + PALETTESIZE)

extern void __CDECL decode_spectrum1(uint8_t *src, uint8_t *dst, uint16_t *palettes) ASM_NAME("decode_spectrum1");
extern void __CDECL decode_spectrum2(uint8_t *src, uint8_t *dst, uint16_t *palettes) ASM_NAME("decode_spectrum2");

static uint8_t const multtab[32 + 16] = {
	0x00, 0x08, 0x10, 0x18, 0x20, 0x29, 0x31, 0x39,
	0x41, 0x4a, 0x52, 0x5a, 0x62, 0x6a, 0x73, 0x7b,
	0x83, 0x8b, 0x94, 0x9c, 0xa4, 0xac, 0xb4, 0xbd,
	0xc5, 0xcd, 0xd5, 0xde, 0xe6, 0xee, 0xf6, 0xff,

	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("SPU\0" "SPC\0" "SPS\0");

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
	}
	return -ENOSYS;
}
#endif



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
	if ((x >= x1) && (x < (x1 + 160)))
	{
		c = c + 16;
	} else if (x >= (x1 + 160))
	{
		c = c + 32;
	}
	return c;
}


static void my_strupr(char *s)
{
	while (*s)
	{
		if (*s >= 'a' && *s <= 'z')
			*s -= 'a' - 'A';
		s++;
	}
}


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	char extension[4];
	uint32_t filesize;
	int i;
	uint8_t *temp;
	int16_t handle;
	uint8_t *bmap;
	uint16_t *palettes;
	short enhanced;

	strcpy(extension, name + strlen(name) - 3);
	my_strupr(extension);
	if ((handle = (int16_t)Fopen(name, FO_READ)) < 0)
		return FALSE;

	filesize = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);					/* reset */
	if (filesize > FILESIZE)
	{
		Fclose(handle);
		return FALSE;
	}
	bmap = malloc(DATASIZE + 256 + 200 * 16 * 3 * sizeof(*palettes));
	if (bmap == NULL)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	palettes = (uint16_t *)(bmap + DATASIZE + 256);
	/* clear palette for first scanline, which is always black */
	for (i = 0; i < 48; i++)
		palettes[i] = 0;

	if (strcmp(extension, "SPU") == 0)
	{
		if (filesize != FILESIZE)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		if (Fread(handle, DATASIZE, bmap) != DATASIZE ||
			Fread(handle, PALETTESIZE, palettes + 48) != PALETTESIZE)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		strcpy(info->compression, "None");
	} else if (strcmp(extension, "SPC") == 0)
	{
		temp = malloc(FILESIZE);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		if (Fread(handle, filesize, temp) != (long)filesize)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		decode_spectrum1(temp, bmap, palettes + 48);
		free(temp);
		strcpy(info->compression, "RLE");
	} else if (strcmp(extension, "SPS") == 0)
	{
		temp = malloc(FILESIZE);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		if (Fread(handle, filesize, temp) != (long)filesize)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		decode_spectrum2(temp, bmap, palettes + 48);
		free(temp);
		strcpy(info->compression, "RLE+");
	}
	Fclose(handle);
	enhanced = 0;
	if (*((uint16_t *)bmap) != 0)
	{
		if (*((uint32_t *)bmap) == 0x35424954UL)
			enhanced = 1;
	}

	info->planes = 12;
	info->width = 320;
	info->height = 200;
	info->components = 3;
	info->indexed_color = FALSE;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	strcpy(info->info, "Spectrum 512");
	if (enhanced)
	{
		strcat(info->info, " (5-Bit RGB)");
		info->planes = 15;
	}
	info->colors = 1L << info->planes;

	info->_priv_var = 0;
	info->_priv_var_more = 0;
	info->_priv_ptr = bmap;
	info->_priv_ptr_more = palettes;
	info->__priv_ptr_more = (void *)(long)enhanced;

	return TRUE;
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	uint16_t *palettes = (uint16_t *)info->_priv_ptr_more;
	short enhanced = (short)(long)info->__priv_ptr_more;
	unsigned short c;
	short bit;
	size_t offset;
	short x;
	short i;

	x = 0;
	offset = 0;
	do
	{
		for (bit = 15; bit >= 0; bit--)
		{
			for (i = c = 0; i < 4; i++)
			{
				if ((((uint16_t *)(bmap + (info->_priv_var + i) * 2))[0] >> bit) & 1)
					c |= 1 << i;
			}
			c = find_index(x++, c);
			c = palettes[info->_priv_var_more * 48 + c];
			if (enhanced)
			{
				int r;

				r = (c >> 7) & 0x0e;
				r += (c >> 11) & 1;
				r <<= 1;
				r += (c >> 15) & 1;
				buffer[offset++] = multtab[r];
				r = (c >> 3) & 0x0e;
				r += (c >> 7) & 1;
				r <<= 1;
				r += (c >> 14) & 1;
				buffer[offset++] = multtab[r];
				r = (c << 1) & 0x0e;
				r += (c >> 3) & 1;
				r <<= 1;
				r += (c >> 13) & 1;
				buffer[offset++] = multtab[r];
			} else
			{
				int r;

				r = (c >> 7) & 0x0e;
				r += (c >> 11) & 1;
				buffer[offset++] = multtab[32 + r];
				r = (c >> 3) & 0x0e;
				r += (c >> 7) & 1;
				buffer[offset++] = multtab[32 + r];
				r = (c << 1) & 0x0e;
				r += (c >> 3) & 1;
				buffer[offset++] = multtab[32 + r];
			}
		}

		info->_priv_var += 4;
	} while (x < info->width);

	info->_priv_var_more += 1;

	return TRUE;
}


void __CDECL reader_quit(IMGINFO info)
{
	free(info->_priv_ptr);
	info->_priv_ptr = NULL;
	info->_priv_ptr_more = NULL;
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
