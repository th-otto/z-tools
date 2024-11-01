#include "general.h"
#include "pic_load.h"
#include "prefs.h"
#include "ztext.h"
#include "zvdi/color.h"
#include "zvdi/raster.h"
#include "zvdi/vdi.h"
#include "plugins.h"
#include "progress.h"
#include "pic_save.h"
#include "plugins/common/zvplugin.h"



static int16 setup_encoder ( IMGINFO in_info, IMGINFO out_info, DECDATA data)
{
	size_t	src_line_size, dst_line_size;
		
	data->IncXfx    = 0x10000uL;

	src_line_size   = ( in_info->width  + 15) * in_info->components;
	dst_line_size   = ( out_info->width + 15) * out_info->components;
	
	data->RowBuf = ( uint8*)malloc( src_line_size);
		
	if( data->RowBuf == NULL)
		return( 0);

	data->DthBuf = NULL;

	/* If the image is inverted like in iNTEL format, we must to decompress
	   the entire picture before to save it */
	if( in_info->orientation == DOWN_TO_UP)
	{	
		data->DstBuf = ( uint8*)malloc( dst_line_size * ( out_info->height + 1));
	}
	else 
		data->DstBuf = ( uint8*)malloc( dst_line_size);	

	if( data->DstBuf == NULL)
		return( 0);

	data->DthWidth 	= in_info->width;
	data->PixMask  	= ( 1 << in_info->planes) - 1;
	data->LnSize   	= out_info->width * out_info->components;

	switch( out_info->planes)
	{
		case 24:
			if( in_info->planes == 1 && in_info->components == 1)			
			{	
				data->Pixel[0] = 0xFFFFFF;
				data->Pixel[1] = 0x000000;
				raster = raster_24;
			}			
			else if( in_info->indexed_color)
			{	
				cnvpal_true( in_info, data);
				raster = raster_24;
			}
			else
				raster = ( in_info->components >= 3 ? dither_24 : gscale_24);

			break;

		default:
			return( 0);
	}

	return( 1);
}


/*==================================================================================*
 * read_img:																		*
 *				Read a image and fit the MFDB within IMAGE structur with the data.	*
 *----------------------------------------------------------------------------------*
 * img: 		The struct. with the MFDB to be fitted.								*
 * info:		The struct. with the line and dither buffer							*
 *----------------------------------------------------------------------------------*
 * returns: 	-																	*
 *==================================================================================*/
static boolean write_img( IMGINFO in_info, IMGINFO out_info, DECDATA data)
{
	int16  	y, img_h = in_info->height;
	uint8  	*dst	 = data->DstBuf;
	boolean ret;

	in_info->page_wanted = 0;

	ret = TRUE;
	if( in_info->orientation == UP_TO_DOWN)
	{	
		for( y = 0; y < img_h; y++)
		{
			switch (curr_input_plugin->type)
			{
			case CODEC_SLB:
				plugin_reader_read(&curr_input_plugin->c.slb, in_info, data->RowBuf);
				break;
			case CODEC_LDG:
				curr_input_plugin->c.ldg.funcs->decoder_read(in_info, data->RowBuf);
				break;
			}
			( *raster)( data, dst);
	
			switch (curr_output_plugin->type)
			{
			case CODEC_SLB:
				ret &= plugin_encoder_write(&curr_output_plugin->c.slb, out_info, dst);
				break;
			case CODEC_LDG:
				ret &= curr_output_plugin->c.ldg.funcs->encoder_write(out_info, dst);
				break;
			}
			if (!ret)
				break;

			if( show_write_progress_bar)
				win_progress(( int16)((( int32)y * 150L) / img_h));
		}
	}
	else /* inversed INTEL format */
	{	
		int16 h = ( img_h << 1);	/* image height * 2 . don't worry about this, is only for the progress bar */
		dst += data->LnSize * ( in_info->height - 1);

		/* First pass, we decode the entire image */
		for( y = 0; y < img_h; y++)
		{
			switch (curr_input_plugin->type)
			{
			case CODEC_SLB:
				plugin_reader_read(&curr_input_plugin->c.slb, in_info, data->RowBuf);
				break;
			case CODEC_LDG:
				curr_input_plugin->c.ldg.funcs->decoder_read(in_info, data->RowBuf);
				break;
			}
			( *raster)( data, dst);
	
			dst -= data->LnSize;

			if( show_write_progress_bar)
				win_progress(( int16)((( int32)y * 75L) / img_h));
		}

		dst	 = data->DstBuf;

		/* second pass, we encode it */
		for( ; y < h; y++)
		{
			switch (curr_output_plugin->type)
			{
			case CODEC_SLB:
				ret &= plugin_encoder_write(&curr_output_plugin->c.slb, out_info, dst);
				break;
			case CODEC_LDG:
				ret &= curr_output_plugin->c.ldg.funcs->encoder_write(out_info, dst);
				break;
			}
			if (!ret)
				break;

			dst += data->LnSize;

			if( show_write_progress_bar)
				win_progress(( int16)((( int32)y * 75L) / img_h));
		}
	}

	return ret;
}



static void exit_pic_save( IMGINFO in_info, IMGINFO out_info, DECDATA data)
{
	if( data->DthBuf) 
		free( data->DthBuf);

	if( data->DstBuf) 
		free( data->DstBuf);

	if( data->RowBuf) 
		free( data->RowBuf);

	if (curr_output_plugin && (curr_output_plugin->state & CODEC_ENCODER_INITIALIZED))
	{
		switch (curr_output_plugin->type)
		{
		case CODEC_SLB:
			plugin_encoder_quit(&curr_output_plugin->c.slb, out_info);
			break;
		case CODEC_LDG:
			curr_output_plugin->c.ldg.funcs->encoder_quit(out_info);
			break;
		}
		curr_output_plugin->state &= ~CODEC_ENCODER_INITIALIZED;
	}
	
	if (curr_input_plugin && (curr_input_plugin->state & CODEC_DECODER_INITIALIZED))
	{
		switch (curr_input_plugin->type)
		{
		case CODEC_SLB:
			plugin_reader_quit(&curr_input_plugin->c.slb, in_info);
			break;
		case CODEC_LDG:
			curr_input_plugin->c.ldg.funcs->decoder_quit( in_info);
			break;
		}
		curr_input_plugin->state &= ~CODEC_DECODER_INITIALIZED;
	}
	
	free( data);
	free( out_info);
	free( in_info);
	
	graf_mouse( ARROW, NULL);	
}


/*==================================================================================*
 * pic_load:																		*
 *				read a supported picture and fill a IMAGE structure.				*
 *----------------------------------------------------------------------------------*
 * file: 		 The file that will be read.										*
 * img: 		 The IMAGE struct. that will be filled. 			 				*
 *----------------------------------------------------------------------------------*
 * returns: 	'0' if error or picture not supported								*
 *==================================================================================*/
  
int16 pic_save( const char *in_file, const char *out_file)
{
	IMGINFO in_info;
	IMGINFO out_info;
	DECDATA data;
	int16 ret;

    graf_mouse(BUSYBEE, NULL);

	in_info 	= (img_info *) calloc(1, sizeof(img_info));
	out_info 	= (img_info *) calloc(1, sizeof(img_info));
	data 		= (dec_data *) malloc(sizeof(dec_data));
	
	if (!in_info || !out_info || !data)
	{
		free(data);
		free(out_info);
		free(in_info);

		errshow(NULL, -ENOMEM);		
		return FALSE;
	}

	data->DthBuf = NULL;
	data->DstBuf = NULL; 
	data->RowBuf = NULL;

	/* We initialise some variables needed by the codecs */
	in_info->background_color	= 0xFFFFFF;
	in_info->thumbnail			= FALSE;

	if (get_pic_info(in_file, in_info) == FALSE)
	{
		exit_pic_save(in_info, out_info, data);
		errshow(in_file, CANT_SAVE_IMG);
		return FALSE;
	}

	if (show_write_progress_bar)
		win_progress_begin(get_string(SAVE_TITLE));

	/* copy information from input's information to output's information struct */
	*out_info = *in_info;

	ret = FALSE;
	switch (curr_output_plugin->type)
	{
	case CODEC_SLB:
		ret = plugin_encoder_init(&curr_output_plugin->c.slb, out_file, out_info);
		break;
	case CODEC_LDG:
		ret = curr_output_plugin->c.ldg.funcs->encoder_init(out_file, out_info);
		break;
	}
	if (ret == FALSE)
	{
		errshow(out_file, CANT_SAVE_IMG);
		exit_pic_save(in_info, out_info, data);
		win_progress_end();
		return FALSE;
	}

	if (!setup_encoder(in_info, out_info, data))
	{
		errshow(NULL, -ENOMEM);	
		exit_pic_save(in_info, out_info, data);
		win_progress_end();
		return FALSE;
	}


	ret = write_img(in_info, out_info, data);

	exit_pic_save(in_info, out_info, data);

	graf_mouse(ARROW, NULL);
	win_progress_end();

	if (!ret)
		errshow(out_file, CANT_SAVE_IMG);

	return ret;
}
