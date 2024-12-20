#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
C.O.L.R. Object Editor (Mural)    *.MUR (ST low resolution only)

16000 words    image data (screen memory)
-----------
32000 bytes    total

_______________________________________________________________________________

Palettes are stored in separate *.PAL files:

48 words    3 words per entry, VDI palette, in VDI order
--------
96 bytes    total
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

#define SCREEN_SIZE 32000u


static int vdi2bios(int idx, int planes)
{
	int xbios;
	
	switch (idx)
	{
	case 1:
		xbios = (1 << planes) - 1;
		break;
	case 2:
		xbios = 1;
		break;
	case 3:
		xbios = 2;
		break;
	case 4:
		xbios = idx;
		break;
	case 5:
		xbios = 6;
		break;
	case 6:
		xbios = 3;
		break;
	case 7:
		xbios = 5;
		break;
	case 8:
		xbios = 7;
		break;
	case 9:
		xbios = 8;
		break;
	case 10:
		xbios = 9;
		break;
	case 11:
		xbios = 10;
		break;
	case 12:
		xbios = idx;
		break;
	case 13:
		xbios = 14;
		break;
	case 14:
		xbios = 11;
		break;
	case 15:
		xbios = 13;
		break;
	case 255:
		xbios = 15;
		break;
	case 0:
	default:
		xbios = idx;
		break;
	}
	return xbios;
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
	int i;
	size_t file_size;
	int16_t handle;
	uint8_t *bmap;
	char palname[256];
	uint16_t paldata[16][3];

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	if (file_size != SCREEN_SIZE)
	{
		Fclose(handle);
		return FALSE;
	}
	Fseek(0, handle, SEEK_SET);
	bmap = malloc(SCREEN_SIZE);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, SCREEN_SIZE, bmap) != SCREEN_SIZE)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	strcpy(palname, name);

	/*
	 * Note: this is safe under the assumption that
	 * this plugin is only ever called with filenames
	 * that have atleast a ".mur" extension
	 */
	strcpy(palname + strlen(palname) - 3, "pal");
	for (i = 0; i < 16; i++)
	{
		info->palette[i].red =
		info->palette[i].green =
		info->palette[i].blue = (i << 4) | i;
	}

	strcpy(info->info, "C.O.L.R. Object Editor");
	strcpy(info->compression, "None");

	handle = (int16_t) Fopen(palname, FO_READ);
	if (handle >= 0)
	{
		if ((size_t)Fseek(0, handle, SEEK_END) == sizeof(paldata))
		{
			int idx;
			
			Fseek(0, handle, SEEK_SET);
			if (Fread(handle, sizeof(paldata), paldata) != sizeof(paldata))
			{
				free(bmap);
				Fclose(handle);
				return FALSE;
			}
			strcat(info->info, "+");
			for (i = 0; i < 16; i++)
			{
				idx = vdi2bios(i, 4);
				info->palette[idx].red = ((((long)paldata[i][0] << 8) - paldata[i][0]) + 500) / 1015;
				info->palette[idx].green = ((((long)paldata[i][1] << 8) - paldata[i][1]) + 500) / 1015;
				info->palette[idx].blue = ((((long)paldata[i][2] << 8) - paldata[i][2]) + 500) / 1015;
			}
		}
		Fclose(handle);
	}
	
	info->planes = 4;
	info->colors = 1L << 4;
	info->width = 320;
	info->height = 200;
	info->indexed_color = TRUE;
	info->components = 3;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

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
	const uint16_t *screen16 = (const uint16_t *)info->_priv_ptr;
	int pos = (int)info->_priv_var;
	int16_t x;
	int16_t i;
	uint16_t byte;
	int plane;
	
	x = 0;
	do
	{
		for (i = 15; i >= 0; i--)
		{
			for (plane = byte = 0; plane < 4; plane++)
			{
				if ((screen16[pos + plane] >> i) & 1)
					byte |= 1 << plane;
			}
			buffer[x] = byte;
			x++;
		}
		pos += 4;
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
