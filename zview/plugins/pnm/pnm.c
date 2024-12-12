#define VERSION		0x0103
#define NAME        "PNM (Portable Any Map)"
#define DATE        __DATE__ " " __TIME__
#define AUTHOR      "Thorsten Otto"

#include "plugin.h"
#include "zvplugin.h"
#define NF_DEBUG 0
#include "nfdebug.h"

static uint16_t filetype;
static uint16_t maxval;
static double maxvalmult;
static uint8_t *file_buffer;
static size_t file_pos;

#define PNM_P1 0x5031
#define PNM_P2 0x5032
#define PNM_P3 0x5033
#define PNM_P4 0x5034
#define PNM_P5 0x5035
#define PNM_P6 0x5036
#define PNM_P7 0x5037

#define ISSPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#define ISDIGIT(c) (((c) - '0') <= '9')



#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
	case OPTION_EXTENSIONS:
		return (long) ("PPM\0PGM\0PBM\0PAM\0");

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


static uint16_t read_int(IMGINFO info)
{
	unsigned char ch;
	int i;
	uint16_t val;

	(void) info;
	i = 0;
	val = 0;
	for (;;)
	{
		ch = file_buffer[file_pos++];
		if (ch == 0)
			return -1;
		if (ch == '#')
		{
			do
			{
				ch = file_buffer[file_pos++];
			} while (ch > 0x0a);
		} else if (ISSPACE(ch))
		{
			if (i != 0)
				break;
		} else if (ISDIGIT(ch))
		{
			if (val >= (65540UL / 10) || (val >= (65530U / 10) && ch >= '6'))
				return -1;
			val = val * 10 + (ch - '0');
			i = 1;
		}
	}
	return val;
}


static int read_pam_header(IMGINFO info)
{
	int i;
	char namebuf[20];
	unsigned char ch;
	
	info->width = 0;
	info->height = 0;
	info->components = 0;
	maxval = 0;
	for (;;)
	{
		i = 0;
		for (;;)
		{
			ch = file_buffer[file_pos++];
			if (ch == 0)
				return FALSE;
			if (ch == '#')
			{
				do
				{
					ch = file_buffer[file_pos++];
				} while (ch > 0x0a);
			} else if (ISSPACE(ch))
			{
				break;
			}
			if (i < (int)sizeof(namebuf) - 1)
				namebuf[i++] = ch;
		}
		namebuf[i] = 0;
		if (strcmp(namebuf, "ENDHDR") == 0)
			break;
		if (strcmp(namebuf, "WIDTH") == 0)
		{
			if ((info->width = read_int(info)) == (uint16_t)-1)
				return FALSE;
			nf_debugprintf(("width: %u\n", info->width));
		} else if (strcmp(namebuf, "HEIGHT") == 0)
		{
			if ((info->height = read_int(info)) == (uint16_t)-1)
				return FALSE;
			nf_debugprintf(("heigth: %u\n", info->width));
		} else if (strcmp(namebuf, "DEPTH") == 0)
		{
			if ((info->components = read_int(info)) == (uint16_t)-1)
				return FALSE;
			nf_debugprintf(("depth: %u\n", info->components));
		} else if (strcmp(namebuf, "MAXVAL") == 0)
		{
			if ((maxval = read_int(info)) == (uint16_t)-1)
				return FALSE;
			nf_debugprintf(("maxval: %u\n", maxval));
		} else if (strcmp(namebuf, "TUPLTYPE") == 0)
		{
			/* ignored for now */
			do
			{
				ch = file_buffer[file_pos++];
			} while (ch > 0x0a);
		} else
		{
			nf_debugprintf(("unknown hdr type: %s\n", namebuf));
			return FALSE;
		}
	}
	if (info->width == 0 || info->height == 0 || info->components == 0 || maxval == 0)
	{
		return FALSE;
	}
	return TRUE;
}


static void cleanup(IMGINFO info)
{
	int16_t handle = info->_priv_var;

	Mfree(file_buffer);
	file_buffer = NULL;
	if (handle > 0)
	{
		Fclose(handle);
		info->_priv_var = 0;
	}
}


static int read_pnm_header(IMGINFO info)
{
	if ((info->width = read_int(info)) == (uint16_t)-1)
	{
		cleanup(info);
		return FALSE;
	}
	if ((info->height = read_int(info)) == (uint16_t)-1)
	{
		cleanup(info);
		return FALSE;
	}
	return TRUE;
}


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
	uint32_t filesize;
	int i;
	uint32_t datasize;
	int16_t handle;
	uint32_t linesize;

	file_buffer = NULL;
	if ((handle = (int16_t) Fopen(name, 0)) < 0)
	{
		cleanup(info);
		return FALSE;
	}
	info->_priv_var = handle;
	filesize = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	file_buffer = (uint8_t *) Malloc(filesize + 1);
	if (file_buffer == NULL)
	{
		cleanup(info);
		return FALSE;
	}

	if (Fread(handle, filesize, file_buffer) != (long)filesize)
	{
		cleanup(info);
		return FALSE;
	}
	file_buffer[filesize] = '\0';
	filetype = (file_buffer[0] << 8) | file_buffer[1];
	file_pos = 3;
	switch (filetype)
	{
	case PNM_P1:
		strcpy(info->info, "PBM BitMap (Ascii)");
		if (read_pnm_header(info) == FALSE)
			return FALSE;
		maxval = 1;
		datasize = 0;
		info->planes = 1;
		info->components = 1;
		info->indexed_color = TRUE;
		info->palette[0].red = info->palette[0].green = info->palette[0].blue = 255;
		info->palette[1].red = info->palette[1].green = info->palette[1].blue = 0;
		break;
	case PNM_P2:
		strcpy(info->info, "PGM GrayMap (Ascii)");
		if (read_pnm_header(info) == FALSE)
			return FALSE;
		if ((maxval = read_int(info)) == (uint16_t)-1)
		{
			cleanup(info);
			return FALSE;
		}
		datasize = 0;
		info->planes = 8;
		info->components = 1;
		info->indexed_color = TRUE;
		for (i = 0; i < 256; i++)
			info->palette[i].red = info->palette[i].green = info->palette[i].blue = i;
		break;
	case PNM_P3:
		strcpy(info->info, "PPM PixMap (Ascii)");
		if (read_pnm_header(info) == FALSE)
			return FALSE;
		if ((maxval = read_int(info)) == (uint16_t)-1)
		{
			cleanup(info);
			return FALSE;
		}
		datasize = 0;
		info->planes = 24;
		info->components = 3;
		info->indexed_color = FALSE;
		break;
	case PNM_P4:
		strcpy(info->info, "PBM BitMap (Binary)");
		if (read_pnm_header(info) == FALSE)
			return FALSE;
		linesize = (info->width + 7) >> 3;
		datasize = linesize * info->height;
		maxval = 1;
		info->planes = 1;
		info->components = 1;
		info->indexed_color = TRUE;
		info->palette[0].red = info->palette[0].green = info->palette[0].blue = 255;
		info->palette[1].red = info->palette[1].green = info->palette[1].blue = 0;
		break;
	case PNM_P5:
		strcpy(info->info, "PGM GrayMap (Binary)");
		if (read_pnm_header(info) == FALSE)
			return FALSE;
		if ((maxval = read_int(info)) == (uint16_t)-1)
		{
			cleanup(info);
			return FALSE;
		}
		datasize = ((uint32_t) info->width * (uint32_t) info->height);
		info->planes = 8;
		info->components = 1;
		info->indexed_color = TRUE;
		for (i = 0; i < 256; i++)
			info->palette[i].red = info->palette[i].green = info->palette[i].blue = i;
		break;
	case PNM_P6:
		strcpy(info->info, "PPM PixMap (Binary)");
		if (read_pnm_header(info) == FALSE)
			return FALSE;
		if ((maxval = read_int(info)) == (uint16_t)-1)
		{
			cleanup(info);
			return FALSE;
		}
		datasize = ((uint32_t) info->width * (uint32_t) info->height) * 3;
		info->planes = 24;
		info->components = 3;
		info->indexed_color = FALSE;
		break;
	case PNM_P7:
		if (read_pam_header(info) == FALSE)
		{
			cleanup(info);
			return FALSE;
		}
		switch (info->components)
		{
		case 1:
			if (maxval == 1)
			{
				strcpy(info->info, "PAM BitMap");
				filetype = PNM_P4;
				linesize = (info->width + 7) >> 3;
				datasize = linesize * info->height;
				info->planes = 1;
				info->components = 1;
				info->indexed_color = TRUE;
				info->palette[0].red = info->palette[0].green = info->palette[0].blue = 255;
				info->palette[1].red = info->palette[1].green = info->palette[1].blue = 0;
			} else
			{
				strcpy(info->info, "PAM GrayMap");
				filetype = PNM_P5;
				datasize = ((uint32_t) info->width * (uint32_t) info->height);
				info->planes = 8;
				info->components = 1;
				info->indexed_color = TRUE;
				for (i = 0; i < 256; i++)
					info->palette[i].red = info->palette[i].green = info->palette[i].blue = i;
			}
			break;
		case 3:
			strcpy(info->info, "PAM PixMap");
			filetype = PNM_P6;
			datasize = ((uint32_t) info->width * (uint32_t) info->height) * 3;
			info->planes = 24;
			info->components = 3;
			info->indexed_color = FALSE;
			break;
		case 4:
			strcpy(info->info, "PAM PixMap (alpha)");
			filetype = PNM_P6;
			datasize = ((uint32_t) info->width * (uint32_t) info->height) * 4;
			info->planes = 32;
			info->components = 3;
			info->indexed_color = FALSE;
			break;
		case 2:
			/* graymap with alpha not supported */
		default:
			cleanup(info);
			return FALSE;
		}
		break;
	default:
		cleanup(info);
		return FALSE;
	}
	
	if (maxval >= 256)
		datasize *= 2;
	if (filetype == PNM_P4 || filetype == PNM_P5 || filetype == PNM_P6)
	{
		if (file_pos + datasize > filesize)
		{
			cleanup(info);
			return FALSE;
		}
		file_pos = filesize - datasize;
	}
	Fclose(handle);
	info->_priv_var = 0;

	maxvalmult = maxval == 0 ? 255.0 : 255.0 / maxval;

	info->real_width = info->width;
	info->real_height = info->height;
	info->colors = 1L << MIN(info->planes, 24);
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	
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
	uint32_t i;
	uint8_t j;
	uint8_t byte;
	uint16_t g;

	i = 0;
	switch (filetype)
	{
	case PNM_P1:
		for (x = 0; x < info->width; x++)
		{
			buffer[x] = read_int(info);
		}
		break;
	case PNM_P2:
		if (maxval == 255)
		{
			for (x = 0; x < info->width; x++)
			{
				buffer[x] = read_int(info);
			}
		} else
		{
			for (x = 0; x < info->width; x++)
			{
				buffer[x] = read_int(info) * maxvalmult;
			}
		}
		break;
	case PNM_P3:
		if (maxval == 255)
		{
			for (x = 0; x < info->width; x++)
			{
				buffer[i++] = read_int(info);
				buffer[i++] = read_int(info);
				buffer[i++] = read_int(info);
			}
		} else
		{
			for (x = 0; x < info->width; x++)
			{
				buffer[i++] = read_int(info) * maxvalmult;
				buffer[i++] = read_int(info) * maxvalmult;
				buffer[i++] = read_int(info) * maxvalmult;
			}
		}
		break;
	case PNM_P4:
		{
			uint32_t linesize;
	
			linesize = (info->width + 7) >> 3;
			for (x = 0; x < linesize; x++)
			{
				byte = file_buffer[file_pos++];
				for (j = 0; j < 8; j++)
				{
					if (byte & (0x80 >> j))
						buffer[i] = 1;
					else
						buffer[i] = 0;
					i++;
				}
			}
		}
		break;
	case PNM_P5:
		if (maxval == 255)
		{
			memcpy(buffer, &file_buffer[file_pos], info->width);
			file_pos += info->width;
		} else if (maxval < 255)
		{
			for (x = 0; x < info->width; x++)
			{
				buffer[x] = file_buffer[file_pos++] * maxvalmult;
			}
		} else
		{
			for (x = 0; x < info->width; x++)
			{
				g = file_buffer[file_pos++] << 8;
				g |= file_buffer[file_pos++];
				buffer[x] = g * maxvalmult;
			}
		}
		break;
	case PNM_P6:
		if (info->planes == 32)
		{
			if (maxval == 255)
			{
				for (x = 0; x < info->width; x++)
				{
					buffer[i++] = file_buffer[file_pos++];
					buffer[i++] = file_buffer[file_pos++];
					buffer[i++] = file_buffer[file_pos++];
					file_pos++;
				}
			} else if (maxval < 255)
			{
				for (x = 0; x < info->width; x++)
				{
					buffer[i++] = file_buffer[file_pos++] * maxvalmult;
					buffer[i++] = file_buffer[file_pos++] * maxvalmult;
					buffer[i++] = file_buffer[file_pos++] * maxvalmult;
					file_pos++;
				}
			} else
			{
				for (x = 0; x < info->width; x++)
				{
					g = file_buffer[file_pos++] << 8;
					g |= file_buffer[file_pos++];
					buffer[i++] = g * maxvalmult;
					g = file_buffer[file_pos++] << 8;
					g |= file_buffer[file_pos++];
					buffer[i++] = g * maxvalmult;
					g = file_buffer[file_pos++] << 8;
					g |= file_buffer[file_pos++];
					buffer[i++] = g * maxvalmult;
					file_pos++;
				}
			}
		} else
		{
			if (maxval == 255)
			{
				uint32_t rowsize;
	
				rowsize = (uint32_t)info->width * 3;
				memcpy(buffer, &file_buffer[file_pos], rowsize);
				file_pos += rowsize;
			} else if (maxval < 255)
			{
				for (x = 0; x < info->width; x++)
				{
					buffer[i++] = file_buffer[file_pos++] * maxvalmult;
					buffer[i++] = file_buffer[file_pos++] * maxvalmult;
					buffer[i++] = file_buffer[file_pos++] * maxvalmult;
				}
			} else
			{
				for (x = 0; x < info->width; x++)
				{
					g = file_buffer[file_pos++] << 8;
					g |= file_buffer[file_pos++];
					buffer[i++] = g * maxvalmult;
					g = file_buffer[file_pos++] << 8;
					g |= file_buffer[file_pos++];
					buffer[i++] = g * maxvalmult;
					g = file_buffer[file_pos++] << 8;
					g |= file_buffer[file_pos++];
					buffer[i++] = g * maxvalmult;
				}
			}
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
	cleanup(info);
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


boolean __CDECL encoder_init(const char *name, IMGINFO info)
{
	char header[128];
	char buf[32];
	int16_t handle;
	long len;

	if ((handle = Fcreate(name, 0)) < 0)
	{
		return FALSE;
	}
	info->_priv_var = handle;
	strcpy(header, "P6\n# written by zView NetPbm module\n");
	myitoa(info->width, buf);
	strcat(header, buf);
	strcat(header, " ");
	myitoa(info->height, buf);
	strcat(header, buf);
	strcat(header, "\n255\n");
	len = strlen(header);
	if (Fwrite(handle, len, header) != len)
	{
		cleanup(info);
		return FALSE;
	}

	info->planes = 24;
	info->components = 3;
	info->colors = 1L << 24;
	info->orientation = UP_TO_DOWN;
	info->memory_alloc = TT_RAM;
	info->indexed_color = FALSE;
	info->page = 1;
	
	return TRUE;
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	long linesize;
	int16_t handle = info->_priv_var;

	linesize = (uint32_t)info->width * 3;
	if (Fwrite(handle, linesize, buffer) != linesize)
	{
		cleanup(info);
		return FALSE;
	}
	return TRUE;
}


void __CDECL encoder_quit(IMGINFO info)
{
	cleanup(info);
}
