#define	VERSION	    0x203
#define NAME        "Pablo Paint"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"
#define NF_DEBUG 0
#include "nfdebug.h"

/*
Pablo Paint    *.PPP (st low resolution, app version 1.1)
               *.PA3 (st high resolution, app version 2.5)

43 bytes    file id, "PABLO PACKED PICTURE: Groupe CDND "+$0D+$0A
?           image data size in bytes stored in plain in ascii text + cr/lf
1 byte      resolution flag [0 = low, 2 = high]
1 byte      compression type [0 = uncompressed, 29 = compressed]
1 word      image data size in bytes (same value as above)
16 words    palette
?           image data

Uncompressed images are simply screen dumps as one might expect.

No description of the compression method exists at this time.

*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("PPP\0" "PA3\0" "BIB\0" "SRC\0");

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
	uint8_t resolution;
	uint8_t compression;
	uint16_t data_size;
	uint16_t palette[16];
};




#ifdef __GNUC__
#include "pppdepac.c"
#else
void __CDECL depack(void *src, void *dst) ASM_NAME("depack");
#endif


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
	size_t file_size;
	size_t data_size;
	size_t pos;
	int16_t handle;
	uint8_t *bmap;
	uint8_t *temp;
	struct file_header header;
	char ext[4];
	char strbuf[80];

	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	strcpy(ext, name + strlen(name) - 3);
	my_strupr(ext);
	
	if (strcmp(ext, "PPP") == 0 || strcmp(ext, "PA3") == 0)
	{
		if (read_str(handle, strbuf, &strbuf[sizeof(strbuf)]) == FALSE ||
			strcmp(strbuf, "PABLO PACKED PICTURE: Groupe CDND ") != 0)
		{
			Fclose(handle);
			return FALSE;
		}
		data_size = read_int(handle);
		nf_debugprintf(("ppp: datasize=%lu\n", data_size));
		bmap = malloc(SCREEN_SIZE + 256);
		temp = malloc(data_size);
		if (bmap == NULL || temp == NULL)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		pos = Fseek(0, handle, SEEK_CUR);
		Fread(handle, sizeof(header), &header);
		Fseek(pos, handle, SEEK_SET);
		nf_debugprintf(("ppp: resolution=%u compression=%u\n", header.resolution, header.compression));
		if ((size_t)Fread(handle, data_size, temp) != data_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		depack(temp, bmap);
	} else if (strcmp(ext, "SRC") == 0)
	{
		data_size = file_size;
		nf_debugprintf(("src: datasize=%lu\n", data_size));
		bmap = malloc(SCREEN_SIZE + 256);
		temp = malloc(data_size);
		if (bmap == NULL || temp == NULL)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		pos = Fseek(0, handle, SEEK_CUR);
		Fread(handle, sizeof(header), &header);
		Fseek(pos, handle, SEEK_SET);
		nf_debugprintf(("src: resolution=%u compression=%u\n", header.resolution, header.compression));
		if ((size_t)Fread(handle, data_size, temp) != data_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		depack(temp, bmap);
	} else if (strcmp(ext, "BIB") == 0)
	{
		uint32_t nlines;
		unsigned int i;
		
		nlines = read_int(handle);
		data_size = read_int(handle);
		nf_debugprintf(("bib: datasize=%lu\n", data_size));
		for (i = 0; i < nlines * 4; i++)
			read_str(handle, strbuf, &strbuf[sizeof(strbuf)]);
		bmap = malloc(SCREEN_SIZE + 256);
		temp = malloc(data_size + 256);
		if (bmap == NULL || temp == NULL)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		pos = Fseek(0, handle, SEEK_CUR);
		Fread(handle, sizeof(header), &header);
		Fseek(pos, handle, SEEK_SET);
		nf_debugprintf(("bib: resolution=%u compression=%u\n", header.resolution, header.compression));
		if ((size_t)Fread(handle, data_size, temp) != data_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		temp[0] = 2;
		header.resolution = 2;
		depack(temp, bmap);
	} else
	{
		Fclose(handle);
		return FALSE;
	}
	free(temp);
	Fclose(handle);
	
	switch (header.resolution)
	{
	case 0:
		info->planes = 4;
		info->components = 3;
		info->width = 320;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 1:
		info->planes = 2;
		info->components = 3;
		info->width = 640;
		info->height = 200;
		info->indexed_color = TRUE;
		break;
	case 2:
		info->planes = 1;
		info->components = 1;
		info->width = 640;
		info->height = 400;
		info->indexed_color = FALSE;
		break;
	default:
		free(bmap);
		return FALSE;
	}

	info->colors = 1L << info->planes;

	if (info->indexed_color)
	{	
		int i;

		for (i = 0; i < (int)info->colors; i++)
		{
			info->palette[i].red = (((header.palette[i] >> 7) & 0x0e) + ((header.palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((header.palette[i] >> 3) & 0x0e) + ((header.palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((header.palette[i] << 1) & 0x0e) + ((header.palette[i] >> 3) & 0x01)) * 17;
		}
	}

	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "Pablo Paint");
	if (strcmp(ext, "PPP") == 0)
		strcat(info->info, " (v1.1)");
	else if (strcmp(ext, "PA3") == 0)
		strcat(info->info, " (v2.5)");
	if (header.compression)
		/* FIXME: populate the compression method somehow */
		strcpy(info->compression, "RLE");
	else
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
	switch (info->planes)
	{
	case 1:
		{
			size_t pos = info->_priv_var;
			uint8_t *bmap = info->_priv_ptr;
			int16_t byte;
			int x;
			
			x = info->width >> 3;
			do
			{
				byte = bmap[pos++];
				*buffer++ = (byte >> 7) & 1;
				*buffer++ = (byte >> 6) & 1;
				*buffer++ = (byte >> 5) & 1;
				*buffer++ = (byte >> 4) & 1;
				*buffer++ = (byte >> 3) & 1;
				*buffer++ = (byte >> 2) & 1;
				*buffer++ = (byte >> 1) & 1;
				*buffer++ = (byte >> 0) & 1;
			} while (--x > 0);
			info->_priv_var = pos;
		}
		break;
	case 2:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t *ptr;
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			info->_priv_var += info->width >> 2;
			x = info->width >> 4;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2);
					plane0 <<= 1;
					plane1 <<= 1;
				}
			} while (--x > 0);
		}
		break;
	case 4:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t *ptr;
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			info->_priv_var += info->width >> 1;
			x = info->width >> 4;
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
		}
		break;
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
