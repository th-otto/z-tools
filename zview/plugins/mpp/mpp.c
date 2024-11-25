#define	VERSION	    0x201
#define NAME        "Multi Palette Picture"
#define AUTHOR      "Zerkman / Sector One"
#define DATE        __DATE__ " " __TIME__
#define MISC_INFO   "Port by Thorsten Otto"

#include "plugin.h"
#include "zvplugin.h"
#define NF_DEBUG 0
#include "nfdebug.h"

/*
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("MPP\0");

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

struct file_header {
	char magic[3];
	uint8_t mode;
	uint8_t flags;
	uint8_t unknown[3];
	uint8_t palette_offset[4];
};

#include "mpp2bmp.c"
#include "pixbuf.c"


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
	Pixel a;
	Pixel b;
	struct file_header header;
	const ModeD *mode;
	long frames;
	long extra;
	long ste;
	uint32_t palette_offset;
	PixBuf *pixbuf;
	PixBuf *pixbuf2;
	
	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}

	if (Fread(handle, sizeof(header), &header) != sizeof(header) ||
		header.magic[0] != 'M' ||
		header.magic[1] != 'P' ||
		header.magic[2] != 'P' ||
		header.mode > 3)
	{
		nf_debugprintf("header failed\n");
		Fclose(handle);
		return FALSE;
	}
	mode = &modes[header.mode];
	frames = (header.flags & 4) >> 2;
	extra = (header.flags & 2) >> 1;
	ste = header.flags & 1;
	palette_offset = read32(header.palette_offset);
	if (palette_offset != 0)
		Fseek(palette_offset, handle, SEEK_CUR);
	pixbuf = decode_mpp(mode, ste, extra, handle);
	if (pixbuf == NULL)
	{
		nf_debugprintf("decode pixbuf failed\n");
		Fclose(handle);
		return FALSE;
	}
	if (frames != 0)
	{
		long k, n;

		pixbuf2 = decode_mpp(mode, ste, extra, handle);
		if (pixbuf2 == NULL)
		{
			nf_debugprintf("decode pixbuf2 failed\n");
			pixbuf_delete(pixbuf);
			Fclose(handle);
			return FALSE;
		}
		n = mode->width * mode->height;
		for (k = 0; k < n; ++k)
		{
			a = pixbuf->array[k];
			b = pixbuf2->array[k];

			a.cp.r = (a.cp.r + b.cp.r) / 2;
			a.cp.g = (a.cp.g + b.cp.g) / 2;
			a.cp.b = (a.cp.b + b.cp.b) / 2;
			pixbuf->array[k] = a;
		}
		pixbuf_delete(pixbuf2);
	}
	
	Fclose(handle);
	
	info->planes = 24;
	info->colors = 1L << 24;
	info->width = mode->width;
	info->height = mode->height;
	info->components = 3;
	info->indexed_color = FALSE;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = pixbuf;

	strcpy(info->info, "Multi Palette Picture (Mode x)");
	info->info[28] = header.mode + '0';
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
	int x;
	Pixel *pixels;
	PixBuf *pixbuf;
	
	pixbuf = info->_priv_ptr;
	pixels = pixbuf->array + info->_priv_var;
	for (x = info->width; --x >= 0; )
	{
		*buffer++ = pixels->cp.r;
		*buffer++ = pixels->cp.g;
		*buffer++ = pixels->cp.b;
		pixels++;
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
	pixbuf_delete(info->_priv_ptr);
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
