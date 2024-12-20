#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
Dali           *.SD0 (ST low resolution)
               *.SD1 (ST medium resolution)
               *.SD2 (ST high resolution)

Files do not seem to have any resolution or bit plane info stored in them. The
file extension seems to be the only way to determine the contents.

1 long         file id? [always 0]
16 words       palette
92 bytes       reserved? [usually 0]
32000 bytes    raw image data
-----------
32128 bytes    total for file


Dali (Compressed)    *.LPK (ST low resolution)
                     *.MPK (ST medium resolution)
                     *.HPK (ST high resolution)

Files do not seem to have any resolution or bit plane info stored in them. The
file extension seems to be the only way to determine the contents.

16 words    palette
?           size of byte array in bytes, stored as plain ASCII text + cr/lf
?           size of long array in bytes, stored as plain ASCII text + cr/lf
?           array of bytes
?           array of longs

?           image data:
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

#define SCREEN_SIZE 32000

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


static long read_int(int16_t fh)
{
	char buf[16];
	
	if (read_str(fh, buf, &buf[sizeof(buf)]) == FALSE)
		return -1;
	return myatol(buf);
}


static void my_strupr(char *str)
{
	while (*str != '\0')
	{
		if (*str >= 'a' && *str <= 'z')
			*str -= 'a' - 'A';
		str++;
	}
}


static void decode_dali(const uint8_t *btab, const uint32_t *ltab, uint32_t *bmap32)
{
	uint8_t flag;
	int i;
	int offset;
	uint32_t data;

	flag = 0;
	for (i = 0; i < 40; i++)
	{
		for (offset = 0; offset < 8000; offset += 40)
		{
			if (flag == 0)
			{
				flag = *btab++;
				data = *ltab++;
			}
			bmap32[offset + i] = data;
			flag--;
		}
	}
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
	char ext[4];
	uint16_t palette[16];
	uint8_t *bmap;
	int res;
	size_t len;

	len = strlen(name);
	if (len <= 3)
		return FALSE;
	strcpy(ext, name + len - 3);
	my_strupr(ext);

	handle = (int16_t)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}

	bmap = malloc(SCREEN_SIZE);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}

	if (strcmp(ext, "SD0") == 0 ||
		strcmp(ext, "SD1") == 0 ||
		strcmp(ext, "SD2") == 0)
	{
		res = ext[2] - '0';
		Fseek(4, handle, SEEK_CUR);
		Fread(handle, sizeof(palette), palette);
		Fseek(128, handle, SEEK_SET);
		if (Fread(handle, SCREEN_SIZE, bmap) != SCREEN_SIZE)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		Fclose(handle);
		strcpy(info->compression, "None");
	} else
	{
		uint8_t *btab;
		uint32_t *ltab;
		long btab_size;
		long ltab_size;

		if (strcmp(ext, "LPK") == 0)
		{
			res = 0;
		} else if (strcmp(ext, "MPK") == 0)
		{
			res = 1;
		} else if (strcmp(ext, "HPK") == 0)
		{
			res = 2;
		} else
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		Fread(handle, sizeof(palette), palette);
		btab_size = read_int(handle);
		ltab_size = read_int(handle);
		if (btab_size <= 0 || ltab_size <= 0 || (btab = malloc(btab_size + ltab_size + 256)) == NULL)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		ltab = (uint32_t *)(btab + btab_size);
		if (Fread(handle, btab_size, btab) != btab_size ||
			Fread(handle, ltab_size, ltab) != ltab_size)
		{
			free(btab);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		Fclose(handle);
		decode_dali(btab, ltab, (uint32_t *)bmap);
		free(btab);
		strcpy(info->compression, "RLE");
	}
	
	info->planes = 4 >> res;
	switch (info->planes)
	{
	case 4:
		info->width = 320;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 2:
		info->width = 640;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 1:
		info->width = 640;
		info->height = 400;
		info->indexed_color = FALSE;
		break;
	}

	info->components = info->planes == 1 ? 1 : 3;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "Dali");

	if (info->indexed_color)
	{
		int i, colors;
		
		colors = 1 << info->planes;
		for (i = 0; i < colors; i++)
		{
			info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((palette[i] << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 0x11;
		}
	}
	
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
	size_t pos = info->_priv_var;
	
	switch (info->planes)
	{
	case 1:
		{
			int16_t x;
			int16_t byte;
			uint8_t *bmap8;
			
			x = 0;
			bmap8 = info->_priv_ptr;
			do
			{								/* 1-bit mono v1.00 */
				byte = bmap8[pos++];
				*buffer++ = (byte >> 7) & 1;
				*buffer++ = (byte >> 6) & 1;
				*buffer++ = (byte >> 5) & 1;
				*buffer++ = (byte >> 4) & 1;
				*buffer++ = (byte >> 3) & 1;
				*buffer++ = (byte >> 2) & 1;
				*buffer++ = (byte >> 1) & 1;
				*buffer++ = (byte >> 0) & 1;
				x += 8;
			} while (x < info->width);
		}
		break;
	default:
		{
			int16_t x;
			int16_t bit;
			uint16_t byte;
			int16_t plane;
			const uint16_t *bmap16 = (const uint16_t *)info->_priv_ptr;
			
			x = 0;
			do
			{
				for (bit = 15; bit >= 0; bit--)
				{
					for (plane = byte = 0; plane < info->planes; plane++)
					{
						if ((bmap16[pos + plane] >> bit) & 1)
							byte |= 1 << plane;
					}
					buffer[x] = byte;
					x++;
				}
				pos += info->planes;
			} while (x < info->width);
		}
		break;
	}
	
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
	if (info->_priv_var_more > 0)
	{
		Fclose(info->_priv_var_more);
		info->_priv_var_more = 0;
	}
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
