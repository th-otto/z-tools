#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
DeskPic   *.GFB

1 long      file id, 'GF25'
1 long      number of colors [2, 4, 16, 256]
1 long      image width in pixels
1 long      image height in pixels
1 long      image data size
--------
20 bytes    total for header

?           image data (st screen dump)
?           palette, VDI format, VDI order

Image data is always an uncompressed raw screen dump 'image data size'.
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

#define SCREEN_SIZE 32000

struct file_header {
	uint32_t magic;
	uint32_t colors;
	uint32_t width;
	uint32_t height;
	uint32_t datasize;
};

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
	int16_t handle;
	uint8_t *bmap;
	uint16_t paldata[256 * 3];
	struct file_header file_header;
	size_t datasize;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	if (Fread(handle, sizeof(file_header), &file_header) != sizeof(file_header) ||
		file_header.magic != 0x47463235L)
	{
		Fclose(handle);
		return FALSE;
	}
	switch ((int)file_header.colors)
	{
	case 2:
		info->planes = 1;
		info->indexed_color = FALSE;
		break;
	case 4:
		info->planes = 2;
		info->indexed_color = TRUE;
		break;
	case 16:
		info->planes = 4;
		info->indexed_color = TRUE;
		break;
	case 256:
		info->planes = 8;
		info->indexed_color = TRUE;
		break;
	default:
		Fclose(handle);
		return FALSE;
	}
	datasize = ((file_header.width + 15) >> 4) * 2 * info->planes * file_header.height;
	if (datasize != file_header.datasize)
	{
		Fclose(handle);
		return FALSE;
	}
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
	if (file_header.colors != 0)
	{
		Fread(handle, file_header.colors * 6, paldata);
	}
	Fclose(handle);
	
	info->width = file_header.width;
	info->height = file_header.height;
	info->components = info->planes == 1 ? 1 : 3;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "DeskPic");
	strcpy(info->compression, "None");
	
	
	if (info->indexed_color)
	{
		int i;
		int j;
		int colors = (int)file_header.colors;
		
		for (i = j = 0; i < colors; i++)
		{
			int idx = vdi2bios(i, info->planes);
			info->palette[idx].red = ((((long)paldata[j] << 8) - paldata[j]) + 500) / 1000;
			info->palette[idx].green = ((((long)paldata[j + 1] << 8) - paldata[j + 1]) + 500) / 1000;
			info->palette[idx].blue = ((((long)paldata[j + 2] << 8) - paldata[j + 2]) + 500) / 1000;
			j += 3;
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
	size_t pos = info->_priv_var;
	int width = (info->width + 15) & -16;
	
	if (info->planes == 1)
	{
		int x;
		int16_t byte;
		uint8_t *bmap;
		
		bmap = info->_priv_ptr;
		x = 0;
		do
		{								/* 1-bit mono v1.00 */
			byte = bmap[pos++];
			buffer[x++] = (byte >> 7) & 1;
			buffer[x++] = (byte >> 6) & 1;
			buffer[x++] = (byte >> 5) & 1;
			buffer[x++] = (byte >> 4) & 1;
			buffer[x++] = (byte >> 3) & 1;
			buffer[x++] = (byte >> 2) & 1;
			buffer[x++] = (byte >> 1) & 1;
			buffer[x++] = (byte >> 0) & 1;
		} while (x < width);
	} else
	{
		int x;
		int i;
		uint16_t byte;
		int plane;
		int planes = info->planes;
		const uint16_t *bmap16;
		
		x = 0;
		bmap16 = (const uint16_t *)info->_priv_ptr;
		do
		{
			for (i = 15; i >= 0; i--)
			{
				for (plane = byte = 0; plane < planes; plane++)
				{
					if ((bmap16[pos + plane] >> i) & 1)
						byte |= 1 << plane;
				}
				buffer[x] = byte;
				x++;
			}
			pos += planes;
		} while (x < width);
	}
	
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
