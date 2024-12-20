#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

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


#define GENERATE_TABLE 0

#if GENERATE_TABLE
static unsigned char const weight_table[128] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x14, 0x18, 0x38, 0x30, 0x20, 0x3c, 0x0c,
	0x1a, 0x1a, 0x30, 0x18, 0x0b, 0x0c, 0x08, 0x18,
	0x36, 0x22, 0x2a, 0x2a, 0x2a, 0x2f, 0x2f, 0x20,
	0x34, 0x2d, 0x10, 0x13, 0x1b, 0x18, 0x1b, 0x20,
	0x34, 0x34, 0x39, 0x2c, 0x34, 0x2e, 0x26, 0x33,
	0x34, 0x28, 0x22, 0x30, 0x20, 0x39, 0x38, 0x34,
	0x2e, 0x33, 0x36, 0x2a, 0x20, 0x32, 0x2c, 0x38,
	0x2c, 0x24, 0x28, 0x20, 0x18, 0x20, 0x18, 0x0e,
	0x0f, 0x29, 0x30, 0x1e, 0x30, 0x27, 0x23, 0x33,
	0x2d, 0x1c, 0x1f, 0x2e, 0x1e, 0x2d, 0x26, 0x28,
	0x2e, 0x2e, 0x1b, 0x24, 0x21, 0x27, 0x20, 0x2c,
	0x22, 0x30, 0x22, 0x24, 0x1c, 0x24, 0x12, 0x1c
};
static unsigned char color_table[128];
#else
static unsigned char const color_table[128] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x50, 0x60, 0xe0, 0xc0, 0x80, 0xf0, 0x30,
	0x68, 0x68, 0xc0, 0x60, 0x2c, 0x30, 0x20, 0x60,
	0xd8, 0x88, 0xa8, 0xa8, 0xa8, 0xbc, 0xbc, 0x80,
	0xd0, 0xb4, 0x40, 0x4c, 0x6c, 0x60, 0x6c, 0x80,
	0xd0, 0xd0, 0xe4, 0xb0, 0xd0, 0xb8, 0x98, 0xcc,
	0xd0, 0xa0, 0x88, 0xc0, 0x80, 0xe4, 0xe0, 0xd0,
	0xb8, 0xcc, 0xd8, 0xa8, 0x80, 0xc8, 0xb0, 0xe0,
	0xb0, 0x90, 0xa0, 0x80, 0x60, 0x80, 0x60, 0x38,
	0x3c, 0xa4, 0xc0, 0x78, 0xc0, 0x9c, 0x8c, 0xcc,
	0xb4, 0x70, 0x7c, 0xb8, 0x78, 0xb4, 0x98, 0xa0,
	0xb8, 0xb8, 0x6c, 0x90, 0x84, 0x9c, 0x80, 0xb0,
	0x88, 0xc0, 0x88, 0x90, 0x70, 0x90, 0x48, 0x70
};
#endif

static char const ascii_art_table[70] = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
static char const crlf[2] = "\r\n";


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
	uint8_t *bmap;
	int16_t handle;
	size_t file_size;
	size_t i;
	int color;
	uint16_t width;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	bmap = malloc(file_size + 2);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, file_size, bmap) != file_size)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);
	if (file_size > 0 && bmap[file_size - 1] != '\n')
		bmap[file_size++] = '\n';

	for (i = 0; i < file_size; i++)
	{
		if (bmap[i] == 0x0d || (bmap[i] != 0x0a && (bmap[i] < 0x20 || bmap[i] > 0x7f)))
			bmap[i] = ' ';
	}

	width = 0;
	info->height = 0;
	info->width = 0;
	for (i = 0; i < file_size; i++)
	{
		if (bmap[i] == 0x0a)
		{
			info->height++;
			info->width = width < info->width ? info->width : width;
			width = 0;
		} else
		{
			width++;
		}
	}
	
#if GENERATE_TABLE
	{
		uint16_t maxc;
		double scale;

		maxc = 0;
		for (i = 0; i < 128; i++)
		{
			maxc = weight_table[i] < maxc ? maxc : weight_table[i];
		}
		scale = (double)(255 / maxc);
		for (i = 0; i < 128; i++)
		{
			color_table[i] = weight_table[i] * scale;
		}
	}
#endif

	info->planes = 8;
	info->components = 3;
	info->indexed_color = FALSE;
	info->colors = 1L << 8;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	strcpy(info->info, "ASCII art (grayscale)");
	strcpy(info->compression, "None");

	info->_priv_ptr = bmap;
	info->_priv_var = 0;				/* y offset */

	/* FIXME: should be setting */
	/* FIXME2: palette actually isn't used since indexed_color is FALSE */
	if (Kbshift(-1) & K_ALT)
	{
		for (color = 0; color < 256; color++)
		{
			info->palette[color].red =
			info->palette[color].green =
			info->palette[color].blue = i;
		}
	} else
	{
		for (color = 0; color < 256; color++)
		{
			info->palette[color].red =
			info->palette[color].green =
			info->palette[color].blue = 255 - color;
		}
	}
	
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
	uint8_t *bmap = info->_priv_ptr;
	int16_t x;
	int16_t i;
	
	x = 0;
	for (i = 0; i < info->width; i++)
		buffer[i] = color_table[' '];
	for (;;)
	{
		if (bmap[pos] == '\n')
		{
			info->_priv_var = pos + 1;
			return TRUE;
		}
		buffer[x] = color_table[bmap[pos]];
		x++;
		pos++;
	}
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


boolean __CDECL encoder_init(const char *name, IMGINFO info)
{
	int16_t handle;
	char *outline;

	if ((handle = (int16_t) Fcreate(name, 0)) < 0)
	{
		return FALSE;
	}
	outline = malloc(info->width);
	if (outline == NULL)
	{
		Fclose(handle);
		return FALSE;
	}

	info->planes = 24;
	info->components = 3;
	info->colors = 1L << 24;
	info->orientation = UP_TO_DOWN;
	info->memory_alloc = TT_RAM;
	info->indexed_color = FALSE;
	info->page = 1;

	info->_priv_var = handle;
	info->_priv_ptr = outline;

	return TRUE;
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	int16_t handle;
	uint16_t x;
	uint16_t i;
	unsigned short c;
	char *outline = info->_priv_ptr;

	x = 0;
	for (i = 0; i < info->width; i++)
	{
		c = buffer[x];
		c += buffer[x + 1];
		c += buffer[x + 2];
		c /= 3;
#if 1
		c = (unsigned short)(c * 69u) / 255u;
#else
		c = c / 3.68;
#endif
		outline[i] = ascii_art_table[c];
		x += 3;
	}
	handle = (int16_t)info->_priv_var;
	if ((uint32_t)Fwrite(handle, info->width, outline) != info->width ||
		Fwrite(handle, 2, crlf) != 2)
	{
		return FALSE;
	}
	return TRUE;
}


void __CDECL encoder_quit(IMGINFO info)
{
	int16_t handle = (int16_t)info->_priv_var;
	char *outline;
	
	if (handle > 0)
	{
		Fclose(handle);
		info->_priv_var = 0;
	}
	outline = info->_priv_ptr;
	if (outline != NULL)
	{
		free(outline);
		info->_priv_ptr = NULL;
	}
}
