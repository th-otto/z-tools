#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

struct file_header {
	char magic[7];
	char version[4];
	uint8_t image_type;
	uint8_t palette_type;
};

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


static const char *const image_type_names[] = {
	/*  0 */ "QImageBitmap",
	/*  1 */ "MultiColorBitmap",
	/*  2 */ "HiresBitmap",
	/*  3 */ "LevelEditor",
	/*  4 */ "CharMapMulticolor",
	/*  5 */ "Sprites",
	/*  6 */ "CharmapRegular",
	/*  7 */ "FullScreenChar",
	/*  8 */ "CharMapMultiColorFixed",
	/*  9 */ "VIC20_MultiColorbitmap",
	/* 10 */ "Sprites2",
	/* 11 */ "CGA",
	/* 12 */ "AMIGA320x200",
	/* 13 */ "AMIGA320x256",
	/* 14 */ "OK64_256x256",
	/* 15 */ "X16_640x480",
	/* 16 */ "NES",
	/* 17 */ "LMetaChunk",
	/* 18 */ "LevelEditorNES",
	/* 19 */ "SpritesNES",
	/* 20 */ "GAMEBOY",
	/* 21 */ "LevelEditorGameboy",
	/* 22 */ "ATARI320x200",
	/* 23 */ "HybridCharset",
	/* 24 */ "AmstradCPC",
	/* 25 */ "AmstradCPCGeneric",
	/* 26 */ "BBC",
	/* 27 */ "VGA",
	/* 28 */ "Spectrum",
	/* 29 */ "SNES",
	/* 30 */ "LevelEditorSNES",
	/* 31 */ "VZ200",
	/* 32 */ "CustomC64",
	/* 33 */ "JDH8",
	/* 34 */ "LImageGeneric",
	/* 35 */ "GenericSprites"
};


static void set_bw(IMGINFO info)
{
	info->palette[0].red = 0;
	info->palette[0].green = 0;
	info->palette[0].blue = 0;
	info->palette[1].red = 255;
	info->palette[1].green = 255;
	info->palette[1].blue = 255;
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
	uint8_t pal_size;
	uint8_t rgb[3];
	struct file_header header;
	char bbc_type;
	size_t image_size;
	int16_t handle;
	uint8_t *bmap;
	int count;
	int i;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	if (Fread(handle, 13, &header) != 13 ||
		memcmp(header.magic, "FLUFF64", 7) != 0)
	{
		Fclose(handle);
		return FALSE;
	}

	switch (header.image_type)
	{
	case 12:
	case 22:
	case 27:
		info->width = 320;
		info->height = 200;
		break;
	
	case 14:
		info->width = 256;
		info->height = 256;
		break;
	
	case 15:
		info->width = 640;
		info->height = 480;
		break;
	
	case 13:
		info->width = 320;
		info->height = 256;
		break;
	
	case 26:
		if (Fread(handle, sizeof(bbc_type), &bbc_type) != sizeof(bbc_type))
		{
			Fclose(handle);
			return FALSE;
		}
		if (bbc_type == 4)
			info->width = 320;
		else if (bbc_type == 5)
			info->width = 160;
		else
		{
			Fclose(handle);
			return FALSE;
		}
		info->height = 256;
		break;
	
	case 33:
		info->width = 256;
		info->height = 240;
		break;
	
	default:
		Fclose(handle);
		return FALSE;
	}
	
	image_size = (size_t)info->width * info->height;

	bmap = malloc(image_size + 256);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, image_size, bmap) != image_size)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}

	switch (header.image_type)
	{
	case 12:
	case 13:
	case 14:
	case 15:
	case 22:
	case 27:
		if (Fread(handle, sizeof(pal_size), &pal_size) != sizeof(pal_size))
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		count = pal_size;
		if (count == 0)
			count = 256;
		for (i = 0; i < count; i++)
		{
			if (Fread(handle, sizeof(rgb), rgb) != sizeof(rgb))
			{
				free(bmap);
				Fclose(handle);
				return FALSE;
			}
			info->palette[i].red = rgb[0];
			info->palette[i].green = rgb[1];
			info->palette[i].blue = rgb[2];
		}
		break;
	case 26:
		if (bbc_type == 4)
		{
			set_bw(info);
		} else if (bbc_type == 5)
		{
			info->palette[0].red = 0;
			info->palette[0].green = 0;
			info->palette[0].blue = 0;
			info->palette[1].red = 255;
			info->palette[1].green = 0;
			info->palette[1].blue = 0;
			info->palette[2].red = 255;
			info->palette[2].green = 255;
			info->palette[2].blue = 0;
			info->palette[3].red = 255;
			info->palette[3].green = 255;
			info->palette[3].blue = 255;
		}
		break;
	case 33:
		set_bw(info);
		break;
	}
	Fclose(handle);

	info->planes = 8;
	info->indexed_color = TRUE;
	info->components = 3;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "FLUFF64 (");
	strcat(info->info, image_type_names[header.image_type]);
	strcat(info->info, ")");
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
	size_t pos;
	uint8_t *bmap = info->_priv_ptr;

	pos = info->_priv_var;
	memcpy(buffer, &bmap[pos], info->width);
	pos += info->width;
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
