#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
GFA Raytrace    *.SUL *.SUH (st low/high resolution image uncompressed)
                *.SCL *.SCH (st low/high resolution image compressed)
                *.SAL *.SAH (st low/high resolution animation compressed)

SUL/SCL/SCH details:

6 bytes    file id in plain ASCII text: "sul" or "scl" + cr/lf
             "sul' = uncompressed, "scl" = compressed
3 bytes    size factor in plain ASCII text: "1", "2", "4", or "8" + cr/lf
             factor = ASC(factor) - 48
             image height = image height/factor
? bytes    palette (first scan line is always black, not included in the file)
             palette size = 18400/factor
? bytes    image data
             bitmap size = 32000/factor

Uncompressed fixed image sizes:
Factor    Size in bytes
1         50408
2         25208
4         12608
8         6308

SUH details: (similar to Degas *.PI3)

1 word         resolution flag [always 2]
16 words       palette (should be ignored, seems to contain bogus values)
32000 bytes    image data
-----------
32034 bytes    total file size

SAL/SAH detail: (up to 10 frames maximum)

5 bytes     file id in plain ASCII text: "sal" or "sah" + cr/lf
              "sal' = st low resolution, "sah" = st high resolution
1 long      frame count - 1
1 long      size factor [1, 2, 4, or 8]
              image height = image height/factor
1 long      image data size
10 longs    offsets to all 10 frames from start of the file [0 = unused entry]
Repeated for each frame:
  ?         palette, size = 18400/factor
  ?         compressed image data, uncompressed size = 32000/factor

Additional information:
Images and animations use the same decompression method. Currently there is no
description of the compression method. The layout of the rasters and how to
calculate the color of a pixel at a given x/y coordinate is unknown.

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

#include "gfatable.c"
#include "gfadepac.c"

#define SCREEN_SIZE 32000L

struct file_header {
	uint32_t frame_count;
	uint32_t size_factor;
	uint32_t frame_sizes[10];
};

/* FIXME: statics */
static uint32_t frame_sizes[20];
static uint32_t frame_positions[20];
static struct file_header header;

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
	char fileid[5];
	int i;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	if (Fread(handle, sizeof(fileid), fileid) != sizeof(fileid) ||
		Fread(handle, sizeof(header), &header) != sizeof(header) ||
		header.frame_count > 9)
	{
		Fclose(handle);
		return FALSE;
	}
	header.frame_count += 1;
	for (i = 0; i < 10; i++)
		frame_sizes[i] = header.frame_sizes[i];

	bmap = malloc(SCREEN_SIZE + SCREEN_SIZE + 20000L);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}

	if (fileid[0] == 's' &&
		fileid[1] == 'a' &&
		fileid[2] == 'l' &&
		fileid[3] == 0x0d &&
		fileid[4] == 0x0a)
	{
		info->planes = 12;
		info->width = 320;
		info->height = 200 / (uint16_t)header.size_factor;
		info->components = 3;
		for (i = 0; i < (int)header.frame_count; i++)
		{
			frame_positions[i] = Fseek(0, handle, SEEK_CUR);
			Fseek(18400 / header.size_factor, handle, SEEK_CUR);
			Fseek(frame_sizes[i], handle, SEEK_CUR);
		}
	} else if (fileid[0] == 's' &&
		fileid[1] == 'a' &&
		fileid[2] == 'h' &&
		fileid[3] == 0x0d &&
		fileid[4] == 0x0a)
	{
		info->planes = 1;
		info->width = 640;
		info->height = 400 / (uint16_t)header.size_factor;
		info->components = 1;
		for (i = 0; i < (int)header.frame_count; i++)
		{
			frame_positions[i] = Fseek(0, handle, SEEK_CUR);
			Fseek(frame_sizes[i], handle, SEEK_CUR);
		}
	} else
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}

	/*
	 * build reverse frame map
	 */
	if (!(Kbshift(-1) & K_ALT))
	{
		int j;
		
		j = (int)header.frame_count;
		i = j - 1;
		while (i >= 0)
		{
			++header.frame_count;
			frame_positions[j] = frame_positions[i];
			frame_sizes[j] = frame_sizes[i];
			j++;
			i--;
		}
	}

	info->indexed_color = FALSE;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = header.frame_count;	/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = 0;
	info->_priv_ptr = bmap;
	info->__priv_ptr_more = (void *)(intptr_t)handle;
	
	strcpy(info->info, "GFA RayTrace (Animation)");
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

	switch (info->planes)
	{
	case 1:
		{
			int16_t byte;
			uint8_t *bmap;
			uint8_t *end;
			
			bmap = info->_priv_ptr;
			if (info->_priv_var_more == 0)
			{
				int handle;
				uint16_t *temp;
				
				handle = (int)(intptr_t)info->__priv_ptr_more;
				Fseek(frame_positions[info->page_wanted], handle, SEEK_SET);
				temp = (uint16_t *)(bmap + SCREEN_SIZE);
				if ((size_t)Fread(handle, frame_sizes[info->page_wanted], temp) != frame_sizes[info->page_wanted])
				{
					return FALSE;
				}
				gfa_depac(temp, (uint16_t *)bmap, SCREEN_SIZE / (uint16_t)header.size_factor);
				info->delay = 10;
			}
			end = buffer + info->width;
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
	
			++info->_priv_var_more;
			if (info->_priv_var_more == info->height)
			{
				info->_priv_var_more = 0;
				pos = 0;
			}
		}
		break;
	
	case 12:
		{
			int16_t x;
			uint16_t w;
			int16_t i;
			uint16_t byte;
			int16_t plane;
			uint16_t *bmap16;
			uint16_t *temp;
			uint16_t *colormaps;
			uint16_t idx;
			uint16_t color;
			
			bmap16 = (uint16_t *)info->_priv_ptr;
			temp = bmap16 + SCREEN_SIZE / 2;
			colormaps = temp + SCREEN_SIZE / 2;

			if (info->_priv_var_more == 0)
			{
				int j;
				int handle;
				
				for (j = 0; j < 63; j++)
					colormaps[j] = 0;
				handle = (int)(intptr_t)info->__priv_ptr_more;
				Fseek(frame_positions[info->page_wanted], handle, SEEK_SET);
				if ((size_t)Fread(handle, 18400 / header.size_factor, colormaps + 47) != 18400 / header.size_factor ||
					(size_t)Fread(handle, frame_sizes[info->page_wanted], temp) != frame_sizes[info->page_wanted])
				{
					return FALSE;
				}
				gfa_depac(temp, bmap16, SCREEN_SIZE / (uint16_t)header.size_factor);
				info->delay = 10;
			}
			x = 0;
			w = 0;
			do
			{
				for (i = 15; i >= 0; i--)
				{
					for (plane = byte = 0; plane < 4; plane++)
					{
						if ((bmap16[pos + plane] >> i) & 1)
							byte |= 1 << plane;
					}
					idx = colormap[byte * 320 + x];
					x++;
					color = colormaps[info->_priv_var_more * 46 + idx];
					buffer[w++] = (((color >> 7) & 0x0e) + ((color >> 11) & 1)) * 17;
					buffer[w++] = (((color >> 3) & 0x0e) + ((color >> 7) & 0x01)) * 17;
					buffer[w++] = (((color << 1) & 0x0e) + ((color >> 3) & 0x01)) * 17;
				}
				pos += 4;
			} while (x < info->width);
	
			++info->_priv_var_more;
			if (info->_priv_var_more == info->height)
			{
				info->_priv_var_more = 0;
				pos = 0;
			}
		}
		break;
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
