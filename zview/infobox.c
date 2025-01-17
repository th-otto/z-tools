#include "general.h"
#include "ztext.h"
#include "infobox.h"
#include "zedit/zedit.h"
#include "close_modal.h"
#include "pic_load.h"
#include "winimg.h"
#include "file/count.h"
#include "file/file.h"
#include "custom_font.h"
#include <math.h>
#include "plugins/common/zvplugin.h"

static uint32 ypos_max, ypos;
static float slider_pos = 0.0;
static float slider_step = 0.0;
static OBJECT *slider_root;


void ObjcStrnCpy(OBJECT *tree, short idx, const char *str)
{
	TEDINFO *ted = tree[idx].ob_spec.tedinfo;
	strncpy(ted->te_ptext, str, ted->te_txtlen - 1);
}


static void CDECL draw_exif_info( WINDOW *win, PARMBLK *pblk, void *data)
{
	int16 xy[8];
	txt_data *exif_box = (txt_data *) data; 
	int16 dum;
	int16 posy;

	xy[0] = pblk->pb_x;
	xy[1] = pblk->pb_y;
	xy[2] = xy[0] + pblk->pb_w - 2;
	xy[3] = xy[1] + pblk->pb_h - 1;

	udef_vsf_color( WIN_GRAF_HANDLE(win), WHITE);
	udef_v_bar( WIN_GRAF_HANDLE(win), xy);

	xy[0] = pblk->pb_x + pblk->pb_w - 1;
	xy[1] = pblk->pb_y;
	xy[2] = pblk->pb_x;
	xy[3] = pblk->pb_y;
	xy[4] = pblk->pb_x;
	xy[5] = pblk->pb_y + pblk->pb_h - 1;
	xy[6] = xy[0];
	xy[7] = xy[5];
	
	udef_vsl_color( WIN_GRAF_HANDLE(win), BLACK);
	udef_v_pline( WIN_GRAF_HANDLE(win), 4, xy);

	posy = pblk->pb_y;  

	for ( dum = ypos; dum < exif_box->lines; dum++)
	{
		if( posy >= ( pblk->pb_y + pblk->pb_h - 1))
			break;

		if (exif_box->txt[dum])
		{
			draw_text( WIN_GRAF_HANDLE(win), pblk->pb_x + 3, posy + 3, BLACK, exif_box->txt[dum]);
			posy += 12;
		}
	}
}


static void calc_exif_slider( WINDOW *win)
{
	int16 max_mover_size;
	int16 full_win_size = ypos_max * 12;

	max_mover_size 	= slider_root[FILE_INFO_VBACK].ob_height;

	slider_root[FILE_INFO_VMOVER].ob_y 		= 0;
	slider_root[FILE_INFO_VMOVER].ob_height = max_mover_size;
		
	if ( slider_root[FILE_INFO_COMMENT].ob_height < full_win_size)
	{	
		float factor 			= ( float)slider_root[FILE_INFO_COMMENT].ob_height / ( float)full_win_size;
		float mover_size 		= MAX(( float)max_mover_size * factor, 6L);
		
		slider_pos = 0;
		slider_step = ( float)max_mover_size / ( float)ypos_max;
		slider_root[FILE_INFO_VMOVER].ob_height	= ( int16)( mover_size);
	}	
}



static void __CDECL slid_up( WINDOW *win, int obj_index, int mode, void *data)
{
	int16 dum;
	int16 res;

	(void)data;
	ObjcChange( mode, win, obj_index, SELECTED, TRUE);
	
	do 
	{
		if ( ypos > 0L) 
		{
			ypos--;
			ObjcDraw( OC_FORM, win, FILE_INFO_COMMENT, 1);
			slider_pos -= slider_step;
			slider_root[FILE_INFO_VMOVER].ob_y	= MAX( 0, ( int16)floor( slider_pos));
			ObjcDraw( OC_FORM, win, FILE_INFO_VBACK, 2);
		}
		evnt_timer(50);
		graf_mkstate( &dum, &dum, &res, &dum);

	} while( res);

	ObjcChange( mode, win, obj_index, ~SELECTED, TRUE);
}


static void slid_down( WINDOW *win, int obj_index, int mode, void *data)
{
	int16 dum;
	int16 res;

	(void)data;
	ObjcChange( mode, win, obj_index, SELECTED, TRUE);

	do 
	{
		if (( ypos < ( ypos_max - slider_root[FILE_INFO_COMMENT].ob_height / 12)) && ( ypos_max  > slider_root[FILE_INFO_COMMENT].ob_height / 12)) 
		{
			ypos ++;
			ObjcDraw( OC_FORM, win, FILE_INFO_COMMENT, 1);
			slider_pos += slider_step;
			slider_root[FILE_INFO_VMOVER].ob_y	= ( int16)floor( slider_pos);
			ObjcDraw( OC_FORM, win, FILE_INFO_VBACK, 2);
		}
		evnt_timer(50);
		graf_mkstate( &dum, &dum, &res, &dum);

	} while( res);

	ObjcChange( mode, win, obj_index, ~SELECTED, TRUE);
}


static void slid_vmover( WINDOW *win, int obj_index, int mode, void *data)
{
	int32 old_ypos = ypos;
	int16 res;

	(void)data;
	ObjcChange( mode, win, obj_index, SELECTED, TRUE);

	graf_mouse( FLAT_HAND, NULL);

	wind_update( BEG_MCTRL);

	res = graf_slidebox( slider_root, FILE_INFO_VBACK, FILE_INFO_VMOVER, 1);

	wind_update( END_MCTRL);

	ypos = MAX( 0, ( ypos_max - slider_root[FILE_INFO_COMMENT].ob_height / 12) * res / 1000L);

	if( old_ypos != ypos)
	{
		slider_pos = ypos * slider_step;
		slider_root[FILE_INFO_VMOVER].ob_y	= ( int16)floor( slider_pos);
		ObjcDraw( OC_FORM, win, FILE_INFO_VBACK, 2);	
		ObjcDraw( OC_FORM, win, FILE_INFO_COMMENT, 1);
	}

	graf_mouse( ARROW, NULL);

	ObjcChange( mode, win, obj_index, ~SELECTED, TRUE);
}


static void objc_multiline(OBJECT *tree, long str, short first, short last)
{
	short i;
	const char *p, *end;
	char temp[40 + 1];

	i = first;
	if (str > 0)
	{
		p = (char *)str;
		while (i <= last)
		{
			size_t len;
			
			end = strchr(p, '\n');
			if (end == NULL)
			{
				ObjcStrnCpy(tree, i, p);
				i++;
				break;
			}
			++end;
			len = end - p;
			len = MIN(len, sizeof(temp));
			zstrncpy(temp, p, len);
			ObjcStrnCpy(tree, i, temp);
			p = end;
			i++;
		}
	}
	while (i <= last)
	{
		ObjcStrnCpy(tree, i, "");
		i++;
	}
}


/*==================================================================================*
 * void infobox:																	*
 *		Show the infobox for a image or a file.										*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		--																			*
 *----------------------------------------------------------------------------------*
 * returns: 																		*
 *      --																			*
 *==================================================================================*/
void infobox( void)
{
	WINDOW 			*wininfo;
	WINDICON 		*wicones;
    WINDATA			*windata;
    IMAGE 			*img;
	uint32_t		uncompressed_size;
	int32			file_size;
	char temp[40 + 1];
	static int const frms[] = { FILE_INFO_PANEL1, FILE_INFO_PANEL2,  FILE_INFO_PANEL3, FILE_INFO_PANEL4 };
	static int const buts[] = { FILE_INFO_FILE,   FILE_INFO_IMAGE,   FILE_INFO_EXIF,   FILE_INFO_CODEC };
	OBJECT *infotext;

	infotext = get_tree( FILE_INFO);

	if(( wicones = ( WINDICON *)DataSearch( wglb.front, WD_ICON)))	/* See if the target picture is in the catalog window */
	{		
		Entry *entry = wicones->first_selected;
		img 		 = &entry->preview;
		file_size 	 = entry->stat.st_size;

		if( wicones->edit)
			exit_edit_mode( win_catalog, wicones->first_selected);
		
		ObjcStrnCpy( infotext, FILE_INFO_NAME, entry->name);
		ObjcStrnCpy( infotext, FILE_INFO_DATE, entry->date);
		ObjcStrnCpy( infotext, FILE_INFO_TIME, entry->time);  

		if( entry->type == ET_DIR)
		{
			fileinfo file;
			
			graf_mouse( BUSYBEE, NULL);	

			if( !count_files( wicones, &file))
			{
				errshow("", -errno);
				return;
			}

			size_to_text( temp, file.size);
			ObjcStrnCpy( infotext, FILE_INFO_SIZE, temp);

			graf_mouse( ARROW, NULL);
		}
		else
			ObjcStrnCpy( infotext, FILE_INFO_SIZE, entry->size);  

	}
	else if(( windata = (WINDATA *)DataSearch( wglb.front, WD_DATA)))		/* See if the picture is in a "normal" window */
	{   
		struct stat		file_stat;
	   	struct tm		*tmt;
		int8			*fullname = windata->name;

		img 	 = &windata->img;
		
		ObjcStrnCpy( infotext, FILE_INFO_NAME, f_basename(fullname));

		lstat( fullname, &file_stat);
		file_size = file_stat.st_size;

		size_to_text( temp, file_stat.st_size);
		ObjcStrnCpy( infotext, FILE_INFO_SIZE, temp);

		tmt = localtime(&file_stat.st_mtime);
		strftime( temp, sizeof(temp), "%A %d %B %Y", tmt);
		ObjcStrnCpy( infotext, FILE_INFO_DATE, temp);

		strftime( temp, sizeof(temp), "%H:%M:%S", tmt);
		ObjcStrnCpy( infotext, FILE_INFO_TIME, temp);
	}	  
	else 
		return;	/* normally, never */	


	if( img->comments)
		infotext[FILE_INFO_EXIF].ob_flags &= ~HIDETREE;
	else
		infotext[FILE_INFO_EXIF].ob_flags |= HIDETREE;

	if( img->codec)
	{
		long ret;
		char extensions[50 + 2];
		char *p;
		const char *src;

		switch (img->codec->type)
		{
		case CODEC_SLB:
			{
				SLB *slb;

				slb = &img->codec->c.slb;
				ret = plugin_get_option(slb, INFO_NAME);
				objc_multiline(infotext, ret, FILE_CODEC_NAME_FIRST, FILE_CODEC_NAME_LAST);

				ret = plugin_get_option(slb, INFO_VERSION);
				if (ret > 0)
					sprintf(temp, "%ld.%02lx", ret >> 8, ret & 0xff);
				else
					*temp = '\0';
				ObjcStrnCpy(infotext, FILE_CODEC_VERSION, temp);

				ret = plugin_get_option(slb, INFO_DATETIME);
				ObjcStrnCpy(infotext, FILE_CODEC_DATE, ret > 0 ? (const char *)ret : "");

				ret = plugin_get_option(slb, INFO_AUTHOR);
				ObjcStrnCpy(infotext, FILE_CODEC_AUTHOR, ret > 0 ? (const char *)ret : "");

				ret = plugin_get_option(slb, INFO_MISC);
				objc_multiline(infotext, ret, FILE_CODEC_INFO_FIRST, FILE_CODEC_INFO_LAST);

				ret = plugin_get_option(slb, INFO_COMPILER);
				ObjcStrnCpy(infotext, FILE_CODEC_COMPILER, ret > 0 ? (const char *)ret : "");
			}
			break;
		case CODEC_LDG:
			{
				LDG *ldg;
				const char *codecname;
				const char *author;
				const char *date;
				const char *misc_info;
				int i;

				ldg = img->codec->c.ldg.ldg;
				codecname = ldg->list[0].info;
				if (codecname)
				{
					if (strncmp(codecname, "Codec: ", 7) == 0)
						codecname += 7;
				} else
				{
					codecname = "";
				}
				objc_multiline(infotext, (long)codecname, FILE_CODEC_NAME_FIRST, FILE_CODEC_NAME_LAST);
				sprintf(temp, "%d.%02x", (unsigned short)ldg->vers >> 8, ldg->vers & 0xff);
				ObjcStrnCpy(infotext, FILE_CODEC_VERSION, temp);
				/*
				 * Just a convention from LP codecs:
				 * - text of function pointer 1 (reader_init) contains the author name, possibly prefixed with "Author: "
				 * - text of function pointer 2 (reader_read) contains the compilation date, prefixed with "Date: "
				 * - text of function pointer 3 (reader_quit) contains the compilation time, prefixed with "Time: "
				 * - In some newer versions, reader_read contains the old version number,
				 *   and reader_quit contains both date & time
				 * - text of function pointer 4 (reader_get_txt) may contain misc info
				 */
				author = NULL;
				date = NULL;
				misc_info = NULL;
				for (i = 0; i < (int)ldg->num; i++)
				{
					if (strcmp(ldg->list[i].name, "reader_init") == 0)
					{
						author = ldg->list[i].info;
					} else if (strcmp(ldg->list[i].name, "reader_read") == 0)
					{
						date = ldg->list[i].info;
					} else if (strcmp(ldg->list[i].name, "reader_quit") == 0)
					{
						if (ldg->list[i].info != NULL && ldg->list[i].info[0] != 'T')
							date = ldg->list[i].info;
					} else if (strcmp(ldg->list[i].name, "reader_get_txt") == 0)
					{
						misc_info = ldg->list[i].info;
					}
				}
				if (author && strncmp(author, "Author: ", 8) == 0)
					author += 8;
				if (date && strncmp(date, "Date: ", 6) == 0)
					date += 6;
				if (author == NULL)
					author = "";
				ObjcStrnCpy(infotext, FILE_CODEC_AUTHOR, author);
				if (date == NULL)
					date = "";
				ObjcStrnCpy(infotext, FILE_CODEC_DATE, date);
				objc_multiline(infotext, (long)misc_info, FILE_CODEC_INFO_FIRST, FILE_CODEC_INFO_LAST);
				ObjcStrnCpy(infotext, FILE_CODEC_COMPILER, ldg->flags & LDG_STDCALL ? "Pure-C" : "GNU-C");
			}
			break;
		}

		ObjcStrnCpy(infotext, FILE_CODEC_FILENAME, img->codec->name);
		p = extensions;
		src = img->codec->extensions;
		/*
		 * extensions is a double '\0' terminated list
		 */
		while (*src)
		{
			size_t len = strlen(src) + 1;
			strcpy(p, src);
			p += len;
			p[-1] = ' ';
			src += len;
		}
		*p = '\0';
		ObjcStrnCpy(infotext, FILE_CODEC_EXTENSIONS, extensions);

		if (infotext[FILE_INFO_EXIF].ob_flags & HIDETREE)
			infotext[FILE_INFO_CODEC].ob_x = infotext[FILE_INFO_IMAGE].ob_x + infotext[FILE_INFO_IMAGE].ob_width + 2;
		else
			infotext[FILE_INFO_CODEC].ob_x = infotext[FILE_INFO_EXIF].ob_x + infotext[FILE_INFO_EXIF].ob_width + 2;

		infotext[FILE_INFO_CODEC].ob_flags &= ~HIDETREE;
	} else
	{
		infotext[FILE_INFO_CODEC].ob_flags |= HIDETREE; 
	}

	if ( img->page)
	{ 
		infotext[FILE_INFO_IMAGE].ob_flags &= ~HIDETREE;
	
		sprintf( infotext[FILE_INFO_COLOR].ob_spec.tedinfo->te_ptext,	"%ld", img->colors);	
		sprintf( infotext[FILE_INFO_WIDTH].ob_spec.tedinfo->te_ptext,  	"%d",  img->img_w);
		sprintf( infotext[FILE_INFO_HEIGHT].ob_spec.tedinfo->te_ptext, 	"%d",  img->img_h);	
		sprintf( infotext[FILE_INFO_PLANES].ob_spec.tedinfo->te_ptext,  "%d",  img->bits);	
		sprintf( infotext[FILE_INFO_PAGE].ob_spec.tedinfo->te_ptext,  	"%d",  img->page);
		ObjcStrnCpy( infotext, FILE_INFO_DEC_TIME, img->working_time);
		ObjcStrnCpy( infotext, FILE_INFO_COMPRESSION, img->compression);
		ObjcStrnCpy( infotext, FILE_INFO_INFO, img->info);
		
		/* Calculate the Image decompressed size */
		uncompressed_size = ((img->img_w * img->bits + 7) >> 3) * img->img_h;	
		size_to_text( infotext[FILE_INFO_MEM].ob_spec.tedinfo->te_ptext, uncompressed_size);
			
		sprintf( infotext[FILE_INFO_RATIO].ob_spec.tedinfo->te_ptext, "%.1f : 1", uncompressed_size / ( float)file_size);				
	}
	else
		infotext[FILE_INFO_IMAGE].ob_flags |= HIDETREE; 
		
	wininfo = FormCreate( infotext, NAME|MOVER|CLOSER, NULL, "Information", NULL, TRUE, FALSE);

	FormThumb( wininfo, frms, buts, 4);

	WindSet( wininfo, WF_BEVENT, BEVENT_MODAL, 0, 0, 0);

	EvntAttach( wininfo, WM_XKEYBD, close_on_esc);
	EvntAttach( wininfo, WM_CLOSED, close_modal);

	if( img->comments)
	{
		ypos 			= 0;
		ypos_max 		= img->comments->lines;
		wininfo->h_u 	= 12;
		
		slider_root 	= FORM( wininfo);
		
		calc_exif_slider( wininfo);

		RsrcUserDraw ( OC_FORM, wininfo, FILE_INFO_COMMENT, draw_exif_info, ( void*)img->comments);

		// draw_exif_info( wininfo);

		ObjcAttachFormFunc( wininfo, FILE_INFO_UP, slid_up, NULL);
		ObjcAttachFormFunc( wininfo, FILE_INFO_DOWN, slid_down, NULL);
		ObjcAttachFormFunc( wininfo, FILE_INFO_VMOVER, slid_vmover, NULL);
	}


	MenuDisable();
}
