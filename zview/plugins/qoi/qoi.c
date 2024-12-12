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


#define QOI_IMPLEMENTATION
#define QOI_MALLOC(sz) (void *)Malloc(sz)
#define QOI_FREE(p) Mfree(p)
#define QOI_NO_STDIO
#include "qoi.h"

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
	qoi_desc desc;
	uint8_t *bmap;
	uint8_t *data;
	size_t file_size;
	int16_t handle;

	handle = (int16_t) Fopen(name, FO_READ);
	if (handle < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	data = QOI_MALLOC(file_size);
	if (data == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}

	if ((size_t)Fread(handle, file_size, data) != file_size)
	{
		QOI_FREE(data);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);
	bmap = qoi_decode(data, file_size, &desc, 0);
	QOI_FREE(data);

	info->planes = desc.channels == 4 ? 32 : 24;
	info->components = 3;
	info->width = desc.width;
	info->height = desc.height;
	info->colors = 1L << 24;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;
	strcpy(info->info, "Quite OK Image Format");
	strcpy(info->compression, "Defl");

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
	
	bmap += info->_priv_var;
	x = info->width;
	switch (info->planes)
	{
	case 24:
		info->_priv_var += x * 3;
		do
		{
			*buffer++ = *bmap++;
			*buffer++ = *bmap++;
			*buffer++ = *bmap++;
		} while (--x > 0);
		break;
	case 32:
		info->_priv_var += x * 4;
		do
		{
			*buffer++ = *bmap++;
			*buffer++ = *bmap++;
			*buffer++ = *bmap++;
			bmap++;
		} while (--x > 0);
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
	QOI_FREE(info->_priv_ptr);
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
	size_t image_size;
	uint8_t *data;

	image_size = (size_t)info->width * info->height * 3;
	data = QOI_MALLOC(image_size);
	if (data == NULL)
		RETURN_ERROR(EC_Malloc);
	handle = (int)Fcreate(name, 0);
	if (handle < 0)
	{
		QOI_FREE(data);
		RETURN_ERROR(EC_Fcreate);
	}
	
	info->planes = 24;
	info->components = 3;
	info->colors = 1L << 24;
	info->orientation = UP_TO_DOWN;
	info->memory_alloc = TT_RAM;
	info->indexed_color = FALSE;
	info->page = 1;

	info->_priv_ptr = data;
	info->_priv_var = 0;
	info->_priv_var_more = handle;

	RETURN_SUCCESS();
}


boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	size_t bytes_per_row;
	uint8_t *data;
	
	data = info->_priv_ptr;
	data += info->_priv_var;
	bytes_per_row = (size_t)info->width * 3;
	memcpy(data, buffer, bytes_per_row);
	info->_priv_var += bytes_per_row;
	/*
	 * write out image when all bytes have been copied.
	 * We do that here rather than in encoder_quit,
	 * because encoder_quit() does not return error codes
	 */
	if ((size_t)info->_priv_var == bytes_per_row * info->height)
	{
		int16_t handle = (int16_t)info->_priv_var_more;
		void *encoded;
		qoi_desc desc;
		size_t size, written;

		desc.width = info->width;
		desc.height = info->height;
		desc.channels = 3;
		desc.colorspace = 0;
		encoded = qoi_encode(info->_priv_ptr, &desc, &size);
		if (encoded == NULL)
			RETURN_ERROR(EC_Malloc);
		written = Fwrite(handle, size, encoded);
		QOI_FREE(encoded);
		if (written != size)
			RETURN_ERROR(EC_Fwrite);
	}

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
	QOI_FREE(ptr);
}
