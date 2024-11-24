#define	VERSION	    0x108
#define NAME        "MacPaint"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
MacPaint        *.MAC

1 long           version number [0=ignore header, 2=header valid]
38 * 8 bytes     8x8 brush/fill patterns. Each byte is a pattern row, and the
                 bytes map the pattern rows top to bottom. The patterns are
                 stored in the order they appear at the bottom of the MacPaint
                 screen top to bottom, left to right.
204 bytes        unused
-------------
512 bytes        total for header

< 51200 bytes    compressed bitmap data
-------------
< 51712 bytes    total

NOTE: The version number is actually a flag to MacPaint to indicate if the
brush/fill patterns are present in the file. If the version is 0, the default
patterns are used. Therefore you can simply save a MacPaint file by writing a
blank header (512 $00 bytes), followed by the packed image data.

Bitmap compression:

The bitmap data is for a 576 pixel by 720 pixel monochrome image. The packing
method is PackBits (see below). There are 72 bytes per scan line. Each bit
represents one pixel; 0 = white, 1 = black.

See also PackBits Compression Algorithm
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("MAC\0" "PNTG\0");

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

#include "../degas/packbits.c"

struct file_header {
	uint32_t version;
	uint8_t brush[38][8];
	uint8_t reserved[204];
};

#define SCREEN_SIZE (576L * 720 / 8)


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
	uint32_t magic;
	int16_t handle;
	uint8_t *bmap;
	struct file_header header;
	
	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	Fseek(65, handle, SEEK_SET);
	
	if (Fread(handle, sizeof(magic), &magic) != sizeof(magic))
	{
		Fclose(handle);
		return FALSE;
	}

	if (magic == 0x504E5447L) /* 'PNTG' */
	{
		Fseek(128, handle, SEEK_SET);
		file_size -= 128;
	} else
	{
		Fseek(0, handle, SEEK_SET);
	}
	file_size -= sizeof(header);
	if (file_size > SCREEN_SIZE ||
		Fread(handle, sizeof(header), &header) != sizeof(header) ||
		header.version > 3)
	{
		Fclose(handle);
		return FALSE;
	}
	
	bmap = malloc(SCREEN_SIZE * 2);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}

	if ((size_t)Fread(handle, file_size, bmap + SCREEN_SIZE) != file_size)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	decode_packbits(bmap, bmap + SCREEN_SIZE, SCREEN_SIZE);

	info->planes = 1;
	info->width = 576;
	info->height = 720;
	info->components = 1;
	info->indexed_color = FALSE;
	info->colors = 1L << 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "MacPaint");
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
	size_t pos = info->_priv_var;
	uint8_t *bmap = info->_priv_ptr;
	int16_t byte;
	int x;
	
	x = (info->width + 7) >> 3;
	do
	{
		byte = bmap[pos++];
		*buffer++ = (byte >> 7) & 1;
		*buffer++ = (byte >> 6) & 1;
		*buffer++ = (byte >> 5) & 1;
		*buffer++ = (byte >> 4) & 1;
		*buffer++ = (byte >> 3) & 1;
		*buffer++ = (byte >> 2) & 1;
		*buffer++ = (byte >> 1) & 1;
		*buffer++ = (byte >> 0) & 1;
	} while (--x > 0);
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
