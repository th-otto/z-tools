#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"
#define NF_DEBUG 0
#include "nfdebug.h"

/*
DuneGraph         *.DG1, *.DC1

The *.DG1 is a simple uncompressed format:
    3 bytes       'DGU' file ID ($444755)
    1 byte        version number (1)
    1 word        xres (320)
    1 word        yres (200)
  256 long        palette, Falcon format, RG0B
64000 bytes       picture data, 320x200, 8 bitplanes
                  (other resolutions have not been seen)

_______________________________________________________________________________

The *.DC1 is a simple compressed format:
    3 bytes       'DGC' file ID  ($444743)
    1 byte         packing method (0, 1, 2 or 3)
    1 word        xres
    1 word        yres
    1 word        unknown ($00ff)
  256 long        palette, Falcon format, RG0B
  ??? bytes       compressed picture data
Compressed picture data:
Method 0: no compression, 64000 bytes, 320x200, 8 bitplanes
For method 1 to 3 the bit planes are unmixed, so first you get all data for bit
plane 0, then bit plane one etc.
Method 1: simple runlength on byte level
     1 long       size of compressed data, including this long
     do
       1 byte       repeat count
       1 byte       data
       write (repeat count+1) times data
     until you run out of compressed data
Method 2: simple runlength on word level
     1 long       size of compressed data, including this long
     do
       1 word       repeat count
       1 word       data
       write (repeat count+1) times data
     until you run out of compressed data
Method 3: simple runlength on long level
     1 long       size of compressed data, including this long
     do
       1 word       repeat count
       1 long       data
       write (repeat count+1) times data
     until you run out of compressed data
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

struct file_header {
	uint8_t magic[3];
	uint8_t method;
	uint16_t xres;
	uint16_t yres;
};


static void decompress_bytes(uint32_t datasize, uint32_t compressed_size, const uint8_t *src, uint8_t *dst)
{
	size_t count;
	uint8_t byte;
	uint8_t *dst8 = dst;
	
	while (compressed_size >= 2 && datasize != 0)
	{
		count = *src++;
		byte = *src++;
		compressed_size -= 2;
		count++;
		if (count > datasize)
			count = datasize;
		datasize -= count;
		while (count != 0)
		{
			*dst8++ = byte;
			count--;
		}
	}
}


static void decompress_words(uint32_t datasize, uint32_t compressed_size, const void *src, uint8_t *dst)
{
	size_t count;
	uint16_t val;
	const struct {
		uint16_t count;
		uint16_t val;
	} *data = src;
	uint16_t *dst16 = (uint16_t *)dst;
	
	datasize >>= 1;
	while (compressed_size >= sizeof(*data) && datasize != 0)
	{
		count = data->count;
		val = data->val;
		data++;
		compressed_size -= sizeof(*data);
		count++;
		if (count > datasize)
			count = datasize;
		datasize -= count;
		while (count != 0)
		{
			*dst16++ = val;
			count--;
		}
	}
}


static void decompress_longs(uint32_t datasize, uint32_t compressed_size, const void *src, uint8_t *dst)
{
	size_t count;
	uint32_t val;
	const struct {
		uint16_t count;
		uint32_t val;
	} *data = src;
	uint32_t *dst32 = (uint32_t *)dst;
	
	datasize >>= 2;
	while (compressed_size >= sizeof(*data) && datasize != 0)
	{
		count = data->count;
		val = data->val;
		data++;
		compressed_size -= sizeof(*data);
		count++;
		if (count > datasize)
			count = datasize;
		datasize -= count;
		while (count != 0)
		{
			*dst32++ = val;
			count--;
		}
	}
}


__attribute__((noinline))
static void tangle_bitplanes(void *dst, const void *src, size_t datasize)
{
	size_t j;
	size_t planesize;
	uint16_t *dst16 = dst;
	const uint16_t *src16 = src;
	
	planesize = datasize >> 4;
	j = planesize;
	while (j != 0)
	{
		*dst16++ = src16[planesize * 0];
		*dst16++ = src16[planesize * 1];
		*dst16++ = src16[planesize * 2];
		*dst16++ = src16[planesize * 3];
		*dst16++ = src16[planesize * 4];
		*dst16++ = src16[planesize * 5];
		*dst16++ = src16[planesize * 6];
		*dst16++ = src16[planesize * 7];
		j--;
		src16++;
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
	int16_t handle;
	uint8_t *bmap;
	uint8_t paldata[256 * 4];
	struct file_header file_header;
	size_t datasize;
	uint8_t method;
	int i, j;
	uint32_t compressed_size;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	if (Fread(handle, sizeof(file_header), &file_header) != sizeof(file_header))
	{
		nf_debugprintf(("read error 1\n"));
		Fclose(handle);
		return FALSE;
	}

	nf_debugprintf(("magic: %c%c%c method %d, %ux%u\n",
		file_header.magic[0], file_header.magic[1], file_header.magic[2],
		file_header.method,
		file_header.xres, file_header.yres));

	if (file_header.magic[0] == 'D' &&
		file_header.magic[1] == 'G' &&
		file_header.magic[2] == 'U' &&
		file_header.method == 1)
	{
		method = 0;
	} else if (file_header.magic[0] == 'D' &&
		file_header.magic[1] == 'G' &&
		file_header.magic[2] == 'C')
	{
		method = file_header.method;
		Fseek(2, handle, SEEK_CUR);
	} else
	{
		nf_debugprintf(("unsupported format\n"));
		Fclose(handle);
		return FALSE;
	}
	Fread(handle, sizeof(paldata), paldata);
	j = 0;
	for (i = 0; i < 256; i++)
	{
		info->palette[i].red = paldata[j++];
		info->palette[i].green = paldata[j++];
		j++;
		info->palette[i].blue = paldata[j++];
	}
	datasize = (size_t)((file_header.xres + 15) & -16) * file_header.yres;
	
	bmap = malloc(datasize);
	if (bmap == NULL)
	{
		nf_debugprintf(("memory exhausted\n"));
		Fclose(handle);
		return FALSE;
	}
	if (method == 0)
	{
		if ((size_t)Fread(handle, datasize, bmap) != datasize)
		{
			nf_debugprintf(("read error 2\n"));
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		strcpy(info->compression, "None");
	} else
	{
		uint8_t *temp;

		if (Fread(handle, sizeof(compressed_size), &compressed_size) != sizeof(compressed_size) ||
			compressed_size < 4)
		{
			nf_debugprintf(("read error 4\n"));
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		
		compressed_size -= 4;
		if (compressed_size > datasize)
		{
			nf_debugprintf(("compressed size mismatch: %lu > %lu\n", (unsigned long)compressed_size, (unsigned long)datasize));
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		
		temp = malloc(datasize);
		if (temp == NULL)
		{
			nf_debugprintf(("memory exhausted\n"));
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		memset(temp, 0, datasize);
		if ((size_t)Fread(handle, compressed_size, bmap) != compressed_size)
		{
			nf_debugprintf(("read error 3\n"));
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		switch (method)
		{
		case 1:
			decompress_bytes(datasize, compressed_size, bmap, temp);
			strcpy(info->compression, "RLE1");
			break;
		case 2:
			decompress_words(datasize, compressed_size, bmap, temp);
			strcpy(info->compression, "RLE2");
			break;
		case 3:
			decompress_longs(datasize, compressed_size, bmap, temp);
			strcpy(info->compression, "RLE4");
			break;
		default:
			nf_debugprintf(("unssupported compression\n"));
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		tangle_bitplanes(bmap, temp, datasize);
		free(temp);
	}
	Fclose(handle);
	
	info->width = file_header.xres;
	info->height = file_header.yres;
	info->planes = 8;
	info->colors = 1L << 8;
	info->components = 3;
	info->indexed_color = TRUE;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "DuneGraph");
	
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
	int16_t x;
	int16_t i;
	uint16_t byte;
	int16_t plane;
	const uint16_t *bmap16;
	
	x = 0;
	bmap16 = (const uint16_t *)info->_priv_ptr;
	do
	{
		for (i = 15; i >= 0; i--)
		{
			for (plane = byte = 0; plane < 8; plane++)
			{
				if ((bmap16[pos + plane] >> i) & 1)
					byte |= 1 << plane;
			}
			buffer[x] = byte;
			x++;
		}
		pos += 8;
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
