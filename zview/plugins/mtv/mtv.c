#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"


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


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	int16_t handle;
	size_t file_size;
	size_t image_size;
	char c;
	char strbuf[16];
	int i;
	unsigned int width;
	unsigned int height;

	if ((handle = (int16_t)Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	for (i = 0; i < (int)sizeof(strbuf) - 1; i++)
	{
		if (Fread(handle, 1, &c) != 1)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		file_size--;
		if (c == ' ')
			break;
		strbuf[i] = c;
	}
	strbuf[i] = '\0';
	width = myatoi(strbuf);
	for (i = 0; i < (int)sizeof(strbuf) - 1; i++)
	{
		if (Fread(handle, 1, &c) != 1)
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		file_size--;
		if (c == '\n')
			break;
		strbuf[i] = c;
	}
	strbuf[i] = '\0';
	height = myatoi(strbuf);

	image_size = (size_t)width * height * 3;
	if (file_size < image_size)
	{
		Fclose(handle);
		RETURN_ERROR(EC_BitmapLength);
	}

	info->indexed_color = FALSE;
	info->planes = 24;
	info->width = width;
	info->height = height;
	info->colors = 1L << 24;
	info->components = 3;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */

	strcpy(info->info, "MTV Raytracer Image");
	strcpy(info->compression, "None");

	info->_priv_var = handle;

	RETURN_SUCCESS();
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	int handle = (int)info->_priv_var;
	size_t linesize = (size_t)info->width * 3;

	if ((size_t)Fread(handle, linesize, buffer) != linesize)
		RETURN_ERROR(EC_Fread);

	RETURN_SUCCESS();
}


void __CDECL reader_quit(IMGINFO info)
{
	int handle = (int)info->_priv_var;
	
	if (handle > 0)
	{
		info->_priv_var = 0;
		Fclose(handle);
	}
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}
