#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"
#define NF_DEBUG 0
#include "nfdebug.h"

/*
PixArt          *.PIX

1 long           file ID 'PIXT'
1 word           version, $0001 or $0002
1 byte           type, 0 to 2 depending on number of planes, see below
1 byte           number of planes
1 word           xres (always a multiple of 16)
1 word           yres
1 word           unknown
1 word           unknown (version $0002 only)
--------------
14 or 16 bytes   total for header

3*n bytes        palette, only for bit depths 2, 4, 8
                   3 bytes per entry in RGB order (0-255)

???? bytes       image data:
                   bit depth 1
                     type = 1, planar
                   bit depths 2, 4, 8
                     type = 0, chunky
                     type = 1, planar
                   bit depth 16 is always chunky:
                     type = 0, xRRRRRGGGGGBBBBB
                     type = 1, RRRRRGGGGGGBBBBB (Falcon)
                     type = 2, GGGBBBBBxRRRRRGG (word swapped)
                   bit depth 24 is always chunky:
                     type = 0, BGR
                     type = 1, RGB
                   bit depth 32 is always chunky:
                     type = 0, ARGB
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
	uint16_t version;
	uint8_t type;
	uint8_t planes;
	uint16_t width;
	uint16_t height;
	uint16_t reserved;
};




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
	struct file_header header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}

	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (header.magic != 0x50495854L || header.version > 2) /* 'PIXT' */
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}

	if (header.version == 2)
		Fseek(2, handle, SEEK_CUR);

	if (header.planes == 2 || header.planes == 4 || header.planes == 8)
	{
		int colors = 1 << header.planes;
		size_t palsize = colors * 3;
		int i;
		uint8_t palette[256][3];
		
		if ((size_t)Fread(handle, palsize, palette) != palsize)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		for (i = 0; i < colors; i++)
		{
			info->palette[i].red = palette[i][0];
			info->palette[i].green = palette[i][1];
			info->palette[i].blue = palette[i][2];
		}
	}
	
	strcpy(info->info, "PixArt v0");
	info->info[8] = header.version + '0';
	strcpy(info->compression, "None");

	info->colors = 1L << (header.planes < 24 ? header.planes : 24);

	switch (header.planes)
	{
	case 1:
		info->components = 1;
		info->indexed_color = FALSE;
		strcat(info->info, " (Planar)");
		break;
	case 2:
	case 4:
	case 8:
		info->components = 3;
		info->indexed_color = TRUE;
		switch (header.type)
		{
		case 0:
			strcat(info->info, " (Chunky)");
			break;
		case 1:
			strcat(info->info, " (Planar)");
			break;
		default:
			Fclose(handle);
			RETURN_ERROR(EC_ImageType);
		}
		break;
	case 15:
		header.planes = 16;
		/* fall through */
	case 16:
		info->components = 3;
		info->indexed_color = FALSE;
		switch (header.type)
		{
		case 0:
			strcat(info->info, " (Chunky 5/5/5)");
			info->colors = 1L << 15;
			break;
		case 1:
			strcat(info->info, " (Chunky 5/6/5)");
			info->colors = 1L << 16;
			break;
		case 2:
			strcat(info->info, " (Chunky 5/5/5 Intel)");
			info->colors = 1L << 15;
			break;
		default:
			Fclose(handle);
			RETURN_ERROR(EC_ImageType);
		}
		break;
	case 24:
		info->components = 3;
		info->indexed_color = FALSE;
		switch (header.type)
		{
		case 0:
			strcat(info->info, " (Chunky BGR)");
			break;
		case 1:
			strcat(info->info, " (Chunky RGB)");
			break;
		default:
			Fclose(handle);
			RETURN_ERROR(EC_ImageType);
		}
		break;
	case 32:
		info->components = 3;
		info->indexed_color = FALSE;
		strcat(info->info, " (Chunky ARGB)");
		break;
	default:
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}

	image_size = (((size_t)header.width + 15) >> 4) * 2 * header.planes * header.height;
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
	Fclose(handle);

	info->planes = header.planes;
	info->width = header.width;
	info->height = header.height;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;
	info->_priv_var_more = header.type;

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
	int x;
	
	switch (info->planes)
	{
	case 1:
		{
			uint16_t plane0;
			uint16_t *bmap = (uint16_t *)info->_priv_ptr;

			bmap += info->_priv_var;
			x = (info->width + 15) >> 4;
			info->_priv_var += x;
			do
			{
				plane0 = *bmap++;
				*buffer++ = (plane0 >> 15) & 1;
				*buffer++ = (plane0 >> 14) & 1;
				*buffer++ = (plane0 >> 13) & 1;
				*buffer++ = (plane0 >> 12) & 1;
				*buffer++ = (plane0 >> 11) & 1;
				*buffer++ = (plane0 >> 10) & 1;
				*buffer++ = (plane0 >> 9) & 1;
				*buffer++ = (plane0 >> 8) & 1;
				*buffer++ = (plane0 >> 7) & 1;
				*buffer++ = (plane0 >> 6) & 1;
				*buffer++ = (plane0 >> 5) & 1;
				*buffer++ = (plane0 >> 4) & 1;
				*buffer++ = (plane0 >> 3) & 1;
				*buffer++ = (plane0 >> 2) & 1;
				*buffer++ = (plane0 >> 1) & 1;
				*buffer++ = (plane0 >> 0) & 1;
			} while (--x > 0);
		}
		break;

	case 2:
		{
			int type = (int)info->_priv_var_more;
			uint16_t *bmap = (uint16_t *)info->_priv_ptr;
			
			bmap += info->_priv_var;
			x = (info->width + 15) >> 4;
			info->_priv_var += x * 2;
			if (type != 0)
			{
				int bit;
				uint16_t plane0;
				uint16_t plane1;

				/* planar */
				do
				{
					plane0 = *bmap++;
					plane1 = *bmap++;
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
				/* chunky */
				uint16_t byte;

				do
				{
					byte = *bmap++;
					*buffer++ = (byte >> 14);
					*buffer++ = (byte >> 12) & 3;
					*buffer++ = (byte >> 10) & 3;
					*buffer++ = (byte >> 8) & 3;
					*buffer++ = (byte >> 6) & 3;
					*buffer++ = (byte >> 4) & 3;
					*buffer++ = (byte >> 2) & 3;
					*buffer++ = (byte >> 0) & 3;
					byte = *bmap++;
					*buffer++ = (byte >> 14);
					*buffer++ = (byte >> 12) & 3;
					*buffer++ = (byte >> 10) & 3;
					*buffer++ = (byte >> 8) & 3;
					*buffer++ = (byte >> 6) & 3;
					*buffer++ = (byte >> 4) & 3;
					*buffer++ = (byte >> 2) & 3;
					*buffer++ = (byte >> 0) & 3;
				} while (--x > 0);
			}
		}
		break;
	
	case 4:
		{
			int type = (int)info->_priv_var_more;
			uint16_t *bmap = (uint16_t *)info->_priv_ptr;
			
			bmap += info->_priv_var;
			x = (info->width + 15) >> 4;
			info->_priv_var += x * 4;
			if (type != 0)
			{
				int bit;
				uint16_t plane0;
				uint16_t plane1;
				uint16_t plane2;
				uint16_t plane3;

				/* planar */
				do
				{
					plane0 = *bmap++;
					plane1 = *bmap++;
					plane2 = *bmap++;
					plane3 = *bmap++;
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
				/* chunky */
				uint16_t byte;

				do
				{
					byte = *bmap++;
					*buffer++ = (byte >> 12);
					*buffer++ = (byte >> 8) & 15;
					*buffer++ = (byte >> 4) & 15;
					*buffer++ = (byte >> 0) & 15;
					byte = *bmap++;
					*buffer++ = (byte >> 12);
					*buffer++ = (byte >> 8) & 15;
					*buffer++ = (byte >> 4) & 15;
					*buffer++ = (byte >> 0) & 15;
					byte = *bmap++;
					*buffer++ = (byte >> 12);
					*buffer++ = (byte >> 8) & 15;
					*buffer++ = (byte >> 4) & 15;
					*buffer++ = (byte >> 0) & 15;
					byte = *bmap++;
					*buffer++ = (byte >> 12);
					*buffer++ = (byte >> 8) & 15;
					*buffer++ = (byte >> 4) & 15;
					*buffer++ = (byte >> 0) & 15;
				} while (--x > 0);
			}
		}
		break;

	case 8:
		{
			int type = (int)info->_priv_var_more;
			uint16_t *bmap = (uint16_t *)info->_priv_ptr;
			
			bmap += info->_priv_var;
			x = (info->width + 15) >> 4;
			info->_priv_var += x * 8;
			if (type != 0)
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

				/* planar */
				do
				{
					plane0 = *bmap++;
					plane1 = *bmap++;
					plane2 = *bmap++;
					plane3 = *bmap++;
					plane4 = *bmap++;
					plane5 = *bmap++;
					plane6 = *bmap++;
					plane7 = *bmap++;
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
				/* chunky */
				uint16_t *buffer16;

				buffer16 = (uint16_t *)buffer;
				do
				{
					*buffer16++ = *bmap++;
					*buffer16++ = *bmap++;
					*buffer16++ = *bmap++;
					*buffer16++ = *bmap++;
					*buffer16++ = *bmap++;
					*buffer16++ = *bmap++;
					*buffer16++ = *bmap++;
					*buffer16++ = *bmap++;
				} while (--x > 0);
			}
		}
		break;

	case 15:
	case 16:
		{
			int type = (int)info->_priv_var_more;
			uint16_t *bmap = (uint16_t *)info->_priv_ptr;
			uint16_t color;
			
			bmap += info->_priv_var;
			x = (info->width + 15) & -16;
			info->_priv_var += x;
			switch (type)
			{
			case 0:
				/* xRRRRRGGGGGBBBBB */
				do
				{
					color = *bmap++;
					*buffer++ = ((color >> 10) & 0x1f) << 3;
					*buffer++ = ((color >> 5) & 0x1f) << 3;
					*buffer++ = ((color) & 0x1f) << 3;
				} while (--x > 0);
				break;
			case 1:
				/* RRRRRGGGGGGBBBBB */
				do
				{
					color = *bmap++;
					*buffer++ = ((color >> 11) & 0x1f) << 3;
					*buffer++ = ((color >> 5) & 0x3f) << 2;
					*buffer++ = ((color) & 0x1f) << 3;
				} while (--x > 0);
				break;
			case 2:
				/* GGGBBBBBxRRRRRGG (word swapped) */
				do
				{
					color = *bmap++;
					*buffer++ = (color & 0x7c) << 1;
					*buffer++ = ((color >> 11) & 0x1c) | ((color & 0x3) << 6);
					*buffer++ = (color & 0x1f00) >> 5;
				} while (--x > 0);
				break;
			}
		}
		break;
		
	case 24:
		{
			int type = (int)info->_priv_var_more;
			uint8_t *bmap = (uint8_t *)info->_priv_ptr;

			bmap += info->_priv_var;
			x = (info->width + 15) & -16;
			info->_priv_var += x * 3;

			if (type != 0)
			{
				/* data is RGB */
				do
				{
					*buffer++ = *bmap++;
					*buffer++ = *bmap++;
					*buffer++ = *bmap++;
				} while (--x > 0);
			} else
			{
				/* data is BGR */
				do
				{
					*buffer++ = bmap[2];
					*buffer++ = bmap[1];
					*buffer++ = bmap[0];
					bmap += 3;
				} while (--x > 0);
			}
		}
		break;

	case 32:
		{
			uint8_t *bmap = (uint8_t *)info->_priv_ptr;

			bmap += info->_priv_var;
			x = (info->width + 15) & -16;
			info->_priv_var += x * 4;

			do
			{
				bmap++; /* skip alpha */
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
