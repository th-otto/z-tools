#define	VERSION	     0x0201
#define NAME        "PC Paintbrush"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"
#define NF_DEBUG 0
#include "nfdebug.h"

/*
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("PCX\0");

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


struct file_header {
	uint8_t magic;
	uint8_t version;
	uint8_t compression;
	uint8_t bits;
	uint16_t left;
	uint16_t top;
	uint16_t right;
	uint16_t bottom;
	uint16_t xdpi;
	uint16_t ydpi;
	uint8_t palette[16][3];
	uint8_t reserved1;
	uint8_t planes;
	uint16_t bytes_per_scanline;
	uint16_t palette_mode;
	uint16_t xscreen;
	uint16_t yscreen;
	uint8_t reserved2[54];
};

static uint8_t const vga_palette[16 * 3] = {
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xaa,
	0x00, 0xaa, 0x00,
	0x00, 0xaa, 0xaa,
	0xaa, 0x00, 0x00,
	0xaa, 0x00, 0xaa,
	0xaa, 0x55, 0x00,
	0xaa, 0xaa, 0xaa,
	0x55, 0x55, 0x55,
	0x55, 0x55, 0xff,
	0x55, 0xff, 0x55,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0x55, 0xff,
	0xff, 0xff, 0x55,
	0xff, 0xff, 0xff
};
static uint8_t const cga_palette[8][3] = {
	{ 0x02, 0x04, 0x06 },
	{ 0x0a, 0x0c, 0x0e },
	{ 0x03, 0x05, 0x07 },
	{ 0x0b, 0x0d, 0x0f },
	{ 0x03, 0x04, 0x07 },
	{ 0x0b, 0x0c, 0x0f },
	{ 0x03, 0x04, 0x07 },
	{ 0x0b, 0x0c, 0x0f }
};

/* FIXME: statics */
static struct file_header header;
static size_t bytes_per_row;
static int planar;


static uint16_t swap16(uint16_t w)
{
	return (w >> 8) | (w << 8);
}


static size_t decode_pcx(const uint8_t *src, uint8_t *dst, size_t dstlen)
{
	size_t srcpos;
	size_t dstpos;
	uint8_t cmd;
	uint8_t c;
	int i;
	
	srcpos = 0;
	dstpos = 0;
	do
	{
		cmd = src[srcpos++];
		if ((cmd & 0xc0) == 0xc0)
		{
			c = src[srcpos++];
			i = cmd & 0x3f;
			while (--i >= 0)
				dst[dstpos++] = c;
		} else
		{
			dst[dstpos++] = cmd;
		}
	} while (dstpos < dstlen);
	return srcpos;
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
	size_t image_size;
	size_t file_size;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (file_size <= sizeof(header) ||
		Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (header.magic != 10)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	if (header.version > 5)
	{
		Fclose(handle);
		RETURN_ERROR(EC_HeaderVersion);
	}
	if (header.compression > 1)
	{
		Fclose(handle);
		RETURN_ERROR(EC_CompType);
	}
	file_size -= sizeof(header);

	header.left = swap16(header.left);
	header.top = swap16(header.top);
	header.right = swap16(header.right);
	header.bottom = swap16(header.bottom);
	header.bytes_per_scanline = swap16(header.bytes_per_scanline);
	header.palette_mode = swap16(header.palette_mode);
	
	info->width = header.right - header.left + 1;
	info->height = header.bottom - header.top + 1;
	info->planes = header.planes * header.bits;
	info->colors = 1L << (info->planes < 24 ? info->planes : 24);
	
	bytes_per_row = header.bytes_per_scanline * (size_t)header.planes;
	image_size = bytes_per_row * info->height;
	nf_debugprintf("bits=%u planes=%u width=%u height=%u scanline=%u pitch=%lu\n", header.bits, header.planes, info->width, info->height, header.bytes_per_scanline, bytes_per_row);

	strcpy(info->info, "PC Paintbrush v0");
	info->info[15] = header.version + '0';

	if (header.planes == 1)
	{
		planar = FALSE;
		strcat(info->info, " Chunky");
	} else
	{
		planar = TRUE;
		strcat(info->info, " Planar");
	}
	
	info->indexed_color = TRUE;
	info->components = 3;
	if (header.planes == 1 && header.bits == 1)
	{
		strcat(info->info, " (Mono)");
		info->indexed_color = FALSE;
		info->components = 1;
	} else if ((header.planes == 1 && header.bits == 2) ||
		(header.planes == 2 && header.bits == 1))
	{
		strcat(info->info, " (CGA)");
	} else if ((header.planes == 3 && header.bits == 1) ||
		(header.planes == 1 && header.bits == 3))
	{
		strcat(info->info, " (EGA)");
	} else if ((header.planes == 1 && header.bits == 4) ||
		(header.planes == 4 && header.bits == 1))
	{
		strcat(info->info, " (EGA/VGA)");
	} else if (header.planes == 1 && header.bits == 8)
	{
		strcat(info->info, " (VGA)");
	} else if (header.planes == 3 && header.bits == 8)
	{
		info->indexed_color = FALSE;
		strcat(info->info, " (VGA/XGA)");
	} else if (header.planes == 8 && header.bits == 1)
	{
		Fclose(handle);
		RETURN_ERROR(EC_ImageType);
	} else
	{
		Fclose(handle);
		RETURN_ERROR(EC_ImageType);
	}

	if (header.compression)
	{
		uint8_t *temp;
		size_t srclen;
		
		bmap = malloc(image_size + file_size);
		if (bmap == NULL)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		temp = bmap + image_size;
		if ((size_t)Fread(handle, file_size, temp) != file_size)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		srclen = decode_pcx(temp, bmap, image_size);
		nf_debugprintf("decoded: %lu of %lu\n", srclen, file_size);
		if (srclen > file_size)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_CompLength);
		}
		file_size -= srclen;
		strcpy(info->compression, "RLE");
	} else
	{
		bmap = malloc(image_size);
		if (bmap == NULL)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		if ((size_t)Fread(handle, image_size, bmap) != image_size)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		file_size -= image_size;
		strcpy(info->compression, "None");
	}

	if (info->planes == 1)
	{
		size_t i;
		
		for (i = 0; i < image_size; i++)
			bmap[i] = ~bmap[i];
	}

	if (info->planes <= 8)
	{
		int i;

		if (info->planes == 1)
		{
		} else
		{
			if (info->colors == 256)
			{
				uint8_t test;
				uint8_t palette[256][3];

				if (file_size >= 769 &&
					Fseek(-769, handle, SEEK_END) > 0 &&
					Fread(handle, sizeof(test), &test) == sizeof(test) &&
					test == 12 &&
					Fread(handle, sizeof(palette), &palette[0][0]) == sizeof(palette))
				{
					for (i = 0; i < 256; i++)
					{
						info->palette[i].red = palette[i][0];
						info->palette[i].green = palette[i][1];
						info->palette[i].blue = palette[i][2];
					}
				} else
				{
					for (i = 0; i < 16; i++)
					{
						info->palette[i].red = header.palette[i][0];
						info->palette[i].green = header.palette[i][1];
						info->palette[i].blue = header.palette[i][2];
					}
				}
			} else
			{
				if (header.version >= 2)
				{
					for (i = 0; i < (int)info->colors; i++)
					{
						info->palette[i].red = header.palette[i][0];
						info->palette[i].green = header.palette[i][1];
						info->palette[i].blue = header.palette[i][2];
					}
				} else if (info->colors == 8)
				{
					for (i = 0; i < 8; i++)
					{
						info->palette[i].red = vga_palette[cga_palette[i][0]];
						info->palette[i].green = vga_palette[cga_palette[i][1]];
						info->palette[i].blue = vga_palette[cga_palette[i][2]];
					}
				} else
				{
					for (i = 0; i < (int)info->colors; i++)
					{
						info->palette[i].red = vga_palette[i * 3 + 0];
						info->palette[i].green = vga_palette[i * 3 + 1];
						info->palette[i].blue = vga_palette[i * 3 + 2];
					}
				}
			}
		}
	}
	
	Fclose(handle);

	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->compression, "None");

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
	uint8_t *bmap = info->_priv_ptr;
	size_t src = info->_priv_var;
	int x;
	
	nf_debugprintf("src=%lu\n", src);
	switch (info->planes)
	{
	case 1:
		{
			int16_t byte;

			x = (info->width + 7) >> 3;
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
		if (planar)
		{
			for (x = 0; x < info->width; x++)
			{
				uint16_t plane;
				uint8_t byte;
				
				byte = 0;
				for (plane = 0; plane < 2; plane++)
				{
					byte = ((bmap[header.bytes_per_scanline * plane + src + (x >> 3)] << (x & 7)) & 0x80) | (byte >> 1);
				}
				byte >>= (8 - 2);
				*buffer++ = byte;
			}
		} else
		{
			for (x = 0; x < info->width; x += 4)
			{
				uint8_t byte = bmap[src++];
				*buffer++ = byte >> 6;
				*buffer++ = (byte >> 4) & 3;
				*buffer++ = (byte >> 2) & 3;
				*buffer++ = (byte) & 3;
			}
		}
		break;
	
	case 3:
		if (planar)
		{
			for (x = 0; x < info->width; x++)
			{
				uint16_t plane;
				uint8_t byte;
				
				byte = 0;
				for (plane = 0; plane < 3; plane++)
				{
					byte = ((bmap[header.bytes_per_scanline * plane + src + (x >> 3)] << (x & 7)) & 0x80) | (byte >> 1);
				}
				byte >>= (8 - 3);
				*buffer++ = byte;
			}
		} else
		{
			RETURN_ERROR(EC_ImageType);
		}
		break;
	
	case 4:
		if (planar)
		{
			for (x = 0; x < info->width; x++)
			{
				uint16_t plane;
				uint8_t byte;
				
				byte = 0;
				for (plane = 0; plane < 4; plane++)
				{
					byte = ((bmap[header.bytes_per_scanline * plane + src + (x >> 3)] << (x & 7)) & 0x80) | (byte >> 1);
				}
				byte >>= (8 - 4);
				*buffer++ = byte;
			}
		} else
		{
			for (x = 0; x < info->width; x += 2)
			{
				uint8_t byte;
				
				byte = bmap[src++];
				*buffer++ = byte >> 4;
				*buffer++ = byte & 15;
			}
		}
		break;

	case 8:
		if (planar)
		{
			RETURN_ERROR(EC_ImageType);
		} else
		{
			memcpy(buffer, &bmap[src], bytes_per_row);
		}
		break;

	case 24:
		if (planar)
		{
			size_t green = src + header.bytes_per_scanline;
			size_t blue = src + header.bytes_per_scanline + header.bytes_per_scanline;
			
			for (x = 0; x < info->width; x++)
			{
				*buffer++ = bmap[x + src];
				*buffer++ = bmap[x + green];
				*buffer++ = bmap[x + blue];
			}
		} else
		{
			memcpy(buffer, &bmap[src], bytes_per_row);
		}
		break;
	}

	info->_priv_var += bytes_per_row;
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
