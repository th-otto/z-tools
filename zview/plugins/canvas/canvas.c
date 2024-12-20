#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
Canvas            *.CNV, *.CPT, *.HBL, *.FUL

Canvas is a simple drawing program for the Atari ST.

The simplest file format it uses is .CNV. The format has no info on the
resolution of the image.

   48 bytes       palette, value 0-7 per byte, RGB in VDI order.
32000 bytes       image data

-------------------------------------------------------------------------------

The CPT format is a compressed format: (Compact Picture Format)
   16 words       palette (xbios format)
    1 word        resolution (0=low, 1=med, 2=high resolution)
  ??? words       compressed data, uncompressed 32000 bytes

Packing algorithm: first all run length data is transferred. Then the gaps that
haven't been filled with run length data is filled with the remaining data from
the file.
Runlength data:
    1 word        repeat count
    1 word        offset
    n words       run length data (n=4 low, 2 med, 1 high)
The n words run length data are repeated (repeat count + 1) times from offset
bytes from the start of the file. When repeat count $FFFF signifies the end of
the run length data (the offset and run length data still follow the $FFFF
repeat count).
  ??? words       data to fill the gaps that haven't been filled with run
                  length data.

-------------------------------------------------------------------------------

*.HBL files:

There's also a special HBL mode with extra colors (ST low and medium only).
Canvas saves the extra colors in a separate file with the same name, but with
an *.HBL file extension. Canvas can change the palette every 4 scan lines, thus
50 palettes can be displayed at once. STe palette is supported.

50 words     palette numbers, 1 per palette [0 to 63 or -1]
             -1 = no change at that scanline, scanline = word index * 4
150 words    unknown [low = all 0's, medium = all -1's]
400 bytes    unknown [all 0's]
48 bytes     default palette when HBLs are off? (RGB in VDI order)
for each flag that is <> -1 {
  48 bytes   palette (RGB in VDI order)
}

Notes: The palettes are stored in reserve. Example: Image uses 4 palettes.
palette ?  (palette when HBLs are off?) <-- offset 800
palette #4 (last palette)
palette #3
palette #2
palette #1 (first palette) <- end of file

The size of the file can be calculated using this method:
count = the number of palette flags that are set to -1
file_size = 800 + ((count + 1) * 48)

Some HBL files contain palette numbers that are clearly out of range. Canvas
manages 64 palettes and seems to dump these values to the HBL file. The palette
number should be ignored and the important value is -1. Canvas dumps all the
palettes in order, regardless of duplicates.

-------------------------------------------------------------------------------

Canvas v1.17 introduced a new file format referred to as FullPic Picture
Format. These use the file extension FUL. Manual states: This is simply the
Compact Picture Format, HBL file and Animate file all in one file.

? bytes    HBL data, identical to HBL file contents
608 bytes  SEQ data, always 608 bytes (skip)
? bytes    CPT data, identical to CTP file contents
---------
? bytes
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
	case INFO_MISC:
		return (long)MISC_INFO;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif


#define SCREEN_SIZE 32000
#define MAX_PALETTE 50

static uint16_t real_palettes[MAX_PALETTE * 4 * 16];
static uint8_t hbl_palettes[(MAX_PALETTE + 1) * 48 + 800];
static uint16_t original_planes;


static void my_strupr(char *str)
{
	while (*str != '\0')
	{
		if (*str >= 'a' && *str <= 'z')
			*str -= 'a' - 'A';
		str++;
	}
}


static int vdi2bios(int idx, int planes)
{
	int xbios;
	
	switch (idx)
	{
	case 1:
		xbios = (1 << planes) - 1;
		break;
	case 2:
		xbios = 1;
		break;
	case 3:
		xbios = 2;
		break;
	case 4:
		xbios = idx;
		break;
	case 5:
		xbios = 6;
		break;
	case 6:
		xbios = 3;
		break;
	case 7:
		xbios = 5;
		break;
	case 8:
		xbios = 7;
		break;
	case 9:
		xbios = 8;
		break;
	case 10:
		xbios = 9;
		break;
	case 11:
		xbios = 10;
		break;
	case 12:
		xbios = idx;
		break;
	case 13:
		xbios = 14;
		break;
	case 14:
		xbios = 11;
		break;
	case 15:
		xbios = 13;
		break;
	case 255:
		xbios = 15;
		break;
	case 0:
	default:
		xbios = idx;
		break;
	}
	return xbios;
}


static int read_cpt_file(uint16_t *buffer, uint16_t *screen, uint16_t *palette)
{
	char *done;
	unsigned int word;
	int i;
	int pln;
	int resolution;
	uint16_t data[4];
	int planes;
	
	word = 0;
	for (i = 0; i < 16; i++)
	{
		palette[i] = buffer[word];
		word++;
		if ((palette[i] & 0xf000) != 0)
			return -1;
	}

	resolution = buffer[word];
	word++;
	if (resolution < 0 || resolution > 2)
		return -1;
	planes = 4 >> resolution;

	done = malloc(SCREEN_SIZE / 2);
	if (done == NULL)
		return -1;
	memset(done, 0, SCREEN_SIZE / 2);
	
	for (;;)
	{
		int16_t repeat_count;
		uint16_t offset;
		uint16_t screen_offset;
		
		repeat_count = buffer[word++];
		offset = buffer[word++];
		for (i = 0; i < planes; i++)
		{
			data[i] = buffer[word++];
		}
		if (repeat_count == -1)
			break;
		screen_offset = offset * planes;
		if (screen_offset > SCREEN_SIZE / 2) /* should be >= */
		{
			free(done);
			return -1;
		}
		for (i = 0; i < repeat_count + 1; i++)
		{
			for (pln = 0; pln < planes; pln++)
			{
				screen[screen_offset] = data[pln];
				done[screen_offset] = TRUE;
				screen_offset++;
			}
		}
	}
	
	for (i = 0; i < SCREEN_SIZE / 2; i++)
	{
		if (done[i] == FALSE)
		{
			screen[i] = buffer[word];
			word++;
		}
	}
	free(done);
	return resolution;
}


static void read_hbl_palettes(int planes, uint8_t *palettes, uint16_t *real)
{
	unsigned int j;
	unsigned int i;
	unsigned char rgb[3];
	uint16_t colors[16];
	uint16_t palette[16];
	uint16_t idx;
	int num_palettes;
	long src_offset;
	int dst_offset;
	
	num_palettes = 0;
	j = 0;
	for (i = 0; i < MAX_PALETTE; i++)
	{
		idx = (palettes[j] << 8) | palettes[j + 1];
		j += 2;
		if (idx != (uint16_t)-1)
			num_palettes++;
	}
	src_offset = num_palettes * 48 + 800;
	j = 0;
	for (i = 0; i < MAX_PALETTE; i++)
	{
		idx = (palettes[j] << 8) | palettes[j + 1];
		j += 2;
		if (idx != (uint16_t)-1)
		{
			long offset;
			int color;
			int r;
			
			offset = src_offset;
			for (color = 0; color < 16; color++)
			{
				for (r = 0; r < 3; r++)
				{
					rgb[r] = palettes[offset];
					offset++;
				}
				colors[color] = (rgb[0] << 8) | (rgb[1] << 4) | rgb[2];
			}
			for (color = 0; color < 16; color++)
				palette[vdi2bios(color, planes)] = colors[color];
			src_offset -= 48;
		}
		dst_offset = i * 4 * 16;
		memcpy(&real[dst_offset], palette, 32);
		memcpy(&real[dst_offset + 16], palette, 32);
		memcpy(&real[dst_offset + 32], palette, 32);
		memcpy(&real[dst_offset + 48], palette, 32);
	}
}



static int read_palette(uint16_t planes, uint16_t *palette, const uint8_t *rgb)
{
	int i;
	int colors = 1 << planes;
	uint16_t color[16];
	
	for (i = 0; i < 16; i++)
	{
		color[i] = (rgb[0] << 8) | (rgb[1] << 4) | rgb[2];
		rgb += 3;
	}
	for (i = 0; i < colors; i++)
		palette[vdi2bios(i, planes)] = color[i];
	return 0;
}


/* code from atarist.c, app code St2BMP (placed into public domain) */
/* written by Hans Wessels */

/*
 * guess resolution from picturedata, 0=low, 1=med, 2=high
 *
 * routine guesses resolution by converting the bitplane pixels
 * to chunky pixels in a byte, just enough to fil a byte (8 
 * pixels for high, 4 for medium and 2 for lowres). And count
 * the number of runlength bytes in each mode. The correct
 * resolution is the one having the most runlength bytes.
 * If there is a draw, mono wins, then low res, medium res is
 * never chosen in a draw.
 */
static int guess_resolution(const uint8_t *buffer)
{
	const uint8_t *scan;
	int16_t reslow;
	int16_t resmed;
	int16_t resmono;
	
	reslow = 0;
	resmed = 0;
	resmono = 0;

	/* test high */
	{
		int16_t i;
		uint8_t last_byte;

		last_byte = 0;
		for (scan = buffer, i = 0; i < SCREEN_SIZE; i++)
		{
			uint8_t byte = *scan++;
			if (byte == last_byte)
				resmono++;
			else
				last_byte = byte;
		}
	}

	/* test_med */
	{
		int16_t i;
		int16_t k;
		uint8_t last_byte;
		uint16_t word;
		uint16_t word2;
		uint16_t mask;
		uint8_t byte;
		
		last_byte = 0;
		for (scan = buffer, i = 0; i < SCREEN_SIZE / 4; i++)
		{
			word = *scan++;
			mask = 0x8000;
			word <<= 8;
			word += *scan++;
			word2 = *scan++;
			word2 <<= 8;
			word2 += *scan++;
			do {
				byte = 0;
				for (k = 0; k < 4; k++)
				{
					byte += byte;
					if (word & mask)
						byte++;
					byte += byte;
					if (word2 & mask)
						byte++;
					mask >>= 1;
				}
				if (byte == last_byte)
					resmed++;
				else
					last_byte = byte;
			} while (mask != 0);
		}
	}
	
	{
		int16_t i;
		int16_t k;
		uint8_t last_byte;
		uint16_t word;
		uint16_t word2;
		uint16_t word3;
		uint16_t word4;
		uint16_t mask;
		uint8_t byte;
		
		last_byte = 0;
		for (scan = buffer, i = 0; i < SCREEN_SIZE / 8; i++)
		{
			word = *scan++;
			mask = 0x8000;
			word <<= 8;
			word += *scan++;
			word2 = *scan++;
			word2 <<= 8;
			word2 += *scan++;
			word3 = *scan++;
			word3 <<= 8;
			word3 += *scan++;
			word4 = *scan++;
			word4 <<= 8;
			word4 += *scan++;
			do {
				byte = 0;
				for (k = 0; k < 2; k++)
				{
					byte += byte;
					if (word & mask)
						byte++;
					byte += byte;
					if (word2 & mask)
						byte++;
					byte += byte;
					if (word3 & mask)
						byte++;
					byte += byte;
					if (word4 & mask)
						byte++;
					mask >>= 1;
				}
				if (byte == last_byte)
					reslow++;
				else
					last_byte = byte;
			} while (mask != 0);
		}
	}

	if (resmono >= reslow && resmono >= resmed)
		return 2;
	if (resmed > reslow)
		return 1;
	return 0;
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
	uint8_t *file_buffer;
	int16_t handle;
	size_t file_size;
	uint8_t *bmap;
	int is_hbl = FALSE;
	int resolution;
	char extension[4];
	uint16_t palette[16];
	int i;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	file_buffer = malloc(file_size);
	if (file_buffer == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, file_size, file_buffer) != file_size)
	{
		free(file_buffer);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	strcpy(extension, name + strlen(name) - 3);
	my_strupr(extension);

	if (strcmp(extension, "CNV") == 0 && file_size == SCREEN_SIZE + 48)
	{
		long kstate;
		
		bmap = file_buffer + 48;
		resolution = guess_resolution(bmap);
		kstate = Kbshift(-1);
		if (kstate & K_ALT)
			resolution = 0;
		else if (kstate & K_CTRL)
			resolution = 1;
		else if (kstate & K_LSHIFT)
			resolution = 2;
		info->planes = 4 >> resolution;
		if (read_palette(info->planes, palette, file_buffer) != 0)
		{
			free(file_buffer);
			return FALSE;
		}
		if (info->planes != 1)
		{
			for (i = 0; i < (1 << info->planes); i++)
			{
				info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 0x11;
				info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 0x11;
				info->palette[i].blue = ((((palette[i]) << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 0x11;
			}
		}
		memmove(file_buffer, bmap, SCREEN_SIZE);
		bmap = file_buffer;
		file_buffer = NULL;
		strcpy(info->compression, "None");
	} else if (strcmp(extension, "CPT") == 0)
	{
		bmap = malloc(SCREEN_SIZE);
		if (bmap == NULL)
		{
			free(file_buffer);
			return FALSE;
		}
		resolution = read_cpt_file((uint16_t *)file_buffer, (uint16_t *)bmap, palette);
		if (resolution < 0)
		{
			free(file_buffer);
			return FALSE;
		}
		info->planes = 4 >> resolution;
		if (info->planes > 1)
		{
			for (i = 0; i < (1 << info->planes); i++)
			{
				info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 0x11;
				info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 0x11;
				info->palette[i].blue = ((((palette[i]) << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 0x11;
			}
		}
		free(file_buffer);
		strcpy(info->compression, "RLE");
	} else if (strcmp(extension, "FUL") == 0)
	{
		unsigned int num_palettes;
		size_t size;
		uint16_t *palette_numbers;
		
		bmap = malloc(SCREEN_SIZE);
		if (bmap == NULL)
		{
			free(file_buffer);
			return FALSE;
		}
		palette_numbers = (uint16_t *)file_buffer;
		num_palettes = 1;
		for (i = 0; i < MAX_PALETTE; i++)
		{
			if (palette_numbers[i] != (uint16_t)-1)
				num_palettes++;
		}
		size = (size_t)num_palettes * 48 + 800;
		memcpy(hbl_palettes, file_buffer, size);
		resolution = read_cpt_file((uint16_t *)(file_buffer + size + 608), (uint16_t *)bmap, palette);
		if (resolution < 0)
		{
			free(file_buffer);
			return FALSE;
		}
		info->planes = 4 >> resolution;
		if (info->planes > 1)
		{
			for (i = 0; i < (1 << info->planes); i++)
			{
				info->palette[i].red = (((palette[i] >> 7) & 0x0e) + ((palette[i] >> 11) & 0x01)) * 0x11;
				info->palette[i].green = (((palette[i] >> 3) & 0x0e) + ((palette[i] >> 7) & 0x01)) * 0x11;
				info->palette[i].blue = ((((palette[i]) << 1) & 0x0e) + ((palette[i] >> 3) & 0x01)) * 0x11;
			}
			read_hbl_palettes(info->planes, hbl_palettes, real_palettes);
			is_hbl = TRUE;
		}
		free(file_buffer);
	} else
	{
		free(file_buffer);
		return FALSE;
	}
	
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
	original_planes = info->planes;

	info->components = info->planes == 1 ? 1 : 3;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	strcpy(info->info, "Canvas");

	info->_priv_ptr = bmap;
	info->_priv_var = 0;				/* y offset */
	info->_priv_var_more = 0;			/* line number */

	if (info->planes > 1)
	{	
		char hbl_name[256];

		strcpy(hbl_name, name);
		strcpy(hbl_name + strlen(hbl_name) - 3, "hbl");
		handle = Fopen(hbl_name, FO_READ);
		if (handle >= 0)
		{
			/* if (!(Kbshift(-1) & K_ALT)) */
			{
				file_size = Fseek(0, handle, SEEK_END);
				Fseek(0, handle, SEEK_SET);
				if (file_size <= sizeof(hbl_palettes))
				{
					if ((size_t)Fread(handle, file_size, hbl_palettes) != file_size)
					{
						free(bmap);
						return FALSE;
					}
					read_hbl_palettes(info->planes, hbl_palettes, real_palettes);
					is_hbl = TRUE;
				}
			}
			Fclose(handle);
		}
	}
	
	if (is_hbl)
	{
		info->planes = 12;
		info->colors = 1L << info->planes;
		info->indexed_color = FALSE;
		strcat(info->info, " (HBL)");
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
	uint32_t pos;

	pos = info->_priv_var;

	switch (info->planes)
	{
	case 1:
		{
			uint8_t b;
			size_t x;
			uint8_t *bmap;

			bmap = info->_priv_ptr;
			x = 0;
			do
			{
				b = bmap[pos++];
				buffer[x++] = (b >> 7) & 1;
				buffer[x++] = (b >> 6) & 1;
				buffer[x++] = (b >> 5) & 1;
				buffer[x++] = (b >> 4) & 1;
				buffer[x++] = (b >> 3) & 1;
				buffer[x++] = (b >> 2) & 1;
				buffer[x++] = (b >> 1) & 1;
				buffer[x++] = (b >> 0) & 1;
			} while (x < info->width);
		}
		break;

	case 2:
	case 4:
		{
			int16_t x;
			int16_t i;
			uint16_t byte;
			int16_t plane;
			uint16_t *bmap;
			
			bmap = (uint16_t *)info->_priv_ptr;
			x = 0;
			do
			{
				for (i = 15; i >= 0; i--)
				{
					for (plane = byte = 0; plane < info->planes; plane++)
					{
						if ((bmap[pos + plane] >> i) & 1)
							byte |= 1 << plane;
					}
					buffer[x] = byte;
					x++;
				}
				pos += info->planes;
			} while (x < info->width);
		}
		break;
	
	case 12:
		{
			int16_t x;
			uint16_t w;
			int16_t i;
			int16_t byte;
			int16_t plane;
			uint16_t color;
			uint16_t *bmap;
			
			bmap = (uint16_t *)info->_priv_ptr;
			x = 0;
			w = 0;
			do
			{
				for (i = 15; i >= 0; i--)
				{
					for (plane = byte = 0; plane < original_planes; plane++)
					{
						if ((bmap[pos + plane] >> i) & 1)
							byte |= 1 << plane;
					}
					color = real_palettes[info->_priv_var_more * 16 + byte];

					buffer[w++] = (((color >> 7) & 0x0e) + ((color >> 11) & 0x01)) * 0x11;
					buffer[w++] = (((color >> 3) & 0x0e) + ((color >> 7) & 0x01)) * 0x11;
					buffer[w++] = (((color << 1) & 0x0e) + ((color >> 3) & 0x01)) * 0x11;
					x++;
				}
				pos += original_planes;
			} while (x < info->width);
			info->_priv_var_more++;
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
