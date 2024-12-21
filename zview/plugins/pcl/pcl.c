#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

static int landscape;

/*
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_ENCODE;
	case OPTION_EXTENSIONS:
		return (long) (EXTENSIONS);

	case OPTION_QUALITY:
		return landscape;

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

long __CDECL set_option(zv_int_t which, zv_int_t value)
{
	switch ((int)which)
	{
	case OPTION_CAPABILITIES:
	case OPTION_EXTENSIONS:
		return -EACCES;
	case OPTION_QUALITY:
		landscape = value != 0;
		return landscape;
	}
	return -ENOSYS;
}

#endif


static void myitoa(unsigned int value, char *buffer)
{
	char *p;
	char tmpbuf[8 * sizeof(long) + 2];
	short i = 0;

	do {
		tmpbuf[i++] = "0123456789"[value % 10];
	} while ((value /= 10) != 0);

	p = buffer;
	while (--i >= 0)	/* reverse it back  */
		*p++ = tmpbuf[i];
	*p = '\0';
}



static void write_int(int fh, int val)
{
	char buf[16];
	
	myitoa(val, buf);
	Fwrite(fh, strlen(buf), buf);
}


static void write_str(int fh, const char *str)
{
	Fwrite(fh, strlen(str), str);
}


boolean __CDECL encoder_init(const char *name, IMGINFO info)
{
	int handle;
	uint8_t *image;

	image = malloc((size_t)info->width * info->height * 3);
	if (image == NULL)
	{
		return FALSE;
	}
	info->planes = 24;
	info->components = 3;
	info->colors = 1L << 24;
	info->orientation = UP_TO_DOWN;
	info->memory_alloc = TT_RAM;
	info->indexed_color = FALSE;
	info->page = 1;

	if ((handle = (int)Fcreate(name, 0)) < 0)
	{
		free(image);
		return FALSE;
	}

	info->_priv_var = handle;
	info->_priv_var_more = 0;
	info->_priv_ptr = image;
	
	return TRUE;
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	uint8_t *image = info->_priv_ptr;
	size_t bytes_per_row = (size_t)info->width * 3;

	memcpy(&image[info->_priv_var_more], buffer, bytes_per_row);
	info->_priv_var_more += bytes_per_row;
	return TRUE;
}


void __CDECL encoder_quit(IMGINFO info)
{
	int16_t handle = (int16_t)info->_priv_var;
	uint8_t *image = info->_priv_ptr;

	if (handle > 0)
	{
		write_str(handle, "\033E");		/* initialize */
		write_str(handle, "\033&l26A");	/* set paper size */
		write_str(handle, "\033*r3F");		/* Raster Graphics Presentation Mode */
		write_str(handle, "\033*t300R");	/* Raster Graphics Resolution */
		write_str(handle, "\033*r");		/* Source Raster Height */
		if (landscape)
			write_int(handle, info->width);
		else
			write_int(handle, info->height);
		write_str(handle, "T");
		write_str(handle, "\033*r");		/* Source Raster Width */
		if (landscape)
			write_int(handle, info->height);
		else
			write_int(handle, info->width);
		write_str(handle, "S");
		write_str(handle, "\033&l0E");		/* Define Top Margin at # Lines */
		write_str(handle, "\033&a0L");		/* Define Left Margin at # Columns */
		write_str(handle, "\033*v6W");		/* Configure Image Data */
		Fwrite(handle, 6, "\000\003\000\008\008\008");
		write_str(handle, "\033*r2A");		/* Start Raster Graphics */
		write_str(handle, "\033*b0Y");		/* move vertically # raster lines... */
		write_str(handle, "\033*b0M");		/* uncompressed */
		if (landscape)
		{
			size_t linesize;
			uint16_t x;
			uint16_t y;
			size_t xpos;
			size_t pos;
	
			linesize = (size_t)info->width * 3;
			for (x = 0; x < info->width; x++)
			{
				write_str(handle, "\033*b");
				write_int(handle, info->height * 3);
				write_str(handle, "W");
				xpos = (size_t)info->height - 1;
				pos = xpos * linesize + (size_t)x * 3;
				for (y = 0; y < info->height; y++)
				{
					Fwrite(handle, 3, &image[pos]);
					pos -= linesize;
				}
				xpos--;
			}
		} else
		{
			size_t pos;
			size_t linesize;
			uint16_t y;
			
			pos = 0;
			linesize = (size_t)info->width * 3;
			for (y = 0; y < info->height; y++)
			{
				write_str(handle, "\033*b");
				write_int(handle, info->width * 3);
				write_str(handle, "W");
				Fwrite(handle, linesize, &image[pos]);
				pos += linesize;
			}
		}
		write_str(handle, "\033*rB");
		write_str(handle, "\033E");		/* initialize */

		Fclose(handle);
		info->_priv_var = 0;
	}
	
	free(info->_priv_ptr);
	info->_priv_ptr = NULL;
}
