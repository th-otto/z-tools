#define	VERSION	     0x0201
#define NAME        "Picworks"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__
#define MISC_INFO   "Some code by Hans Wessels"

#include "plugin.h"
#include "zvplugin.h"

/*
Picworks    *.CP3 (ST high resolution)

Files do not seem to have any resolution or bit plane info stored in them. The
file extension seems to be the only way to determine the contents. They are
always compressed images.
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("CP3\0");

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


#define SCREEN_SIZE 32000L


/* ported to PureC from Picworks v1 GFABASIC source code */
/* Written by Lonny Pursell - placed in to Public Domain 2/8/2017 */
static void decode_cp3(uint8_t *data, uint32_t *bmap)
{
	uint16_t *dpeek;
	uint16_t cnt;
	uint16_t i;
	uint16_t c;
	uint32_t *lpeek;
	size_t src = 0;
	size_t dst = 0;
	size_t offset;
	uint32_t tmp1;
	uint32_t tmp2;

	dpeek = (uint16_t *) data;
	cnt = dpeek[src++];
	offset = 2L + 2L + 4L * (uint32_t) cnt;	/* offset into file */
	lpeek = (uint32_t *) &data[offset];	/* base of long table */
	offset = 0;							/* reset index */

	for (c = 0; c < cnt; c++)
	{
		src++;
		if (dpeek[src])
		{
			for (i = 0; i < dpeek[src]; i++)
			{
				bmap[dst++] = lpeek[offset++];
				bmap[dst++] = lpeek[offset++];
			}
		}
		src++;
		tmp1 = lpeek[offset++];
		tmp2 = lpeek[offset++];
		for (i = 0; i < dpeek[src]; i++)
		{
			bmap[dst++] = tmp1;
			bmap[dst++] = tmp2;
		}
	}
	while (dst < 8000)
	{									/* 32000/4 */
		bmap[dst++] = lpeek[offset++];
		bmap[dst++] = lpeek[offset++];
	}
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
	uint8_t *temp;
	size_t file_size;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	bmap = malloc(SCREEN_SIZE + 1024);
	temp = malloc(file_size + 256);
	if (bmap == NULL || temp == NULL)
	{
		free(temp);
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, file_size, temp) != file_size)
	{
		free(temp);
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);
	decode_cp3(temp, (uint32_t *)bmap);
	free(temp);
	
	info->width = 640;
	info->height = 400;
	info->planes = 1;
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

	strcpy(info->info, "Picworks");
	strcpy(info->compression, "RLE");

	RETURN_SUCCESS();
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
	int16_t byte;
	uint8_t *bmap = info->_priv_ptr;
	size_t src = info->_priv_var;

	x = info->width >> 3;
	info->_priv_var += x;
	do
	{
		byte = bmap[src++];
		*buffer++ = (byte >> 7) & 1;
		*buffer++ = (byte >> 6) & 1;
		*buffer++ = (byte >> 5) & 1;
		*buffer++ = (byte >> 4) & 1;
		*buffer++ = (byte >> 3) & 1;
		*buffer++ = (byte >> 2) & 1;
		*buffer++ = (byte >> 1) & 1;
		*buffer++ = (byte >> 0) & 1;
	} while (--x > 0);
	RETURN_SUCCESS();
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
