#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
TRE - Run Length Encoded True Color Picture

1 long    file id ['tre1']
1 word    picture width in pixels
1 word    picture height in pixels
1 long    number of chunks
This is followed by all the data chunks. The first chunk is a raw data chunk.

raw data chunk:

1 byte    Number of raw data pixels. If this byte is 255 the number of pixels
          will be the following word+255 Number of pixels words of raw data.
This chunk is followed by an RLE chunk.

RLE chunk:

1 byte    Number of RLE pixels. If this byte is 255 the number of RLE pixels
          will be the following word+255. This is the number of times you
          should draw the previous color again.
This chunk is followed by a raw data chunk.
*/


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)(EXTENSIONS);

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
	uint16_t xres;
	uint16_t yres;
	uint32_t num_chunks;
};


static void decode_tre(uint8_t *src, uint16_t *dst, uint32_t num_chunks, size_t dstlen)
{
	uint16_t *end;
	
	end = dst + (dstlen >> 1);
	do
	{
		size_t count;
		uint16_t pixel;

		count = *src++;
		if (count == 255)
		{
			count += (uint16_t)(src[0] << 8) | src[1];
			src += 2;
		}
		while (count > 0)
		{
			*dst++ = (src[0] << 8) | src[1];
			src += 2;
			count--;
		}
		if (--num_chunks == 0)
			break;
		pixel = dst[-1];
		count = *src++;
		if (count == 255)
		{
			count += (uint16_t)(src[0] << 8) | src[1];
			src += 2;
		}
		while (count > 0)
		{
			*dst++ = pixel;
			count--;
		}
		if (--num_chunks == 0)
			break;
	} while (dst < end);
}


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	int16_t handle;
	uint16_t *bmap;
	uint8_t *temp;
	size_t file_size;
	size_t image_size;
	struct file_header header;

	if ((handle = (int16_t)Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	if ((size_t)Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}

	if (header.magic != 0x74726531L || /* 'tre1' */
		header.num_chunks == 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	image_size = ulmul(header.xres, header.yres) * 2;

	file_size -= sizeof(header);
	temp = malloc(file_size);
	if (temp == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, file_size, temp) != file_size)
	{
		free(temp);
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	bmap = malloc(image_size);
	if (bmap == NULL)
	{
		free(temp);
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}

	decode_tre(temp, bmap, header.num_chunks, image_size);
	free(temp);
	
	info->planes = 16;
	info->width = header.xres;
	info->height = header.yres;
	info->components = 3;
	info->indexed_color = FALSE;
	info->colors = 1L << 16;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */

	info->_priv_var = 0;
	info->_priv_ptr = bmap;

	strcpy(info->info, "Spooky Sprites");
	strcpy(info->compression, "RLE");

	RETURN_SUCCESS();
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	int x;
	const uint16_t *screen16;
	uint16_t color;
	
	screen16 = (const uint16_t *)info->_priv_ptr + info->_priv_var;
	x = info->width;
	info->_priv_var += x;
	do
	{
		color = *screen16++;
		*buffer++ = ((color >> 11) & 0x1f) << 3;
		*buffer++ = ((color >> 5) & 0x3f) << 2;
		*buffer++ = ((color) & 0x1f) << 3;
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
