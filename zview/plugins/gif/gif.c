/*
  2.00 initial port from zview archive v1.0.1, built with giflib 5.1.1
  2.01 fixed snowy white backgrounds, transparent color issue
  2.02 fixed animations not animating, added minimum default speed of 10ms
  2.03 improved frame delay fix, removed debug code
  2.04 updated build info
  2.05 fixed animations with mixed interlaced and non-interlaced frames
       added file version information
  2.06 fixed animations where the 1st frame uses a transparent color
       initial background color is passed in via zview
  2.07 added file id check, put image size check back in (fixed typo)
  2.08 firebee fixes
  2.09 updated build info
  2.10 fixed: animation delay out of sync with images
  2.11 Code cleanup
*/

#define VERSION		0x0211
#define NAME        "Graphics Interchange Format"
#define AUTHOR      "Lonny Pursell, Thorsten Otto"
#define DATE        __DATE__ " " __TIME__
#define MISCINFO    "GIFLIB: " __STRINGIFY(GIFLIB_MAJOR) "." __STRINGIFY(GIFLIB_MINOR) "." __STRINGIFY(GIFLIB_RELEASE)

#include <stdlib.h>
#include <gif_lib.h>
#include "plugin.h"
#include "imginfo.h"
#include "zvplugin.h"

#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) ("GIF\0");

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
	case INFO_MISC:
		return (long)MISCINFO;
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif

/*==================================================================================*
 * void decodecolormap:																*
 *		Convert 8 bits + palette data to standard RGB								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		src			->	The 8 bits source.											*
 *		dst			->	The destination where put the RGB data.						*
 *		cm			->	The source's colormap.										*
 *		width		->	picture's width.											*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *     --																			*
 *==================================================================================*/
static void decodecolormap(uint8_t *src, uint8_t *dst, GifColorType *cm, uint16_t width, int16_t trans_index,
						   uint32_t transparent_color, int16_t draw_trans)
{
	GifColorType *cmentry;
	uint16_t i;

	for (i = 0; i < width; i++)
	{
		if (src[i] == trans_index)
		{
			if (draw_trans)
			{
				dst += 3;
				continue;
			}
			*(dst++) = ((transparent_color >> 16) & 0xFF);
			*(dst++) = ((transparent_color >> 8) & 0xFF);
			*(dst++) = ((transparent_color) & 0xFF);
		} else
		{
			cmentry = &cm[src[i]];
			*(dst++) = cmentry->Red;
			*(dst++) = cmentry->Green;
			*(dst++) = cmentry->Blue;
		}
	}
}


static void free_info(IMGINFO info)
{
	txt_data *comment = (txt_data *) info->_priv_ptr;
	img_data *img = (img_data *) info->_priv_ptr_more;
	int16_t i;

	if (comment)
	{
		for (i = 0; i < comment->lines; i++)
		{
			free(comment->txt[i]);
		}
		free(comment);
		info->_priv_ptr = NULL;
	}

	if (img)
	{
		for (i = 0; i < img->imagecount; i++)
		{
			free(img->image_buf[i]);
		}
		free(img);
		info->_priv_ptr_more = NULL;
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
	static int16_t const InterlacedOffset[] = { 0, 4, 2, 1 };
	static int16_t const InterlacedJumps[] = { 8, 8, 4, 2 };
	int16_t i, j;
	int16_t error = 0;
	int16_t transpar = -1;
	int16_t interlace = FALSE;
	int16_t draw_trans = FALSE;
	uint16_t delaycount = 0;
	uint16_t delay;
	GifFileType *gif;
	GifRecordType rec;
	txt_data *comment;
	img_data *img;

	if ((gif = DGifOpenFileName(name, NULL)) == NULL)
		return FALSE;

	if ((comment = malloc(sizeof(*comment))) == NULL ||
		(img = malloc(sizeof(*img))) == NULL)
	{
		return FALSE;
	}
	info->_priv_ptr = comment;
	info->_priv_ptr_more = img;
	img->imagecount = 0;
	comment->lines = 0;

	do
	{
		if (DGifGetRecordType(gif, &rec) == GIF_ERROR)
		{
			error = 1;
			break;
		}

		if (rec == IMAGE_DESC_RECORD_TYPE)
		{
			if (DGifGetImageDesc(gif) == GIF_ERROR)
			{
				error = 1;
				break;
			} else
			{
				ColorMapObject *map = (gif->Image.ColorMap ? gif->Image.ColorMap : gif->SColorMap);
				int Row, Col, Width, Height;
				int32_t line_size;
				uint8_t *line_buffer = NULL;
				uint8_t *img_buffer;

				if (map == NULL)
				{
					error = 1;
					break;
				}

				Row = gif->Image.Top;		/* Image Position relative to Screen. */
				Col = gif->Image.Left;
				Width = gif->Image.Width;
				Height = gif->Image.Height;

				if (Col + Width > gif->SWidth || Row + Height > gif->SHeight)
				{
					error = 1;
					break;
				}
				info->components = map->BitsPerPixel == 1 ? 1 : 3;
				info->planes = map->BitsPerPixel;
				info->colors = map->ColorCount;
				info->width = (uint16_t) gif->SWidth;
				info->height = (uint16_t) gif->SHeight;
				line_size = (int32_t) gif->SWidth * (int32_t) info->components;

				img->image_buf[img->imagecount] = (uint8_t *) malloc(line_size * (gif->SHeight + 2));
				img_buffer = img->image_buf[img->imagecount];

				if (img_buffer == NULL)
				{
					error = 1;
					break;
				}

				if (map->BitsPerPixel != 1)
				{
					line_buffer = (uint8_t *) malloc(line_size + 128);

					if (line_buffer == NULL)
					{
						free(img->image_buf[img->imagecount]);
						error = 1;
						break;
					}
				}

				if (img->imagecount)
				{
					/* the transparency is the background picture */
					draw_trans = TRUE;

					memcpy(img_buffer, img->image_buf[img->imagecount - 1], line_size * gif->SHeight);
				} else
				{
					/* 1st frame, force it to the background color */
					memset(img_buffer, (int)info->background_color, line_size * gif->SHeight);
				}

				if (gif->Image.Interlace)
				{
					interlace = TRUE;	/* at least one frame is interlaced [lp] */
					for (i = 0; i < 4; i++)
					{
						for (j = Row + InterlacedOffset[i]; j < Row + Height; j += InterlacedJumps[i])
						{
							if (map->BitsPerPixel != 1)
							{
								if (DGifGetLine(gif, line_buffer, Width) != GIF_OK)
								{
									error = 1;
									free(img->image_buf[img->imagecount]);
									break;
								}

								decodecolormap(line_buffer, &img_buffer[(j * line_size) + (Col * 3)], map->Colors,
											   Width, transpar, info->background_color, draw_trans);
							} else if (DGifGetLine(gif, &img_buffer[(j * line_size) + Col], Width) != GIF_OK)
							{
								free(img->image_buf[img->imagecount]);
								error = 1;
								break;
							}
						}
					}
				} else
				{
					for (i = 0; i < Height; i++, Row++)
					{
						if (map->BitsPerPixel != 1)
						{
							if (DGifGetLine(gif, line_buffer, Width) != GIF_OK)
							{
								error = 1;
								free(img->image_buf[img->imagecount]);
								break;
							}

							decodecolormap(line_buffer, &img_buffer[(Row * line_size) + (Col * 3)], map->Colors, Width,
										   transpar, info->background_color, draw_trans);
						} else if (DGifGetLine(gif, &img_buffer[(Row * line_size) + Col], Width) != GIF_OK)
						{
							free(img->image_buf[img->imagecount]);
							error = 1;
							break;
						}
					}
				}

				if (line_buffer)
				{
					free(line_buffer);
					line_buffer = NULL;
				}

				img->imagecount++;
			}

			if (info->thumbnail)
				break;

		} else if (rec == EXTENSION_RECORD_TYPE)
		{
			int code;
			GifByteType *block;

			if (DGifGetExtension(gif, &code, &block) == GIF_ERROR)
			{
				error = 1;
				break;
			} else
			{
				while (block != NULL)
				{
					switch (code)
					{
					case COMMENT_EXT_FUNC_CODE:
						if (comment->lines < MAX_TXT_DATA)
						{
							/* Convert gif's pascal-like string */
							size_t len = block[0];
							comment->txt[comment->lines] = (char *) malloc(len + 1);

							if (comment->txt[comment->lines] != NULL)
							{
								memcpy(comment->txt[comment->lines], block + 1, len);
								comment->txt[comment->lines][len] = '\0';
								comment->max_lines_length = MAX(comment->max_lines_length, (int16_t) len);
								comment->lines++;
							}
						}
						break;

					case GRAPHICS_EXT_FUNC_CODE:
						if (block[1] & 1)
							transpar = (uint16_t) block[4];
						else
							transpar = -1;

						/* gif spec: delay expressed as (1/100th) of a second */
						delay = (block[2] + (block[3] << 8)) << 1;	/* *2 */
						if (delay == 0)
							delay = 10 << 1;	/* 10ms */

						img->delay[delaycount++] = delay;

						break;

#if 0
						/*
						   In version 2.0 beta Netscape, introduce a special extention for
						   set a maximum loop playback.. Netscape itself doesn't use it anymore
						   and play the animation infinitly... so we don't care about it.
						 */
					case APPLICATION_EXT_FUNC_CODE:
						if (reading_netscape_ext == 0)
						{
							if (memcmp(block + 1, "NETSCAPE", 8) != 0)
								break;

							if (block[0] != 11)
								break;

							if (memcmp( block + 9, "2.0", 3) != 0)
								break;

							reading_netscape_ext = 1;
						} else
						{
							if ((block[1] & 7) == 1)
								info->max_loop_count = block[2] | ( block[3] << 8);
						}
						break;
#endif
					default:
						break;
					}

					if (DGifGetExtensionNext(gif, &block) == GIF_ERROR)
					{
						error = 1;
						break;
					}
				}
			}
		} else
			break;
	} while (rec != TERMINATE_RECORD_TYPE);


	if (error)
	{
		free_info(info);
		DGifCloseFile(gif, NULL);
		return FALSE;
	}

	info->real_height = info->height;
	info->real_width = info->width;
	info->orientation = UP_TO_DOWN;
	info->page = img->imagecount;
	info->num_comments = comment->lines;
	info->max_comments_length = comment->max_lines_length;
	info->indexed_color = FALSE;
	info->memory_alloc = TT_RAM;
	info->_priv_var = 0;				/* page line counter */
	info->_priv_var_more = 0;			/* current page returned */

	strcpy(info->info, "GIF");

#if 0
	if (gif->Version)
		strcat(info->info, "89a"); 
	else
		strcat(info->info, "87a"); 
#endif

	if (interlace)
		strcat(info->info, " (Interlaced)");

	strcpy(info->compression, "LZW");

	DGifCloseFile(gif, NULL);

	return TRUE;
}


/*==================================================================================*
 * boolean __CDECL reader_get_txt													*
 *		This function , like other function mus be always present.					*
 *		It fills txtdata struct. with the text present in the picture ( if any).	*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		txtdata		->	The destination text buffer.								*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_get_txt(IMGINFO info, txt_data * txtdata)
{
	int16_t i;
	txt_data *comment = (txt_data *) info->_priv_ptr;

	if (comment)
	{
#ifdef PLUGIN_SLB
		for (i = 0; i < comment->lines; i++)
		{
			free(txtdata->txt[i]);
			txtdata->txt[i] = comment->txt[i];
			comment->txt[i] = NULL;
		}
#else
		for (i = 0; i < comment->lines; i++)
		{
			strcpy(txtdata->txt[i], comment->txt[i]);
		}
#endif
		txtdata->lines = comment->lines;
	}
}


/*==================================================================================*
 * boolean __CDECL reader_read:														*
 *		This function fits the buffer with image data								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		buffer		->	The destination buffer.										*
 *		info		->	The IMGINFO struct.											*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_read(IMGINFO info, uint8_t * buffer)
{
	img_data *img = (img_data *) info->_priv_ptr_more;
	int32_t line_size = (int32_t) info->width * (int32_t) info->components;
	uint8_t *line_src;

	if ((int32_t) info->page_wanted != info->_priv_var_more)
	{
		info->_priv_var_more = (int32_t) info->page_wanted;
		info->_priv_var = 0;
	}

	line_src = &img->image_buf[info->page_wanted][info->_priv_var];

	info->_priv_var += line_size;
	info->delay = img->delay[info->page_wanted];

	memcpy(buffer, line_src, line_size);

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
	free_info(info);
}
