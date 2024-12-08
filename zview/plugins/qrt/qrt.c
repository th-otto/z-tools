#include "plugin.h"
#include "zvplugin.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
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
	uint16_t width;
	uint16_t height;
};

static uint16_t swap16(uint16_t w)
{
	return (w >> 8) | (w << 8);
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
	uint8_t *bmap;
	size_t file_size;
	size_t image_size;
	int16_t handle;
	size_t bytes_per_row;
	struct file_header header;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	file_size -= sizeof(header);
	header.width = swap16(header.width);
	header.height = swap16(header.height);
	bytes_per_row = (size_t)header.width * 3 + 2;
	image_size = bytes_per_row * header.height;
	if (file_size < image_size)
	{
		Fclose(handle);
		RETURN_ERROR(EC_BitmapLength);
	}

	bmap = (uint8_t *)Malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, image_size, bmap) != image_size)
	{
		Mfree(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	
	Fclose(handle);

	info->planes = 24;
	info->components = 3;
	info->width = header.width;
	info->height = header.height;
	info->colors = 1L << 24;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	strcpy(info->info, "Quick Ray Tracer");
	strcpy(info->compression, "None");

	info->_priv_ptr = bmap;
	info->_priv_var = 0;				/* y offset */

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
	uint8_t *bmap = (uint8_t *)info->_priv_ptr;
	int x;
	size_t offset;
	
	bmap += info->_priv_var;
	x = info->width;
	info->_priv_var += x * 3 + 2;
	bmap += 2; /* skip lineno (could maybe check that here) */
	offset = x;
	do
	{
		*buffer++ = bmap[0];
		*buffer++ = bmap[offset];
		*buffer++ = bmap[offset * 2];
		bmap++;
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
	Mfree(info->_priv_ptr);
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


boolean __CDECL encoder_init(const char *name, IMGINFO info)
{
	int handle;
	size_t bytes_per_row;
	uint8_t *data;
	struct file_header header;

	bytes_per_row = (size_t)info->width * 3 + 2;
	data = (uint8_t *)Malloc(bytes_per_row);
	if (data == NULL)
		RETURN_ERROR(EC_Malloc);
	handle = Fcreate(name, 0);
	if (handle < 0)
	{
		Mfree(data);
		RETURN_ERROR(EC_Fcreate);
	}
	header.width = swap16(info->width);
	header.height = swap16(info->height);
	if (Fwrite(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		Mfree(data);
		RETURN_ERROR(EC_Fwrite);
	}
	
	info->planes = 24;
	info->components = 3;
	info->colors = 1L << 24;
	info->orientation = UP_TO_DOWN;
	info->memory_alloc = TT_RAM;
	info->indexed_color = FALSE;
	info->page = 1;

	strcpy(info->info, "Quick Ray Tracer");
	strcpy(info->compression, "None");

	info->_priv_ptr = data;
	info->_priv_var = 0;
	info->_priv_var_more = handle;

	RETURN_SUCCESS();
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	size_t bytes_per_row;
	uint8_t *data;
	uint16_t *lineno;
	int x;
	uint16_t y;
	size_t offset;
	int16_t handle;
	
	data = info->_priv_ptr;
	lineno = (uint16_t *)data;
	y = info->_priv_var++;
	*lineno = swap16(y);
	data += 2;
	x = info->width;
	offset = x;
	bytes_per_row = offset * 3 + 2;
	do
	{
		data[0] = *buffer++;
		data[offset] = *buffer++;
		data[offset * 2] = *buffer++;
	} while (--x > 0);
	handle = (int16_t)info->_priv_var_more;
	if ((size_t)Fwrite(handle, bytes_per_row, info->_priv_ptr) != bytes_per_row)
		RETURN_ERROR(EC_Fwrite);

	RETURN_SUCCESS();
}


void __CDECL encoder_quit(IMGINFO info)
{
	int16_t handle;
	void *ptr;
	
	handle = (int16_t)info->_priv_var_more;
	if (handle > 0)
	{
		info->_priv_var_more = 0;
		Fclose(handle);
	}
	ptr = info->_priv_ptr;
	info->_priv_ptr = NULL;
	Mfree(ptr);
}
