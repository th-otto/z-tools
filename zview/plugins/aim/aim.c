#include "plugin.h"
#include "zvplugin.h"

#define VERSION		0x0201
#define NAME        "AIM (Atari Image Manager)"
#define DATE        __DATE__ " " __TIME__
#define AUTHOR      "Thorsten Otto"

struct file_header
{
	uint32_t address;		/* image address in memory */
	char name[16];			/* name of the image file */
	int16_t format;			/* format */
	int16_t sizex;			/* dimension 1 */
	int16_t sizey;			/* dimension 2 */
	int16_t sizez;			/* dimension 3 */
	uint16_t commsz;		/* size of comment in bytes */
	int16_t data[16];		/* user image dependant data */
};

#define F_WORDS 0 /* format words in the image */
#define F_BYTES 1 /* format bytes in the image */
#define F_FOBIT 2 /* four bits image, packed in 4 pix/word */
#define F_TWBIT 3 /* two bits image, packed in 8 pix/word */
#define F_SBITS 4 /* format single bits in the image */

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("COL\0" "IM\0");

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


char *strrchr(const char *s, int c)
{
	const char *cp = s + strlen(s);

	do
	{
		if (*cp == (char) c)
			return (char*)cp;
	} while (--cp >= s);

	return NULL;
}


static int header_open(const char *filename, IMGINFO info)
{
	char tmpname[256];
	char *p;
	int16_t tmp_handle;
	struct file_header header;

	strcpy(tmpname, filename);
	p = strrchr(tmpname, '.');
	if (p == NULL)
		return FALSE;
	strcpy(p + 1, "hd");
	if ((tmp_handle = (int16_t) Fopen(tmpname, FO_READ)) < 0)
		return FALSE;
	if (Fread(tmp_handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(tmp_handle);
		return FALSE;
	}
	Fclose(tmp_handle);
	if (header.address != 0)
		return FALSE;

	info->width = header.sizex;
	info->height = header.sizey;

	return TRUE;
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
	uint32_t file_size;
	int16_t handle;
	uint8_t *data;
	char extension[4];
	int custom_size;
	
	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}

	file_size = filesize(handle);
	data = malloc(file_size);
	if (data == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((uint32_t)Fread(handle, file_size, data) != file_size)
	{
		free(data);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);
	custom_size = FALSE;
	
	strcpy(info->info, "Atari Image Manager ");

	strcpy(extension, name + strlen(name) - 3);
	if (strcmp(extension, "COL") == 0 || strcmp(extension, "col") == 0)
	{
		info->planes = 24;
		info->components = 3;
		info->indexed_color = FALSE;
		if (file_size == 256L * 256L)
		{
			info->width = 128;
			info->height = 128;
		} else if (file_size == 4L * 256L * 256L)
		{
			info->width = 256;
			info->height = 256;
		} else if (header_open(name, info))
		{
			custom_size = TRUE;
		} else
		{
			free(data);
			return FALSE;
		}
		strcat(info->info, "(color)");
	} else
	{
		int i;

		info->planes = 8;
		info->components = 1;
		info->indexed_color = TRUE;
		
		if (file_size == 256L * 256L)
		{
			info->width = 256;
			info->height = 256;
		} else if (file_size == 128L * 128L)
		{
			info->width = 128;
			info->height = 128;
		} else if (header_open(name, info))
		{
			custom_size = TRUE;
		} else
		{
			return FALSE;
		}

		for (i = 0; i < 256; i++)
		{
			info->palette[i].red =
			info->palette[i].green =
			info->palette[i].blue = i;
		}
		strcat(info->info, "(graycale)");
	}
	
	info->real_width = info->width;
	info->real_height = info->height;
	info->colors = 1L << info->planes;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = data;
	
	if (custom_size)
	{
		/* strcat(info->info, " (custom size)"); */
	}
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
	uint16_t x;
	uint8_t *file_buffer = info->_priv_ptr;
	uint32_t file_pos = info->_priv_var;
	
	if (info->planes == 8)
	{
		memcpy(buffer, &file_buffer[file_pos], info->width);
		file_pos += info->width;
	} else
	{
		int16_t i;
		uint32_t datasize;

		datasize = (unsigned long)info->width * info->height;
		for (i = 0, x = 0; x < info->width; x++)
		{
			buffer[i++] = file_buffer[file_pos + datasize];
			buffer[i++] = file_buffer[file_pos + datasize + datasize];
			buffer[i++] = file_buffer[file_pos + datasize + datasize + datasize];
			file_pos++;
		}
	}
	info->_priv_var = file_pos;

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
	uint8_t *file_buffer = info->_priv_ptr;
	
	free(file_buffer);
	info->_priv_ptr = NULL;
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