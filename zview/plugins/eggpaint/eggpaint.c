#define	VERSION	     0x0207
#define NAME        "EggPaint/Spooky Sprites"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
EggPaint        *.TRP

1 long    file id, 'TRUP'
1 word    xres
1 word    yres
xres*yres*2 bytes Falcon high color data,  word RRRRRGGGGGGBBBBB

Packed EggPaint pictures are packed with ICE 2.4.


Spooky Sprites    *.TRE (RLE compressed)
                  *.TRP (uncompressed)

Falcon high-color images.

TRP - True Color Picture

1 long    file id ['tru?']
1 word    picture width in pixels
1 word    picture height in pixels
width*height words of picture data

_______________________________________________________________________________

TRE - Run Length Encoded True Color Picture

1 long    file id ['tre1']
1 word    picture width in pixels
1 word    picture height in pixels
1 long    number of chunks
This is followed by all the data chunks. The first chunk is a raw data chunk.

raw data chunk:

1 byte    Number of raw data pixels. If this byte is 255 the number of pixels
          will be the following word+255
Number of pixels words of raw data.
This chunk is followed by an RLE chunk.

RLE chunk:

1 byte    Number of RLE pixels. If this byte is 255 the number of RLE pixels
          will be the following word+255. This is the number of times you
          should draw the previous color again.
This chunk is followed by a raw data chunk.
*/


#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
	case OPTION_EXTENSIONS:
		return (long) ("TRP\0");

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


struct file_header {
	uint32_t magic;
	int16_t xres;
	int16_t yres;
};

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
	struct file_header file_header;
	size_t screen_size;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	Fread(handle, sizeof(file_header), &file_header);
	if (file_header.magic != 0x54525550L && /* 'TRUP' */
		file_header.magic != 0x7472753FL) /* 'tru?' */
	{
		Fclose(handle);
		return FALSE;
	}
	screen_size = (size_t)file_header.xres * file_header.yres * 2;
	
	bmap = malloc(screen_size + 256);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	Fread(handle, screen_size, bmap);
	Fclose(handle);
	
	info->width = file_header.xres;
	info->height = file_header.yres;
	info->planes = 16;
	info->indexed_color = FALSE;
	info->colors = 1L << 16;
	info->components = 3;
	info->colors = 1L << MIN(info->planes, 24);
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;

	if (file_header.magic == 0x54525550L)
		strcpy(info->info, "EggPaint");
	else if (file_header.magic == 0x7472753FL)
		strcpy(info->info, "Spooky Sprites");
	strcpy(info->compression, "None");

	info->_priv_ptr = bmap;
	info->_priv_var = 0;				/* y offset */

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
	int16_t x;
	const uint16_t *screen16;
	uint16_t color;
	
	screen16 = (const uint16_t *)info->_priv_ptr + info->_priv_var;
	for (x = 0; x < info->width; x++)
	{
		color = *screen16++;
		*buffer++ = ((color >> 11) & 0x1f) << 3;
		*buffer++ = ((color >> 5) & 0x3f) << 2;
		*buffer++ = ((color) & 0x1f) << 3;
	}
	info->_priv_var += info->width;
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


boolean __CDECL encoder_init(const char *name, IMGINFO info)
{
	uint16_t *outline;
	size_t linesize;
	int16_t handle;
	struct file_header header;
	
	linesize = (size_t)info->width * 2;
	outline = malloc(linesize);
	if (outline == NULL)
	{
		return FALSE;
	}
	if ((handle = Fcreate(name, 0)) < 0)
	{
		free(outline);
		return FALSE;
	}
	header.magic = 0x54525550L;
	header.xres = info->width;
	header.yres = info->height;
	if (Fwrite(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		free(outline);
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
	info->_priv_var_more = linesize;

	return TRUE;
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	int16_t handle;
	size_t linesize;
	uint16_t *outline = (uint16_t *)info->_priv_ptr;
	uint16_t x;
	uint16_t color;

	for (x = 0; x < info->width; x++)
	{
		color = (*buffer++ & 0xf8) << 8;
		color |= (*buffer++ & 0xfc) << 3;
		color |= *buffer++ >> 3;
		outline[x] = color;
	}
	handle = (int16_t)info->_priv_var;
	linesize = (size_t)info->width * 2;
	if ((size_t)Fwrite(handle, linesize, outline) != linesize)
	{
		return FALSE;
	}
	return TRUE;
}


void __CDECL encoder_quit(IMGINFO info)
{
	int16_t handle = (int16_t)info->_priv_var;

	if (handle > 0)
	{
		Fclose(handle);
		info->_priv_var = 0;
	}
	free(info->_priv_ptr);
	info->_priv_ptr = 0;
}
