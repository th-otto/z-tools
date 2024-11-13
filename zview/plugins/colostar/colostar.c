#define	VERSION	     0x0201
#define NAME        "colorSTar (Object), monoSTar (Object)"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

struct file_header {
	int16_t width;
	int16_t height;
	int16_t planes;
};

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("OBJ\0");

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


static size_t calc_screensize(int16_t width, int16_t height, uint16_t planes)
{
	uint16_t wdwidth;
	
	wdwidth = ((width + 15) >> 4);
	return (size_t)wdwidth * 2 * planes * height;
}


static int myatoi(const char *str)
{
	int val = 0;
	
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


static int read_int(int16_t fh, uint16_t *val)
{
	int err;
	char buf[16];
	
	err = read_str(fh, buf, &buf[sizeof(buf)]);
	*val = myatoi(buf);
	return err;
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
	uint8_t *bmap;
	int16_t handle;
	struct file_header file_header;
	uint16_t color;
	int i;
	size_t screen_size;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	if ((size_t)Fread(handle, sizeof(file_header), &file_header) != sizeof(file_header))
	{
		Fclose(handle);
		return FALSE;
	}
	if (file_header.planes != 1 && file_header.planes != 4)
	{
		Fseek(0, handle, SEEK_SET);
		for (i = 0; i < 16; i++)
		{
			if (read_int(handle, &color) == FALSE)
			{
				Fclose(handle);
				return FALSE;
			}
			info->palette[i].red = (((color >> 7) & 0x0e) + ((color >> 11) & 0x01)) * 17;
			info->palette[i].green = (((color >> 3) & 0x0e) + ((color >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((color << 1) & 0x0e) + ((color >> 3) & 0x01)) * 17;
		}
		if ((size_t)Fread(handle, sizeof(file_header), &file_header) != sizeof(file_header))
		{
			Fclose(handle);
			return FALSE;
		}
	}
	if (file_header.planes != 1 && file_header.planes != 4)
	{
		Fclose(handle);
		return FALSE;
	}
	
	file_header.width++;
	file_header.height++;
	screen_size = calc_screensize(file_header.width, file_header.height, file_header.planes);
	bmap = malloc(screen_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	
	if (file_header.planes == 4)
	{
		strcpy(info->info, "colorSTar (Object)");
		info->indexed_color = TRUE;
		info->components = 3;
	} else
	{
		strcpy(info->info, "monoSTar (Object)");
		info->indexed_color = FALSE;
		info->components = 1;
	}

	if ((size_t)Fread(handle, screen_size, bmap) != screen_size)
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}

	info->planes = file_header.planes;
	info->colors = 1L << info->planes;
	info->width = file_header.width;
	info->height = file_header.height;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;
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
	const uint16_t *screen16 = (const uint16_t *)info->_priv_ptr;
	int pos = info->_priv_var;
	uint16_t x;
	int16_t i;
	uint16_t byte;
	int plane;

	switch (info->planes)
	{
	case 1:
		x = 0;
		do
		{
			byte = screen16[pos++];
			*buffer++ = (byte >> 15) & 1;
			*buffer++ = (byte >> 14) & 1;
			*buffer++ = (byte >> 13) & 1;
			*buffer++ = (byte >> 12) & 1;
			*buffer++ = (byte >> 11) & 1;
			*buffer++ = (byte >> 10) & 1;
			*buffer++ = (byte >> 9) & 1;
			*buffer++ = (byte >> 8) & 1;
			*buffer++ = (byte >> 7) & 1;
			*buffer++ = (byte >> 6) & 1;
			*buffer++ = (byte >> 5) & 1;
			*buffer++ = (byte >> 4) & 1;
			*buffer++ = (byte >> 3) & 1;
			*buffer++ = (byte >> 2) & 1;
			*buffer++ = (byte >> 1) & 1;
			*buffer++ = (byte >> 0) & 1;
			x += 16;
		} while (x < info->width);
		break;

	case 4:
		x = 0;
		do
		{
			for (i = 15; i >= 0; i--)
			{
				for (plane = byte = 0; plane < 4; plane++)
				{
					if ((screen16[pos + plane] >> i) & 1)
						byte |= 1 << plane;
				}
				buffer[x] = byte;
				x++;
			}
			pos += 4;
		} while (x < info->width);
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
