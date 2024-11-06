#include "plugin.h"
#include "zvplugin.h"

#define VERSION		0x0200
#define AUTHOR      "Thorsten Otto"
#define NAME        "Animaster (Sprite Bank)"
#define DATE        __DATE__ " " __TIME__


struct frame_header {
	uint16_t width;
	uint16_t height;
	uint16_t planes;
};
struct file_header {
	uint16_t num_frames;
	uint16_t frame_size;
	uint8_t icon_w;
	uint8_t icon_h;
	uint16_t palette[16];
};


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("ASB\0" "MSK\0");

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


static uint32_t filesize(int16_t fhand)
{
	uint32_t flen = Fseek(0, fhand, SEEK_END);
	Fseek(0, fhand, SEEK_SET);					/* reset */
	return flen;
}


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	uint32_t file_size;
	int16_t handle;
	uint8_t *data;
	img_data *img;
	uint8_t *ptr;
	uint8_t *end;
	struct file_header *file_header;
	struct frame_header *frame_header;
	uint16_t i;
	uint16_t num_frames;
	
	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	if ((img = malloc(sizeof(*img))) == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	file_size = filesize(handle);
	data = malloc(file_size);
	if (data == NULL)
	{
		free(img);
		Fclose(handle);
		return FALSE;
	}
	if (Fread(handle, file_size, data) != file_size)
	{
		free(data);
		free(img);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);
	
	ptr = data;
	end = data + file_size;
	file_header = (struct file_header *)ptr;

	num_frames = file_header->num_frames + 1;
	ptr += sizeof(*file_header);
	
	for (i = 0; i < num_frames && i < ZVIEW_MAX_IMAGES && ptr < end; i++)
	{
		frame_header = (struct frame_header *)ptr;
		if (frame_header->planes != 4)
		{
			free(data);
			free(img);
			return FALSE;
		}
		if (i == 0)
		{
			info->width = frame_header->width + 1;
			info->height = frame_header->height + 1;
		} else if (frame_header->width + 1 != info->width || frame_header->height + 1 != info->height)
		{
			free(data);
			free(img);
			return FALSE;
		}
		img->image_buf[i] = ptr + sizeof(*frame_header);
		img->delay[i] = 10;
		ptr += file_header->frame_size;
	}
	img->imagecount = i;
	if (i == 0)
	{
		free(data);
		img->image_buf[0] = NULL;
	}
	info->planes = 4;
	info->components = 3;
	info->indexed_color = TRUE;
	info->colors = 16;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = img->imagecount;				/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = -1;			/* current page returned */
	info->_priv_ptr = img;
	strcpy(info->info, "Animaster (Sprite Bank)");
	strcpy(info->compression, "None");
	
	/* if (info->indexed_color) */
	{
		for (i = 0; i < 16; i++)
		{
			uint16_t c;
			c = (((file_header->palette[i] >> 7) & 0xE) + ((file_header->palette[i] >> 11) & 0x1));
			info->palette[i].red = (c << 4) | c;
			c = (((file_header->palette[i] >> 3) & 0xE) + ((file_header->palette[i] >> 7) & 0x1));
			info->palette[i].green = (c << 4) | c;
			c = (((file_header->palette[i] << 1) & 0xE) + ((file_header->palette[i] >> 3) & 0x1));
			info->palette[i].blue = (c << 4) | c;
		}
	}

	return TRUE;
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	img_data *img = (img_data *) info->_priv_ptr;
	uint16_t *xmap;
	uint32_t pos;
	uint16_t x;
	int16_t bit;
	uint8_t ndx;

	pos = info->_priv_var;
	if ((int32_t) info->page_wanted != info->_priv_var_more)
	{
		info->_priv_var_more = info->page_wanted;
		pos = 0;
		if (info->_priv_var_more < 0 || info->_priv_var_more >= img->imagecount)
			return FALSE;
		info->delay = img->delay[info->page_wanted];
	}
	xmap = (uint16_t *)img->image_buf[info->page_wanted];	/* as word array */
	xmap += pos;
	
	x = (info->width + 15) >> 4;
	pos += x * 4;
	info->_priv_var = pos;
	do
	{
		for (bit = 15; bit >= 0; bit--)
		{
			ndx = 0;
			if ((xmap[0] >> bit) & 1)
				ndx |= 1;
			if ((xmap[1] >> bit) & 1)
				ndx |= 2;
			if ((xmap[2] >> bit) & 1)
				ndx |= 4;
			if ((xmap[3] >> bit) & 1)
				ndx |= 8;
			*buffer++ = ndx;
		}
		xmap += 4;	/* next plane */
	} while (--x > 0);		/* next x */

	return TRUE;
}


void __CDECL reader_quit(IMGINFO info)
{
	img_data *img = (img_data *) info->_priv_ptr;

	if (img)
	{
		uint8_t *data = img->image_buf[0];
		if (data != NULL)
		{
			data -= sizeof(struct file_header) + sizeof(struct frame_header);
			free(data);
		}
		free(img);
	}
	info->_priv_ptr = NULL;
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
