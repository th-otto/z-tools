#define	VERSION	     0x0102
#define NAME        "D-GRAPH"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
D-GRAPH    *.P32 *.P3C (ST low resolution)

This format appears to be from a very early version of DuneGraph. Perhaps
scrapped when the Falcon030 arrived?

These are essentially 2 ST low resolution screens interlaced together.
When viewed the human eye merges the two alternating images into one.

P32 details:

1 word         file id, value unknown (no samples found)
16 words       palette
32000 bytes    image 1 (screen memory)
32000 bytes    image 2 (screen memory)
-----------
64034 byes     total

_______________________________________________________________________________

P3C details:

? bytes     compressed size of screen 1 stored in plain ascii text + cr/lf
16 words    palette
? bytes     screen 1 (packed)
? bytes     compressed size of screen 2 stored in plain ascii text + cr/lf
? bytes     screen 2 (packed)

Compressed image data is identical to CrackArt.

See also CrackArt file format
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("P3C\0" "P32\0");

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


struct file_header {
	uint16_t id;
	uint16_t palette[16];
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

#include "../crackart/ca_unpac.c"


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
	uint8_t *screen1;
	uint8_t *screen2;
	uint8_t *temp;
	uint32_t compressed_size;
	char extension[4];
	int i, j;
	struct file_header file_header;
	COLOR_MAP palette[16];

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}

	strcpy(extension, name + strlen(name) - 3);
	my_strupr(extension);
	
	screen1 = malloc(SCREEN_SIZE * 2);
	temp = malloc(SCREEN_SIZE);
	if (screen1 == NULL || temp == NULL)
	{
		free(temp);
		free(screen1);
		Fclose(handle);
		return FALSE;
	}
	screen2 = screen1 + SCREEN_SIZE;

	if (strcmp(extension, "P32") == 0)
	{
		if ((size_t)Fread(handle, sizeof(file_header), &file_header) != sizeof(file_header) ||
			Fread(handle, SCREEN_SIZE, screen1) != SCREEN_SIZE ||
			Fread(handle, SCREEN_SIZE, screen2) != SCREEN_SIZE)
		{
			free(temp);
			free(screen1);
			Fclose(handle);
			return FALSE;
		}
		strcpy(info->compression, "None");
	} else if (strcmp(extension, "P3C") == 0)
	{
		compressed_size = read_int(handle);
		if (Fread(handle, sizeof(file_header.palette), file_header.palette) != sizeof(file_header.palette) ||
			compressed_size > SCREEN_SIZE ||
			(size_t)Fread(handle, compressed_size, temp) != compressed_size)
		{
			free(temp);
			free(screen1);
			Fclose(handle);
			return FALSE;
		}
		ca_decompress(temp, screen1);
		compressed_size = read_int(handle);
		if (compressed_size > SCREEN_SIZE ||
			(size_t)Fread(handle, compressed_size, temp) != compressed_size)
		{
			free(temp);
			free(screen1);
			Fclose(handle);
			return FALSE;
		}
		ca_decompress(temp, screen2);
		strcpy(info->compression, "RLE");
	} else
	{
		free(temp);
		free(screen1);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);
	free(temp);

	for (i = 0; i < 16; i++)
	{
		palette[i].red = (((file_header.palette[i] >> 7) & 0x0e) + ((file_header.palette[i] >> 11) & 0x01)) * 17;
		palette[i].green = (((file_header.palette[i] >> 3) & 0x0e) + ((file_header.palette[i] >> 7) & 0x01)) * 17;
		palette[i].blue = (((file_header.palette[i] << 1) & 0x0e) + ((file_header.palette[i] >> 3) & 0x01)) * 17;
	}
	for (i = 0; i < 16; i++)
		for (j = 0; j < 16; j++)
		{
			info->palette[i * 16 + j].red = (palette[i].red + palette[j].red) >> 1;
			info->palette[i * 16 + j].green = (palette[i].green + palette[j].green) >> 1;
			info->palette[i * 16 + j].blue = (palette[i].blue + palette[j].blue) >> 1;
		}
		
	info->width = 320;
	info->height = 200;
	info->indexed_color = TRUE;
	info->components = 3;
	info->planes = 8;
	info->colors = 1L << 8;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = screen1;

	strcpy(info->info, "D-GRAPH");
	
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
	int width = info->width;
	int x;
	int w;
	int i;
	uint16_t byte;
	const uint16_t *screen1;
	const uint16_t *screen2;
	
	screen1 = (const uint16_t *)info->_priv_ptr;
	screen2 = (const uint16_t *)(info->_priv_ptr + SCREEN_SIZE);
	x = 0;
	w = 0;
	do
	{
		for (i = 15; i >= 0; i--)
		{
			byte = 0;
			if ((screen1[pos + 0] >> i) & 1)
				byte |= 1 << 4;
			if ((screen1[pos + 1] >> i) & 1)
				byte |= 1 << 5;
			if ((screen1[pos + 2] >> i) & 1)
				byte |= 1 << 6;
			if ((screen1[pos + 3] >> i) & 1)
				byte |= 1 << 7;

			if ((screen2[pos + 0] >> i) & 1)
				byte |= 1 << 0;
			if ((screen2[pos + 1] >> i) & 1)
				byte |= 1 << 1;
			if ((screen2[pos + 2] >> i) & 1)
				byte |= 1 << 2;
			if ((screen2[pos + 3] >> i) & 1)
				byte |= 1 << 3;
			buffer[w++] = byte;
		}
		pos += 4;
		x += 16;
	} while (x < width);

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
