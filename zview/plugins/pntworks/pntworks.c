#include "plugin.h"
#include "zvplugin.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"

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

--------------------------------------------------------------------------------

Graphics Processor    *.PG1 (ST low resolution)
                      *.PG2 (ST medium resolution)
                      *.PG3 (ST high resolution)

1 word       resolution + 10 if compressed [0/10=low, 1/11=medium, 2/12=high]
16 words     palette 1 (seems to be the main palette)
16 words     palette 2
16 words     palette 3
16 words     palette 4
16 words     palette 5
16 words     palette 6
16 words     palette 7
16 words     palette 8
16 words     palette 9
16 words     palette 10
1 byte       unknown
1 word       animation limit?
1 byte       animation direction?
1 byte       animation ?
1 word       animation ?
1 word       animation speed?
---------
331 bytes    total for header

??           image data:
If uncompressed data is simply screen memory (32000 bytes) as expected.

If compressed a very simple RLE method is used. The unpacked data is standard
st bitmap.
1 word       size of compressed data
do
  1 byte     repeat count
  ? data     data to be repeated count times
loop until 32000 bytes are unpacked
data size: 1 plane=byte, 2 planes=word, 4 planes=long
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

struct pntworks_header {
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

struct gp_header {
	uint16_t resolution;
	uint16_t palette[10][16];
};


/* Written by Lonny Pursell - placed in to Public Domain 1/28/2017 */

static void decode_rle(uint8_t *src, uint8_t *dst, size_t bms)
{
	int16_t cmd;
	uint8_t chr;
	uint8_t *end = dst + bms;

	do
	{
		cmd = *src++;
		if (cmd & 0x80)
		{								/* literal? */
			cmd &= 0x7f;
			while (--cmd >= 0)
				*dst++ = *src++;
		} else
		{								/* repeat? */
			chr = *src++;
			while (--cmd >= 0)
				*dst++ = chr;
		}
	} while (dst < end);
}


static void decode_gp(uint8_t *src, uint8_t *dst, uint16_t datasize)
{
	int16_t count;
	uint8_t *end = dst + SCREEN_SIZE;
	
	src += 2; /* skip compressed size field */
	switch (datasize)
	{
	case 1:
		do
		{
			uint8_t data0;
			
			count = *src++;
			data0 = *src++;
			while (--count >= 0)
				*dst++ = data0;
		} while (dst < end);
		break;

	case 2:
		do
		{
			uint8_t data0;
			uint8_t data1;
	
			count = *src++;
			data0 = *src++;
			data1 = *src++;
			while (--count >= 0)
			{
				*dst++ = data0;
				*dst++ = data1;
			}
		} while (dst < end);
		break;

	case 4:
		do
		{
			uint8_t data0;
			uint8_t data1;
			uint8_t data2;
			uint8_t data3;
	
			count = *src++;
			data0 = *src++;
			data1 = *src++;
			data2 = *src++;
			data3 = *src++;
			while (--count >= 0)
			{
				*dst++ = data0;
				*dst++ = data1;
				*dst++ = data2;
				*dst++ = data3;
			}
		} while (dst < end);
		break;
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
	size_t image_size = SCREEN_SIZE;
	int16_t handle;
	uint8_t *bmap;
	uint8_t *temp;
	int compressed;
	int resolution;
	size_t file_size;
	struct pntworks_header pntworks_header;
	struct gp_header gp_header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	if (Fread(handle, sizeof(pntworks_header), &pntworks_header) != sizeof(pntworks_header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (pntworks_header.file_id == 0 && memcmp(pntworks_header.magic, "ANvisionA", 9) == 0)
	{
		int type;
		int inverse;

		/*
		 * Paintworks code
		 */
		compressed = pntworks_header.flags & 0x80;
		resolution = (pntworks_header.flags & 0x30) >> 4;
		type = pntworks_header.flags & 0x0f;
		inverse = pntworks_header.palette[0] & 1;
		
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
		
		bmap = malloc(image_size);
		if (bmap == NULL)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		file_size -= sizeof(pntworks_header);
		if (compressed)
		{
			if (file_size > image_size ||
				(size_t)Fread(handle, file_size, bmap) != file_size)
			{
				free(bmap);
				Fclose(handle);
				RETURN_ERROR(EC_Fread);
			}
			temp = malloc(image_size);
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
				free(bmap);
				Fclose(handle);
				RETURN_ERROR(EC_Fread);
			}
			strcpy(info->compression, "None");
		}

		if (info->planes == 1)
		{
			if (!inverse)
			{
				size_t i;
				
				for (i = 0; i < image_size; i++)
				{
					bmap[i] = ~bmap[i];
				}
			}
		}
	
		if (info->indexed_color)
		{	
			int i;
	
			for (i = 0; i < 16; i++)
			{
				info->palette[i].red = (((pntworks_header.palette[i] >> 7) & 0x0e) + ((pntworks_header.palette[i] >> 11) & 0x01)) * 17;
				info->palette[i].green = (((pntworks_header.palette[i] >> 3) & 0x0e) + ((pntworks_header.palette[i] >> 7) & 0x01)) * 17;
				info->palette[i].blue = (((pntworks_header.palette[i] << 1) & 0x0e) + ((pntworks_header.palette[i] >> 3) & 0x01)) * 17;
			}
		}
	} else
	{
		uint8_t animation[9];

		/*
		 * Graphics Processor code
		 */
		Fseek(0, handle, SEEK_SET);
		if (Fread(handle, sizeof(gp_header), &gp_header) != sizeof(gp_header) ||
			Fread(handle, sizeof(animation), animation) != sizeof(animation))
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}

		if (gp_header.resolution > 2)
		{
			compressed = TRUE;
			resolution = gp_header.resolution - 10;
		} else
		{
			compressed = FALSE;
			resolution = gp_header.resolution;
		}
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

		bmap = malloc(image_size);
		if (bmap == NULL)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}

		file_size -= sizeof(gp_header) + sizeof(animation);
		if (compressed)
		{
			temp = malloc(file_size);
			if (temp == NULL)
			{
				free(bmap);
				Fclose(handle);
				RETURN_ERROR(EC_Malloc);
			}
			if ((size_t)Fread(handle, file_size, temp) != file_size)
			{
				free(temp);
				free(bmap);
				Fclose(handle);
				RETURN_ERROR(EC_Malloc);
			}
			decode_gp(temp, bmap, info->planes);
			free(temp);
			strcpy(info->compression, "RLE");
		} else
		{
			if ((size_t)Fread(handle, image_size, bmap) != image_size)
			{
				free(bmap);
				Fclose(handle);
				RETURN_ERROR(EC_Fread);
			}
			strcpy(info->compression, "None");
		}

		strcpy(info->info, "Graphics Processor");

		if (info->indexed_color)
		{
			int i;
	
			for (i = 0; i < 16; i++)
			{
				info->palette[i].red = (((gp_header.palette[0][i] >> 7) & 0x0e) + ((gp_header.palette[0][i] >> 11) & 0x01)) * 17;
				info->palette[i].green = (((gp_header.palette[0][i] >> 3) & 0x0e) + ((gp_header.palette[0][i] >> 7) & 0x01)) * 17;
				info->palette[i].blue = (((gp_header.palette[0][i] << 1) & 0x0e) + ((gp_header.palette[0][i] >> 3) & 0x01)) * 17;
			}
		}
	}
	
	Fclose(handle);
	
	info->components = 1;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

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
