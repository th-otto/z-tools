#include "plugin.h"
#include "zvplugin.h"

#define VERSION		0x207
#define NAME        "EZ-Art Professional"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

/*
EZ-Art Professional    *.EZA (ST low resolution)

1 word      file id, 'EZ'
1 word      version? [200]
16 words    palette
1 word      unknown
1 word      unknown
1 word      unknown
1 word      unknown
--------
44 bytes    total for header

?           image data:
32000 bytes of ST low image data compressed exactly like Degas

See also DEGAS file format and PackBits Compression Algorithm
*/


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("EZA\0");

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
#include "../degas/tanglbts.c"

#define SCREEN_SIZE 32000L

struct file_header {
	uint16_t magic;
	uint16_t version;
	uint16_t palette[16];
	uint16_t unknown[4];
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
boolean __CDECL reader_init( const char *name, IMGINFO info)
{
	int16_t handle;
	uint8_t *bmap;
	uint8_t *temp;
	struct file_header header;
	uint32_t file_size;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	file_size -= sizeof(header);

	if ((size_t)Fread(handle, sizeof(header), &header) != sizeof(header) ||
		file_size > SCREEN_SIZE ||
		header.magic != 0x455A || header.version != 200)
	{
		Fclose(handle);
		return FALSE;
	}

	info->planes = 4;
	info->width = 320;
	info->height = 200;

	bmap = malloc(SCREEN_SIZE);
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
	temp = malloc(SCREEN_SIZE + 256);
	if (temp == NULL)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	decode_packbits(temp, bmap, SCREEN_SIZE);
	tangle_bitplanes(bmap, temp, info->width, info->height, info->planes);
	free(temp);

	info->components = 3;
	info->indexed_color = TRUE;
	info->colors = 1L << 4;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->orientation = UP_TO_DOWN;
	info->page = 1;						/* required - more than 1 = animation */
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "EZ-Art Professional");
	strcpy(info->compression, "RLE");

	if (info->indexed_color)
	{
		int i;
		
		for (i = 0; i < 16; i++)
		{
			uint16_t c;
			c = (((header.palette[i] >> 7) & 0xE) + ((header.palette[i] >> 11) & 0x1));
			info->palette[i].red = (c << 4) | c;
			c = (((header.palette[i] >> 3) & 0xE) + ((header.palette[i] >> 7) & 0x1));
			info->palette[i].green = (c << 4) | c;
			c = (((header.palette[i] << 1) & 0xE) + ((header.palette[i] >> 3) & 0x1));
			info->palette[i].blue = (c << 4) | c;
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
	int x;
	int i;
	uint16_t byte;
	uint16_t *bmap16;
	
	bmap16 = (uint16_t *)info->_priv_ptr + info->_priv_var;	/* as word array */

	x = info->width >> 4;
	info->_priv_var += x * info->planes;
	do
	{
		for (i = 15; i >= 0; i--)
		{
			byte = 0;
			if ((bmap16[0] >> i) & 1)
				byte |= 1 << 0;
			if ((bmap16[1] >> i) & 1)
				byte |= 1 << 1;
			if ((bmap16[2] >> i) & 1)
				byte |= 1 << 2;
			if ((bmap16[3] >> i) & 1)
				byte |= 1 << 3;
			*buffer++ = byte;
		}
		bmap16 += 4;	/* next plane */
	} while (--x > 0);		/* next x */

	return TRUE;
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
void __CDECL reader_get_txt( IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
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
	info->_priv_ptr = NULL;
}
