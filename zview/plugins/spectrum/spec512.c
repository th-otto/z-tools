#include "plugin.h"
#include "zvplugin.h"
#include "symbols.h"

#define VERSION		0x0116
#define AUTHOR      "Lonny Pursell, Thorsten Otto"
#define NAME        "Spectrum 512"
#define DATE        __DATE__ " " __TIME__
#define MISC_INFO   "Some code by Hans Wessels"

/*
############################################################################
Spectrum 512    *.SPU

80 words       first scan line of picture (unused) -- should be zeroes
15920 words    picture data (screen memory) for scan lines 1 through 199
9552 words     3 palettes for each scan line (the top scan line is not
               included because Spectrum 512 can't display it)
-----------
51104 bytes     total

Note that the Spectrum 512 mode's three palette changes per scan line allow
more colors on the screen than normally possible, but a tremendous amount of
CPU time is required to maintain the image.

The Spectrum format specifies a palette of 48 colors for each scan line. To
decode a Spectrum picture, one must be know which of these 48 colors are in
effect for a given horizontal pixel position.

############################################################################

Spectrum 512 Enhanced    *.SPU

Spectrum 512 images with a 15-bit RGB palette.
Created by Sascha Springer: http://blog.anides.de/

These are identical to Spectrum 512 uncompressed images with two exceptions.
The very first long of the image data will be the file id: '5BIT'

Spectrum 512 can't display the first scan line anyway, so the ID should cause
no issues.

The upper 3 bits of a palette word are used to provide a 5th bit of RGB. Thus:
  red:  0xxx1432xxxxxxxx
  green x0xxxxxx1432xxxx
  blue: xx0xxxxxxxxx1432

These look like normal Spectrum 512 images by old viewers when the upper 3 bits
are not used.

As described by the creator:
The new least significant bit for each channel are now stored in the upper
nibble of the color word and so the image .SPU format itself is kept completely
"backwards" compatible.

See also Spectrum 512 file format

############################################################################

Spectrum 512 (Compressed)    *.SPC

1 word            file ID, 'SP' ($5350)
1 word            reserved for future use [always 0]
1 long            length of data bit map
1 long            length of color bit map
<= 32092 bytes    compressed data bit map
<= 17910 bytes    compressed color bit map
--------------
<= 50014 bytes    total

Data compression:

Compression is via a modified run length encoding (RLE) scheme, similar to
DEGAS compressed and Tiny. The data map is stored as a sequence of records.
Each record consists of a header byte followed by one or more data bytes. The
meaning of the header byte is as follows:

        For a given header byte, x:

           0 <= x <= 127   Use the next x + 1 bytes literally (no repetition)
        -128 <= x <=  -1   Use the next byte -x + 2 times

The data appears in the following order:

        1. Picture data, bit plane 0, scan lines 1 - 199
        2. Picture data, bit plane 1, scan lines 1 - 199
        3. Picture data, bit plane 2, scan lines 1 - 199
        4. Picture data, bit plane 3, scan lines 1 - 199

Decompression of data ends when 31840 data bytes have been used.

Color map compression:

Each 16-word palette is compressed separately. There are three palettes for
each scan line (597 total). The color map is stored as a sequence of records.
Each record starts with a 1-word bit vector which specifies which of the 16
palette entries are included in the data following the bit vector (1 =
included, 0 = not included). If a palette entry is not included, it is assumed
to be zero (black). The least significant bit of the bit vector refers to
palette entry zero, while the most significant bit refers to palette entry 15.
Bit 15 must be zero, since Spectrum 512 does not use palette entry 15. Bit 0
should also be zero, since Spectrum 512 always makes the background color black.

The words specifying the values for the palette entries indicated in the bit
vector follow the bit vector itself, in order (0 - 15).

NOTE:   Regarding Spectrum pictures, Shamus McBride reports the following:

        "... [The Picture Formats List] says bit 15 of the color map vector
        must be zero. I've encountered quite a few files where [bit 15] is set
        (with no associated palette entry)..."

See also Spectrum 512 file format

############################################################################

Spectrum 512 (Smooshed)    *.SPS

This format compresses Spectrum 512 pictures better than the standard method.
There are at least two programs that support this format, SPSLIDEX and ANISPEC,
although the two seem to differ slightly in their interpretation of the format.

One point of interest: Shamus McBride deciphered this format without an ST!

1 word            file ID, 'SP' ($5350)
1 word            0 (reserved for future use)
1 long            length of data bit map
1 long            length of color bit map
<= 32092 bytes    compressed data bit map
<= 17910 bytes    compressed color bit map
--------------
< 50014  bytes    total

Data compression:

Compression is via a modified run length encoding (RLE) scheme, similar to that
used in Spectrum Compressed (*.SPC) images.

The data map is stored as a sequence of records. Each record consists of a
header byte followed by one or more data bytes. The meaning of the header byte
is as follows:

        For a given header byte, x (unsigned):

          0 <= x <= 127    Use the next byte x + 3 times
        128 <= x <= 255    Use the next x - 128 + 1 bytes literally
                           (no repetition)

There are two kinds of *.SPS files. The type is defined by the least
significant bit of the last byte in the color bit map.

If the bit is set the data appears in the same order as *.SPC files:

        1. Picture data, bit plane 0, scan lines 1 - 199
        2. Picture data, bit plane 1, scan lines 1 - 199
        3. Picture data, bit plane 2, scan lines 1 - 199
        4. Picture data, bit plane 3, scan lines 1 - 199

If the bit is not set the data is encoded as byte wide vertical strips:

        Picture data, bit plane 0, scan line   1, MSB.
        Picture data, bit plane 0, scan line   2, MSB.
        Picture data, bit plane 0, scan line   3, MSB.
        . . .
        Picture data, bit plane 0, scan line 199, MSB.
        Picture data, bit plane 0, scan line   1, LSB.
        Picture data, bit plane 0, scan line   2, LSB.
        . . .
        Picture data, bit plane 0, scan line 199, LSB.
        Picture data, bit plane 1, scan line   1, MSB.
        . . .
        Picture data, bit plane 3, scan line 198, LSB
        Picture data, bit plane 3, scan line 199, LSB.

A for loop to process that data would look like

        for (plane = 0; plane < 4; plane++)
            for (x = 0; x < 320; x += 8)
                for (y = 1; y < 200; y++)
                    for (x1 = 0; x1 < 8; x1++)
                        image[y, x + x1] = ...

Color map compression:

Color map compression is similar to *.SPC color map compression, but the map is
compressed as a string of bits, rather than words. There are 597 records (one
for each palette). Each record is composed of a 14-bit header followed by a
number of 9-bit palette entries. The 14-bit header specifies which palette
entries follow (1 = included, 0 = not included). The most significant bit of
the header refers to palette entry 1, while the least significant bit refers to
palette 14. Palette entries 0 and 15 are forced to black (zero). Each palette
entry is encoded as "rrrgggbbb".

The format of the palette is described above in the section on uncompressed
Spectrum pictures (*.SPU).

See also Spectrum 512 file format

*/

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
	case INFO_MISC:
		return (long)MISC_INFO;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif



/*
 * Given an x-coordinate (from 0 to 319) and a color index (from 0 to 15)
 * returns the corresponding Spectrum palette index (from 0 to 47).
 * by Steve Belczyk; placed in the public domain December, 1990.
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
