#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
Cyber Paint Cell    *.CEL (ST low resolution)

Saved much like a sequence frame, however some fields are not used. They can be
any size up to 320x200.

1 word      file ID, $FFFF
1 word      resolution [always 0 = low res]
16 words    palette
12 bytes    filename [usually "        .   "]
1 word      limits [not used]
1 word      speed [not used]
1 word      steps [not used]
1 word      x offset [0 - 319]
1 word      y offset [0 - 199]
1 word      width in pixels [max. 320]
1 word      height in pixels [max. 200]
1 byte      operation [always 0 = copy]
1 byte      storage method [always 0 = uncompressed]
1 long      length of data in bytes
30 bytes    reserved
30 bytes    padding
---------
128 bytes   total for header

? bytes     image data

The image data is always uncompressed and is simply copied onto the screen at
position (x, y) specified in the header.
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


struct file_header {
	uint16_t id;
	uint16_t resolution;
	uint16_t palette[16];
	char filename[12];
	uint16_t limits;
	uint16_t speed;
	uint16_t steps;
	uint16_t x_offset;
	uint16_t y_offset;
	int16_t width;
	int16_t height;
	uint8_t operation;
	uint8_t compression;
	uint32_t datasize;
	uint8_t reserved[30];
	uint8_t pad[30];
};

#define SCREEN_SIZE 32000

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
	size_t datasize;
	int i;
	struct file_header header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	if ((size_t)Fread(handle, sizeof(header), &header) != sizeof(header) ||
		header.id != (uint16_t)-1 ||
		header.resolution != 0)
	{
		Fclose(handle);
		return FALSE;
	}
	datasize = (((((size_t)header.width + 15) >> 4) * 2) * 4) * header.height;

	bmap = malloc(datasize);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, datasize, bmap) != datasize)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	
	Fclose(handle);
	
	info->planes = 4;
	info->width = header.width;
	info->height = header.height;
	info->indexed_color = TRUE;
	info->components = 3;
	info->colors = 1L << 4;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;
	strcpy(info->info, "Cyber Paint (Image)");
	strcpy(info->compression, "None");
	
	for (i = 0; i < 16; i++)
	{
		info->palette[i].red = (((header.palette[i] >> 7) & 0x0e) + ((header.palette[i] >> 11) & 0x01)) * 17;
		info->palette[i].green = (((header.palette[i] >> 3) & 0x0e) + ((header.palette[i] >> 7) & 0x01)) * 17;
		info->palette[i].blue = (((header.palette[i] << 1) & 0x0e) + ((header.palette[i] >> 3) & 0x01)) * 17;
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
	const uint16_t *bmap = (const uint16_t *)info->_priv_ptr;
	int16_t x;
	int16_t i;
	uint16_t byte;
	int16_t plane;
	int16_t width = (info->width + 15) & -16;

	x = 0;
	do
	{
		for (i = 15; i >= 0; i--)
		{
			for (plane = byte = 0; plane < info->planes; plane++)
			{
				if ((bmap[pos + plane] >> i) & 1)
					byte |= 1 << plane;
			}
			buffer[x] = byte;
			x++;
		}
		pos += info->planes;
	} while (x < width);

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
