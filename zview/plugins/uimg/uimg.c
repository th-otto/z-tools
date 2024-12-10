#include "plugin.h"
#include "zvplugin.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"

/*
 See https://github.com/mikrosk/uconvert
*/


#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
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


typedef struct {
	uint32_t    id;
	uint16_t    version;
	uint16_t    flags;
	uint8_t     bitsPerPixel;
	int8_t      bytesPerChunk;
	uint16_t	width;
	uint16_t	height;
} FileHeader;

typedef enum {
    PaletteTypeNone,
    PaletteTypeSTE,
    PaletteTypeTT,
    PaletteTypeFalcon
} PaletteType;

static uint8_t const greytab_2[2] = { 0x00, 0xff };
static uint8_t const greytab_4[4] = { 0x00, 0x55, 0xaa, 0xff };
static uint8_t const greytab_8[8] = { 0x00, 0x24, 0x48, 0x6d, 0x91, 0xb6, 0xda, 0xff };
static uint8_t const greytab_16[16] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};
static uint8_t const greytab_32[32] = {
	0x00, 0x08, 0x10, 0x18, 0x20, 0x29, 0x31, 0x39,
	0x41, 0x4a, 0x52, 0x5a, 0x62, 0x6a, 0x73, 0x7b,
	0x83, 0x8b, 0x94, 0x9c, 0xa4, 0xac, 0xb4, 0xbd,
	0xc5, 0xcd, 0xd5, 0xde, 0xe6, 0xee, 0xf6, 0xff
};
static uint8_t const greytab_64[64] = {
	0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
	0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c,
	0x40, 0x44, 0x48, 0x4c, 0x50, 0x55, 0x59, 0x5d,
	0x61, 0x65, 0x69, 0x6d, 0x71, 0x75,	0x79, 0x7d,
	0x81, 0x85, 0x89, 0x8d, 0x91, 0x95, 0x99, 0x9d,
	0xa1, 0xa5, 0xaa, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
	0xc2, 0xc6,	0xca, 0xce, 0xd2, 0xd6, 0xda, 0xde,
	0xe2, 0xe6, 0xea, 0xee, 0xf2, 0xf6, 0xfa, 0xff
};
static uint8_t const greytab_128[128] = {
	0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e,
	0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
	0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e,
	0x30, 0x32, 0x34, 0x36, 0x38, 0x3a,	0x3c, 0x3e,
	0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e,
	0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e,
	0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e,
	0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e,
	0x80, 0x82, 0x84, 0x86, 0x88, 0x8a,	0x8c, 0x8e,
	0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e,
	0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae,
	0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe,
	0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce,
	0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde,
	0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee,
	0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xff
};
static uint8_t greytab_256[256];



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
	uint8_t *bmap;
	int16_t handle;
	size_t bytes_per_row;
	FileHeader header;
	int palette_type;
	size_t image_size;
	
	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (header.id != 0x55494D47L) /* 'UIMG' */
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	if (header.version < 0x100 || header.version >= 0x200)
	{
		Fclose(handle);
		RETURN_ERROR(EC_HeaderVersion);
	}
	info->planes = header.bitsPerPixel;
	switch (info->planes)
	{
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 16:
	case 24:
	case 32:
		break;
	default:
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}

	palette_type = header.flags & 3;
	strcpy(info->info, "uConvert bitmap format");
	if (info->planes <= 8)
	{
		const uint8_t *colormap;
		int colors;
		int i;
		size_t palette_size;

		colors = 1 << info->planes;
		info->colors = colors;
		info->indexed_color = info->planes > 1;
		info->components = 1;

		/*
		 * bytesPerChunk can be
		 * - 0: planar,
		 * - 1: chunky, 1 byte per pixel
		 * - -1: chunky, packed pixels
		 * 
		 * packed pixels is only supported for 1, 2, 4 & 8 planes
		 */
		if (header.bytesPerChunk > 1)
		{
			Fclose(handle);
			RETURN_ERROR(EC_PixelDepth);
		}
		if (header.bytesPerChunk < 0 && info->planes != 1 && info->planes != 2 && info->planes != 4 && info->planes != 8)
		{
			Fclose(handle);
			RETURN_ERROR(EC_PixelDepth);
		}
		switch (palette_type)
		{
		case PaletteTypeNone:
			switch (info->planes)
			{
			case 1:
				colormap = greytab_2;
				break;
			case 2:
				colormap = greytab_4;
				break;
			case 3:
				colormap = greytab_8;
				break;
			case 4:
				colormap = greytab_16;
				break;
			case 5:
				colormap = greytab_32;
				break;
			case 6:
				colormap = greytab_64;
				break;
			case 7:
				colormap = greytab_128;
				break;
			default:
				for (i = 0; i < 256; i++)
					greytab_256[i] = i;
				colormap = greytab_256;
				break;
			}
			for (i = 0; i < colors; i++)
			{
				info->palette[i].red = 
				info->palette[i].green = 
				info->palette[i].blue = colormap[i];
			}
			break;

		case PaletteTypeSTE:
			{
				uint16_t palette[256];

				palette_size = (size_t)colors * 2;
				strcat(info->info, " (ST/E)");
				if ((size_t)Fread(handle, palette_size, palette) != palette_size)
				{
					Fclose(handle);
					RETURN_ERROR(EC_Fread);
				}
				for (i = 0; i < colors; i++)
				{
					info->palette[i].red = greytab_16[((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)];
					info->palette[i].green = greytab_16[((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)];
					info->palette[i].blue = greytab_16[((palette[i] << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)];
				}
			}
			break;


		case PaletteTypeTT:
			{
				uint16_t palette[256];

				palette_size = (size_t)colors * 2;
				strcat(info->info, " (TT030)");
				if ((size_t)Fread(handle, palette_size, palette) != palette_size)
				{
					Fclose(handle);
					RETURN_ERROR(EC_Fread);
				}
				for (i = 0; i < colors; i++)
				{
					info->palette[i].red = greytab_16[(palette[i] >> 8) & 0x0f];
					info->palette[i].green = greytab_16[(palette[i] >> 4) & 0x0f];
					info->palette[i].blue = greytab_16[palette[i] & 0x0f];
				}
			}
			break;
		
		case PaletteTypeFalcon:
			{
				uint8_t palette[256][4];

				palette_size = (size_t)colors * 4;
				strcat(info->info, " (Falcon030)");
				if ((size_t)Fread(handle, palette_size, palette) != palette_size)
				{
					Fclose(handle);
					RETURN_ERROR(EC_Fread);
				}
				for (i = 0; i < colors; i++)
				{
					info->palette[i].red = greytab_64[palette[i][0] >> 2];
					info->palette[i].green = greytab_64[palette[i][1] >> 2];
					info->palette[i].blue = greytab_64[palette[i][3] >> 2];
				}
			}
			break;
		}
	} else
	{
		info->colors = 1L << MIN(info->planes, 24);
		info->indexed_color = FALSE;
		info->components = 3;
		if (palette_type != PaletteTypeNone)
		{
			Fclose(handle);
			RETURN_ERROR(EC_ColorMapType);
		}
		if (info->planes != header.bytesPerChunk * 8)
		{
			Fclose(handle);
			RETURN_ERROR(EC_PixelDepth);
		}
	}
	
	if (header.bytesPerChunk > 0)
		bytes_per_row = (size_t)header.bytesPerChunk * header.width;
	else
		bytes_per_row = (((size_t)header.width + 15) >> 4) * 2 * (size_t)info->planes;
	image_size = bytes_per_row * header.height;
	bmap = (void *)Malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, image_size, bmap) != image_size)
	{
		Mfree(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);

	if (info->planes == 1)
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

	info->width = header.width;
	info->height = header.height;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;

	strcpy(info->compression, "None");

	info->_priv_ptr = bmap;
	info->_priv_var = 0;		/* y offset */
	info->_priv_var_more = header.bytesPerChunk;

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
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	int chunky = (int)info->_priv_var_more;
	int x;
	
	bmap += info->_priv_var;
	x = info->width;
	
	switch (info->planes)
	{
	case 1:
		if (chunky <= 0)
		{
			int16_t byte;

			/* packed or bitplane */
			x = ((x + 15) >> 4) * 2;
			info->_priv_var += x;
			do
			{
				byte = *bmap++;
				*buffer++ = (byte >> 7) & 1;
				*buffer++ = (byte >> 6) & 1;
				*buffer++ = (byte >> 5) & 1;
				*buffer++ = (byte >> 4) & 1;
				*buffer++ = (byte >> 3) & 1;
				*buffer++ = (byte >> 2) & 1;
				*buffer++ = (byte >> 1) & 1;
				*buffer++ = (byte >> 0) & 1;
			} while (--x > 0);
		} else
		{
			/* 1 byte per pixel */
			info->_priv_var += x;
			do
			{
				*buffer++ = *bmap++;
			} while (--x > 0);
		}
		break;
		
	case 2:
		if (chunky < 0)
		{
			uint16_t byte;

			/* packed */
			x = ((x + 15) >> 4) * 4;
			info->_priv_var += x;
			do
			{
				byte = *bmap++;
				*buffer++ = (byte >> 6) & 3;
				*buffer++ = (byte >> 4) & 3;
				*buffer++ = (byte >> 2) & 3;
				*buffer++ = (byte >> 0) & 3;
			} while (--x > 0);
		} else if (chunky == 0)
		{
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t *bmap16 = (uint16_t *)bmap;

			/* bitplanes */
			x = ((x + 15) >> 4);
			info->_priv_var += (size_t)x * 4;
			do
			{
				plane0 = *bmap16++;
				plane1 = *bmap16++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2);
					plane0 <<= 1;
					plane1 <<= 1;
				}
			} while (--x > 0);
		} else
		{
			/* 1 byte per pixel */
			info->_priv_var += x;
			do
			{
				*buffer++ = *bmap++;
			} while (--x > 0);
		}
		break;
	
	case 3:
		if (chunky < 0)
		{
			/* packed */
			RETURN_ERROR(EC_PixelDepth);
		} else if (chunky == 0)
		{
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t *bmap16 = (uint16_t *)bmap;

			/* bitplanes */
			x = ((x + 15) >> 4);
			info->_priv_var += (size_t)x * 6;
			do
			{
				plane0 = *bmap16++;
				plane1 = *bmap16++;
				plane2 = *bmap16++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
				}
			} while (--x > 0);
		} else
		{
			/* 1 byte per pixel */
			info->_priv_var += x;
			do
			{
				*buffer++ = *bmap++;
			} while (--x > 0);
		}
		break;
	
	case 4:
		if (chunky < 0)
		{
			int16_t byte;

			/* packed */
			x = ((x + 15) >> 4) * 8;
			info->_priv_var += x;
			do
			{
				byte = *bmap++;
				*buffer++ = (byte >> 4) & 15;
				*buffer++ = (byte >> 0) & 15;
			} while (--x > 0);
		} else if (chunky == 0)
		{
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t *bmap16 = (uint16_t *)bmap;

			/* bitplanes */
			x = (x + 15) >> 4;
			info->_priv_var += (size_t)x * 8;
			do
			{
				plane0 = *bmap16++;
				plane1 = *bmap16++;
				plane2 = *bmap16++;
				plane3 = *bmap16++;
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
		} else
		{
			/* 1 byte per pixel */
			info->_priv_var += x;
			do
			{
				*buffer++ = *bmap++;
			} while (--x > 0);
		}
		break;
	
	case 5:
		if (chunky < 0)
		{
			/* packed */
			RETURN_ERROR(EC_PixelDepth);
		} else if (chunky == 0)
		{
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t plane4;
			uint16_t *bmap16 = (uint16_t *)bmap;

			/* bitplanes */
			x = ((x + 15) >> 4);
			info->_priv_var += (size_t)x * 10;
			do
			{
				plane0 = *bmap16++;
				plane1 = *bmap16++;
				plane2 = *bmap16++;
				plane3 = *bmap16++;
				plane4 = *bmap16++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4) |
						((plane3 >> 12) & 8) |
						((plane4 >> 11) & 16);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
					plane4 <<= 1;
				}
			} while (--x > 0);
		} else
		{
			/* 1 byte per pixel */
			info->_priv_var += x;
			do
			{
				*buffer++ = *bmap++;
			} while (--x > 0);
		}
		break;
	
	case 6:
		if (chunky < 0)
		{
			/* packed */
			RETURN_ERROR(EC_PixelDepth);
		} else if (chunky == 0)
		{
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t plane4;
			uint16_t plane5;
			uint16_t *bmap16 = (uint16_t *)bmap;

			/* bitplanes */
			x = ((x + 15) >> 4);
			info->_priv_var += (size_t)x * 12;
			do
			{
				plane0 = *bmap16++;
				plane1 = *bmap16++;
				plane2 = *bmap16++;
				plane3 = *bmap16++;
				plane4 = *bmap16++;
				plane5 = *bmap16++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4) |
						((plane3 >> 12) & 8) |
						((plane4 >> 11) & 16) |
						((plane5 >> 10) & 32);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
					plane4 <<= 1;
					plane5 <<= 1;
				}
			} while (--x > 0);
		} else
		{
			/* 1 byte per pixel */
			info->_priv_var += x;
			do
			{
				*buffer++ = *bmap++;
			} while (--x > 0);
		}
		break;
	
	case 7:
		if (chunky < 0)
		{
			/* packed */
			RETURN_ERROR(EC_PixelDepth);
		} else if (chunky == 0)
		{
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t plane4;
			uint16_t plane5;
			uint16_t plane6;
			uint16_t *bmap16 = (uint16_t *)bmap;

			/* bitplanes */
			x = ((x + 15) >> 4);
			info->_priv_var += (size_t)x * 14;
			do
			{
				plane0 = *bmap16++;
				plane1 = *bmap16++;
				plane2 = *bmap16++;
				plane3 = *bmap16++;
				plane4 = *bmap16++;
				plane5 = *bmap16++;
				plane6 = *bmap16++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4) |
						((plane3 >> 12) & 8) |
						((plane4 >> 11) & 16) |
						((plane5 >> 10) & 32) |
						((plane6 >> 9) & 64);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
					plane4 <<= 1;
					plane5 <<= 1;
					plane6 <<= 1;
				}
			} while (--x > 0);
		} else
		{
			/* 1 byte per pixel */
			info->_priv_var += x;
			do
			{
				*buffer++ = *bmap++;
			} while (--x > 0);
		}
		break;
	
	case 8:
		if (chunky < 0)
		{
			/* packed */
			x = ((x + 15) >> 4) * 16;
			info->_priv_var += x;
			do
			{
				*buffer++ = *bmap++;
			} while (--x > 0);
		} else if (chunky == 0)
		{
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t plane4;
			uint16_t plane5;
			uint16_t plane6;
			uint16_t plane7;
			uint16_t *bmap16 = (uint16_t *)bmap;

			/* bitplanes */
			x = ((x + 15) >> 4);
			info->_priv_var += (size_t)x * 16;
			do
			{
				plane0 = *bmap16++;
				plane1 = *bmap16++;
				plane2 = *bmap16++;
				plane3 = *bmap16++;
				plane4 = *bmap16++;
				plane5 = *bmap16++;
				plane6 = *bmap16++;
				plane7 = *bmap16++;
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
		} else
		{
			/* 1 byte per pixel */
			info->_priv_var += x;
			do
			{
				*buffer++ = *bmap++;
			} while (--x > 0);
		}
		break;
	
	case 16:
		/* always 2 bytes per pixel */
		{
			const uint16_t *bmap16;
			uint16_t color;
			
			bmap16 = (const uint16_t *)bmap;
			info->_priv_var += (size_t)x * 2;
			do
			{
				color = *bmap16++;
				*buffer++ = ((color >> 11) & 0x1f) << 3;
				*buffer++ = ((color >> 5) & 0x3f) << 2;
				*buffer++ = ((color) & 0x1f) << 3;
			} while (--x > 0);
		}
		break;

	case 24:
		/* always 3 bytes per pixel */
		info->_priv_var += (size_t)x * 3;
		do
		{
			/* RGB */
			*buffer++ = *bmap++;
			*buffer++ = *bmap++;
			*buffer++ = *bmap++;
		} while (--x > 0);
		break;
	
	case 32:
		/* always 4 bytes per pixel */
		info->_priv_var += (size_t)x * 4;
		do
		{
			/* xRGB */
			bmap++;
			*buffer++ = *bmap++;
			*buffer++ = *bmap++;
			*buffer++ = *bmap++;
		} while (--x > 0);
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
	Mfree(info->_priv_ptr);
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
