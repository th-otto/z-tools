#include "plugin.h"
#include "zvplugin.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"

/*
ZZ_ROUGH    *.RGH (ST low resolution)

12 bytes    file id, "(c)F.MARCHAL"
?           size of byte array in bytes, stored as plain ASCII text + cr/lf
16 words    palette
?           array of bytes
?           array of longs (size in bytes = size of byte array * 4)

?           image data:
Same as Dali Compressed.

See also Dali Compressed file format
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
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif

#define SCREEN_SIZE 32000L
#define SCREEN_W 320
#define SCREEN_H 200

struct file_header {
	char magic[12];
};



static long myatol(const char *str)
{
	long val = 0;
	
	while (*str != 0)
	{
		if (*str < '0' || *str > '9')
			return -1;
		val = val * 10 + (*str - '0');
		str++;
	}
	return val;
}


static int read_str(int16_t fh, char *str, char *end)
{
	char c;
	
	*str = '\0';
	for (;;)
	{
		if (str >= end)
			return FALSE;
		if (Fread(fh, 1, &c) <= 0)
			return FALSE;
		if (c == '\r' || c == '\n')
		{
			*str = '\0';
			if (c == '\r')
				Fread(fh, 1, &c);
			return TRUE;
		}
		*str++ = c;
	}
}


static void decode_dali(uint8_t *btab, uint32_t *ltab, uint32_t *bmap32)
{
	uint8_t flag;
	uint16_t i;
	uint16_t index;
	uint16_t offset;
	uint32_t data;

	flag = index = 0;
	for (i = 0; i <= 156; i += 4)
	{
		for (offset = 0; offset <= 31840; offset += 160)
		{
			if (flag == 0)
			{
				flag = btab[index];
				data = ltab[index];
				index++;
			}
			bmap32[(offset + i) >> 2] = data;
			flag--;
		}
	}
}


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	int16_t handle;
	uint8_t *bmap;
	size_t btab_size;
	size_t ltab_size;
	uint8_t *btab;
	uint32_t *ltab;
	char strbuf[20];
	uint16_t palette[16];
	struct file_header header;

	if ((handle = (int16_t)Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}

	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (memcmp(header.magic, "(c)F.MARCHAL", sizeof(header.magic)) != 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	if (read_str(handle, strbuf, &strbuf[sizeof(strbuf)]) == FALSE ||
		Fread(handle, sizeof(palette), palette) != sizeof(palette))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	btab_size = myatol(strbuf);
	ltab_size = btab_size * sizeof(*ltab);

	bmap = malloc(SCREEN_SIZE + btab_size + ltab_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	btab = bmap + SCREEN_SIZE;
	ltab = (uint32_t *)(btab + btab_size);
	if ((size_t)Fread(handle, btab_size, btab) != btab_size ||
		(size_t)Fread(handle, ltab_size, ltab) != ltab_size)
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);

	decode_dali(btab, ltab, (uint32_t *)bmap);
	
	info->planes = 4;
	info->colors = 1L << 4;
	info->width = 320;
	info->height = 200;
	info->indexed_color = TRUE;
	info->components = 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */

	info->_priv_var = 0;
	info->_priv_ptr = bmap;

	strcpy(info->info, "ZZ_ROUGH");
	strcpy(info->compression, "RLE");

	{	
		int i;

		for (i = 0; i < 16; i++)
		{
			info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((palette[i] << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 17;
		}
	}

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
