#define	VERSION	     0x0103
#define NAME        "ColorBurst II"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
ColorBurst II    *.BST

16*200 words	 palette, 1 palette per scan line
                   rgb bits are not in compatibility order (0321), instead 3210
1 word           flags bits: vvvvvvvvvvvvvvvm
                   v=version? 5=v1.2 10=v1.3
                   m=mode: 0=st low, 1=st medium
???? bytes       compressed image data

Versions 1.2 & 1.3 appear to have different compression schemes.
Currently there is no description of the compression methods.
*/


#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("BST\0");

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif

#define SCREEN_SIZE 32000u
#define SCREEN_HEIGHT 200


#define PALETTE_SIZE (SCREEN_HEIGHT * 16 * sizeof(*palettes))

#if 0
void __CDECL bst_decompress(const uint32_t *source, uint32_t *screen, int16_t *my_control) ASM_NAME("bst_decompress");
#else

static void bst_decompress(const uint32_t *source, uint32_t *screen)
{
	int16_t off;
	const int16_t *my_control;
	int cntrl_max;
	int count;

	my_control = (const int16_t *) source;
	if (*my_control >= 30)
		return;							/* punt, if unknown version     */
	cntrl_max = *(my_control + 1);			/* get number of control words  */
	cntrl_max -= 2;
	off = cntrl_max / 2 + 1;
	source += off;
	my_control += 2;
	while (--cntrl_max >= 0)
	{
		count = *my_control++;
		if (count < 0)			/* deal with compressed stuff */
		{
			count = -count;
			while (--count >= 0)
				*screen++ = *source;
			source++;
		} else							/* deal with raw stuff */
		{
			while (--count >= 0)
				*screen++ = *source++;
		}
	}
}
#endif



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
	int resolution;
	int16_t handle;
	uint16_t *file_data;
	uint16_t *palettes;
	uint8_t *bmap;
	uint16_t flags;
	uint32_t file_size;
	uint16_t planes;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(PALETTE_SIZE, handle, SEEK_SET);
	if ((size_t)Fread(handle, sizeof(flags), &flags) != sizeof(flags) ||
		flags > 30)
	{
		Fclose(handle);
		return FALSE;
	}
	Fseek(0, handle, SEEK_SET);
	palettes = malloc(PALETTE_SIZE);
	bmap = malloc(SCREEN_SIZE);
	file_data = malloc(SCREEN_SIZE * 2);
	
	if (bmap == NULL || palettes == NULL || file_data == NULL ||
		Fread(handle, PALETTE_SIZE, palettes) != PALETTE_SIZE ||
		(size_t)Fread(handle, file_size - PALETTE_SIZE, file_data) != file_size - PALETTE_SIZE)
	{
		free(file_data);
		free(bmap);
		free(palettes);
		Fclose(handle);
		return FALSE;
	}

	Fclose(handle);
	resolution = flags & 1;
	planes = 4 >> resolution;
	bst_decompress((const uint32_t *)file_data, (uint32_t *)bmap);
	free(file_data);

	info->width = (resolution + 1) * 320;
	info->height = 200;
	info->planes = 12;
	info->colors = 1L << 12;
	info->indexed_color = FALSE;
	info->components = 3;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = 0;			/* current line */
	info->_priv_ptr = bmap;
	info->_priv_ptr_more = palettes;
	info->__priv_ptr_more = (void *)(intptr_t)planes;

	strcpy(info->info, "ColorBurst II");
	strcpy(info->compression, "RLE");
	
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
	uint32_t pos = info->_priv_var;
	int y;
	const uint16_t *screen16;
	const uint16_t *palettes;
	int16_t x;
	int16_t w;
	int16_t i;
	int byte;
	int16_t plane;
	uint16_t color;
	int16_t planes;
	
	screen16 = (const uint16_t *)info->_priv_ptr;
	palettes = (const uint16_t *)info->_priv_ptr_more;
	planes = (int16_t)(intptr_t)info->__priv_ptr_more;
	y = info->_priv_var_more;
	palettes += y * 16;
	y++;
	info->_priv_var_more = y;

	w = 0;
	x = 0;
	do
	{
		for (i = 15; i >= 0; i--)
		{
			for (plane = byte = 0; plane < planes; plane++)
			{
				if ((screen16[pos + plane] >> i) & 1)
					byte |= 1 << plane;
			}
			color = palettes[byte];
			byte = ((color >> 8) & 0x0f);
			buffer[w++] = (byte << 4) | byte;
			byte = ((color >> 4) & 0x0f);
			buffer[w++] = (byte << 4) | byte;
			byte = ((color) & 0x0f);
			buffer[w++] = (byte << 4) | byte;
			x++;
		}
		pos += planes;
	} while (x < info->width);

	info->_priv_var = pos;
	
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
	free(info->_priv_ptr_more);
	info->_priv_ptr_more = 0;
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
