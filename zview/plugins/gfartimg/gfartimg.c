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

#define SCREEN_SIZE 32000L

struct file_header {
	char fileid[5];
	char size_factor;
	char crlf[2];
};
#include "../gfartani/gfatable.c"
#include "../gfartani/gfadepac.c"


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
	uint32_t file_size;
	uint32_t data_size;
	uint8_t *bmap;
	uint16_t *colormaps;
	struct file_header header;
	uint16_t size_factor;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		return FALSE;
	}

	bmap = malloc(SCREEN_SIZE + 20000L);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	colormaps = (uint16_t *)(bmap + SCREEN_SIZE);

	if (header.fileid[0] == 's' &&
		header.fileid[1] == 'u' &&
		header.fileid[2] == 'l' &&
		header.fileid[3] == 0x0d &&
		header.fileid[4] == 0x0a)
	{
		int j;
		
		/*
		 * Uncompressed, low resolution
		 */
		for (j = 0; j < 63; j++)
			colormaps[j] = 0;
		size_factor = header.size_factor - '0';
		
		info->planes = 12;
		info->width = 320;
		info->height = 200 / size_factor;
		info->components = 3;

		if (Fread(handle, SCREEN_SIZE / size_factor, bmap) != SCREEN_SIZE / size_factor ||
			Fread(handle, 18400 / size_factor, colormaps + 63) != 18400 / size_factor)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		strcpy(info->compression, "None");
	} else if (
		header.fileid[0] == 's' &&
		header.fileid[1] == 'c' &&
		header.fileid[2] == 'l' &&
		header.fileid[3] == 0x0d &&
		header.fileid[4] == 0x0a)
	{
		int j;
		uint16_t *temp;

		/*
		 * Compressed, low resolution
		 */
		for (j = 0; j < 63; j++)
			colormaps[j] = 0;
		size_factor = header.size_factor - '0';
		
		info->planes = 12;
		info->width = 320;
		info->height = 200 / size_factor;
		info->components = 3;

		data_size = 18400 / size_factor;
		if ((size_t)Fread(handle, data_size, colormaps + 63) != data_size)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		temp = malloc(SCREEN_SIZE);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		data_size = file_size - data_size - 8;
		if (data_size > SCREEN_SIZE || (size_t)Fread(handle, data_size, temp) != data_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		gfa_depac(temp, (uint16_t*)bmap, SCREEN_SIZE / size_factor);
		free(temp);
		strcpy(info->compression, "RLE");
	} else if (
		header.fileid[0] == 's' &&
		header.fileid[1] == 'c' &&
		header.fileid[2] == 'h' &&
		header.fileid[3] == 0x0d &&
		header.fileid[4] == 0x0a)
	{
		uint16_t *temp;
		
		/* Compressed, high resolution */
		size_factor = header.size_factor - '0';
		info->planes = 1;
		info->width = 640;
		info->height = 400 / size_factor;
		info->components = 1;
		
		temp = malloc(SCREEN_SIZE);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		data_size = file_size - 8;
		if (data_size > SCREEN_SIZE || (size_t)Fread(handle, data_size, temp) != data_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		gfa_depac(temp, (uint16_t*)bmap, SCREEN_SIZE / size_factor);
		free(temp);
		strcpy(info->compression, "RLE");
	} else if (
		header.fileid[0] == 0x00 &&
		header.fileid[1] == 0x02 &&
		file_size == SCREEN_SIZE + 34)
	{
		/* Uncompressed, high resolution */
		info->planes = 1;
		info->width = 640;
		info->height = 400;
		info->components = 1;
		
		Fseek(34, handle, SEEK_SET);
		if (Fread(handle, SCREEN_SIZE, bmap) != SCREEN_SIZE)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		strcpy(info->compression, "None");
	} else
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}

	Fclose(handle);
	
	info->indexed_color = FALSE;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = 0;
	info->_priv_ptr = bmap;
	
	strcpy(info->info, "GFA RayTrace (Image)");
	
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
			uint16_t *colormaps;
			uint16_t idx;
			uint16_t color;
			
			bmap16 = (uint16_t *)info->_priv_ptr;
			colormaps = bmap16 + SCREEN_SIZE / 2;

			x = 0;
			w = 0;
			do
			{
				for (i = 15; i >= 0; i--)
				{
					/*
					 * FIXME: unroll inner loop
					 */
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
