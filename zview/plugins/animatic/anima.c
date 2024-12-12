#include "plugin.h"
#include "zvplugin.h"
#define NF_DEBUG 0
#include "nfdebug.h"

#define VERSION		0x0200
#define AUTHOR      "Thorsten Otto"
#define NAME        "Animatic (Film)"
#define DATE        __DATE__ " " __TIME__

/*
Animatic Film   *.FLM (ST low resolution only)

1 word      number of frames
16 words    palette
1 word      speed (0 - 99; value is 99 - # vblanks to delay between frames)
1 word      direction (0 = forwards, 1 = backwards)
1 word      end action (what to do after the last frame)
            0 = pause, then repeat from beginning
            1 = immediately repeat from beginning
            2 = reverse (change direction)
1 word      width of film in pixels
1 word      height of film in pixels
1 word      Animatic version number (major) [< 2]
1 word      Animatic version number (minor)
1 long      magic number $27182818
3 longs     reserved for expansion (should be all zeros)
--------
32 words    total for header

? words     image data (words of screen memory) for each frame, in order
*/


struct file_header {
	uint16_t num_frames;
	uint16_t palette[16];
	uint16_t speed;
	uint16_t direction;
	uint16_t end_action;
	int16_t width;
	int16_t height;
	uint16_t version_major;
	uint16_t version_minor;
	uint32_t magic;
	uint32_t reserved[3];
};


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("FLM\0");

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
	uint16_t i, j;
	uint16_t num_frames;
	size_t frame_size;
		
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
	if ((uint32_t) Fread(handle, file_size, data) != file_size)
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
	if (file_header->magic != 0x27182818L)
	{
		free(data);
		free(img);
		return FALSE;
	}
	
	num_frames = file_header->num_frames;
	ptr += sizeof(*file_header);
	frame_size = (((((size_t)file_header->width + 15) >> 4) * 2) * 4) * file_header->height;
	
	for (i = 0; i < num_frames && i < ZVIEW_MAX_IMAGES && ptr < end; i++)
	{
		img->image_buf[i] = ptr;
		img->delay[i] = file_header->speed / 5;
		if (img->delay[i] == 0)
			img->delay[i] = 1;
		ptr += frame_size;
	}
	num_frames = i;
	nf_debugprintf(("frames = %u\n", num_frames));
	if (num_frames == 0)
	{
		free(data);
		data = NULL;
	} else
	{
		if (file_header->direction != 0)
		{
			uint8_t *tmp;
			
			/* play backwards */
			nf_debugprintf(("play backwards\n"));
			j = num_frames - 1;
			for (i = 0; i < j; j--, i++)
			{
				tmp = img->image_buf[i];
				img->image_buf[i] = img->image_buf[j];
				img->image_buf[j] = tmp;
			}
		}

		if (file_header->end_action == 2 && num_frames > 1)
		{
			uint16_t k;
			
			/* reverse direction at end of play */
			nf_debugprintf(("reverse direction at end\n"));
			k = num_frames;
			j = num_frames;
			for (i = 0; i < num_frames && k < ZVIEW_MAX_IMAGES; k++, i++)
			{
				--j;
				img->image_buf[k] = img->image_buf[j];
				img->delay[k] = img->delay[j];
			}
			num_frames = k;
		}
	}
	
	img->imagecount = num_frames;
	info->width = file_header->width;
	info->height = file_header->height;
	info->planes = 4;
	info->components = 3;
	info->indexed_color = TRUE;
	info->colors = 16;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = num_frames;			/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = -1;			/* current frame displayed */
	info->_priv_ptr = img;
	info->_priv_ptr_more = data;
	strcpy(info->info, "Animatic (Film)");
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
		nf_debugprintf(("decode frame %u of %u\n", info->page_wanted, img->imagecount));
		if (info->page_wanted >= img->imagecount)
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
	img_data *img;
	uint8_t *data;

	data = info->_priv_ptr_more;
	if (data != NULL)
	{
		free(data);
	}
	img = (img_data *) info->_priv_ptr;
	if (img)
	{
		free(img);
	}
	info->_priv_ptr = NULL;
	info->_priv_ptr_more = NULL;
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
