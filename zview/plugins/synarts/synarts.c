#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
Synthetic Arts    *.SRT (ST medium resolution)

This description is only relavant to Synthetic Arts version 1 as later versions
abandoned ST medium resolution in favor of ST low resolution and existing image
formats.

32000 bytes    image data (screen memory)
1 long         file id 'JHSy'
1 word         always (1), possibly version or mode code?
16 words       palette (xbios format)
-----------
32038 total    total for file
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

static uint16_t const medpal[4] = {
	0x0fff, 0x0f00, 0x00f0, 0x0000
};
static uint16_t const lowpal[16] = {
	0x0fff, 0x0f00, 0x00f0, 0x0ff0,
	0x000f, 0x0f0f, 0x00ff, 0x0555,
	0x0333, 0x0f33, 0x03f3, 0x0ff3,
	0x033f, 0x0f3f, 0x03ff, 0x0000
};

struct file_header {
	uint32_t magic;
	uint16_t resolution;
	uint16_t palette[16];
};


/* code from atarist.c, app code St2BMP (placed into public domain) */
/* written by Hans Wessels */

/*
 * guess resolution from picturedata, 0=low, 1=med, 2=high
 *
 * routine guesses resolution by converting the bitplane pixels
 * to chunky pixels in a byte, just enough to fil a byte (8 
 * pixels for high, 4 for medium and 2 for lowres). And count
 * the number of runlength bytes in each mode. The correct
 * resolution is the one having the most runlength bytes.
 * If there is a draw, mono wins, then low res, medium res is
 * never chosen in a draw.
 */
static int guess_resolution(const uint8_t *buffer)
{
	const uint8_t *scan;
	int16_t reslow;
	int16_t resmed;
	int16_t resmono;
	
	reslow = 0;
	resmed = 0;
	resmono = 0;

	/* test high */
	{
		int16_t i;
		uint8_t last_byte;

		last_byte = 0;
		for (scan = buffer, i = 0; i < SCREEN_SIZE; i++)
		{
			uint8_t byte = *scan++;
			if (byte == last_byte)
				resmono++;
			else
				last_byte = byte;
		}
	}

	/* test med */
	{
		int16_t i;
		int16_t k;
		uint8_t last_byte;
		uint16_t word;
		uint16_t word2;
		uint16_t mask;
		uint8_t byte;
		
		last_byte = 0;
		for (scan = buffer, i = 0; i < SCREEN_SIZE / 4; i++)
		{
			word = *scan++;
			mask = 0x8000;
			word <<= 8;
			word += *scan++;
			word2 = *scan++;
			word2 <<= 8;
			word2 += *scan++;
			do {
				byte = 0;
				for (k = 0; k < 4; k++)
				{
					byte += byte;
					if (word & mask)
						byte++;
					byte += byte;
					if (word2 & mask)
						byte++;
					mask >>= 1;
				}
				if (byte == last_byte)
					resmed++;
				else
					last_byte = byte;
			} while (mask != 0);
		}
	}
	
	/* test low */
	{
		int16_t i;
		int16_t k;
		uint8_t last_byte;
		uint16_t word;
		uint16_t word2;
		uint16_t word3;
		uint16_t word4;
		uint16_t mask;
		uint8_t byte;
		
		last_byte = 0;
		for (scan = buffer, i = 0; i < SCREEN_SIZE / 8; i++)
		{
			word = *scan++;
			mask = 0x8000;
			word <<= 8;
			word += *scan++;
			word2 = *scan++;
			word2 <<= 8;
			word2 += *scan++;
			word3 = *scan++;
			word3 <<= 8;
			word3 += *scan++;
			word4 = *scan++;
			word4 <<= 8;
			word4 += *scan++;
			do {
				byte = 0;
				for (k = 0; k < 2; k++)
				{
					byte += byte;
					if (word & mask)
						byte++;
					byte += byte;
					if (word2 & mask)
						byte++;
					byte += byte;
					if (word3 & mask)
						byte++;
					byte += byte;
					if (word4 & mask)
						byte++;
					mask >>= 1;
				}
				if (byte == last_byte)
					reslow++;
				else
					last_byte = byte;
			} while (mask != 0);
		}
	}

	if (resmono >= reslow && resmono >= resmed)
		return 2;
	if (resmed > reslow)
		return 1;
	return 0;
}


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	int16_t handle;
	uint8_t *bmap;
	size_t file_size;
	struct file_header header;
	int resolution;

	if ((handle = (int16_t)Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	bmap = malloc(SCREEN_SIZE);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}

	if (file_size == SCREEN_SIZE + sizeof(header))
	{
		resolution = 1;
		if (Fread(handle, SCREEN_SIZE, bmap) != SCREEN_SIZE ||
			Fread(handle, sizeof(header), &header) != sizeof(header))
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_FileLength);
		}
		if (header.magic != 0x4A485379L) /* 'JHSy' */
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_FileId);
		}
	} else if (file_size == SCREEN_SIZE)
	{
		long shift;
		int i;

		if (Fread(handle, SCREEN_SIZE, bmap) != SCREEN_SIZE)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_FileLength);
		}
		Fread(handle, SCREEN_SIZE, bmap);
		resolution = guess_resolution(bmap);
		shift = Kbshift(-1);
		if (shift & K_ALT)
			resolution = 0;
		else if (shift & K_CTRL)
			resolution = 1;
		else if (shift & K_LSHIFT)
			resolution = 2;
		if (resolution == 0)
		{
			for (i = 0; i < 16; i++)
				header.palette[i] = lowpal[i];
		} else if (resolution == 1)
		{
			for (i = 0; i < 4; i++)
				header.palette[i] = medpal[i];
		}
	} else
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_FileLength);
	}
	Fclose(handle);
	
	info->planes = 4 >> resolution;
	info->colors = 1L << info->planes;
	switch (info->planes)
	{
	case 4:
		info->width = 320;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 2:
		info->width = 640;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 1:
		info->width = 640;
		info->height = 400;
		info->indexed_color = FALSE;
		break;
	}

	if (info->indexed_color)
	{	
		int i;

		for (i = 0; i < 16; i++)
		{
			info->palette[i].red = (((header.palette[i] >> 7) & 0x0e) + ((header.palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((header.palette[i] >> 3) & 0x0e) + ((header.palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((header.palette[i] << 1) & 0x0e) + ((header.palette[i] >> 3) & 0x01)) * 17;
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

	strcpy(info->info, "Synthetic Arts");
	strcpy(info->compression, "None");

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
