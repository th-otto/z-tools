#include "../general.h"
#include "../ztext.h"
#include "../mfdb.h"
#include "catalog_entry.h"
#include "../plugins.h"
#include "catalog_icons.h"
#include "catalog.h"
#include "../pic_load.h"

/* Global variable */
MFDB mini_big			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_big_on		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_small			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_small_on		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_printer		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_printer_on	= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_find			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_find_on		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_hdd			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_folder		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_forward		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_forward_on	= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_back			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_back_on		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_up			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_up_on			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_reload		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_reload_on		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_info			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_info_on		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_delete		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_delete_on		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_fullscreen	= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_fullscreen_on	= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_open 			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_open_on 		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_save			= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};
MFDB mini_save_on		= { NULL, 16, 16, 1, 0, 1, 0, 0, 0};

static IMAGE icon_image;
static IMAGE icon_file;
static IMAGE icon_folder;
static IMAGE icon_prg;
static IMAGE icon_pdf;

static IMAGE small_icon;


/* local variable */
static uint16 initialised 		= 0;



/*==================================================================================*
 * void icons_exit:																	*
 *		Free the memories previously allocated for the icon and pattern.			*
 *----------------------------------------------------------------------------------*
 * input:																			*
 * 		--																			*
 *----------------------------------------------------------------------------------*
 * returns: 																		*	
 *		--																			*
 *==================================================================================*/

void icons_exit(void)
{
	if ( initialised != 0)
	{
		( void)delete_mfdb( small_icon.image, small_icon.page);
		( void)delete_mfdb( icon_pdf.image, icon_pdf.page);
		( void)delete_mfdb( icon_prg.image, icon_prg.page);
		( void)delete_mfdb( icon_file.image, icon_file.page);
		( void)delete_mfdb( icon_image.image, icon_image.page);	
		( void)delete_mfdb( icon_folder.image, icon_folder.page);
	}	
	initialised = 0;
}


/*==================================================================================*
 * int16 icons_init:																*
 *		Initializes default icons and pattern. 										*
 *		May be called any number of times.											*
 *----------------------------------------------------------------------------------*
 * input:																			*
 * 		--																			*
 *----------------------------------------------------------------------------------*
 * returns: 																		*	
 *		0 if error else 1.															*
 *==================================================================================*/

static boolean icon_load( const char *file, IMAGE *img)
{
	char path[MAX_PATH];
	boolean ret = pic_load(strcat(strcpy(path, zview_path), file), img, TRUE);
	return ret;
}

boolean icons_init( void)
{
	uint16 *small_icon_buffer_addr;
	uint32 small_icon_buffer_size;

	if ( initialised == 0)
	{
		initialised = 1;

		icon_image.view_mode 		= full_size;
		icon_file.view_mode 		= full_size;
		icon_folder.view_mode 		= full_size;
		icon_prg.view_mode 			= full_size;
		icon_pdf.view_mode 			= full_size;

		icon_image.progress_bar 	= 0;
		icon_file.progress_bar 		= 0;
		icon_folder.progress_bar 	= 0;
		icon_prg.progress_bar 		= 0;
		icon_pdf.progress_bar 		= 0;

		
		small_icon.view_mode 		= full_size;				
		small_icon.progress_bar 	= 0;

		
		if ( !icon_load( "icons\\folder.god", &icon_folder)	||
			 !icon_load( "icons\\image.god", &icon_image)		||		
			 !icon_load( "icons\\file.god", &icon_file)		||	
			 !icon_load( "icons\\prg.god", &icon_prg)		||
			 !icon_load( "icons\\pdf.god", &icon_pdf)		||			 
			 !icon_load( "icons\\small.god",	&small_icon))
			{
				icons_exit();
				return FALSE;
			}
	}

	small_icon_buffer_addr	= ( uint16*)small_icon.image[0].fd_addr;
	small_icon_buffer_size	= app.nplanes << 4;
		
	mini_hdd.fd_nplanes 	= app.nplanes;
	mini_hdd.fd_addr 		= ( void*)small_icon_buffer_addr;

	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_folder.fd_nplanes 	= app.nplanes;
	mini_folder.fd_addr 	= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_big.fd_nplanes 	= app.nplanes;
	mini_big.fd_addr 		= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_big_on.fd_nplanes 	= app.nplanes;
	mini_big_on.fd_addr 	= ( void*)small_icon_buffer_addr;

	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_small.fd_nplanes 	= app.nplanes;
	mini_small.fd_addr 		= ( void*)small_icon_buffer_addr;	
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_small_on.fd_nplanes= app.nplanes;
	mini_small_on.fd_addr 	= ( void*)small_icon_buffer_addr;	
			
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_forward.fd_nplanes	= app.nplanes;
	mini_forward.fd_addr 	= ( void*)small_icon_buffer_addr;	
		
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_forward_on.fd_nplanes	= app.nplanes;
	mini_forward_on.fd_addr 	= ( void*)small_icon_buffer_addr;	

	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_back.fd_nplanes	= app.nplanes;
	mini_back.fd_addr	 	= ( void*)small_icon_buffer_addr;		
				
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_back_on.fd_nplanes	= app.nplanes;
	mini_back_on.fd_addr	= ( void*)small_icon_buffer_addr;		

	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_up.fd_nplanes		= app.nplanes;
	mini_up.fd_addr			= ( void*)small_icon_buffer_addr;			

	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_up_on.fd_nplanes	= app.nplanes;
	mini_up_on.fd_addr		= ( void*)small_icon_buffer_addr;			

	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_reload.fd_nplanes	= app.nplanes;
	mini_reload.fd_addr		= ( void*)small_icon_buffer_addr;			
		
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_reload_on.fd_nplanes	= app.nplanes;
	mini_reload_on.fd_addr	= ( void*)small_icon_buffer_addr;	
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_printer.fd_nplanes	= app.nplanes;
	mini_printer.fd_addr	= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_printer_on.fd_nplanes	= app.nplanes;
	mini_printer_on.fd_addr	= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_info.fd_nplanes	= app.nplanes;
	mini_info.fd_addr		= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_info_on.fd_nplanes	= app.nplanes;
	mini_info_on.fd_addr	= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_find.fd_nplanes	= app.nplanes;
	mini_find.fd_addr		= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_find_on.fd_nplanes	= app.nplanes;
	mini_find_on.fd_addr	= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_delete.fd_nplanes	= app.nplanes;
	mini_delete.fd_addr		= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_delete_on.fd_nplanes= app.nplanes;
	mini_delete_on.fd_addr	= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_fullscreen.fd_nplanes= app.nplanes;
	mini_fullscreen.fd_addr	= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_fullscreen_on.fd_nplanes= app.nplanes;
	mini_fullscreen_on.fd_addr	= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_open.fd_nplanes	= app.nplanes;
	mini_open.fd_addr		= ( void*)small_icon_buffer_addr;
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_open_on.fd_nplanes	= app.nplanes;
	mini_open_on.fd_addr	= ( void*)small_icon_buffer_addr;

	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_save.fd_nplanes	= app.nplanes;
	mini_save.fd_addr		= ( void*)small_icon_buffer_addr;	
	
	small_icon_buffer_addr	+= small_icon_buffer_size;
	mini_save_on.fd_nplanes	= app.nplanes;
	mini_save_on.fd_addr	= ( void*)small_icon_buffer_addr;	
		
	return TRUE;
} 


/*==================================================================================*
 * void set_entry_icon:																*
 *		Set the entry's icon.														*
 *----------------------------------------------------------------------------------*
 * input:																			*
 * 		entry	->	pointer to the entry to set										*
 *----------------------------------------------------------------------------------*
 * returns: 																		*	
 *		--																			*
 *==================================================================================*/

void set_entry_icon( Entry *entry) 
{
	char extension[MAXNAMLEN];
	const char *dot;
	
	if ( S_ISDIR( entry->stat.st_mode))
	{
		entry->type 	= ET_DIR;
		entry->icon		= icon_folder.image;
		return;
	}

	dot = strrchr(entry->name, '.');
	if (dot != NULL)
	{
		strcpy(extension, dot + 1);
		str2upper(extension);
		if(( strcmp ( extension, "TTP") == 0)
		|| ( strcmp ( extension, "TOS") == 0)
		|| ( strcmp ( extension, "GTP") == 0)
		|| ( strcmp ( extension, "PRG") == 0)
		|| ( strcmp ( extension, "APP") == 0))
		{
			entry->type 	= ET_PRG;
			entry->icon		= &icon_prg.image[0];
			return;
		}
	
		if( strcmp ( extension, "PDF") == 0)
		{
			entry->type 	= ET_PDF;
			entry->icon		= &icon_pdf.image[0];
			return;
		}
	}

	if (find_codec(entry->name))
	{
		entry->type	= ET_IMAGE;
		entry->icon	= &icon_image.image[0];
		return;
	}
	
	entry->type		= ET_FILE;
	entry->icon		= &icon_file.image[0];
}


/*==================================================================================*
 * void redraw_icon:																*
 *		send a redraw event at the entry's position.								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 * 		win   		-> the target window.											*
 * 		entry 		-> the icon to be redraw.										*
 *----------------------------------------------------------------------------------*
 * returns: -																		*
 *==================================================================================*/

void redraw_icon( WINDOW *win, Entry *entry)
{
	int16 xw, yw, hw, x, y, w, h;

	WindGet ( win, WF_WORKXYWH, &xw, &yw, &w, &hw);

	x = entry->case_pos.x1 - 2;
	y = entry->case_pos.y1 - 1;
	w = entry->case_pos.x2 - x + 4;
	h = entry->case_pos.y2 - y + 2;

	x += xw;
	y += yw;

	if (( y + entry->case_pos.y2 > yw) && ( y < yw + hw))
	{
	    draw_frame_slider = 0;
		draw_page( win, x, y, w, h);
		draw_frame_slider = 1;
	}
}


/*==================================================================================*
 * void draw_icon:																	*
 *		draw a icon.																*
 *----------------------------------------------------------------------------------*
 * input:																			*
 * 		handle     	-> the target VDI handle.										*
 * 		entry 		-> the icon entry to be draw.									*
 *		selected	-> the icon is selected or not.									*
 * 		dst_rect	-> Destination Bitmap for the icon.								*
 *----------------------------------------------------------------------------------*
 * returns: -																		*
 *==================================================================================*/

void draw_icon( int16 handle, Entry *entry, boolean selected, RECT16 *dst_rect)
{
	MFDB 	*icon  = ( entry->preview.page ? &entry->preview.image[0] : entry->icon), 
			screen = {0};
	int16	xy[8], color[2] = { BLACK, WHITE};


	/* Source Rectangle */
	xy[0] = 0;
	xy[1] = 0;
	xy[2] = icon->fd_w - 1;
	xy[3] = icon->fd_h - 1;

	/* destination Rectangle */
	xy[4] = dst_rect->x1;
	xy[5] = dst_rect->y1;
	xy[6] = dst_rect->x2;
	xy[7] = dst_rect->y2;


	if ( icon->fd_nplanes == 1)
		vrt_cpyfm( handle, MD_REPLACE, xy, icon, &screen, color);
	else
		vro_cpyfm( handle, S_ONLY, xy, icon, &screen);
}


/*==================================================================================*
 * boolean icon_is_visible:															*
 *		check if the icon is visible.												*
 *----------------------------------------------------------------------------------*
 * input:																			*
 * 		entry 			-> the icon entry to be checked.							*
 * 		window_height	-> the window height.										*
 *----------------------------------------------------------------------------------*
 * returns: TRUE if visible else FALSE.												*
 *==================================================================================*/

boolean icon_is_visible( Entry *entry, int16 window_height)
{
	if ( ( entry->case_pos.y2 > 1) && ( entry->case_pos.y1 < window_height))
		return TRUE;
	
	return FALSE;
}
