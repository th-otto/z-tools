#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"
#define NF_DEBUG 0
#include "nfdebug.h"

/*
TruePaint      *.TPI
Prism Paint    *.PNT

1 long       file ID, $504e5400 'PNT\0'
1 word       $0100 (file version?)
1 word       palette_size (might be there in true color)
1 word       x_size
1 word       y_size
1 word       bits_per_pixel
1 word       compression, $0000 uncompressed, $0001 packbits compressed
1 long       image_data_size = ((x_size+15)&0xffff0)*y_size*bits_per_pixel/8
             (uncompressed)
108 bytes    $0
---------
128 bytes    total for header

3*palette_size   words    VDI palette, RGB values 0-1000, VDI order
image_data_size  bytes    The line length is a multiple of 16 pixels for all
                            bit depths
                          Interleaved bitplanes for bits_per_pixel<16
                          16 bit word per pixel for 16-bit pictures,
                            RRRRrGGGGGgBBBBBb
                          24 bit is stored R, G, B in that order

The Prism Paint compressed format uses the Packbits compression algorithm. For
every scan line the bitplanes are compressed separately. The falcon high-color
resolution (16 bit) is compressed as if there were 16 bitplanes, which there
are not. The same rule applies to 24 bits per pixel.

Additional information:
The TruePaint manual describes the format as designed for high speed and only
writes uncompressed files. Prism Paint on the other hand offers compression
every time a file is saved with an alert box (yes/no).

See also PackBits Compression Algorithm

--------------------------------------------------------------------------------

TruePaint Animation    *.TPA

1 long       file id, $54504100 'TPA\0'
1 word       version? [$0100]
1 word       palette size [0 = none]
1 word       width in pixels
1 word       height in pixels
1 word       planes
1 word       compression [always 0 = uncompressed]
1 long       image data size = ((width+15)&0xffff0)*height*bits_per_pixel/8
104 bytes    reserved? [$0]
1 word       speed
1 word       length of sequence (not necessarily total frames)
---------
128 bytes    total for header

3*palette_size words     VDI palette, RGB values 0-1000, VDI order
image_data_size bytes    1st frame, always a full frame width x height in size

For each frame, until end of file {
1 long                   frame data size
frame_data_size bytes    frame data (structure unknown)
}

The first frame is always a standard Atari ST bitmap, including Falcon modes.

The structure of individual frames is still unknown.
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

#include "../degas/packbits.c"
#include "../degas/tanglbts.c"

struct file_header {
	uint32_t magic;
	uint16_t version;
	uint16_t palette_size;
	uint16_t width;
	uint16_t height;
	uint16_t planes;
	uint16_t compression;
	uint32_t image_size;
	char reserved[104];
	uint16_t speed;
	uint16_t num_frames;
};

#if 0
static uint8_t const vdi2bios_4[4] = { 0, 2, 3, 1 };
static uint8_t const vdi2bios_16[16] = {
	0x00, 0x0f, 0x01, 0x02, 0x04, 0x06, 0x03, 0x05,
	0x07, 0x08, 0x09, 0x0a, 0x0c, 0x0e, 0x0b, 0x0d
};
static uint8_t const vdi2bios_256[256] = {
	0x00, 0xff, 0x01, 0x02, 0x04, 0x06, 0x03, 0x05,
	0x07, 0x08, 0x09, 0x0a, 0x0c, 0x0e, 0x0b, 0x0d,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0x0f
};
#endif

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
	uint8_t *temp;
	struct file_header header;
	uint16_t palette[256][3];
	size_t bytes_per_line;
	size_t file_size;
	size_t image_size;

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
	nf_debugprintf(("magic: 0x%08x planes=%u compression=%u\n", header.magic, header.planes, header.compression));
	if (header.magic != 0x504E5400L && /* 'PNT\0' */
		header.magic != 0x54504100L)   /* 'TPA\0' */
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	file_size -= sizeof(header);

	switch (header.planes)
	{
	case 1:
	case 16:
	case 24:
		info->indexed_color = FALSE;
		break;
	case 2:
	case 4:
	case 8:
		info->indexed_color = TRUE;
		break;
	default:
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}
	
	bytes_per_line = (header.width + 15) >> 4;
	bytes_per_line = bytes_per_line * 2 * header.planes;
	image_size = bytes_per_line * header.height;
	
	if (header.palette_size != 0)
	{
		size_t palette_size;
		
		if (header.palette_size > 256)
			header.palette_size = 256;
		palette_size = (size_t)header.palette_size * 3 * 2;
		if ((size_t)Fread(handle, palette_size, palette) != palette_size)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		file_size -= palette_size;
	}
	nf_debugprintf(("filesize=%lu imagesize=%lu datasize=%u\n", file_size, image_size, header.image_size));
	
	bmap = malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if (header.compression == 0)
	{
		if ((size_t)Fread(handle, image_size, bmap) != image_size)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		strcpy(info->compression, "None");
		strcpy(info->info, "TruePaint");
		if (header.magic == 0x54504100L)
			strcat(info->info, " (Animation)");
	} else if (header.compression == 1)
	{
		if (file_size > image_size)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_FileLength);
		}
		temp = malloc(image_size);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		if ((size_t)Fread(handle, file_size, bmap) != file_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		decode_packbits(temp, bmap, image_size);
		if (header.planes > 1 && header.planes <= 8)
			tangle_bitplanes(bmap, temp, header.width, header.height, header.planes);
		else
			memcpy(bmap, temp, image_size);
		free(temp);
		strcpy(info->compression, "RLE");
		strcpy(info->info, "Prism Paint");
	} else
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_CompType);
	}
	Fclose(handle);
	
	info->planes = header.planes;
	info->components = info->planes == 1 ? 1 : 3;
	info->width = header.width;
	info->height = header.height;
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

		for (i = 0; i < (int)header.palette_size; i++)
		{
			info->palette[vdi2bios(i, header.planes)].red = ((((long)palette[i][0] << 8) - palette[i][0]) + 500) / 1000;
			info->palette[vdi2bios(i, header.planes)].green = ((((long)palette[i][1] << 8) - palette[i][1]) + 500) / 1000;
			info->palette[vdi2bios(i, header.planes)].blue = ((((long)palette[i][2] << 8) - palette[i][2]) + 500) / 1000;
		}
	}
	
	if (header.planes == 1)
	{
		unsigned int color0 = info->palette[0].red + info->palette[0].green + info->palette[0].blue;
		unsigned int color1 = info->palette[1].red + info->palette[1].green + info->palette[1].blue;
		if (color0 < color1)
		{
			size_t i;
			
			for (i = 0; i < image_size; i++)
				bmap[i] = ~bmap[i];
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
		
			x = ((info->width + 15) >> 4) << 1;
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
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			x = (info->width + 15) >> 4;
			info->_priv_var += x << 2;
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
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			x = (info->width + 15) >> 4;
			info->_priv_var += x << 3;
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
	
	case 8:
		{
			int x;
			unsigned int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t plane4;
			uint16_t plane5;
			uint16_t plane6;
			uint16_t plane7;
			uint16_t *ptr;
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			x = (info->width + 15) >> 4;
			info->_priv_var += x << 4;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				plane2 = *ptr++;
				plane3 = *ptr++;
				plane4 = *ptr++;
				plane5 = *ptr++;
				plane6 = *ptr++;
				plane7 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4) |
						((plane3 >> 12) & 8) |
						((plane4 >> 11) & 16) |
						((plane5 >> 10) & 32) |
						((plane6 >> 9) & 64) |
						((plane7 >> 8) & 128);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
					plane4 <<= 1;
					plane5 <<= 1;
					plane6 <<= 1;
					plane7 <<= 1;
				}
			} while (--x > 0);
		}
		break;
	
	case 15:
	case 16:
		{
			int x;
			const uint16_t *screen16;
			uint16_t color;
			
			x = (info->width + 15) & -16;
			screen16 = (const uint16_t *)info->_priv_ptr + info->_priv_var;
			info->_priv_var += x;
			do
			{
				color = *screen16++;
				*buffer++ = ((color >> 11) & 0x1f) << 3;
				*buffer++ = ((color >> 5) & 0x3f) << 2;
				*buffer++ = ((color) & 0x1f) << 3;
			} while (--x > 0);
		}
		break;
	
	case 24:
		{
			int x;
			const uint8_t *bmap;
			
			x = ((info->width + 15) & -16);
			bmap = (const uint8_t *)info->_priv_ptr + info->_priv_var;
			info->_priv_var += x * 3;
			do
			{
				*buffer++ = *bmap++;
				*buffer++ = *bmap++;
				*buffer++ = *bmap++;
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
	void *p = info->_priv_ptr;
	info->_priv_ptr = 0;
	free(p);
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
