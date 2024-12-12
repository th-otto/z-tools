#define	VERSION	     0x0102
#define NAME        "DelmPaint"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

/*
DelmPaint       *.DEL, *.DPH

Delmpaint is a 256 color painting package. It works in 320x200 and 320x240 but
there is no difference in the file type. From a .DEL file you can not tel
whether it is a 320x200 or a 320x240 file. The same is true for a *.DPH file,
which is either a 640x400 or a 640x480 file. The bigger format is always
assumed.

*.DEL format:
The *.DEL format contains 3 compressed blocks which are decompressed 32000
bytes. Those three blocks joined together are the depacked file.
Paked:
  1 long        length of 1st packed block
  1 long        length of 2nd packed block
??? bytes       1st packed block, depacked 32000 bytes
??? bytes       2nd packed block, depacked 32000 bytes
??? bytes       3rd packed block, depacked 32000 bytes
The blocks are packed with the same algorithm as used in Crackart

Depacked:
  256 longs     Palette 256 colors, in Falcon format, R, G, 0, B
38400 words     Picture data, 8 interleaved bitplanes

_______________________________________________________________________________

*.DPH format:
The *.DPH format contains 11 compressed blocks which are decompressed 32000
bytes. Those three blocks joined together are the depacked file.
Packed:
 10 long        length of 11st to 10th packed block

??? bytes       1st packed block, depacked 32000 bytes
??? bytes       2nd packed block, depacked 32000 bytes
....
??? bytes       1th packed block, depacked 32000 bytes
The blocks are packed with the same algorithm as used in Crackart

Depacked:
  256 longs     Palette 256 colors, in Falcon format, R, G, 0, B
38400 words     Picture 1 320x240 data, 8 interleaved bitplanes
38400 words     Picture 2 320x240 data, 8 interleaved bitplanes
38400 words     Picture 3 320x240 data, 8 interleaved bitplanes
38400 words     Picture 4 320x240 data, 8 interleaved bitplanes
Finally you have to join the pictures into one huge 640x480 picture. Take 320
bytes from picture 1, then 320 from 2, repeat 240 times, do the same for
picture 3 and 4.
*/

#include "plugin.h"
#include "zvplugin.h"

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("DEL\0" "DPH\0");

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

#define SCREEN_SIZE 32000

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
	uint8_t *bmap;
	uint8_t *temp;
	size_t num_blocks;
	size_t i;
	int j;
	uint32_t file_size;
	char ext[4];
	uint32_t block_length[10];

	handle = (int16_t)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	strcpy(ext, name + strlen(name) - 3);
	my_strupr(ext);

	if (strcmp(ext, "DEL") == 0)
	{
		num_blocks = 2;
		info->width = 320;
		info->height = 240;
	} else if (strcmp(ext, "DPH") == 0)
	{
		num_blocks = 10;
		info->width = 640;
		info->height = 480;
	} else
	{
		Fclose(handle);
		return FALSE;
	}

	temp = malloc(SCREEN_SIZE + 256);
	if (temp == NULL)
	{
		Fclose(handle);
		return FALSE;
	}

	bmap = malloc((num_blocks + 1) * SCREEN_SIZE);
	if (bmap == NULL)
	{
		free(temp);
		Fclose(handle);
		return FALSE;
	}

	Fread(handle, num_blocks * sizeof(block_length[0]), block_length);
	file_size -= num_blocks * sizeof(block_length[0]);
	for (i = 0; i < num_blocks; i++)
	{
		if ((size_t)Fread(handle, block_length[i], temp) != block_length[i])
		{
			free(bmap);
			free(temp);
			Fclose(handle);
			return FALSE;
		}
		ca_decompress(temp, &bmap[i * SCREEN_SIZE]);
		file_size -= block_length[i];
	}
	Fread(handle, file_size, temp);
	ca_decompress(temp, &bmap[num_blocks * SCREEN_SIZE]);
	Fclose(handle);
	
	if (num_blocks == 10)
	{
		free(temp);
		temp = malloc((num_blocks + 1) * SCREEN_SIZE);
		if (temp == NULL)
		{
			free(bmap);
			return FALSE;
		}
		memcpy(temp, bmap, (num_blocks + 1) * SCREEN_SIZE);
		for (i = 0; i < 240; i++)
		{
			memcpy(bmap + (i * 640 + 1024), temp + (i * 320 + 1024L), 320);
			memcpy(bmap + (i * 640 + 1344), temp + (i * 320 + 77824L), 320);
			memcpy(bmap + (i * 640 + 154624L), temp + (i * 320 + 154624L), 320);
			memcpy(bmap + (i * 640 + 154944L), temp + (i * 320 + 231424L), 320);
		}
	}
	free(temp);

	j = 0;
	for (i = 0; i < 256; i++)
	{
		info->palette[i].red = bmap[j++];
		info->palette[i].green = bmap[j++];
		j++;
		info->palette[i].blue = bmap[j++];
	}
		
	info->planes = 8;
	info->components = 3;
	info->indexed_color = TRUE;
	info->colors = 1L << 8;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 1024;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "DelmPaint");
	strcpy(info->compression, "RLE");
	
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
	uint16_t x;
	unsigned int bit;
	uint16_t plane0;
	uint16_t plane1;
	uint16_t plane2;
	uint16_t plane3;
	uint16_t plane4;
	uint16_t plane5;
	uint16_t plane6;
	uint16_t plane7;
	uint16_t *ptr;
	
	ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + pos);
	info->_priv_var += info->width;
	x = 0;
	while (x < info->width)
	{
		plane0 = *ptr++;
		plane1 = *ptr++;
		plane2 = *ptr++;
		plane3 = *ptr++;
		plane4 = *ptr++;
		plane5 = *ptr++;
		plane6 = *ptr++;
		plane7 = *ptr++;
		for (bit = 0; bit < 16; bit++)
		{
			buffer[x++] =
				((plane0 >> 15) & 1) |
				((plane1 >> 14) & 2) |
				((plane2 >> 13) & 4) |
				((plane3 >> 12) & 8) |
				((plane4 >> 11) & 16) |
				((plane5 >> 10) & 32) |
				((plane6 >> 9) & 64) |
				((plane7 >> 8) & 128);
			plane0 <<= 1;
			plane1 <<= 1;
			plane2 <<= 1;
			plane3 <<= 1;
			plane4 <<= 1;
			plane5 <<= 1;
			plane6 <<= 1;
			plane7 <<= 1;
		}
	}
	
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
