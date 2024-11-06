#include "plugin.h"
#include "zvplugin.h"

#define VERSION		0x0201
#define AUTHOR      "Thorsten Otto"
#define NAME        "Atari Portfolio Graphics"
#define DATE        __DATE__ " " __TIME__

struct frame_header {
	uint8_t type;
	uint8_t fsize[2];
	uint8_t control;
	uint8_t res1;
	uint8_t res2;
	uint8_t res3;
	uint8_t res4;
};

struct file_header {
	uint8_t signature[3];
	uint8_t version;
	uint8_t reserved[4];
};


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("PGF\0" "PGC\0" "PGX\0");

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


static void decompress(const uint8_t *src, uint8_t *dst, uint32_t decompressed_size)
{
	uint8_t idx;
	uint8_t data;
	unsigned int k;
	uint8_t *end;
	
	end = &dst[decompressed_size];
	do
	{
		idx = *src++;
		if ((idx & 0x80) != 0)			/* High Bit ist gesetzt				*/
		{									/* Gleiche Bytefolge				*/
			idx -= 0x80;					/* Anzahl der Bytes 				*/
			data = *src++;
			for (k = 0; k < idx; k++)
				*dst++ = data;
		} else
		{									/* Unterschiedliche Bytefolge */
			for (k = 0; k < idx; k++)
				*dst++ = *src++;
		}
	} while (dst < end);
}


static uint16_t get_frame_size(struct frame_header *frame_header)
{
	/* stored in little-endian */
	return (frame_header->fsize[0]) | (frame_header->fsize[1] << 8);
}


static int rand_range(int min, int max)
{
	return rand() % (max - min + 1) + min;
}


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	uint32_t file_size;
	int16_t handle;
	uint8_t *data;
	img_data *img;
	int frames;

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
		free(img);
		free(data);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	strcpy(info->info, "Atari Portfolio Graphics");
	strcpy(info->compression, "RLE");
	if (data[0] == 'P' && data[1] == 'G' && data[2] == 1)
	{
		uint8_t *bmap;

		/* compressed PGC file */
		bmap = malloc(1920);
		if (bmap == NULL)
		{
			free(img);
			free(data);
			Fclose(handle);
			return FALSE;
		}
		decompress(&data[3], bmap, 1920);
		free(data);
		img->image_buf[0] = bmap;
		img->delay[0] = 0;
		frames = 1;
	} else if (data[0] == 'P' && data[1] == 'G' && data[2] == 'X' && data[3] == 1)
	{
		uint8_t *bmap;
		uint8_t *ptr;
		uint8_t *end;
		uint16_t frame_size;
		struct frame_header *frame_header;
		int16_t delay;
		
		/* animation with multiple frames */
		strcpy(info->info, "Atari Portfolio Animation");
		/*
		 * frames are small, so just decode them all now
		 */
		frames = 0;
		ptr = data + sizeof(struct file_header);
		end = data + file_size;
		while (ptr < end)
		{
			frame_header = (struct frame_header *)ptr;
			frame_size = get_frame_size(frame_header);
			ptr += sizeof(struct frame_header);
			switch (frame_header->type)
			{
			case 0:
				/* PGX compressed file */
				if (frames >= ZVIEW_MAX_IMAGES)
				{
					ptr = end;
					break;
				}
				if (frame_header->control == 1)
				{
					/* wait for keypress */
					delay = 2000;
				} else if (frame_header->control == 2)
				{
					delay = frame_header->res1 * 200;
				} else if (frame_header->control == 3)
				{
					delay = frame_header->res1 * 2;
				} else if (frame_header->control == 4)
				{
					delay = rand_range(frame_header->res1, frame_header->res2) * 200;
				} else if (frame_header->control == 5)
				{
					delay = rand_range(frame_header->res1, frame_header->res2) * 2;
				} else
				{
					delay = 1;
				}
				if (delay == 0)
					delay = 1;
				bmap = malloc(1920);
				if (bmap != NULL)
				{
					decompress(ptr, bmap, 1920);
					img->delay[frames] = delay;
					img->image_buf[frames] = bmap;
					frames++;
				}
				ptr += frame_size;
				break;
			case 1:
				/* text screen dump */
				ptr += 320;
				break;
			case 0xfe:
				/* application specific */
				ptr += frame_size;
				break;
			case 0xff:
				/* EOF */
				ptr = end;
				break;
			}
		}
		free(data);
		if (frames > 0)
		{
			delay = img->delay[frames - 1];
			memmove(&img->delay[1], &img->delay[0], (frames - 1) * sizeof(img->delay[0]));
			img->delay[0] = delay;
		}
	} else
	{
		/* uncompressed PGF file */
		if (file_size != 1920)
		{
			return FALSE;
		}
		img->image_buf[0] = data;
		img->delay[0] = 0;
		frames = 1;
		strcpy(info->compression, "None");
	}
	
	info->planes = 1;
	info->width = 240;
	info->height = 64;
	info->components = 1;
	info->indexed_color = FALSE;
	info->colors = 2;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = frames;				/* required - more than 1 = animation */
	img->imagecount = frames;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = 0;			/* current line in bmap */
	info->_priv_ptr = img;
	
	return TRUE;
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	img_data *img = (img_data *) info->_priv_ptr;
	uint8_t *bmap;
	uint32_t pos;
	uint8_t b;
	uint16_t x;
	
	pos = info->_priv_var;
	if (pos == 0)
	{
		if (info->page_wanted >= (uint16_t)img->imagecount)
			return FALSE;
		info->delay = img->delay[info->page_wanted];
		info->_priv_var_more = 0;
	}
	bmap = img->image_buf[info->page_wanted];

	x = 0;
	do
	{								/* 1-bit mono v1.00 */
		b = bmap[pos++];
		buffer[x++] = b & 0x80 ? 1 : 0;
		buffer[x++] = b & 0x40 ? 1 : 0;
		buffer[x++] = b & 0x20 ? 1 : 0;
		buffer[x++] = b & 0x10 ? 1 : 0;
		buffer[x++] = b & 0x08 ? 1 : 0;
		buffer[x++] = b & 0x04 ? 1 : 0;
		buffer[x++] = b & 0x02 ? 1 : 0;
		buffer[x++] = b & 0x01 ? 1 : 0;
	} while (x < info->width);

	if (pos == 1920)
	{
		pos = 0;
	}

	info->_priv_var = pos;

	return TRUE;
}


void __CDECL reader_quit(IMGINFO info)
{
	int16_t i;
	img_data *img = (img_data *) info->_priv_ptr;

	if (img)
	{
		for (i = 0; i < img->imagecount; i++)
		{
			free(img->image_buf[i]);
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
