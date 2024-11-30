#define	VERSION	     0x0201
#define NAME        "Picture Packer"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__
#define MISC_INFO   "Some code by Hans Wessels"

#include "plugin.h"
#include "zvplugin.h"
#define NF_DEBUG 0
#include "nfdebug.h"

/*
Picture Packer    *.PP1 (ST low resolution)
                  *.PP2 (ST medium resolution)
                  *.PP3 (ST high resolution)

This is a STOS packed screen without the STOS MBK header in front of the data.
Although the STOS screen format has a flag for the resolution this flag is set
to medium resolution for all *.PP? files. The only correct resolution info is
in the extension. A really brain died format.

See also STOS Memory Bank file format
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("PP1\0" "PP2\0" "PP3\0");

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
	case INFO_MISC:
		return (long)MISC_INFO;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif


typedef uint16_t coord_t;				/* coords are alway 16 bit but product of coords should be 32 bit */
typedef uint16_t stcolor_t;

#define SCREEN_SIZE 32000L
#define STOS_HD 0x06071963L

static unsigned long get_long(const uint8_t *p)
{
	unsigned long res;

	res = *p++;
	res <<= 8;
	res += *p++;
	res <<= 8;
	res += *p++;
	res <<= 8;
	res += *p;
	return res;
}


static uint16_t get_word(const uint8_t *p)
{
	uint16_t res;

	res = *p++;
	res <<= 8;
	res += *p;
	return res;
}


static void get_colors(const void *p, stcolor_t *colors, int cnt)
{
	int i;
	const uint8_t *q = p;

	for (i = 0; i < cnt; i++)
	{
		colors[i] = get_word(q + 2 * i);
	}
}



static uint8_t *stos_compressed(uint8_t *file, size_t size, uint16_t *colors)
{
	/* forceres: use this to force a different resolution, else: -1 */
	uint8_t *screen;
	unsigned int res;
	uint32_t literal;
	uint32_t pointer0;
	uint32_t pointer1;
	static uint16_t const bytes_per_line_tab[] = { 160, 160, 80 };
	static uint16_t const bytes_per_plane_tab[] = { 8, 4, 2 };
	static uint16_t const planes_tab[] = { 4, 2, 1 };
	static uint16_t const lines_tab[] = { 200, 200, 400 };
	coord_t line_size;
	coord_t plane_size;
	coord_t planes;
	coord_t y_size;
	coord_t x_res;
	coord_t blocks;
	coord_t block_size;
	coord_t block_line;
	uint8_t byte1;
	uint8_t cmd0;
	uint8_t cmd1;
	uint8_t mask0;
	uint8_t mask1;
	uint16_t plane_offset = 0;

	if (get_long(file) != STOS_HD)
	{
		nf_debugprintf("This is not valid STOS compressed screen!\n");
		return NULL;
	}
	screen = malloc(SCREEN_SIZE);
	if (screen == NULL)
	{
		nf_debugprintf("Out of memory!\n");
		return NULL;
	}
	memset(screen, 0, SCREEN_SIZE);	/* clear screen */
	res = get_word(file + 4);
	if (res > 2)
	{
		nf_debugprintf("This is not valid STOS compressed screen!\n");
		free(screen);
		return NULL;
	}
	line_size = bytes_per_line_tab[res];
	plane_size = bytes_per_plane_tab[res];
	planes = planes_tab[res];
	y_size = lines_tab[res];
	x_res = get_word(file + 10);
	if (x_res * plane_size > line_size || x_res == 0)
	{
		nf_debugprintf("This is not valid STOS compressed screen!\n");
		free(screen);
		return NULL;
	}
	blocks = get_word(file + 12);
	block_size = get_word(file + 16);
	if (blocks * block_size > y_size || blocks * block_size == 0)
	{
		nf_debugprintf("This is not valid STOS compressed screen!\n");
		free(screen);
		return NULL;
	}
	block_line = block_size * line_size;
	literal = 70;
	pointer0 = get_long(file + 20);
	pointer1 = get_long(file + 24);
	if (literal >= size || pointer0 >= size || pointer1 >= size)
	{
		nf_debugprintf("STOS decompression error!\n");
		free(screen);
		return NULL;
	}
	get_colors(file + 38, colors, 16);
	mask0 = 0x80;
	mask1 = 0x80;
	byte1 = file[literal++];
	cmd0 = file[pointer0++];
	cmd1 = file[pointer1++];
	if (mask1 & cmd1)
	{
		cmd0 = file[pointer0++];
	}
	mask1 >>= 1;
	do
	{
		coord_t block_line_offset = plane_offset;
		coord_t j = blocks;

		do
		{
			coord_t line_offset = block_line_offset;
			coord_t k = x_res;

			do
			{
				int m;

				for (m = 0; m < 2; m++)
				{
					coord_t line = line_offset + m;
					coord_t l = block_size;

					do
					{
						if (mask0 & cmd0)
						{
							byte1 = file[literal++];
						}
						if (line >= SCREEN_SIZE)
						{
							nf_debugprintf("STOS decompression error!\n");
							free(screen);
							return NULL;
						}
						screen[line] = byte1;
						line += line_size;
						mask0 >>= 1;
						if (mask0 == 0)
						{
							mask0 = 0x80;
							if (mask1 & cmd1)
							{
								cmd0 = file[pointer0++];
							}
							mask1 >>= 1;
							if (mask1 == 0)
							{
								mask1 = 0x80;
								cmd1 = file[pointer1++];
								if (literal > size || pointer0 > size || pointer1 > size)
								{
									nf_debugprintf("STOS decompression error!\n");
									free(screen);
									return NULL;
								}
							}
						}
					} while (--l);
				}
				line_offset += plane_size;
			} while (--k);
			block_line_offset += block_line;
		} while (--j);
		plane_offset += 2;
	} while (--planes);

#if 0
	if (forceres >= 0)
	{
		res = forceres;
	}
	if (res == 0)
	{
		h = convert_4bit(320, 200, screen, colors, NULL);
		sprintf(compressionText, "STOS compressed screen");
	} else if (res == 1)
	{
		h = convert_2bit(640, 200, screen, colors, NULL);
		sprintf(compressionText, "STOS compressed screen");
	} else if (res == 2)
	{
		h = convert_1bit(640, 400, screen, colors, NULL);
		sprintf(compressionText, "STOS compressed screen");
	} else
	{
		sprintf(errorText, "This is not valid STOS compressed screen!");
	}
	free(screen);
	screen = h;
#endif

	return screen;
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
	uint8_t *bmap;
	uint8_t *file;
	size_t file_size;
	int forceres;
	uint16_t palette[16];

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	forceres = name[strlen(name) - 1] - '1';

	switch (forceres)
	{
	case 0:
		info->planes = 4;
		info->width = 320;
		info->height = 200;
		break;
	case 1:
		info->planes = 2;
		info->width = 640;
		info->height = 200;
		break;
	case 2:
		info->planes = 1;
		info->width = 640;
		info->height = 400;
		break;
	default:
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	
	file = malloc(40000L);
	if (file == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	
	Fread(handle, file_size, file);
	Fclose(handle);

	if (get_long(file) != STOS_HD)
	{
		free(file);
		RETURN_ERROR(EC_FileId);
	}

	bmap = stos_compressed(file, file_size, palette);
	if (bmap == NULL)
	{
		free(file);
		RETURN_ERROR(EC_Malloc);
	}
	
	info->indexed_color = info->planes == 1 ? FALSE : TRUE;
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

	strcpy(info->info, "Picture Packer");
	strcpy(info->compression, "RLE");

	if (info->indexed_color)
	{
		int i;
		
		for (i = 0; i < (int)info->colors; i++)
		{
			info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((palette[i] << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 17;
		}
	}
	
	RETURN_SUCCESS();
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
	int x;
	
	switch (info->planes)
	{
	case 1:
		{
			int16_t byte;
			uint8_t *bmap = info->_priv_ptr;
			size_t src = info->_priv_var;

			x = info->width >> 3;
			info->_priv_var += x;
			do
			{
				byte = bmap[src++];
				*buffer++ = (byte >> 7) & 1;
				*buffer++ = (byte >> 6) & 1;
				*buffer++ = (byte >> 5) & 1;
				*buffer++ = (byte >> 4) & 1;
				*buffer++ = (byte >> 3) & 1;
				*buffer++ = (byte >> 2) & 1;
				*buffer++ = (byte >> 1) & 1;
				*buffer++ = (byte >> 0) & 1;
			} while (--x > 0);
		}
		break;

	case 2:
		{
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t *ptr;
			
			ptr = (uint16_t *)(info->_priv_ptr + info->_priv_var);
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
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t *ptr;
			
			ptr = (uint16_t *)(info->_priv_ptr + info->_priv_var);
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

	RETURN_SUCCESS();
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
