#include "plugin.h"
#include "zvplugin.h"

#define VERSION		0x0200
#define AUTHOR      "Thorsten Otto"
#define NAME        "Atari Portfolio Graphics"
#define DATE        __DATE__ " " __TIME__

#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)("PGF\0" "PGC\0");

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


static uint32_t lof(int16_t fhand)
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


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	uint32_t file_size;
	int16_t handle;
	uint8_t *bmap;
	uint8_t *data;

	bmap = NULL;
	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = lof(handle);
	data = malloc(file_size);
	if (data == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if (Fread(handle, file_size, data) != file_size)
	{
		free(data);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);
	if (data[0] == 'P' && data[1] == 'G' && data[2] == 1)
	{
		bmap = malloc(1920);
		if (bmap == NULL)
		{
			free(data);
			Fclose(handle);
			return FALSE;
		}
		decompress(&data[3], bmap, 1920);
		strcpy(info->compression, "RLE");
		free(data);
	} else
	{
		if (file_size != 1920)
		{
			return FALSE;
		}
		bmap = data;
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
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;
	strcpy(info->info, "Atari Portfolio Graphics");
	
	return TRUE;
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	uint32_t pos = info->_priv_var;
	uint8_t b;
	uint16_t x;
	
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
	info->_priv_var = pos;

	return TRUE;
}


void __CDECL reader_quit(IMGINFO info)
{
	free(info->_priv_ptr);
	info->_priv_ptr = 0;
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
