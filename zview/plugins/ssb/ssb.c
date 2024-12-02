#include "plugin.h"
#include "zvplugin.h"

#define VERSION		0x202
#define AUTHOR      "Thorsten Otto"
#define NAME        "Slideshow Sinbad"
#define DATE        __DATE__ " " __TIME__

/*
Sinbad Slideshow    *.SSB (ST low resolution)

These images are from a series of slideshows released by Martin Cubitt (aka
Sinbad). Packed by ICE v2.3/v2.4

32000 bytes    image data (screen memory)
16 words       palette
-----------
32032 bytes    total (unpacked)
*/


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("SSB\0");

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


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	int16_t handle;
	uint16_t *bmap;
	uint16_t palette[16];
	size_t file_size;

	if ((handle = (int16_t)Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	/*
	 * In practise, all files seem to have some garbage added at the end?
	 */
	if (file_size != SCREEN_SIZE + 768)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileLength);
	}

	bmap = malloc(SCREEN_SIZE);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}

	if (Fread(handle, SCREEN_SIZE, bmap) != SCREEN_SIZE ||
		Fread(handle, sizeof(palette), palette) != sizeof(palette))
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}

	{	
		int i;

		for (i = 0; i < 16; i++)
		{
			info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((palette[i] << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 17;
		}
	}

	info->planes = 4;
	info->width = 320;
	info->height = 200;
	info->components = 1;
	info->indexed_color = TRUE;
	info->colors = 1L << 4;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */

	info->_priv_var = 0;
	info->_priv_ptr = bmap;

	strcpy(info->info, "Slideshow Sinbad");
	strcpy(info->compression, "None");

	RETURN_SUCCESS();
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	int x;
	int bit;
	uint16_t plane0;
	uint16_t plane1;
	uint16_t plane2;
	uint16_t plane3;
	uint16_t *ptr;
	
	ptr = (uint16_t *)(info->_priv_ptr + info->_priv_var);
	x = info->width >> 4;
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
	RETURN_SUCCESS();
}


void __CDECL reader_quit(IMGINFO info)
{
	void *p = info->_priv_ptr;
	info->_priv_ptr = NULL;
	free(p);
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
