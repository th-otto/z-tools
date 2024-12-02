#include "plugin.h"
#include "zvplugin.h"

/*
GodPaint     *.GOD

1 word       file ID, 'G4' and $0400 have been seen. GodPaint ignores the ID.
1 word       xres
1 word       yres
xres*yres*2 bytes Falcon high color data,  word RRRRRGGGGGGBBBBB
*/

#define VERSION 0x202
#define NAME    "GodPaint"
#define AUTHOR  "Zorro"
#define DATE     __DATE__ " " __TIME__

typedef struct
{
	int16_t header;
	int16_t width;
	int16_t height;
} GODHDR;



#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
	case OPTION_EXTENSIONS:
		return (long)("GOD\0");

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
boolean __CDECL reader_init( const char *name, IMGINFO info)
{
	int16_t 		handle;
	int32_t		file_size;
	GODHDR		*god;

	if ( ( handle = ( int16_t)Fopen( name, 0)) < 0)
		RETURN_ERROR(EC_Fopen);

	file_size = Fseek( 0L, handle, 2);

	Fseek( 0L, handle, 0);

	god = ( GODHDR *)malloc( file_size);

	if ( god == NULL)
	{
		Fclose( handle);
		RETURN_ERROR(EC_Malloc);
	}

	if ( Fread( handle, file_size, god) != file_size)
	{
		free( god);
		Fclose( handle);
		RETURN_ERROR(EC_Fread);
	}

	Fclose( handle);

	if( ( god->header != 1024) /* some beta version of godpaint? I don't know but some files have this header */
	&& ( god->header != 0x4734   /* G4 */))
	{
		free( god);
		RETURN_ERROR(EC_FileId);
	}

	info->planes 					= 16;
	info->real_width  = info->width = god->width;
	info->real_height = info->height= god->height;
	info->components				= 3;
	info->indexed_color				= FALSE;
	info->colors  					= 1L << info->planes;
	info->memory_alloc 				= TT_RAM;
	info->page	 					= 1;
	info->orientation				= UP_TO_DOWN;
	info->num_comments				= 0;
	info->max_comments_length 		= 0;
	info->_priv_ptr					= ( void*)god;
	info->_priv_var					= sizeof(GODHDR) / sizeof(uint16_t);

	strcpy( info->info, "Reservoir Gods - Godpaint");
	strcpy( info->compression, "None");

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
boolean __CDECL reader_read( IMGINFO info, uint8_t *buffer)
{
	GODHDR		*god	= ( GODHDR*)info->_priv_ptr;
	uint16_t 		*source = ( uint16_t*)god + info->_priv_var;
	int x;

	x = info->width;
	info->_priv_var += x;
	do
	{
		uint16_t src16 = *source++;

		*buffer++ = (( src16 >> 11) & 0x001F) << 3;
        *buffer++ = (( src16 >> 5)  & 0x003F) << 2;
        *buffer++ = (( src16)       & 0x001F) << 3;
	} while (--x > 0);

	RETURN_SUCCESS();
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
void __CDECL reader_get_txt( IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
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
void __CDECL reader_quit( IMGINFO info)
{
	GODHDR		*god	= ( GODHDR*)info->_priv_ptr;

	info->_priv_ptr = 0;
	free( god);
}


/*==================================================================================*
 * boolean __CDECL encoder_init:													*
 *		Open the file "name", fit the "info" struct. ( see zview.h) and make others	*
 *		things needed by the encoder.												*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		name		->	The file to open.											*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL encoder_init( const char *name, IMGINFO info)
{
	uint16_t 		*line_buffer = NULL;
	int 		file;
	GODHDR header;

	line_buffer	= ( uint16_t*) malloc( ( info->width + 1) << 1);

	if ( line_buffer == NULL)
	{
		RETURN_ERROR(EC_Malloc);
	}

	if ( ( file = (int)Fcreate( name, 0)) < 0)
	{
		free(line_buffer);
		RETURN_ERROR(EC_Fcreate);
	}

	header.header = 0x4734;
	header.width = info->width;
	header.height = info->height;

	if( Fwrite( file, sizeof(header), &header) != sizeof(header))
	{
		free( line_buffer);
		Fclose( file);
		RETURN_ERROR(EC_Fwrite);
	}

	info->planes   			= 24;	/* the wanted input data format.. currently Zview return only 24 bits data, so we make the convertion to 16 bits ourself */
	info->components 		= 3;
	info->colors  			= 1L << 24;
	info->orientation		= UP_TO_DOWN;
	info->indexed_color	 	= FALSE;
	info->page			 	= 1;
	info->_priv_ptr	 		= line_buffer;
	info->_priv_var	 		= file;

	RETURN_SUCCESS();
}


/*==================================================================================*
 * boolean __CDECL encoder_write:													*
 *		This function write data from buffer to file								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		buffer		->	The source buffer.											*
 *		info		->	The IMGINFO struct		.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL encoder_write( IMGINFO info, uint8_t *buffer)
{
	uint16_t *line_buffer = ( uint16_t*)info->_priv_ptr;
	size_t byte_to_write = info->width << 1;
	uint16_t i;

	for( i = 0; i < info->width; i++)
	{
		*line_buffer++ = (((uint16_t)buffer[0] & 0xF8) << 8) | (((uint16_t)buffer[1] & 0xFC) << 3) | ( buffer[2] >> 3);
		buffer += 3;
	}

	if((size_t)Fwrite( (int)info->_priv_var, byte_to_write, info->_priv_ptr) != byte_to_write)
		return FALSE;

	return TRUE;
}



/*==================================================================================*
 * boolean __CDECL encoder_quit:													*
 *		This function makes the last job like close the file handler and free the	*
 *		allocated memory.															*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL encoder_quit( IMGINFO info)
{
	uint8_t *buffer;

	if (info->_priv_var > 0)
	{
		Fclose( (int)info->_priv_var);
		info->_priv_var = 0;
	}
	buffer = ( uint8_t*)info->_priv_ptr;
	info->_priv_ptr = 0;
	free( buffer);
}
