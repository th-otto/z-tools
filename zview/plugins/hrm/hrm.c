#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
HighresMedium    *.HRM

This format is found in only one demo: "HighResMode" by Paradox. The pictures
are interlaced in medium resolution with 35 colors per line. The files are
packed with Ice.

The unpacked format is as follows:
64000 bytes      screen data, 160 bytes per scanline, 400 lines
28000 bytes      palette data, 70 bytes per line
-----------
96000 bytes
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


static uint8_t const colortable[] = {
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
	0xFF,								/* 1111 */
};


/*
 *  Given an x-coordinate and a color index, returns the corresponding
 *  HighResMedium palette index.
 *
 *  by Hans Wessels; placed in the public domain January, 2008.
 */
static int find_hrm_index(int x, int c)
{
	x += 80;
	if (c == 0)
		return -1 + 4 * ((x + 0) / 80);
	if (c == 1)
		return 4 * ((x - 8) / 80);
	if (c == 2)
		return 1 + 4 * ((x - 40) / 80);
	return 2 + 4 * ((x - 48) / 80);
}


static uint8_t *convert_hrm(uint8_t *gfx)
{
	uint8_t *data;

	data = malloc(640L * 400L * 3);
	if (data != NULL)
	{
		int32_t y;
		uint8_t *p = data;

		for (y = 0; y < 400; y++)
		{
			uint16_t *q = (uint16_t *)(gfx + y * 160);
			uint16_t *cp = (uint16_t *)(gfx + 64000L + y * 70);
			int x;

			for (x = 0; x < 40; x++)
			{
				uint16_t plane0;
				uint16_t plane1;
				int i;

				plane0 = *q++;
				plane1 = *q++;
				for (i = 0; i < 16; i++)
				{
					int16_t color;

					color = ((plane1 >> 14) & 2);
					color += ((plane0 >> 15) & 1);
					plane0 <<= 1;
					plane1 <<= 1;
					color = find_hrm_index(x * 16 + i, color);
					color = cp[color];
					*p++ = colortable[(color >> 8) & 0xf];
					*p++ = colortable[(color >> 4) & 0xf];
					*p++ = colortable[color & 0xf];
				}
			}
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
	size_t file_size;
	int16_t handle;
	uint8_t *bmap;
	uint8_t *temp;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (file_size != 92000L)
	{
		Fclose(handle);
		return FALSE;
	}

	temp = malloc(file_size);
	if (temp == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	file_size = Fread(handle, file_size, temp);
	Fclose(handle);
	if (file_size != 92000L)
	{
		free(temp);
		return FALSE;
	}

	bmap = convert_hrm(temp);
	if (bmap == NULL)
	{
		free(temp);
		return FALSE;
	}
	free(temp);
	
	info->planes = 24;
	info->colors = 1L << 24;
	info->width = 640;
	info->height = 400;
	info->indexed_color = FALSE;
	info->components = 3;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "HighResMode");
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
	
	memcpy(buffer, &bmap[info->_priv_var], 640 * 3);
	info->_priv_var += 640 * 3;

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
