#include "general.h"
#include "pic_load.h"
#include "prefs.h"
#include "progress.h"
#include "plugins.h"
#include "mfdb.h"
#include "ztext.h"
#include "chrono.h"
#include "txt_data.h"
#include "zvdi/color.h"
#include "zvdi/raster.h"
#include "zvdi/vdi.h"
#include <math.h>
#include "resample.h"
#include "plugins/common/zvplugin.h"

void 		  (*raster)	  			(DECDATA, void *dst);
void 		  (*raster_cmap) 		(DECDATA, void *);
void 		  (*raster_true) 		(DECDATA, void *);
void 		  (*cnvpal_color)		(IMGINFO, DECDATA);
void 		  (*raster_gray) 		(DECDATA, void *);
CODEC *curr_input_plugin;
CODEC *curr_output_plugin;



static int16 setup(IMAGE *img, IMGINFO info, DECDATA data)
{
	int16   i, n_planes = info->planes == 1 && info->components == 1 ? 1 : app.nplanes;
	uint16	display_w, display_h;
	double	precise_x, precise_y, factor;
	size_t	src_line_size;

	if (img->view_mode && (info->width > thumbnail[thumbnail_size][0] || info->height > thumbnail[thumbnail_size][1]) && (!smooth_thumbnail || n_planes < 16))
	{
		factor	  = MAX(((double)info->height / (double)thumbnail[thumbnail_size][1]), ((double)info->width / (double)thumbnail[thumbnail_size][0]));
		precise_x = (double)info->width  / factor;
		precise_y = (double)info->height / factor;

		display_w 		= (uint16)(precise_x > 0 ? precise_x : 16);
		display_h 		= (uint16)(precise_y > 0 ? precise_y : 16);
		data->IncXfx    = (((uint32)info->width  << 16) + (display_w >> 1)) / display_w;
		data->IncYfx    = (((uint32)info->height << 16) + (display_h >> 1)) / display_h;
	}
	else
	{
		display_w 		= info->width;
		display_h 		= info->height;
		data->IncXfx    = 0x10000uL;
		data->IncYfx	= 0x10000uL;
	}

	if (img->view_mode)
		/* if we are in preview mode, we decompress only one image */
		img->page = 1;
	else
		img->page = info->page;


	if ((img->image = (MFDB *)calloc(img->page, sizeof(MFDB))) == NULL)
		return FALSE;


	for (i = 0 ; i < img->page ; i++)
	{
		if (!init_mfdb(&img->image[i], display_w, display_h, n_planes))
			return FALSE;
	}

	/* 	we initialise the txt_data struct...  */
	if (!init_txt_data(img, info->num_comments, info->max_comments_length))
		return FALSE;

	if (curr_input_plugin)
	{
		switch (curr_input_plugin->type)
		{
		case CODEC_SLB:
			/*
			 * SLB plugins can allocate the strings through callbacks,
			 * and may not have set up the num_comments member until now
			 */
			plugin_reader_get_txt(&curr_input_plugin->c.slb, info, img->comments);
			info->num_comments = img->comments->lines;
			break;
		case CODEC_LDG:
			if (info->num_comments > 0)
			{
				/*
				 * LDG plugins need the strings preallocated, so we must only
				 * call decoder_get_txt if there are actually infos
				 */
				curr_input_plugin->c.ldg.funcs->decoder_get_txt(info, img->comments);
			}
			break;
		}
	}
	if (img->comments && img->comments->lines == 0)
		delete_txt_data(img);
	img->codec = curr_input_plugin;

	/*
	 * we assume that the pixel size is minimum 8 bits because some GNU libraries return 1 and 4 bits format like 8 bits ones.
	 * We add also a little more memory for avoid buffer overflow for plugin badly coded.
	 */
	src_line_size = (info->width + 64) * info->components;

	data->RowBuf = (uint8*)malloc(src_line_size);

	if (!data->RowBuf)
		return FALSE;

	if ((info->planes == 1 && info->components == 1) || app.nplanes > 8)
	{
		data->DthBuf = NULL;
	} else
	{
		size_t size = (display_w + 15) * 3;

		if ((data->DthBuf = malloc(size)) == NULL)
			return FALSE;

		memset(data->DthBuf, 0, size);
	}

	data->DthWidth 	= display_w;
	data->PixMask  	= (1 << info->planes) - 1;
	data->LnSize   	= img->image[0].fd_wdwidth * n_planes;

	if (info->planes == 1 && info->components == 1)
	{
		cnvpal_mono(info, data);
		raster = raster_mono;
	}
	else
	{
		if (info->indexed_color)
		{
			(*cnvpal_color)(info, data);
			raster = raster_cmap;
		}
		else
			raster = (info->components >= 3 ? raster_true : raster_gray);
	}

	img->img_w      = info->real_width;
	img->img_h      = info->real_height;
	img->colors		= info->colors;
	img->bits   	= info->planes;

	strcpy(img->info, 			info->info);
	strcpy(img->compression, 	info->compression);

	return TRUE;
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
static inline void read_img(IMAGE *img, IMGINFO info, DECDATA data)
{
	uint16 		*dst;
	int16  		y_dst, i, y, progress_counter = 0, img_h 	= info->height;
	int32		line_size = data->LnSize, line_to_decode = img_h * img->page;
	uint8  		*buf = data->RowBuf;

	for (i = 0; i < img->page; i++)
	{
		uint32 scale = (data->IncYfx + 1) >> 1;
		y_dst  		 = img->image[0].fd_h;
		dst   		 = img->image[i].fd_addr;

		if (info->orientation == DOWN_TO_UP)
		{
			dst += data->LnSize * (y_dst - 1);
			line_size = -data->LnSize;
		}

		info->page_wanted = i;

		for (y = 1; y <= img_h && y_dst; y++)
		{
			switch (curr_input_plugin->type)
			{
			case CODEC_SLB:
				if (!plugin_reader_read(&curr_input_plugin->c.slb, info, buf))
					return;
				break;
			case CODEC_LDG:
				if (!curr_input_plugin->c.ldg.funcs->decoder_read(info, buf))
					return;
				break;
			}

			while ((scale >> 16) < y)
			{
				(*raster)(data, dst);
				dst   += line_size;
				scale += data->IncYfx;
				if (!--y_dst) break;
			}

			if (img->progress_bar)
			{
				progress_counter++;
				win_progress((int16)(((int32)progress_counter * 150L) / line_to_decode));
			}
		}

		img->delay[i]  	= info->delay;
	}
}



/*==================================================================================*
 * quit_img:																		*
 *				Close the image's file and free the line/dither/info buffer.		*
 *----------------------------------------------------------------------------------*
 * info:		The struct. to liberate												*
 *----------------------------------------------------------------------------------*
 * returns: 	-																	*
 *==================================================================================*/

static void quit_img(IMGINFO info, DECDATA data)
{
	if (curr_input_plugin && (curr_input_plugin->state & CODEC_DECODER_INITIALIZED))
	{
		switch (curr_input_plugin->type)
		{
		case CODEC_SLB:
			plugin_reader_quit(&curr_input_plugin->c.slb, info);
			break;
		case CODEC_LDG:
			curr_input_plugin->c.ldg.funcs->decoder_quit(info);
			break;
		}
		curr_input_plugin->state &= ~CODEC_DECODER_INITIALIZED;
	}

	if (data->DthBuf != NULL)
	   	free(data->DthBuf);

	if (data->RowBuf != NULL)
	   free(data->RowBuf);

	free(data);
	free(info);
}


static boolean codec_handles_extension(const char *extensions, const char *extension)
{
	const char *p;

	p = extensions;
	while (*p)
	{
		if (strcmp(extension, p) == 0)
		{
			return TRUE;
		}
		p += strlen(p) + 1;
	}
	return FALSE;
}


/*
 * Find the codec that is responsible to handle an image file.
 * Only checks already parsed meta-information, but does not load it
 */
CODEC *find_codec(const char *file)
{
	int16 i;
	const char *dot;
	char extension[MAXNAMLEN];
	CODEC *codec;

	dot = strrchr(file, '.');
	if (dot == NULL)
		return NULL;
	strcpy(extension, dot + 1);
	str2upper(extension);

	for (i = 0; i < plugins_nbr; i++)
	{
		codec = codecs[i];
		if (codec_handles_extension(codec->extensions, extension))
			return codec;
	}

	return NULL;
}


/*
 * Find & load the codec that is responsible to handle an image file,
 * and make it the current input plugin
 */
CODEC *get_codec(const char *file)
{
	CODEC *codec;

	curr_input_plugin = NULL;

	if ((codec = find_codec(file)) != NULL)
		if (plugin_ref(codec))
		{
			curr_input_plugin = codec;
			return codec;
		}

	/* I wish that it will never happen ! */
	return NULL;
}


boolean get_pic_info(const char *file, IMGINFO info)
{
	boolean ret = FALSE;
	CODEC *codec;

	if ((codec = get_codec(file)) != NULL)
	{
		if (codec->capabilities & CAN_DECODE)
		{
			switch (codec->type)
			{
			case CODEC_SLB:
				ret = plugin_reader_init(&codec->c.slb, file, info);
				/*
				 * must not call deinit if initialization fails,
				 * some badly written plugins may have already freed buffers,
				 * but not initialized them to NULL, causing double-frees
				 */
				if (ret)
					codec->state |= CODEC_DECODER_INITIALIZED;
				break;
			case CODEC_LDG:
				ret = codec->c.ldg.funcs->decoder_init(file, info);
				/*
				 * must not call deinit if initialization fails,
				 * some badly written plugins may have already freed buffers,
				 * but not initialized them to NULL, causing double-frees
				 */
				if (ret)
					codec->state |= CODEC_DECODER_INITIALIZED;
				break;
			}
			/*
			 * Most likely, the codec will be needed by more than one image.
			 * Make it resident to avoid loading/unloading it every time
			 */
			codec->state |= CODEC_RESIDENT;
		} else
		{
			errshow(codec->extensions, LDG_ERR_BASE + LDG_NO_FUNC);
		}
	}
	return ret;
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

boolean pic_load(const char *file, IMAGE *img, boolean quiet)
{
	IMGINFO info;
	DECDATA data;

	info = (img_info *) calloc(1, sizeof(img_info));

	if (!info)
	{
		errshow(NULL, -ENOMEM);
		return FALSE;
	}

	data = (dec_data *) calloc(1, sizeof(dec_data));

	if (!data)
	{
		errshow(NULL, -ENOMEM);
		free(info);
		return FALSE;
	}

	if (img->progress_bar)
		win_progress_begin(get_string(PLEASE_WAIT));

	/* We initialise some variables needed by the codecs */
	info->background_color	= 0xFFFFFF;
	info->thumbnail			= img->view_mode;

	chrono_on();

	if (get_pic_info(file, info) == FALSE)
	{
		quit_img(info, data);
		win_progress_end();
		if (!quiet)
			errshow(NULL, IMG_NO_VALID);
		return FALSE;
	}

	if (!setup(img, info, data))
	{
		errshow(NULL, -ENOMEM);
		quit_img(info, data);
		delete_mfdb(img->image, img->page);
		img->image = NULL;
		win_progress_end();
		return FALSE;
	}

	read_img(img, info, data);

	quit_img(info, data);
	plugin_unref(curr_input_plugin);

	win_progress_end();

	if (img->view_mode && (img->image[0].fd_w > thumbnail[thumbnail_size][0] || img->image[0].fd_h > thumbnail[thumbnail_size][1]) && smooth_thumbnail && img->image[0].fd_nplanes >= 16)
	{
		MFDB 	resized_image;
		double	precise_x, precise_y, factor;
		uint16	display_w, display_h;

		factor	  = MAX(((double)img->image[0].fd_h / (double)thumbnail[thumbnail_size][1]), ((double)img->image[0].fd_w / (double)thumbnail[thumbnail_size][0]));
		precise_x = (double)img->image[0].fd_w  / factor;
		precise_y = (double)img->image[0].fd_h / factor;
		display_w = (uint16)(precise_x > 0 ? precise_x : 16);
		display_h = (uint16)(precise_y > 0 ? precise_y : 16);

		init_mfdb(&resized_image, display_w, display_h, img->image[0].fd_nplanes);

		smooth_resize(&img->image[0], &resized_image, smooth_thumbnail);

		free(img->image[0].fd_addr);

		img->image[0].fd_addr 		= resized_image.fd_addr;
		img->image[0].fd_w 			= resized_image.fd_w;
		img->image[0].fd_h 			= resized_image.fd_h;
		img->image[0].fd_wdwidth	= resized_image.fd_wdwidth;
	}

	chrono_off(img->working_time);

	return TRUE;
}
