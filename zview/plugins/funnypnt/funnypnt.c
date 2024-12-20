#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
Funny Paint    *.FUN

Supports animation and image data is never compressed.

1 long      file id, $000ACFE2
1 word      image width in pixels
1 word      image height in pixels
1 word      planes [1, 2, 4, 8, 16]
1 word      number of frames [1 = single image]
1 byte      ? [usually 0]
--------
13 bytes    total for header

??          image data:
  1 to 8 planes: standard Atari interleaved bitmap
  16 planes: Falcon high-color, word RRRRRGGGGGGBBBBB
  This section is repeated for each frame

1 long      total colors - 1
1 long      ? (varies)
1 long      ? (varies)

??          palette data:
  The palette is stored in VDI format (0-1000) in VDI order
  3 words per entry for R, G, and B
  1 plane palette should be ignored, seems to contain bogus values
  16 planes will have a palette (predefined pens, should be ignored)
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
	uint32_t magic;
	uint16_t width;
	uint16_t height;
	uint16_t planes;
	uint16_t num_frames;
};

struct file_trailer {
	int32_t num_colors;
	int32_t reserved1;
	int32_t reserved2;
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
	size_t image_size;
	int16_t handle;
	uint8_t *bmap;
	struct file_header header;
	struct file_trailer trailer;
	uint32_t *frame_positions;
	uint8_t dummy;
	uint16_t frame;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}

	if (Fread(handle, sizeof(header), &header) != sizeof(header) ||
		header.magic != 0xACFE2L ||
		Fread(handle, 1, &dummy) != 1)
	{
		Fclose(handle);
		return FALSE;
	}
		
	switch (header.planes)
	{
	case 1:
	case 16:
		info->indexed_color = FALSE;
		break;
	case 2:
	case 4:
	case 8:
		info->indexed_color = TRUE;
		break;
	default:
		Fclose(handle);
		return FALSE;
	}
	
	image_size = (((size_t)((header.width + 15) & -16) * header.planes) / 8) * header.height;

	if (header.num_frames == 0)
		header.num_frames = 1;
	bmap = malloc(image_size);
	frame_positions = malloc(header.num_frames * sizeof(*frame_positions));
	if (bmap == NULL || frame_positions == NULL)
	{
		free(frame_positions);
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	
	for (frame = 0; frame < header.num_frames; frame++)
	{
		frame_positions[frame] = Fseek(0, handle, SEEK_CUR);
		Fseek(image_size, handle, SEEK_CUR);
	}

	Fread(handle, sizeof(trailer), &trailer);
	
	info->planes = header.planes;

	if (info->indexed_color)
	{
		int i;
		int j;
		int colors = 1 << info->planes;
		uint16_t palette[256 * 3];

		if (Fread(handle, colors * 3 * 2, palette) != colors * 3 * 2)
		{
			free(frame_positions);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}

		for (j = 0, i = 0; i < colors; i++)
		{
			int idx = vdi2bios(i, info->planes);
			info->palette[idx].red = ((((long)palette[j] << 8) - palette[j]) + 500) / 1000;
			info->palette[idx].green = ((((long)palette[j + 1] << 8) - palette[j + 1]) + 500) / 1000;
			info->palette[idx].blue = ((((long)palette[j + 2] << 8) - palette[j + 2]) + 500) / 1000;
			j += 3;
		}
	}

	info->components = info->planes == 1 ? 1 : 3;
	info->width = header.width;
	info->height = header.height;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = header.num_frames;		/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = 0;
	info->_priv_ptr = bmap;
	info->_priv_ptr_more = frame_positions;
	info->__priv_ptr_more = (void *)(intptr_t)handle;

	strcpy(info->info, "Funny Paint");
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
	uint8_t *bmap = info->_priv_ptr;
	
	if (info->_priv_var_more == 0)
	{
		int handle = (int)(intptr_t)info->__priv_ptr_more;
		uint32_t *frame_positions = (uint32_t *)info->_priv_ptr_more;
		size_t image_size = (((size_t)((info->width + 15) & -16) * info->planes) / 8) * info->height;
		
		Fseek(frame_positions[info->page_wanted], handle, SEEK_SET);
		if ((size_t)Fread(handle, image_size, bmap) != image_size)
			return FALSE;
		info->delay = 10;
		pos = 0;
	}
	
	switch (info->planes)
	{
	case 1:
		{
			int16_t byte;
			uint8_t *end = buffer + info->width;
			
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
			} while (buffer < end);
		}
		break;
	case 16:
		{
			uint16_t color;
			uint16_t *bmap16 = (uint16_t *)bmap;
			uint8_t *end = buffer + info->width * 3;

			do
			{
				color = bmap16[pos++];
				*buffer++ = ((color >> 11) & 0x1f) << 3;
				*buffer++ = ((color >> 5) & 0x3f) << 2;
				*buffer++ = ((color) & 0x1f) << 3;
			} while (buffer < end);
		}
		break;
	default:
		{
			int16_t x;
			int16_t i;
			const uint16_t *bmap16;
			uint16_t byte;
			int16_t plane;
			
			x = 0;
			bmap16 = (const uint16_t *)bmap;
			do
			{
				for (i = 15; i >= 0; i--)
				{
					for (plane = byte = 0; plane < info->planes; plane++)
					{
						if ((bmap16[pos + plane] >> i) & 1)
							byte |= 1 << plane;
					}
					buffer[x] = byte;
					x++;
				}
				pos += info->planes;
			} while (x < info->width);
		}
		break;
	}
	
	++info->_priv_var_more;
	if (info->_priv_var_more == info->height)
	{
		info->_priv_var_more = 0;
		pos = 0;
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
	int handle;
	
	free(info->_priv_ptr);
	info->_priv_ptr = 0;
	free(info->_priv_ptr_more);
	info->_priv_ptr_more = 0;
	handle = (int)(intptr_t)info->__priv_ptr_more;
	if (handle > 0)
	{
		Fclose(handle);
		info->__priv_ptr_more = 0;
	}
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
