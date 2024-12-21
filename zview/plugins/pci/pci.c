#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
Overscan Interlaced    *.PCI (ST low resolution)

This format is found in only one demo: "Tobias Richter Fullscreen Slideshow".
The pictures are in an interlaced overscan format and use a separate palette
per scan line (16 colors). The resolution of the screen data is 352 x 278
pixels, with 176 words per line. The files are packed with the ICE.

The unpacked format is as follows:

48928 bytes     screen data first screen
48928 bytes     screen data second screen
8896 bytes      color data first screen
8896 bytes      color data second screen
------------
115648 bytes    total

The screen data is stored in 4 seperate bitplane blocks: first all data of
bitplane 0, then all data of bitplane 1, then the data of bitplane 2 and last
the data of bitplane 3.
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


#define SCREEN_W 352
#define SCREEN_H 278
#define SCREEN_SIZE ((long) SCREEN_W * SCREEN_H / 8 * 4)
#define PALETTE_SIZE (SCREEN_H * 16 * 2)
#define PLANE_SIZE ((long)SCREEN_W / 8 * SCREEN_H)
#define FILE_SIZE (2 * SCREEN_SIZE + 2 * PALETTE_SIZE)
#define BMAP_SIZE ((long) SCREEN_W * SCREEN_H * 3)


/*
 * pci functions ported from the IrfanView module
 * atarist.c from Hans Wessels
 */

static void reorder_bitplanes(uint8_t *dst, uint8_t *src, long offset)
{
	uint8_t *plane0 = src;
	uint8_t *plane1 = plane0 + offset;
	uint8_t *plane2 = plane1 + offset;
	uint8_t *plane3 = plane2 + offset;
	
	offset >>= 1;
	while (offset > 0)
	{
		*dst++ = *plane0++;
		*dst++ = *plane0++;
		*dst++ = *plane1++;
		*dst++ = *plane1++;
		*dst++ = *plane2++;
		*dst++ = *plane2++;
		*dst++ = *plane3++;
		*dst++ = *plane3++;
		offset--;
	}
}


static uint8_t *convert_pci(uint8_t *gfx0, uint8_t *pal0, uint8_t *gfx1, uint8_t *pal1)
{
	uint8_t *data;

	data = malloc(BMAP_SIZE);
	if (data != NULL)
	{
		uint8_t *p = data;
		long y;

		for (y = 1; y < SCREEN_H; y++)
		{
			uint8_t *q0 = gfx0 + y * 176;
			uint8_t *cp0 = pal0 + y * 32;
			uint8_t *q1 = gfx1 + y * 176;
			uint8_t *cp1 = pal1 + y * 32;
			int x;

			for (x = 0; x < SCREEN_W / 16; x++)
			{
				uint16_t plane00;
				uint16_t plane01;
				uint16_t plane02;
				uint16_t plane03;
				uint16_t plane10;
				uint16_t plane11;
				uint16_t plane12;
				uint16_t plane13;
				int i;

				plane00 = *q0++;
				plane00 <<= 8;
				plane00 += *q0++;
				plane01 = *q0++;
				plane01 <<= 8;
				plane01 += *q0++;
				plane02 = *q0++;
				plane02 <<= 8;
				plane02 += *q0++;
				plane03 = *q0++;
				plane03 <<= 8;
				plane03 += *q0++;
				plane10 = *q1++;
				plane10 <<= 8;
				plane10 += *q1++;
				plane11 = *q1++;
				plane11 <<= 8;
				plane11 += *q1++;
				plane12 = *q1++;
				plane12 <<= 8;
				plane12 += *q1++;
				plane13 = *q1++;
				plane13 <<= 8;
				plane13 += *q1++;
				for (i = 0; i < 16; i++)
				{
					uint16_t data0;
					uint16_t data1;

					data0 = ((plane03 >> 12) & 8);
					data0 += ((plane02 >> 13) & 4);
					data0 += ((plane01 >> 14) & 2);
					data0 += ((plane00 >> 15) & 1);
					data1 = ((plane13 >> 12) & 8);
					data1 += ((plane12 >> 13) & 4);
					data1 += ((plane11 >> 14) & 2);
					data1 += ((plane10 >> 15) & 1);
					plane00 <<= 1;
					plane01 <<= 1;
					plane02 <<= 1;
					plane03 <<= 1;
					plane10 <<= 1;
					plane11 <<= 1;
					plane12 <<= 1;
					plane13 <<= 1;
					data0 = (cp0[data0 * 2] << 8) + cp0[data0 * 2 + 1];
					data1 = (cp1[data1 * 2] << 8) + cp1[data1 * 2 + 1];
					*p++ = ((colortable[(data0 >> 8) & 0xf] & 0xff) + (colortable[(data1 >> 8) & 0xf] & 0xff)) / 2;
					*p++ = ((colortable[(data0 >> 4) & 0xf] & 0xff) + (colortable[(data1 >> 4) & 0xf] & 0xff)) / 2;
					*p++ = ((colortable[data0 & 0xf] & 0xff) + (colortable[data1 & 0xf] & 0xff)) / 2;
				}
			}
		}
		/* 1st line is black */
		for (y = 0; y < 960; y++)
		{
			*p++ = 0;
		}
	}
	return data;
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
	int16_t handle;
	uint8_t *bmap;
	uint8_t *file;
	uint8_t *screen;
	uint8_t *scr0;
	uint8_t *scr1;
	uint8_t *pal0;
	uint8_t *pal1;
	size_t file_size;
	size_t nread;

	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (file_size != FILE_SIZE)
	{
		Fclose(handle);
		return FALSE;
	}
	
	file = malloc(214000L); /* 116000 packed data, 2x176x278 for screen data */
	if (file == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	nread = Fread(handle, FILE_SIZE, file);
	Fclose(handle);
	if (nread != FILE_SIZE)
	{
		free(file);
		return FALSE;
	}
	scr0 = file + FILE_SIZE + SCREEN_W;
	scr1 = file + FILE_SIZE + SCREEN_W + 49000L;
	pal0 = file + 2 * SCREEN_SIZE;
	pal1 = pal0 + PALETTE_SIZE;
	screen = file;
	reorder_bitplanes(scr0, screen, PLANE_SIZE);
	screen += SCREEN_SIZE;
	reorder_bitplanes(scr1, screen, PLANE_SIZE);
	bmap = convert_pci(scr0, pal0, scr1, pal1);
	if (bmap == NULL)
	{
		free(file);
		return FALSE;
	}
	
	free(file);

	info->planes = 24;
	info->width = SCREEN_W;
	info->height = SCREEN_H;
	info->components = 3;
	info->indexed_color = FALSE;
	info->colors = 1L << 18;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "Overscan Interlaced");
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
	uint8_t *bmap = info->_priv_ptr;

	memcpy(buffer, bmap + info->_priv_var, SCREEN_W * 3);
	info->_priv_var += SCREEN_W * 3;
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
