#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
DUO    *.DU1 *.DUO (low resolution)
       *.DU2       (medium resolution)

Created by Anders Eriksson: http://ae.dhs.nu/
These are essentially 2 screens with over-scan interlaced together. Both
screens use the same palette, but when viewed the human eye merges the two
alternating images into one.

DU1 low resolution files are 416x273 pixels:

16 words        palette
56784 bytes     screen 1
56784 bytes     screen 2
-----------
113600 bytes    total

_______________________________________________________________________________

DU2 medium resolution files are 832x273 pixels:

4 words         palette
56784 bytes     screen 1
56784 bytes     screen 2
24 bytes        padding if present, otherwise total size is 113576 bytes
-----------
113600 bytes    total
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

#define SCREEN_SIZE 56784L

static void my_strupr(char *str)
{
	while (*str != '\0')
	{
		if (*str >= 'a' && *str <= 'z')
			*str -= 'a' - 'A';
		str++;
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
	uint32_t file_size;
	char extension[4];
	int planes;
	int i;
	int colors;
	uint16_t palette[16];

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	
	if (file_size != SCREEN_SIZE * 2 + 4 * 2 && file_size != SCREEN_SIZE * 2 + 16 * 2)
	{
		Fclose(handle);
		return FALSE;
	}
	Fseek(0, handle, SEEK_SET);
	
	strcpy(extension, name + strlen(name) - 3);
	my_strupr(extension);

	if (strcmp(extension, "DUO") == 0 ||
		strcmp(extension, "DU1") == 0)
	{
		info->width = 416;
		info->height = 273;
		planes = 4;
	} else if (strcmp(extension, "DU2") == 0)
	{
		info->width = 832;
		info->height = 273;
		planes = 2;
	} else
	{
		Fclose(handle);
		return FALSE;
	}
	colors = 1 << planes;
	if ((size_t)Fread(handle, sizeof(palette[0]) * colors, palette) != sizeof(palette[0]) * colors)
	{
		Fclose(handle);
		return FALSE;
	}
	for (i = 0; i < colors; i++)
	{
		info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 17;
		info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 17;
		info->palette[i].blue = (((palette[i] << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 17;
	}
	bmap = malloc(SCREEN_SIZE * 2);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, SCREEN_SIZE * 2, bmap) != SCREEN_SIZE * 2)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);
	
	info->indexed_color = FALSE;
	info->components = 3;
	info->planes = 8;
	info->colors = 1L << 8;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;
	info->_priv_var_more = planes;

	strcpy(info->info, "DUO");
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
	size_t pos = info->_priv_var;
	int width = info->width;
	int planes = (int)info->_priv_var_more;
	int x;
	int w;
	int i;
	uint16_t byte;
	uint16_t byte2;
	int plane;
	const uint16_t *screen1;
	const uint16_t *screen2;
	
	screen1 = (const uint16_t *)info->_priv_ptr;
	screen2 = screen1 + SCREEN_SIZE / 2;
	x = 0;
	w = 0;
	/*
	 * TODO: unroll the inner loop for 2/4 planes
	 */
	do
	{
		for (i = 15; i >= 0; i--)
		{
			for (plane = byte = 0; plane < planes; plane++)
			{
				if ((screen1[pos + plane] >> i) & 1)
					byte |= 1 << plane;
			}
			for (plane = byte2 = 0; plane < planes; plane++)
			{
				if ((screen2[pos + plane] >> i) & 1)
					byte2 |= 1 << plane;
			}
			buffer[w++] = (info->palette[byte].red + info->palette[byte2].red) >> 1;
			buffer[w++] = (info->palette[byte].green + info->palette[byte2].green) >> 1;
			buffer[w++] = (info->palette[byte].blue + info->palette[byte2].blue) >> 1;
		}
		pos += planes;
		x += 16;
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
