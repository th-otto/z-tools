#define	VERSION	     0x204
#define NAME        "Rembrandt True Color Picture"
#define AUTHOR      "Thorsten Otto"
#define DATE        __DATE__ " " __TIME__

#include "plugin.h"
#include "zvplugin.h"

/*
Rembrandt (True Color Picture)    *.TCP

The main header of the file:

8 bytes     file id, "TRUECOLR"
1 long      total length of file in bytes
1 word      length of this header in bytes [usually 18]
1 word      format version number [1]
1 word      number of images stored [1 = single image]
--------
18 bytes    total for main header

The following is repeated for each image in the file.

The header of each image:

1 long      image id, 'PICT'
1 long      total length of image data in bytes
1 word      length of this header in bytes [usually 198]
1 word      image width in pixels
1 word      image height in pixels
1 word      default color transparency
1 word      color of the default pen
1 byte      compression mode [0 = none]
1 byte      presence of a palette [0 = none]
1 byte      indicates over scan
1 byte      indicates double width
1 byte      indicates double height
175 bytes   comment text, each line null terminated? [5 * 35]
---------
198 bytes   total for image header

?           image data:
width*height*2 bytes Falcon high-color data,  word RRRRRGGGGGGBBBBB

Note: Compression flag appears to have never been implemented.
*/


#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("TCP\0");

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
	char magic[8];
	uint32_t file_length;
	uint16_t header_length;
	uint16_t version;
	uint16_t num_images;
};
struct frame_header {
	uint32_t magic;
	uint32_t image_size;
	uint16_t header_length;
	uint16_t width;
	uint16_t height;
	uint16_t tranparency;
	uint16_t default_pen;
	uint8_t compression;
	uint8_t palette;
	uint8_t overscan;
	uint8_t double_width;
	uint8_t double_height;
	char comment[5][35];
};

#ifdef __PUREC__
static unsigned long ulmul(unsigned short a, unsigned short b) 0xc0c1; /* mulu.w d1,d0 */
#else
static unsigned long ulmul(unsigned short a, unsigned short b)
{
	return (unsigned long)a * b;
}
#endif



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
	struct file_header header;
	struct frame_header frame_header;
	size_t image_size;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}

	if (memcmp(header.magic, "TRUECOLR", 8) != 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	if (Fread(handle, sizeof(frame_header), &frame_header) != sizeof(frame_header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (frame_header.magic != 0x50494354L) /* 'PICT' */
	{
		Fclose(handle);
		RETURN_ERROR(EC_ImageType);
	}
	if (frame_header.compression != 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_CompType);
	}
	if (frame_header.palette != 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_ColorMapType);
	}
	Fseek((size_t)header.header_length + frame_header.header_length, handle, SEEK_SET);
	image_size = ulmul(frame_header.width, frame_header.height) * 2;
	if (image_size != frame_header.image_size)
	{
		Fclose(handle);
		RETURN_ERROR(EC_BitmapLength);
	}
	
	bmap = malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, image_size, bmap) != image_size)
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	/*
	 * TODO: get comments from header?
	 */
	Fclose(handle);

	info->planes = 16;
	info->width = frame_header.width;
	info->height = frame_header.height;
	info->components = 3;
	info->indexed_color = FALSE;
	info->colors = 1L << 16;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "Rembrandt True Color Picture");
	strcpy(info->compression, "None");

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
	const uint16_t *screen16;
	uint16_t color;
	
	screen16 = (const uint16_t *)info->_priv_ptr + info->_priv_var;
	x = info->width;
	info->_priv_var += x;
	do
	{
		color = *screen16++;
		*buffer++ = ((color >> 11) & 0x1f) << 3;
		*buffer++ = ((color >> 5) & 0x3f) << 2;
		*buffer++ = ((color) & 0x1f) << 3;
	} while (--x > 0);
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
