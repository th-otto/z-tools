#define	VERSION	     0x0201
#define NAME        "Paintworks"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
Paintworks    *.SC0 *.CL0 (st low resolution)
              *.SC1 *.CL1 (st medium resolution)
              *.SC2 *.CL2 (st high resolution)
              *.PG0 (st low resolution, double high)
              *.PG1 (st medium resolution, double high)
              *.PG2 (st high resolution, double high)

Early versions of Paintworks were called N-Vision. These are often mistaken as
NEOchrome files. One should not rely on the file extension to determine the
contents.

1 word       file id, $00
                 recommend using 'secondary file id' for identification
1 word       resolution [0 = low, 1 = medium, 2 = high]
                 recommend using 'flags' to determine resolution
16 words     palette
12 bytes     file name, usually ["        .   "]
1 word       ?
1 word       ?
1 word       ?
9 bytes      secondary file id, "ANvisionA"
1 byte       flags, bit field: cxrrtttt
                 c = compression flag [1 = compressed]
                 x = not used
                 r = resolution [0 = low, 1 = medium, 2 = high]
                 t = type [0 = page, 1 = screen, 2 = clipart, 4 = pattern]
                     page    = double high, thus 320x400, 640x400, 640x800
                     screen  = standard st resolutions
                     clipart = standard st resolutions
                     pattern = no documentation at this time
64 bytes     ?
---------
128 bytes    total for header

?            image data:
Uncompressed images are simply screen dumps as one might expect.

Compressed images are a two step process, RLE decoding and re-ordering.
RLE decompression:
see decode_rle() below.

The uncompressed data is arranged as follows:

ST low resolution:
    All of plane 0..., all of plane 1..., all of plane 2..., all of plane 3...
ST medium resolution
    All of plane 0..., all of plane 1...
ST high resolution:
    No reordering required.
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("CL0\0" "CL1\0" "CL2\0" "PG0\0" "PG1\0" "PG2\0" "SC0\0" "SC1\0" "SC2\0");

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

#define SCREEN_SIZE 32000L

struct file_header {
	uint16_t file_id;
	uint16_t resolution;
	uint16_t palette[16];
	char filename[12];
	uint16_t unknown1;
	uint16_t unknown2;
	uint16_t unknown3;
	char magic[9];
	uint8_t flags;
	char reserved[64];
};


/* Written by Lonny Pursell - placed in to Public Domain 1/28/2017 */

static void decode_rle(uint8_t *src, uint8_t *dst, size_t bms)
{
	uint8_t cmd;
	uint8_t chr;
	uint16_t i;
	size_t srcpos = 0;
	size_t dstpos = 0;

	do
	{
		cmd = src[srcpos++];
		if (cmd & 0x80)
		{								/* literal? */
			cmd = cmd & 0x7f;
			memcpy(&dst[dstpos], &src[srcpos], (size_t) cmd);
			dstpos = dstpos + (size_t) cmd;
			srcpos = srcpos + (size_t) cmd;
		} else
		{								/* repeat? */
			chr = src[srcpos++];
			for (i = 0; i < cmd; i++)
			{
				dst[dstpos++] = chr;
			}
		}
	} while (dstpos < bms);
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
	size_t image_size = SCREEN_SIZE;
	int16_t handle;
	uint8_t *bmap;
	uint8_t *temp;
	int compressed;
	int resolution;
	int type;
	int inverse;
	size_t file_size;
	struct file_header header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (header.file_id != 0 || memcmp(header.magic, "ANvisionA", 9) != 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	
	compressed = header.flags & 0x80;
	resolution = (header.flags & 0x30) >> 4;
	type = header.flags & 0x0f;
	info->planes = 4 >> resolution;
	inverse = header.palette[0] & 1;
	
	switch (resolution)
	{
	case 0:
		info->planes = 4;
		info->width = 320;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 1:
		info->planes = 2;
		info->width = 640;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 2:
		info->planes = 1;
		info->width = 640;
		info->height = 400;
		info->indexed_color = FALSE;
		break;
	default:
		Fclose(handle);
		RETURN_ERROR(EC_ResolutionType);
	}

	strcpy(info->info, "Paintworks");
	
	switch (type)
	{
	case 0:
		image_size = image_size * 2;
		info->height = info->height * 2;
		strcat(info->info, " (Page)");
		break;
	case 1:
		strcat(info->info, " (Screen)");
		break;
	case 2:
		strcat(info->info, " (Clipart)");
		break;
	case 4:
	default:
		Fclose(handle);
		RETURN_ERROR(EC_ImageType);
	}
	
	bmap = malloc(image_size + 1024);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	file_size -= sizeof(header);
	if (compressed)
	{
		if (file_size > image_size ||
			(size_t)Fread(handle, file_size, bmap) != file_size)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		temp = malloc(image_size + 1024);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		decode_rle(bmap, temp, image_size);
		switch (info->planes)
		{
		case 1:
			memcpy(bmap, temp, image_size);
			break;

		case 2:
			{
				size_t plane0;
				size_t plane1;
				uint16_t *dst;
				uint16_t *temp16;
				uint16_t *end;
	
				plane0 = 0;
				plane1 = image_size / 4;
				dst = (uint16_t *)bmap;
				temp16 = (uint16_t *)temp;
				end = &dst[image_size / 2];
				do
				{
					*dst++ = temp16[plane0++];
					*dst++ = temp16[plane1++];
				} while (dst < end);
			}
			break;

		case 4:
			{
				size_t plane_size;
				size_t plane0;
				size_t plane1;
				size_t plane2;
				size_t plane3;
				uint16_t *dst;
				uint16_t *temp16;
				uint16_t *end;
	
				plane_size = image_size / 8;
				plane0 = 0;
				plane1 = plane_size;
				plane2 = plane_size * 2;
				plane3 = plane_size * 3;
				dst = (uint16_t *)bmap;
				temp16 = (uint16_t *)temp;
				end = &dst[image_size / 2];
				do
				{
					*dst++ = temp16[plane0++];
					*dst++ = temp16[plane1++];
					*dst++ = temp16[plane2++];
					*dst++ = temp16[plane3++];
				} while (dst < end);
			}
			break;
		}
		
		free(temp);
		strcpy(info->compression, "RLE");
	} else
	{
		if ((size_t) Fread(handle, image_size, bmap) != image_size)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		strcpy(info->compression, "None");
	}
	Fclose(handle);
	
	switch (info->planes)
	{
	case 2:
		if (Kbshift(-1) & K_CAPSLOCK)
		{
			size_t y;
			
			temp = malloc(image_size);
			if (temp == NULL)
			{
				free(bmap);
				RETURN_ERROR(EC_Malloc);
			}
			memcpy(temp, bmap, image_size);
			free(bmap);
			bmap = malloc(image_size * 2);
			if (bmap == NULL)
			{
				free(temp);
				RETURN_ERROR(EC_Malloc);
			}
			for (y = 0; y < info->height; y++)
			{
				memcpy(&bmap[y * 2 * 160], &temp[y * 160], 160);
				memcpy(&bmap[y * 2 * 160 + 160], &temp[y * 160], 160);
			}
			free(temp);
			info->height *= 2;
		}
		break;
	case 1:
		if (!inverse)
		{
			size_t i;
			
			for (i = 0; i < image_size; i++)
			{
				bmap[i] = ~bmap[i];
			}
		}
		break;
	}
	
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

	if (info->indexed_color)
	{	
		int i;

		for (i = 0; i < 16; i++)
		{
			info->palette[i].red = (((header.palette[i] >> 7) & 0x0e) + ((header.palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((header.palette[i] >> 3) & 0x0e) + ((header.palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((header.palette[i] << 1) & 0x0e) + ((header.palette[i] >> 3) & 0x01)) * 17;
		}
	}

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
	switch (info->planes)
	{
	case 1:
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
		}
		break;

	case 2:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t *ptr;
			
			ptr = (uint16_t *)(info->_priv_ptr + info->_priv_var);
			info->_priv_var += info->width >> 2;
			x = info->width >> 4;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2);
					plane0 <<= 1;
					plane1 <<= 1;
				}
			} while (--x > 0);
		}
		break;

	case 4:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t *ptr;
			
			ptr = (uint16_t *)(info->_priv_ptr + info->_priv_var);
			info->_priv_var += info->width >> 1;
			x = info->width >> 4;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				plane2 = *ptr++;
				plane3 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4) |
						((plane3 >> 12) & 8);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
				}
			} while (--x > 0);
		}
		break;
	}

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
