#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
Tiny STuff   *.TNY (any resolution)
             *.TN1 (st low resolution)
             *.TN2 (st medium resolution)
             *.TN3 (st high resolution)

Several people have reported sightings of mutated Tiny pictures that do not
follow the standard format, so let's be careful out there. What is described
here is the format that David Mumper's original TNYSTUFF.PRG produces. The TN4
extension has been found on animated low resolution files. One can assume the
same person(s) will also use TN5 and TN6 for extensions for animated medium and
high resolution.

1 byte          resolution (same as NEO, but +3 indicates color rotation
                information also follows)

If resolution > 2 {
1 byte          left and right color animation limits. High 4 bits hold left
                (start) limit; low 4 bits hold right (end) limit
1 byte          direction and speed of color animation (negative value
                indicates left, positive indicates right, absolute value is
                delay in 1/60's of a second.
1 word          color rotation duration (number of iterations)
}

16 words        palette
1 word          number of control bytes
1 word          number of data words
3-10667 bytes   control bytes
1-16000 words   data words
-------------
42-32044 bytes  total

Control byte meanings:

    For a given control byte, x:

    x < 0   Copy -x of unique words to take from the data section
            (from 1 to 128)
    x = 0   1 word is taken from the control section which specifies the number
            of times to repeat the next data word (from 128 to 32767)
    x = 1   1 word is taken from the control section which specifies the number
            of unique words to be taken from the data section
            (from 128 - 32767)
    x > 1   Specifies the number of times to repeat the next word taken from
            the data section (from 2 to 127)

Format of expanded data:

The expanded data is not simply screen memory bitmap data; instead, the data is
divided into four sets of vertical columns. (This results in better
compression.) A column consists of one specific word taken from each scan line,
going from top to bottom. For example, column 1 consists of word 1 on scanline
1 followed by word 1 on scanline 2, etc., followed by word 1 on scanline 200.

   The columns appear in the following order:

   1st set contains columns 1, 5,  9, 13, ..., 69, 73, 77 in order
   2nd set contains columns 2, 6, 10, 14, ..., 70, 74, 78 in order
   3rd set contains columns 3, 7, 11, 15, ..., 71, 75, 79 in order
   4th set contains columns 4, 8, 12, 16, ..., 72, 76, 80 in order

Note that Tiny partitions the screen this way regardless of resolution; i.e.,
these aren't bitplanes. For example, medium resolution only has two bitplanes,
but Tiny still divides medium resolution pictures into four parts.
*/


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)(EXTENSIONS);

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

#define SCREEN_SIZE 32000L
#define SCREEN_W 320
#define SCREEN_H 200


/* 320 x 200, 4 bitplanes */
static void reorder_column_bitplanes(uint8_t *_dst, uint8_t *_src)
{
	int i;
	uint16_t *src = (uint16_t *)_src;
	uint16_t *dst = (uint16_t *)_dst;
	
	for (i = 0; i < SCREEN_H; i++)
	{
		int j;

		for (j = 0; j < SCREEN_W / 16; j++)
		{
			*dst++ = src[j * SCREEN_H + 0 * SCREEN_H * (SCREEN_W / 16)];
			*dst++ = src[j * SCREEN_H + 1 * SCREEN_H * (SCREEN_W / 16)];
			*dst++ = src[j * SCREEN_H + 2 * SCREEN_H * (SCREEN_W / 16)];
			*dst++ = src[j * SCREEN_H + 3 * SCREEN_H * (SCREEN_W / 16)];
		}
		src++;
	}
}


static void decode_tiny(uint8_t *dst, uint8_t *src, long cnt)
{
	uint16_t cmds;
	uint8_t *p;

	cmds = *src++;
	cmds <<= 8;
	cmds += *src++;
	src += 2;
	p = src + cmds;
	cnt >>= 1;
	while (cnt > 0 && cmds > 0)
	{
		uint8_t cmd = *src++;

		cmds--;
		if (cmd == 0)
		{
			uint8_t data0;
			uint8_t data1;
			uint16_t tmp;

			tmp = *src++;
			tmp <<= 8;
			tmp += *src++;
			cmds -= 2;
			data0 = *p++;
			data1 = *p++;
			if (tmp > cnt)
			{
				tmp = (uint16_t) cnt;
			}
			cnt -= tmp;
			while (tmp > 0)
			{
				*dst++ = data0;
				*dst++ = data1;
				tmp--;
			}
		} else if (cmd == 1)
		{
			uint16_t tmp;

			tmp = *src++;
			tmp <<= 8;
			tmp += *src++;
			cmds -= 2;
			if (tmp > cnt)
			{
				tmp = (uint16_t) cnt;
			}
			cnt -= tmp;
			while (tmp > 0)
			{
				*dst++ = *p++;
				*dst++ = *p++;
				tmp--;
			}
		} else if ((cmd & 0x80) == 0x80)
		{
			int i = 128 - (cmd & 0x7f);

			if (i > cnt)
			{
				i = (int) cnt;
			}
			cnt -= i;
			while (i > 0)
			{
				*dst++ = *p++;
				*dst++ = *p++;
				i--;
			}
		} else
		{
			int i = cmd & 0x7f;
			uint8_t data0;
			uint8_t data1;

			data0 = *p++;
			data1 = *p++;
			if (i > cnt)
			{
				i = (int) cnt;
			}
			cnt -= i;
			while (i > 0)
			{
				*dst++ = data0;
				*dst++ = data1;
				i--;
			}
		}
	}
	if (cnt != 0)
	{
		memset(dst, 0, cnt * 2);
	}
}



boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	int16_t handle;
	uint8_t *bmap;
	uint8_t *file;
	size_t file_size;
	uint16_t palette[16];
	uint8_t resolution;

	if ((handle = (int16_t)Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (Fread(handle, sizeof(resolution), &resolution) != sizeof(resolution))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	file_size -= 1;
	if (resolution > 5)
	{
		Fclose(handle);
		RETURN_ERROR(EC_ResolutionType);
	}
	if (resolution > 2)
	{
		resolution -= 3;
		/* skip animation info (not supported currently) */
		Fseek(4, handle, SEEK_CUR);
		file_size -= 4;
	}
	if (Fread(handle, sizeof(palette), palette) != sizeof(palette))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	file_size -= sizeof(palette);
	if (file_size > SCREEN_SIZE)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileLength);
	}
		
	bmap = malloc(SCREEN_SIZE * 2);
	if (bmap == NULL)
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	file = bmap + SCREEN_SIZE;

	if ((size_t)Fread(handle, file_size, bmap) != file_size)
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);
	
	decode_tiny(file, bmap, SCREEN_SIZE);
	reorder_column_bitplanes(bmap, file);
	
	switch (resolution)
	{
	case 0:
		info->planes = 4;
		info->width = 320;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 1:
		info->planes = 2;
		info->width = 640;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 2:
		info->planes = 1;
		info->width = 640;
		info->height = 400;
		info->indexed_color = FALSE;
		break;
	}
	info->colors = 1L << info->planes;

	{	
		int i;

		for (i = 0; i < 16; i++)
		{
			info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((palette[i] << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 17;
		}
	}

	info->components = 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */

	info->_priv_var = 0;
	info->_priv_ptr = bmap;

	strcpy(info->info, "Tiny Stuff");
	strcpy(info->compression, "RLE");

	RETURN_SUCCESS();
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	switch (info->planes)
	{
	case 1:
		{
			uint8_t *bmap = info->_priv_ptr;
			int16_t byte;
			int x;
			
			bmap += info->_priv_var;
			x = info->width >> 3;
			info->_priv_var += x;
			do
			{
				byte = *bmap++;
				*buffer++ = (byte >> 7) & 1;
				*buffer++ = (byte >> 6) & 1;
				*buffer++ = (byte >> 5) & 1;
				*buffer++ = (byte >> 4) & 1;
				*buffer++ = (byte >> 3) & 1;
				*buffer++ = (byte >> 2) & 1;
				*buffer++ = (byte >> 1) & 1;
				*buffer++ = (byte >> 0) & 1;
			} while (--x > 0);
		}
		break;

	case 2:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t *ptr;
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			x = info->width >> 4;
			info->_priv_var += x << 2;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2);
					plane0 <<= 1;
					plane1 <<= 1;
				}
			} while (--x > 0);
		}
		break;

	case 4:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t *ptr;
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			x = info->width >> 4;
			info->_priv_var += x << 3;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				plane2 = *ptr++;
				plane3 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4) |
						((plane3 >> 12) & 8);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
				}
			} while (--x > 0);
		}
		break;
	}

	RETURN_SUCCESS();
}


void __CDECL reader_quit(IMGINFO info)
{
	void *p = info->_priv_ptr;
	info->_priv_ptr = NULL;
	free(p);
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
