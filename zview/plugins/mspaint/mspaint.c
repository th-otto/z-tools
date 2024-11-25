#define	VERSION	    0x208
#define NAME        "Microsoft Paint"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*

In the Microsoft Paint header, Key1 and Key2 contain identification
values used to determine the version of the file format. For version
1.x of the Microsoft Paint format, the values of the Key1 and Key2
fields are 6144h and 4D6Eh respectively. For version 2.0, the Key1 and
Key2 field values are 694Ch and 536Eh respectively.

Width and Height are the size of the bitmap in pixels. The size of the
bitmap in bytes is calculated by dividing Width by 8 and multiplying it
by Height.

XARBitmap and YARBitmap contain the aspect ratio in pixels of the
screen used to create the bitmapped image.

XARPrinter and YARPrinter contain the aspect ratio in pixels of the
output device used to render the bitmapped image. When an MSP file is
created by a non-Windows application, these four fields typically
contains the same values as the Width and Height fields.

PrinterWidth and PrinterHeight contain the size in pixels of the output
device for which the image is specifically formatted. Typical values
for these fields are the same values as those stored in Width and
Height.

XAspectCorr and YAspectCorr are used to store aspect ratio correction
information, but are not used in version 2.0 or earlier versions of the
Microsoft Paint format and should be set to 0.

Checksum contains the XORed values of the first 12 WORDs of the header.
When an MSP file is read, the first 13 WORDs, including the Checksum
field, are XORed together, and if the resulting value is 0, the header
information is considered valid.

Padding extends the header out to a full 32 bytes in length and is
reserved for future use.

The image data directly follows the header. The format of this image
data depends upon the version of the Microsoft Paint file. For image
files prior to version 2.0, the image data immediately follows the
header. There are eight pixels stored per byte, and the data is not
encoded.

Each scan line in a version 2.0 or later Microsoft Paint bitmap is
always RLE-encoded to reduce the size of the data. Each encoded scan
line varies in size depending upon the bit patterns it contains. To aid
in the decoding process, a scan-line map immediately follows the
header. The scan-line map is used to seek to a specific scan line in
the encoded image data without needing to decode all image data prior
to it. There is one element in the map per scan line in the image. Each
element in the scan-line map is 16 bits in size and contains the number
of bytes used to encode the scan line it represents. The scan-line map
starts at offset 32 in the MSP file and is sizeof(WORD).

Consider the following example. If an application needs to seek
directly to the start of scan-line 20, it adds together the first 20
values in the scan-line map. This sum is the offset from the beginning
of the image data of the 20th encoded scan line. The scan-line map
values can also be used to double-check that the decoding process read
the proper number of bytes for each scan line.

Following the scan-line map is the run-length encoded monochrome
bitmapped data. A byte-wise run-length encoding scheme is used to
compress the monochrome bitmapped data contained in an MSP-format image
file. Each scan line is encoded as a series of packets containing runs
of identical byte values. If there are very few runs of identical byte
values, or if all the runs are very small, then a way to encode a
literal run of different byte values may be used.

The following pseudocode illustrates the decoding process: 

Read a BYTE value as the RunType
	If the RunType value is zero
		Read next byte as the RunCount
		Read the next byte as the RunValue
		Write the RunValue byte RunCount times
	If the RunType value is non-zero
		Use this value as the RunCount
		Read and write the next RunCount bytes literally
*/

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("MSP\0");

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

struct file_header {
	uint32_t magic;			/* Magic number */
	uint16_t width;			/* Width of the bitmap in pixels */
	uint16_t height;		/* Height of the bitmap in pixels */
	uint16_t XARBitmap; 	/* X Aspect ratio of the bitmap */
	uint16_t YARBitmap; 	/* Y Aspect ratio of the bitmap */
	uint16_t XARPrinter;	/* X Aspect ratio of the printer */
	uint16_t YARPrinter;	/* Y Aspect ratio of the printer */
	uint16_t PrinterWidth;	/* Width of the printer in pixels */
	uint16_t PrinterHeight;	/* Height of the printer in pixels */
	uint16_t XAspectCorr;	/* X aspect correction (unused) */
	uint16_t YAspectCorr;	/* Y aspect correction (unused) */
	uint16_t checksum;		/* Checksum of previous 24 bytes */
	uint16_t padding[3];    /* Unused padding */
};


static uint16_t swap16(uint16_t w)
{
	return (w >> 8) | (w << 8);
}


static void decompress_msp(uint8_t *src, uint8_t *dst, size_t dstlen)
{
	int16_t n;
	int16_t c;
	uint8_t *end = dst + dstlen;

	do
	{
		n = *src++;
		if (n != 0)
		{
			while (--n >= 0)
				*dst++ = *src++;
		} else
		{
			n = *src++;
			c = *src++;
			while (--n >= 0)
				*dst++ = c;
		}
	} while (dst < end);
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
	size_t image_size;
	size_t bytes_per_row;
	int16_t handle;
	uint8_t *bmap;
	struct file_header header;
	
	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (file_size <= sizeof(header) ||
		Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		return FALSE;
	}

	{
		uint16_t valid;
		uint16_t *ptr = (uint16_t *)&header;
		int i;
		
		valid = 0;
		for (i = 0; i < 13; i++)
			valid ^= *ptr++;
		if (valid != 0)
		{
			Fclose(handle);
			return FALSE;
		}
	}
	
	header.width = swap16(header.width);
	header.height = swap16(header.height);
	bytes_per_row = ((size_t)header.width + 7) / 8;
	image_size = bytes_per_row * header.height;
	
	bmap = malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	strcpy(info->info, "Microsoft Paint");

	if (header.magic == 0x44616e4DL) /* 'DanM' */
	{
		if ((size_t)Fread(handle, image_size, bmap) != image_size)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		strcpy(info->compression, "None");
		strcat(info->info, " (v1)");
	} else if (header.magic == 0x4c696e53L) /* 'LinS' */
	{
		uint8_t *temp;
		size_t scanmap_size;

		scanmap_size = (size_t)header.height * 2;
		if (file_size <= scanmap_size + sizeof(header))
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		file_size -= scanmap_size + sizeof(header);
		temp = malloc(file_size);
		if (temp == NULL)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		/* skip scanline map */
		Fseek(scanmap_size, handle, SEEK_CUR);
		if ((size_t)Fread(handle, file_size, temp) != file_size)
		{
			free(temp);
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		decompress_msp(temp, bmap, image_size);
		free(temp);
		strcpy(info->compression, "RLE");
		strcat(info->info, " (v2)");
	} else
	{
		free(bmap);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);
	
	{
		size_t i;

		/* Maybe better to move this to reader? */		
		for (i = 0; i < image_size; i++)
			bmap[i] = ~bmap[i];
	}
	
	info->planes = 1;
	info->colors = 1L << 1;
	info->width = header.width;
	info->height = header.height;
	info->indexed_color = FALSE;
	info->components = 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

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
	uint8_t *bmap = info->_priv_ptr;
	int16_t byte;
	int x;
	
	x = (info->width + 7) >> 3;
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
