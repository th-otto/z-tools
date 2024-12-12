/*************************************************************/
/*        Color Burst II ST/STe version 1.3                  */
/*             September 25th 1990                           */
/*************************************************************/
#include "cbnew.h"
#include <linea.h>
#include <fcntl.h>
#include <gemdefs.h>
#include <obdefs.h>
#include <osbind.h>
#include <stdio.h>
#define PAUSE(a) pause_flag=1;while(vcount<a);pause_flag=0
#define RING_BELL() Bconout(2,7)
#define HELP 0x6200
#define UNDO 0x6100
#define BACKSPACE 0x0E08
#define DELETE 0x537F
#define SPACE 0x3920
#define CNTL_C 0x2e03
#define CNTL_Z 0x2c1a
#define CNTL_Y 0x1519
#define CNTL_X 0x2D18
#define CNTL_T 0x1414
#define C      0x2E43
#define sC     0x2E63
#define T      0x1474
#define sT     0x1454
#define S      0x1F53
#define sS     0x1F73
#define M      0x324D
#define sM     0x326D
#define E      0x1245
#define sE     0x1265
#define CLEAR  0x4700
#define ESC 0x011B
#define F1  0x3B00
#define F2  0x3C00
#define F3  0x3D00
#define F4  0x3E00
#define F5  0x3F00
#define F6  0x4000
#define F7  0x4100
#define F8  0x4200
#define F9  0x4300
#define F10 0x4400

#define TRUE   1
#define SAVE_S 1
#define SAVE_Z 2
#define FALSE  0
#define NEW    2
#define OLD    1
#define NEITHER  0
#define LEFT     1
#define RIGHT    2
#define BOTH     3
#define NO_REDRAW  2
#define TRANSPARENT 2
#define OVERWRITE   1
#define LOW     0
#define MEDIUM  1
#define TEN     10						/* Max number of screens */

extern long save_A4();
extern int global[];

lineaport *myport;

OBJECT *dialog,
*help_1,
*help_2,
*help_3,
*help_4,
*credits,
*colorbox,
*colorset,
*drawmode,
*rom_vers;

int my_control[8000];
long test,
 my_buffer[8000];

char default_drive,
 filename[81],
 s[81];

int gl_hchar,
 gl_wchar,
 gl_wbox,
 gl_hbox,
 ap_id,
 message[8],
 handle,
 contrl[12],
 intin[128],
 ptsin[128],
 intout[128],
 spray_xr = 16,
	width,
	spray_yr = 8,
	ptsout[128],
	work_in[11],
	work_out[57],
	KEY,
	old_pen = 3,
	height = 0,
	vcount = 0,
	pause_flag,
	ptoggle = 1,
	copy_mode = 3,
	lwidth = 1,
	desk_palette[16],
	pic_palette[10][16],
	palette[16],
	max_num_screens,
	my_palette[16] = { 0x000, 0x700, 0x007, 0x777, 0x500, 0x252, 0x333,
	0x224, 0x727, 0x266, 0x047, 0x470, 0x407, 0x740, 0x074, 0x704
}, map[16] = { 0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1 }, text_h,
	medmap[4] = { 0, 2, 3, 1 }, cb1_palette[3200], cb2_palette[3200],
	cb_pal[10][3200], set_flag = LINE_S, active_pen, old_shade[16],
	linewmode = 1, textwmode = 2, textstyle = 0, demo_count = 0, max_shade,
	STe_rgb[16] = { 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15 },
	STe_exists, locked = TRUE, d1, d2, d3, d4, list[200];

long menu_address,
 MASK,
 SPRITE,
 SHOWN,
 HIDDEN,
 SCREEN[10],
 temp_line,
 RAW_HIDDEN;

int _1st_enable_register,
 _2nd_enable_register,
 hblank(),
 vblank(),
 _1st_mask_register,
 timer_setup(),
 clean_up(),
 pseudo_burst(),
 clear_vblank(),
 exit_to_desktop();
unsigned myseed;

char Linked[] = " Linked ",
	Unlinked[] = "Unlinked",
	Pal_linked[] = " Palettes Linked ",
	Pal_unlinked[] = "Palettes Unlinked";

static char exit_text[] = "[1][ | EXIT PROGRAM | Have you saved | your work? ][ EXIT | Cancel ]",
	clear_text[] = "[0][ | Clear Current Screen ?   | ][ Yes | No ]";

union
{
	int tos_version;
	char part[2];
} rom_version;

main()
{
	int back = TRUE,
		last = TRUE;

	setup();
	do_introduction();
	while (back != FALSE)
	{
		back = get_event(last);
		last = back;
	}
	exit_to_desktop();
}



setup()
{
	int I,
	 J,
	 K,
	 N,
	 r,
	 g,
	 b,
	 clip[4];

		/***** Basic ST Gem Set up *****/
	ap_id = appl_init();
	handle = graf_handle(&gl_wchar, &gl_hchar, &gl_wbox, &gl_hbox);
	for (I = 0; I < 10; work_in[I++] = 1) ;
	work_in[10] = 2;
	v_opnvwk(work_in, &handle, work_out);
	clip[0] = clip[1] = 0;
	clip[2] = work_out[0];
	clip[3] = work_out[1];
	vs_clip(handle, TRUE, clip);
	myseed = 1;
		/***** Load & Set Resource File *****/
	rsrc_load("cbnew.rsc");
	rsrc_gaddr(R_TREE, TOP_MENU, &menu_address);
	rsrc_gaddr(R_TREE, TOOL_BOX, &dialog);
	rsrc_gaddr(R_TREE, HELP_1, &help_1);
	rsrc_gaddr(R_TREE, HELP_2, &help_2);
	rsrc_gaddr(R_TREE, HELP_3, &help_3);
	rsrc_gaddr(R_TREE, HELP_4, &help_4);
	rsrc_gaddr(R_TREE, CREDITS, &credits);
	rsrc_gaddr(R_TREE, COLORBOX, &colorbox);
	rsrc_gaddr(R_TREE, DRAWMODE, &drawmode);
	rsrc_gaddr(R_TREE, ROM_VERS, &rom_vers);
		/***** Program Set up *****/
	myport = a_init();
	default_drive = Dgetdrv() + 'A';
	graf_mouse(ARROW, 0);
	v_hide_c(handle);
	for (I = 0; I < 16; desk_palette[I++] = Setcolor(I, -1)) ;
	if (Getrez() == 0)
	{
		I = my_palette[3];
		my_palette[3] = my_palette[15];
		my_palette[15] = I;
	}
	STe_exists = show_tos_version();	/* Does an STe exist? */
	vsf_color(handle, 3);
	vsf_interior(handle, 1);
	vsf_style(handle, 16);
	vsl_width(handle, 1);
	vr_recfl(handle, clip);
	vsf_color(handle, 2);
	vst_color(handle, 2);
	vsm_color(handle, 2);
	vsl_color(handle, 2);
	vsf_color(handle, 2);
	active_pen = 2;
	text_h = 6;
	shadow_text(30, 70, "Color Burst II ST/STe", 2, TRANSPARENT);
	shadow_text(30, 85, "Version 1.3 ", 2, TRANSPARENT);
		 /***** Allocate Screen Memory Buffers *****/
	max_num_screens = 0;
	SPRITE = ((Malloc(32768L) & 0xffff00) + 0x0100);
	MASK = ((Malloc(32768L) & 0xffff00) + 0x0100);
	clear_screen(SPRITE);
	clear_screen(MASK);
	SHOWN = Physbase();
	HIDDEN = SCREEN[0] = (((RAW_HIDDEN = Malloc(32768L)) & 0xffff00) + 0x0100);
	KEY = 0;
	for (I = 1; (I < TEN && SCREEN[I - 1] > 0x0200); I++)
	{
		SCREEN[I] = ((Malloc(32768L) & 0xffff00) + 0x0100);
		max_num_screens = I;
	}
	if (SCREEN[I] < 0x0200)
		max_num_screens--;
	copy_picture(SHOWN, SCREEN[0]);
	for (J = 0; J < 3200; J++)
	{
		r = my_palette[J % 16] & 0x0f00;
		g = my_palette[J % 16] & 0x00f0;
		b = my_palette[J % 16] & 0x000f;
		if (STe_exists == TRUE)
		{
			r = (r > 0x0700) ? 0x4000 + (r - 0x800) * 2 : r * 2;
			g = (g > 0x0070) ? 0x2000 + (g - 0x080) * 2 : g * 2;	/* STe mode        */
			b = (b > 0x0007) ? 0x1000 + (b - 0x008) * 2 : b * 2;
		} else
		{
			r *= 2;
			g *= 2;						/* normal ST mode */
			b *= 2;
		}
		cb_pal[0][J] = r + g + b;
	}
	for (I = 1; I <= max_num_screens; I++)
	{
		copy_picture(SHOWN, SCREEN[I]);
		for (J = 0; J < 3200; J++)
			cb_pal[I][J] = cb_pal[0][J];
	}
	N = 30;
	for (J = 2; J < 3200; J += 16)
	{
		b = N;
		if (N > 30)
			b = 60 - N;
		if (N > 60)
			b = N - 60;
		if (N > 90)
			b = 120 - N;
		if (N > 120)
			b = N - 120;
		if (N > 150)
			b = 180 - N;
		if (N > 180)
			b = N - 180;
		if (N > 210)
			b = 240 - N;
		N += 1;
		if (STe_exists == TRUE)
		{
			b = (b > 0x000f) ? 0x1000 + (b - 0x010) : b;
		} else
		{
			b = (b - 1) / 2;
		}
		cb_pal[0][J] = b;
	}
	stuff_current_cbpal(NEW);
	for (I = 0; I < 200; I++)
		list[I] = 0;
	reset_color(NEW);
	v_show_c(handle, 0);
}



exit_to_desktop()
{
	RING_BELL();
	Setpalette(desk_palette);
	v_show_c(handle, 0);
	Supexec(clear_vblank);
	v_clsvwk(handle);
	v_clrwk(handle);
	appl_exit();
}



do_introduction()
{
	int bx,
	 by,
	 bw,
	 bh,
	 I;

	form_center(credits, &bx, &by, &bw, &bh);
	v_hide_c(handle);
	form_dial(FMD_GROW, 0, 0, 10, 10, 10, 10, 30, 169);
	form_dial(FMD_GROW, 10, 169, 30, 30, work_out[0] - bw, 166, bw, 32);
	form_dial(FMD_GROW, work_out[0] - bw, 166, bw, 32, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	RING_BELL();
	objc_draw(credits, 0, 10, bx, by, bw, bh);
	goto_colorburst_mode(800);
	form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
	form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
	copy_picture(HIDDEN, SHOWN);
	v_show_c(handle, 0);
}



check_tool_box(x, y, bx, by, bw, bh)
int x,
 y,
 bx,
 by,
 bw,
 bh;
{
	int selection,
	 button,
	 d;

	selection = objc_find(dialog, 0, 1, x, y);
	switch (selection)
	{
	case CIRCLE:
	case BOX:
	case FREEHAND:
	case SPRAY:
	case STIPPLE:
	case LINEFAN:
	case ZOOM:
	case FILL:
	case TEXT:
	case COLORS:
	case CUT_SPR:
	case PASTE:
	case VERTICAL:
	case HORIZON:
	case MENU:
	case QUARTER:
	case HIDETOOL:
		v_hide_c(handle);
		form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
		form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
		copy_picture(HIDDEN, SHOWN);
		v_show_c(handle, 0);
		break;
	case LOCK_INF:
		if (locked == TRUE)
		{
			((TEDINFO *) dialog[LOCK_INF].ob_spec)->te_ptext = Unlinked;
			locked = FALSE;
		} else
		{
			((TEDINFO *) dialog[LOCK_INF].ob_spec)->te_ptext = Linked;
			locked = TRUE;
		}
		((TEDINFO *) dialog[LOCK_INF].ob_spec)->te_txtlen = 8;
		objc_offset(dialog, LOCK_INF, &d1, &d2);
		d3 = dialog[LOCK_INF].ob_width;
		d4 = dialog[LOCK_INF].ob_height;
		objc_draw(dialog, LOCK_INF, 1, d1, d2, d3, d4);
		while (button != NONE)
			vq_mouse(handle, &button, &x, &y);
	default:
		return (NO_REDRAW);
		break;
	}
	switch (selection)
	{
	case CIRCLE:
		make_ellipse();
		break;
	case BOX:
		make_box();
		break;
	case FREEHAND:
		free_hand();
		break;
	case SPRAY:
		spray_can();
		break;
	case STIPPLE:
		stippler();
		break;
	case LINEFAN:
		linefan();
		break;
	case TEXT:
		edit_text();
		break;
	case FILL:
		fill_it();
		break;
	case CUT_SPR:
		edit();
		break;
	case PASTE:
		paste_sprite();
		break;
	case VERTICAL:
		flip_vertical();
		break;
	case HORIZON:
		flip_horizontal();
		break;
	case QUARTER:
		quarter_screen();
		break;
	case MENU:
		if (get_menu() == FALSE)
			return (FALSE);
		break;
	case HIDETOOL:
		button = NONE;
		goto_colorburst_mode(1000);
		break;
	case ZOOM:
		zoom_edit();
		break;
	case COLORS:
		set_pen();
		break;
	}
	graf_mouse(ARROW, 0);
	copy_picture(SHOWN, HIDDEN);
	return (TRUE);
}



get_menu()
{
	int event,
	 message[8],
	 x,
	 y,
	 button,
	 state,
	 key,
	 n,
	 I;
	unsigned size;
	long off[2];

	v_hide_c(handle);
	copy_picture(HIDDEN, SHOWN);
	menu_bar(menu_address, TRUE);
	v_show_c(handle, 0);
	clear_keyboard();
	for (;;)
	{
		event = evnt_multi(MU_MESAG | MU_BUTTON | MU_KEYBD, 1, 3, 1, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, message, 0, 0, &x, &y, &button, &state, &key, &n);
		if (event == MU_MESAG && message[0] == 10)
		{
			v_hide_c(handle);
			menu_tnormal(menu_address, message[3], 1);
			menu_bar(menu_address, FALSE);
			copy_picture(HIDDEN, SHOWN);
			v_show_c(handle, 0);
			switch (message[4])
			{
			case TITLE:
				show_credits();
				break;
			case PICTUREL:
			case NEO:
			case DEGAS:
				if (select_picture(message[4], &size, off) != FALSE)
					load_picture(size, off);
				copy_picture(HIDDEN, SHOWN);
				break;
			case QUIT:
				return (FALSE);
				break;
			case S_NEO:
			case S_DEGAS:
			case PICTURES:
				if (select_picture(message[4], &size, off) != FALSE)
					save_picture(size, off);
				copy_picture(HIDDEN, SHOWN);
				break;
			case PROGRAM:
				call_other_program();
				break;
			case HIDE:
				return (TRUE);
				break;
			}
			v_hide_c(handle);
			menu_bar(menu_address, TRUE);
			v_show_c(handle, 0);
		}
		if (event == MU_BUTTON)
		{
			v_hide_c(handle);
			menu_bar(menu_address, FALSE);
			copy_picture(HIDDEN, SHOWN);
			menu_bar(menu_address, TRUE);
			v_show_c(handle, 0);
		}
		if (event == MU_KEYBD)
		{
			v_hide_c(handle);
			menu_bar(menu_address, FALSE);
			copy_picture(HIDDEN, SHOWN);
			v_show_c(handle, 0);
			switch (key)
			{
			case CNTL_C:
			case CNTL_Z:
			case CNTL_Y:
			case ESC:
				return (FALSE);
				break;
			case T:
			case sT:
				return (TRUE);
				break;
			case S:
			case sS:
			case C:
			case sC:
			case E:
			case sE:
				set_pen();
				break;
			case CLEAR:
				if (form_alert(1, clear_text) == 1)
				{
					clear_screen(HIDDEN);
					copy_picture(HIDDEN, SHOWN);
				}
				break;
			case SPACE:
				copy_picture(HIDDEN, SHOWN);
				goto_colorburst_mode(400);
				break;
			case BACKSPACE:
				for (I = 0; I < 16; I++)
					palette[I] = my_palette[I];
				Setpal(palette);
				break;
			case HELP:
				display_help();
				break;
			case F1:					/* Change Active Buffer */
			case F2:
			case F3:
			case F4:
			case F5:
			case F6:					/* 'key' becomes active */
			case F7:					/* buffer.              */
			case F8:
			case F9:
			case F10:
				trade_buffers(key, x, y, 40, 40, 2, 2);
				break;
			}
			v_hide_c(handle);
			menu_bar(menu_address, TRUE);
			v_show_c(handle, 0);
		}
	}
}



get_event(last)
int last;
{
	int event,
	 message[8],
	 x,
	 y,
	 button,
	 state,
	 key,
	 n,
	 I,
	 bx,
	 by,
	 bw,
	 bh,
	 sx,
	 sy,
	 sw,
	 sh;

	if (last == TRUE)
	{
		form_center(dialog, &bx, &by, &bw, &bh);
		sx = work_out[0] - 31;
		sy = 160;
		v_hide_c(handle);
		form_dial(FMD_GROW, 0, 0, 10, 10, sx, sy, 30, 30);
		form_dial(FMD_GROW, sx, sy, 30, 30, sx - 20, 90, 50, 50);
		form_dial(FMD_GROW, sx - 20, 90, 50, 50, bx, by, bw, bh);
		form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
		if (locked == TRUE)
		{
			((TEDINFO *) dialog[LOCK_INF].ob_spec)->te_ptext = Linked;
		} else
		{
			((TEDINFO *) dialog[LOCK_INF].ob_spec)->te_ptext = Unlinked;
		}
		((TEDINFO *) dialog[LOCK_INF].ob_spec)->te_txtlen = 8;
		objc_draw(dialog, 0, 10, bx, by, bw, bh);
		vq_mouse(handle, &button, &x, &y);
		v_show_c(handle, 0);
	}
	clear_keyboard();
	event = evnt_multi(MU_BUTTON | MU_KEYBD, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, message, 0, 0, &x, &y, &button, &state, &key, &n);
	if (event == MU_BUTTON)
	{
		switch (check_tool_box(x, y, bx, by, bw, bh))
		{
		case FALSE:
			if (form_alert(1, exit_text) == 1)
				return (FALSE);
			return (TRUE);
			break;
		case TRUE:
			return (TRUE);
			break;
		}
	}
	if (event == MU_KEYBD)
	{
		switch (key)
		{
		case CNTL_C:
		case CNTL_Z:
		case CNTL_Y:
		case ESC:
			if (form_alert(1, exit_text) == 1)
				return (FALSE);
			break;
		case SPACE:
			copy_picture(HIDDEN, SHOWN);
			return (goto_colorburst_mode(400));
			break;
		case C:
		case sC:
		case E:
		case sE:
		case S:
		case sS:
			form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
			form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
			copy_picture(HIDDEN, SHOWN);
			set_pen();
			return (TRUE);
			break;
		case M:
		case sM:
			form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
			form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
			copy_picture(HIDDEN, SHOWN);
			return (get_menu());
			break;
		case CLEAR:
			if (form_alert(1, clear_text) == 1)
			{
				form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
				form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
				clear_screen(HIDDEN);
				copy_picture(HIDDEN, SHOWN);
				return (TRUE);
			}
			break;
		case BACKSPACE:
			for (I = 0; I < 16; I++)
				palette[I] = my_palette[I];
			Setpal(palette);
			break;
		case HELP:
			display_help();
			return (TRUE);
			break;
		case F1:						/* Change Active Buffer */
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:						/* 'key' becomes active */
		case F7:						/* buffer.              */
		case F8:
		case F9:
		case F10:
			return (trade_buffers(key, x, y, bx, by, bw, bh));
			break;
		}
	}
	return (NO_REDRAW);
}



clear_keyboard()
{
	int event,
	 message[8],
	 x,
	 y,
	 button,
	 state,
	 key,
	 n;

	while (event != MU_TIMER)
		event = evnt_multi(MU_BUTTON | MU_KEYBD | MU_TIMER, 1, 3, 1, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, message, 300, 0, &x, &y, &button, &state, &key, &n);
}




set_drawing_mode()
{
	int bx,
	 by,
	 bw,
	 bh,
	 cw,
	 ch,
	 button,
	 x,
	 y,
	 selection;

	v_hide_c(handle);
	form_center(drawmode, &bx, &by, &bw, &bh);
	form_dial(FMD_GROW, 0, work_out[1], 10, 10, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	objc_draw(drawmode, 0, 10, bx, by, bw, bh);
	v_show_c(handle, 0);
	vq_mouse(handle, &button, &x, &y);
	while (button != BOTH)
	{
		vq_mouse(handle, &button, &x, &y);
		if (button == LEFT)
		{
			selection = objc_find(drawmode, 0, 1, x, y);
			switch (selection)
			{
			case DM_DRAWO:
				set_object(drawmode, selection, CHECKED);
				set_object(drawmode, DM_DRAWT, NORMAL);
				linewmode = 1;
				vswr_mode(handle, linewmode);
				break;
			case DM_DRAWT:
				set_object(drawmode, selection, CHECKED);
				set_object(drawmode, DM_DRAWO, NORMAL);
				linewmode = 2;
				vswr_mode(handle, linewmode);
				break;
			case DM_TEXTO:
				set_object(drawmode, selection, CHECKED);
				set_object(drawmode, DM_TEXTT, NORMAL);
				textwmode = 1;
				break;
			case DM_TEXTT:
				set_object(drawmode, selection, CHECKED);
				set_object(drawmode, DM_TEXTO, NORMAL);
				textwmode = 2;
				break;
			case DM_TEXTN:
				set_object(drawmode, selection, CHECKED);
				set_object(drawmode, DM_TEXTI, NORMAL);
				set_object(drawmode, DM_TEXTL, NORMAL);
				textstyle = 0;
				vst_effects(handle, 0);
				break;
			case DM_TEXTI:
				set_object(drawmode, selection, CHECKED);
				set_object(drawmode, DM_TEXTN, NORMAL);
				textstyle = textstyle | 0x004;
				vst_effects(handle, textstyle);
				break;
			case DM_TEXTL:
				set_object(drawmode, selection, CHECKED);
				set_object(drawmode, DM_TEXTN, NORMAL);
				textstyle = textstyle | 0x010;
				vst_effects(handle, textstyle);
				break;
			case DM_DONE:
				button = BOTH;
				break;
			}
		}
	}
	form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
	v_hide_c(handle);
	form_center(colorbox, &bx, &by, &bw, &bh);
	form_dial(FMD_GROW, 0, work_out[1], 10, 10, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	objc_draw(colorbox, 0, 10, bx, by, bw, bh);
	v_show_c(handle, 0);
	return;
}


set_pen()
{
	int bx,
	 by,
	 bw,
	 bh,
	 cw,
	 ch,
	 button,
	 x,
	 y,
	 w,
	 h,
	 r[4],
	 selection,
	 c,
	 I,
	 attrib[6],
	 temp,
	 changed = FALSE;

	v_hide_c(handle);
	form_center(colorbox, &bx, &by, &bw, &bh);
	form_dial(FMD_GROW, 0, work_out[1], 10, 10, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	if (locked == TRUE)
	{
		((TEDINFO *) colorbox[PAL_LOCK].ob_spec)->te_ptext = Pal_linked;
	} else
	{
		((TEDINFO *) colorbox[PAL_LOCK].ob_spec)->te_ptext = Pal_unlinked;
	}
	((TEDINFO *) colorbox[PAL_LOCK].ob_spec)->te_txtlen = 17;
	objc_draw(colorbox, 0, 10, bx, by, bw, bh);
	cw = colorbox[COLOR_1].ob_width;	/* Get size of color blocks */
	ch = colorbox[COLOR_1].ob_height;
	c = ((int) colorbox[old_pen].ob_spec) & 0x000f;
	set_rgb(colorbox, old_pen, set_flag);
	set_object(colorbox, old_pen, CHECKED);
	if (c == 1)
	{
		set_object(colorbox, old_pen, NORMAL);
		set_object(colorbox, old_pen, DISABLED);
	}
	set_object(colorbox, set_flag, SELECTED);
	switch (set_flag)
	{
	case SELECT_F:
		set_fill();
		break;
	case SELECT_T:
		set_text();
		break;
	case SELECT_S:
		break;
	case LINE_W:
	case LINE_S:
		set_line();
		break;
	}
	v_show_c(handle, 0);
	vq_mouse(handle, &button, &x, &y);
	while (button != BOTH)
	{
		if (Bconstat(2))
			Bconin(2);
		vq_mouse(handle, &button, &x, &y);
		if (button == LEFT)
		{
			if (objc_find(colorbox, 0, 1, x, y) == MORE)
			{
				form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
				set_drawing_mode();
				form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
				set_rgb(colorbox, old_pen, set_flag);
				switch (set_flag)
				{
				case SELECT_F:
					set_fill();
					break;
				case SELECT_T:
					set_text();
					break;
				case SELECT_S:
					break;
				case LINE_W:
				case LINE_S:
					set_line();
					break;
				}
			}
			selection = objc_find(colorbox, 0, 1, x, y);
			switch (selection)
			{
			case SELECT_F:
			case SELECT_S:
			case SELECT_T:
			case LINE_W:
			case LINE_S:
				set_object(colorbox, set_flag, NORMAL);
				set_object(colorbox, LINE_S, NORMAL);
				set_flag = selection;
				set_object(colorbox, set_flag, SELECTED);
				while (button != NONE)
					vq_mouse(handle, &button, &x, &y);
				break;
			}
			switch (selection)
			{
			case SELECT_F:
				set_fill();
				break;
			case SELECT_T:
				set_text();
				break;
			case SELECT_S:
				copy_picture(SHOWN, MASK);
				v_hide_c(handle);
				r[0] = (Getrez() == 0 ? 80 : 160);
				r[1] = (Getrez() == 2 ? 100 : 50);
				r[2] = 2 * r[0];
				r[3] = 2 * r[1];
				objc_offset(colorbox, SELECT_S, &x, &y);
				w = colorbox[SELECT_S].ob_width;
				h = colorbox[SELECT_S].ob_height;
				form_dial(FMD_GROW, x, y, w, h, r[0], r[1], r[2], r[3]);
				vqf_attributes(handle, attrib);
				vsf_interior(handle, 0);
				vsf_style(handle, 0);
				temp = r[1] + 1;
				r[1] = r[1] + r[3] + 1;
				r[2] = r[0] + r[2] + 1;
				r[3] = r[3] - temp;
				r[0] = r[0] - 1;
				v_rfbox(handle, r);
				select_sprayer();
				vsf_interior(handle, attrib[0]);
				vsf_style(handle, attrib[2]);
				set_object(colorbox, SELECT_S, NORMAL);
				v_show_c(handle, 0);
				break;
			case LINE_W:
				set_line();
				break;
			case LINE_S:
				vsl_width(handle, lwidth = 1);
				set_line();
				break;
			case COLOR_UP:
				advance_selection(set_flag);
				break;
			case COL_DOWN:
				decrement_selection(set_flag);
				break;
			case R_DOWN:
			case G_DOWN:
			case B_DOWN:
			case R_UP:
			case G_UP:
			case B_UP:
				set_rgb(colorbox, old_pen, selection);
				changed == TRUE;
				for (I = 0; I < 5; I++)
					Vsync();
				break;
			case PAL_LOCK:
				if (locked == TRUE)
				{
					((TEDINFO *) colorbox[PAL_LOCK].ob_spec)->te_ptext = Pal_unlinked;
					locked = FALSE;
				} else
				{
					((TEDINFO *) colorbox[PAL_LOCK].ob_spec)->te_ptext = Pal_linked;
					locked = TRUE;
				}
				((TEDINFO *) colorbox[PAL_LOCK].ob_spec)->te_txtlen = 17;
				objc_offset(colorbox, PAL_LOCK, &d1, &d2);
				d3 = colorbox[PAL_LOCK].ob_width;
				d4 = colorbox[PAL_LOCK].ob_height;
				objc_draw(colorbox, PAL_LOCK, 1, d1, d2, d3, d4);
				while (button != NONE)
					vq_mouse(handle, &button, &x, &y);
				break;
			case DONE_CB:
				v_hide_c(handle);
				set_object(colorbox, set_flag, NORMAL);
				set_object(colorbox, LINE_S, NORMAL);
				form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
				form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
				if (locked == TRUE)
					reset_color(NEW);
				copy_picture(HIDDEN, SHOWN);
				for (I = 0; I < 15; I++)
					Vsync();
				while (button != NONE)
					vq_mouse(handle, &button, &x, &y);
				return;
				break;
			default:
				if (colorbox[selection].ob_width == cw && colorbox[selection].ob_height == ch)
				{
					v_hide_c(handle);
					c = Getrez() == 1 ? medmap[a_getpixel(x, y)] : map[a_getpixel(x + 2, y)];
					v_show_c(handle, 0);
					set_object(colorbox, old_pen, NORMAL);
					set_object(colorbox, selection, CHECKED);
					if (c == 1)
					{
						set_object(colorbox, selection, NORMAL);
						set_object(colorbox, selection, DISABLED);
					}
					if (locked == TRUE && changed == TRUE)
						reset_color(NEW);
					changed = FALSE;
					old_pen = selection;
					set_rgb(colorbox, old_pen, selection);
					vsf_color(handle, c);
					vst_color(handle, c);
					vsm_color(handle, c);
					vsl_color(handle, c);
					switch (set_flag)
					{
					case SELECT_F:
						set_fill();
						break;
					case SELECT_T:
						set_text();
						break;
					case SELECT_S:
						break;
					case LINE_W:
						set_line();
						break;
					case LINE_S:
						set_line();
						break;
					}
					while (button != NEITHER)
						vq_mouse(handle, &button, &x, &y);
					break;
				}
			}
		}
	}
	v_hide_c(handle);
	set_object(colorbox, set_flag, NORMAL);
	set_object(colorbox, LINE_S, NORMAL);
	form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
	form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
	if (locked == TRUE)
		reset_color(NEW);
	copy_picture(HIDDEN, SHOWN);
	for (I = 0; I < 15; I++)
		Vsync();
	while (button != NONE)
		vq_mouse(handle, &button, &x, &y);
}




show_credits()
{
	int bx,
	 by,
	 bw,
	 bh,
	 I,
	 x,
	 y,
	 button;

	form_center(credits, &bx, &by, &bw, &bh);
	v_hide_c(handle);
	form_dial(FMD_GROW, 0, 0, 10, 10, 10, 10, 30, 169);
	form_dial(FMD_GROW, 10, 169, 30, 30, work_out[0] - bw, 166, bw, 32);
	form_dial(FMD_GROW, work_out[0] - bw, 166, bw, 32, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	objc_draw(credits, 0, 10, bx, by, bw, bh);
	v_show_c(handle, 0);
	for (I = 0; I < 30; I++)
		Vsync();
	vq_mouse(handle, &button, &x, &y);
	while (button == NONE)
		vq_mouse(handle, &button, &x, &y);
	v_hide_c(handle);
	form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
	form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
	copy_picture(HIDDEN, SHOWN);
	v_show_c(handle, 0);
}





make_ellipse()
{
	int p[4],
	 button,
	 I,
	 hold_old,
	 hold_new,
	 II,
	 Ilow,
	 Ihigh;

	hold_old = active_pen;
	for (I = 0; I < 200; I++)
		list[I] = 0;
	graf_mouse(THIN_CROSS, 0);
	v_show_c(handle, 0);
	for (;;)
	{
		button = NONE;
		while (button == NONE || button == RIGHT)
		{
			if (check_keyboard(SAVE_S) == TRUE)
			{
				if (locked == FALSE)
					reset_color(NEW);
				for (I = 0; I < 200; I++)
					list[I] = 0;
				set_pen();
			}
			vq_mouse(handle, &button, &p[0], &p[1]);
			p[2] = p[0];
			p[3] = p[1];
		}
		if (button == BOTH)
		{
			if (locked == FALSE)
				reset_color(NEW);
			return (TRUE);
		}
		if (rubber_ellipse(p[0], p[1], &p[2], &p[3]) == TRUE)
		{
			v_hide_c(handle);
			v_ellarc(handle, p[0], p[1], p[2] - p[0], p[3] - p[1], 0, 7200);
			if (p[1] > p[3])
			{
				p[0] = p[1];			/* hold p[1] temporarily */
				p[1] = p[3];
				p[3] = p[0];
			}
			p[0] = p[3] - p[1];
			p[1] = p[1] - p[0];
			if (p[1] < 0)
				p[1] = 0;
			II = (lwidth + 1) / (2 * (1 + Getrez()));
			if (II > 3)
				II -= 1;
			if (Getrez() == 0 && (II < 5 || II > 6))
				II -= 1;
			Ilow = (p[1] - II < 0 ? 0 : p[1] - II);
			Ihigh = (p[3] + II > 199 ? 199 : p[3] + II);
			for (I = Ilow; I < Ihigh + 1; I++)
				list[I] = 1;
			v_show_c(handle, 0);
		}
	}
}





make_box()
{
	int p[4],
	 box_array[10],
	 button,
	 I,
	 hold_old,
	 hold_new,
	 Ilow,
	 Ihigh,
	 II;

	hold_old = active_pen;
	for (I = 0; I < 200; I++)
		list[I] = 0;
	graf_mouse(THIN_CROSS, 0);
	v_show_c(handle, 0);
	for (;;)
	{
		button = NONE;
		while (button == NONE || button == RIGHT)
		{
			if (check_keyboard(SAVE_S) == TRUE)
			{
				if (locked == FALSE)
					reset_color(NEW);
				for (I = 0; I < 200; I++)
					list[I] = 0;
				set_pen();
			}
			vq_mouse(handle, &button, &p[0], &p[1]);
			p[2] = p[0];
			p[3] = p[1];
		}
		if (button == BOTH)
		{
			if (locked == FALSE)
				reset_color(NEW);
			return (TRUE);
		}
		if (rubber_box(p[0], p[1], &p[2], &p[3]) == TRUE)
		{
			v_hide_c(handle);
			box_array[0] = box_array[6] = box_array[8] = p[0];
			box_array[1] = box_array[3] = box_array[9] = p[1];
			box_array[2] = box_array[4] = p[2];
			box_array[5] = box_array[7] = p[3];
			v_pline(handle, 5, box_array);
			if (p[1] > p[3])
			{
				p[0] = p[1];			/* hold p[1] temporarily */
				p[1] = p[3];
				p[3] = p[0];
			}
			II = (lwidth + 1) / (2 * (1 + Getrez()));
			if (II > 3)
				II -= 1;
			if (Getrez() == 0 && (II < 5 || II > 6))
				II -= 1;
			Ilow = (p[1] - II < 0 ? 0 : p[1] - II);
			Ihigh = (p[3] + II > 199 ? 199 : p[3] + II);
			for (I = Ilow; I < Ihigh + 1; I++)
				list[I] = 1;
			v_show_c(handle, 0);
		}
	}
}




free_hand()
{
	int p[4],
	 button = NONE,
		I,
		first,
		last,
		hold_old,
		hold_new,
		II,
		Ilow,
		Ihigh;
	unsigned form[37];

	form[0] = 2;
	form[1] = 15;
	form[2] = 1;
	form[3] = 0;
	form[4] = 1;
	stuffbits(&form[5], "0000000000000000");
	stuffbits(&form[6], "0000000000001101");
	stuffbits(&form[7], "0000000000011010");
	stuffbits(&form[8], "0000000000110100");
	stuffbits(&form[9], "0000000001101000");
	stuffbits(&form[10], "0000000011010000");
	stuffbits(&form[11], "0000000110100000");
	stuffbits(&form[12], "0000001101000000");
	stuffbits(&form[13], "0000011010000000");
	stuffbits(&form[14], "0000110100000000");
	stuffbits(&form[15], "0001101000000000");
	stuffbits(&form[16], "0011010000000000");
	stuffbits(&form[17], "0011100000000000");
	stuffbits(&form[18], "1111110000000000");
	stuffbits(&form[19], "1111100000000000");
	stuffbits(&form[20], "0110000000000000");
	for (I = 21; I < 37; I++)
		form[I] = form[I - 16];
	graf_mouse(255, form);
	hold_old = active_pen;
	for (I = 0; I < 200; I++)
		list[I] = 0;
	v_show_c(handle, 0);
	vq_mouse(handle, &button, &p[0], &p[1]);
	while (button != BOTH)
	{
		p[0] = p[2];
		p[1] = p[3];
		vq_mouse(handle, &button, &p[2], &p[3]);
		Vsync();
		if (check_keyboard(SAVE_S) == TRUE)
		{
			if (locked == FALSE)
				reset_color(NEW);
			for (I = 0; I < 200; I++)
				list[I] = 0;
			set_pen();
		}
		if (button == LEFT)
		{
			v_hide_c(handle);
			v_pline(handle, 2, p);
			if (p[1] > p[3])
			{
				first = p[3];
				last = p[1];
			} else
			{
				first = p[1];
				last = p[3];
			}
			II = (lwidth + 1) / (2 * (1 + Getrez()));
			if (II > 3)
				II -= 1;
			if (Getrez() == 0 && (II < 5 || II > 6))
				II -= 1;
			Ilow = (first - II < 0 ? 0 : first - II);
			Ihigh = (last + II > 199 ? 199 : last + II);
			for (I = Ilow; I < Ihigh + 1; I++)
				list[I] = 1;
			v_show_c(handle, 0);
		}
	}
	if (locked == FALSE)
		reset_color(NEW);
	return (TRUE);
}



stippler()
{
	int p[2],
	 rez,
	 button = NONE;
	int I,
	 r,
	 x,
	 y,
	 x2,
	 y2,
	 climit,
	 rlimit,
	 c;
	float a,
	 b;
	unsigned form[37];

	form[0] = 8;
	form[1] = 7;
	form[2] = 1;
	form[3] = 0;
	form[4] = 1;
	stuffbits(&form[5], "0000000000000000");
	stuffbits(&form[6], "0000000000000000");
	stuffbits(&form[7], "0000000000000000");
	stuffbits(&form[8], "0000000000000000");
	stuffbits(&form[9], "0000001110000000");
	stuffbits(&form[10], "0001100010110000");
	stuffbits(&form[11], "0110010000101100");
	stuffbits(&form[12], "1101000100010110");
	stuffbits(&form[13], "0110010000101100");
	stuffbits(&form[14], "0001100010110000");
	stuffbits(&form[15], "0000001110000000");
	stuffbits(&form[16], "0000000000000000");
	stuffbits(&form[17], "0000000000000000");
	stuffbits(&form[18], "0000000000000000");
	stuffbits(&form[19], "0000000000000000");
	stuffbits(&form[20], "0000000000000000");
	for (I = 21; I < 37; I++)
		form[I] = form[I - 16];
	graf_mouse(255, form);
	r = spray_xr * spray_yr;
	a = (float) spray_yr / (float) spray_xr;
	b = (float) spray_xr / (float) spray_yr;
	climit = (rez == 0 ? 320 : 640);
	rlimit = (rez == 2 ? 400 : 200);
	while (button != BOTH)
	{
		if (check_keyboard(SAVE_S) == TRUE)
		{
			r = spray_xr * spray_yr;
			a = (float) spray_yr / (float) spray_xr;
			b = (float) spray_xr / (float) spray_yr;
		}
		v_show_c(handle, 0);
		vq_mouse(handle, &button, &p[0], &p[1]);
		if (button == LEFT)
			v_hide_c(handle);
		while (button == LEFT)
		{
			vq_mouse(handle, &button, &p[0], &p[1]);

			do
			{
				x = (spray_xr - (abs(rand()) % (2 * spray_xr + 1)));
				y = (spray_yr - (abs(rand()) % (2 * spray_yr + 1)));
			} while ((int) (x * x * a + y * y * b) > r);
			do
			{
				x2 = (spray_xr - (abs(rand()) % (2 * spray_xr + 1)));
				y2 = (spray_yr - (abs(rand()) % (2 * spray_yr + 1)));
			} while ((int) (x2 * x2 * a + y2 * y2 * b) > r);
			x = p[0] + x;
			y = p[1] + y;
			x2 = p[0] + x2;
			y2 = p[1] + y2;
			if (x > -1 && x < climit && y > -1 && y < rlimit && x2 > -1 && x2 < climit && y2 > -1 && y2 < rlimit)
			{
				c = a_getpixel(x, y);
				a_putpixel(x, y, a_getpixel(x2, y2));
				a_putpixel(x2, y2, c);
			}
		}
	}
	v_show_c(handle, 0);
	return (TRUE);
}





spray_can()
{
	int p[2],
	 rez,
	 button = NONE,
		hold_old,
		hold_new,
		Ilow,
		Ihigh;
	int I,
	 x,
	 y,
	 r,
	 climit,
	 rlimit,
	 c,
	 medmap[4],
	 lowmap[16],
	 attrib[10];
	float a,
	 b;
	unsigned form[37];

	hold_old = active_pen;
	for (I = 0; I < 200; I++)
		list[I] = 0;
	form[0] = 8;
	form[1] = 3;
	form[2] = 1;
	form[3] = 0;
	form[4] = 1;
	stuffbits(&form[5], "0000001110000000");
	stuffbits(&form[6], "0001100010110000");
	stuffbits(&form[7], "0110010000101100");
	stuffbits(&form[8], "1101010100110110");
	stuffbits(&form[9], "0110001111001100");
	stuffbits(&form[10], "0001101111010000");
	stuffbits(&form[11], "0000001110000000");
	stuffbits(&form[12], "0000000110000000");
	stuffbits(&form[13], "0001111000110000");
	stuffbits(&form[14], "0001111000010000");
	stuffbits(&form[15], "0001111000010000");
	stuffbits(&form[16], "0001111000010000");
	stuffbits(&form[17], "0001111000010000");
	stuffbits(&form[18], "0001111000010000");
	stuffbits(&form[19], "0001111000010000");
	stuffbits(&form[20], "0001111111110000");
	for (I = 21; I < 37; I++)
		form[I] = form[I - 16];
	graf_mouse(255, form);
	a = (float) spray_yr / (float) spray_xr;
	b = (float) spray_xr / (float) spray_yr;
	r = spray_xr * spray_yr;
	medmap[0] = 0;
	medmap[1] = 3;
	medmap[2] = 1;
	medmap[3] = 2;
	lowmap[0] = 0;
	lowmap[1] = 15;
	lowmap[2] = 1;
	lowmap[3] = 2;
	lowmap[4] = 4;
	lowmap[5] = 6;
	lowmap[6] = 3;
	lowmap[7] = 5;
	lowmap[8] = 7;
	lowmap[9] = 8;
	lowmap[10] = 9;
	lowmap[11] = 10;
	lowmap[12] = 12;
	lowmap[13] = 14;
	lowmap[14] = 11;
	lowmap[15] = 13;
	rez = Getrez();
	vqt_attributes(handle, attrib);
	c = (rez ? medmap[attrib[1]] : lowmap[attrib[1]]);
	climit = (rez == 0 ? 320 : 640);
	rlimit = (rez == 2 ? 400 : 200);
	while (button != BOTH)
	{
		v_show_c(handle, 0);
		vq_mouse(handle, &button, &p[0], &p[1]);
		if (check_keyboard(SAVE_S) == TRUE)
		{
			if (locked == FALSE)
				reset_color(NEW);
			for (I = 0; I < 200; I++)
				list[I] = 0;
			set_pen();
			vqt_attributes(handle, attrib);
			c = (rez ? medmap[attrib[1]] : lowmap[attrib[1]]);
			r = spray_xr * spray_yr;
			a = (float) spray_yr / (float) spray_xr;
			b = (float) spray_xr / (float) spray_yr;
		}
		if (button == LEFT)
			v_hide_c(handle);
		while (button == LEFT)
		{
			vq_mouse(handle, &button, &p[0], &p[1]);
			Ilow = (p[1] - spray_yr < 0 ? 0 : p[1] - spray_yr);
			Ihigh = (p[1] + spray_yr > 200 ? 200 : p[1] + spray_yr + 1);
			for (I = Ilow; I < Ihigh; I++)
				list[I] = 1;
			do
			{
				x = spray_xr - (abs(rand()) % (2 * spray_xr + 1));
				y = spray_yr - (abs(rand()) % (2 * spray_yr + 1));
			} while ((x * x * a + y * y * b) > r);
			p[0] = p[0] + x;
			p[1] = p[1] + y;
			if (p[0] > -1 && p[0] < climit && p[1] > -1 && p[1] < rlimit)
				a_putpixel(p[0], p[1], c);
		}
	}
	v_show_c(handle, 0);
	if (locked == FALSE)
		reset_color(NEW);
	return (TRUE);
}




linefan()
{
	int p[6],
	 button,
	 x,
	 y,
	 test = 0,
		I,
		flag,
		first,
		last,
		hold_old,
		hold_new,
		II,
		Ilow,
		Ihigh;

	hold_old = active_pen;
	for (I = 0; I < 200; I++)
		list[I] = 0;
	last = 0;
	first = 200;
	flag = 0;
	graf_mouse(OUTLN_CROSS, 0);
	v_show_c(handle, 0);
	for (;;)
	{
		vq_mouse(handle, &button, &x, &y);
		if (check_keyboard(SAVE_S) == TRUE)
		{
			if (locked == FALSE)
				reset_color(NEW);
			for (I = 0; I < 200; I++)
				list[I] = 0;
			set_pen();
		}
		switch (button)
		{
		case RIGHT:		   /*** Set New Starting Point  ***/
			p[0] = p[4] = x;
			p[1] = p[5] = y;
			test = 1;
			break;
		case LEFT:			   /*** Draw A Line To This New ***/
			p[2] = x;		   /*** Ending Point.           ***/
			p[3] = y;
			if (test == 0)
			{
				test = 1;	   /*** Set initial starting point ***/
				p[0] = p[4] = x;
				p[1] = p[5] = y;
			}
			flag = 1;
			if (p[1] < first)
				first = p[1];
			if (p[3] < first)
				first = p[3];
			if (p[1] > last)
				last = p[1];
			if (p[3] > last)
				last = p[3];
			v_hide_c(handle);
			v_pline(handle, 3, p);
			v_show_c(handle, 0);
			break;
		case NEITHER:
			if (flag == 1)
			{
				II = (lwidth + 1) / (2 * (1 + Getrez()));
				if (II > 3)
					II -= 1;
				if (Getrez() == 0 && (II < 5 || II > 6))
					II -= 1;
				Ilow = (first - II < 0 ? 0 : first - II);
				Ihigh = (last + II > 199 ? 199 : last + II);
				for (I = Ilow; I < Ihigh + 1; I++)
					list[I] = 1;
				last = 0;
				first = 200;
				flag = 0;
			}
			break;
		case BOTH:
			if (locked == FALSE)
				reset_color(NEW);
			return (TRUE);
			break;
		}
	}
}



edit_text()
{
	int I,
	 x,
	 y,
	 button;
	char text[81];

	for (I = 0; I < 81; I++)
		text[I] = '\0';
	graf_mouse(TEXT_CRSR, 0);
	vswr_mode(handle, 3);
	v_show_c(handle, 0);
	for (;;)
	{
		button = NONE;
		I = 0;
		while (button == NONE)
		{
			vq_mouse(handle, &button, &x, &y);
			if (Bconstat(2) != FALSE)
			{
				text[I++] = (char) (Bconin(2) & 127);
				if ((int) text[I - 1] == 8 || (int) text[I - 1] == 127)
				{
					text[--I] = '\0';
					text[--I] = '\0';
				}
				if ((int) text[I - 1] == 0)
					text[--I] = '\0';
				if (I > 80)
					I = 80;
			}
			if (I > 0)
			{
				v_hide_c(handle);
				v_gtext(handle, x, y, text);
				Vsync();
				Vsync();
				Vsync();
				v_gtext(handle, x, y, text);
				Vsync();
			}
		}
		switch (button)
		{
		case BOTH:
			vswr_mode(handle, linewmode);
			return (TRUE);
			break;
		case LEFT:
			v_hide_c(handle);
			vswr_mode(handle, textwmode);
			v_gtext(handle, x, y, text);
			for (I = 0; I < 81; I++)
				text[I] = '\0';
			vswr_mode(handle, 3);
			v_show_c(handle, 0);
			break;
		case RIGHT:
			for (I = 0; I < 81; I++)
				text[I] = '\0';
			v_show_c(handle, 0);
			break;
		}
		v_show_c(handle, 0);
	}
}





fill_it()
{
	int p[4],
	 button,
	 I;
	unsigned form[37];

	form[0] = 15;
	form[1] = 15;
	form[2] = 1;
	form[3] = 1;
	form[4] = 1;
	stuffbits(&form[5], "0000000000000000");
	stuffbits(&form[6], "0000000000000000");
	stuffbits(&form[7], "0000000000000000");
	stuffbits(&form[8], "0000000100000000");
	stuffbits(&form[9], "0000000100000000");
	stuffbits(&form[10], "0000010100000000");
	stuffbits(&form[11], "0000100100000000");
	stuffbits(&form[12], "0001000101000000");
	stuffbits(&form[13], "0010000100010000");
	stuffbits(&form[14], "0100000100000100");
	stuffbits(&form[15], "1000000000001000");
	stuffbits(&form[16], "0010000000010010");
	stuffbits(&form[17], "0000100000100000");
	stuffbits(&form[18], "0000001001000010");
	stuffbits(&form[19], "0000000010000110");
	stuffbits(&form[20], "0000000000000000");
	stuffbits(&form[21], "0000000000000000");
	stuffbits(&form[22], "0000000000000000");
	stuffbits(&form[23], "0000000000000000");
	stuffbits(&form[24], "0000000100000000");
	stuffbits(&form[25], "0000000100000000");
	stuffbits(&form[26], "0000010100000000");
	stuffbits(&form[27], "0000111100000000");
	stuffbits(&form[28], "0001100101000000");
	stuffbits(&form[29], "0011000100010000");
	stuffbits(&form[30], "0110000110001100");
	stuffbits(&form[31], "1111111111111000");
	stuffbits(&form[32], "0011111111110010");
	stuffbits(&form[33], "0000111111100000");
	stuffbits(&form[34], "0000001111000010");
	stuffbits(&form[35], "0000000010000110");
	stuffbits(&form[36], "0000000000000000");
	graf_mouse(255, form);
	v_show_c(handle, 0);
	for (;;)
	{
		button = NONE;
		while (button == NONE || button == RIGHT)
		{
			vq_mouse(handle, &button, &p[0], &p[1]);
			check_keyboard(SAVE_S);
		}
		if (button == BOTH)
			return (TRUE);
		v_hide_c(handle);
		v_contourfill(handle, p[0], p[1], -1);
		v_show_c(handle, 0);
	}
}





flip_vertical()
{
	int I,
	 J,
	 temp;

	v_hide_c(handle);
	for (I = 0; I < 200; I++)
		for (J = 0; J < 40; J++)
			*((long *) SHOWN + I * 40 + J) = *((long *) HIDDEN + (199 - I) * 40 + J);
	for (I = 0; I < 100; I++)
		for (J = 0; J < 16; J++)
		{								/* flip colors too! */
			temp = cb_pal[KEY][J + I * 16];
			cb_pal[KEY][J + I * 16] = cb_pal[KEY][J + (199 - I) * 16];
			cb_pal[KEY][J + (199 - I) * 16] = temp;
		}
	stuff_current_cbpal(NEW);
	v_show_c(handle, 0);
}


flip_horizontal()
{
	register int I,
	 J,
	 K;
	register unsigned int ying,
	*shown,
	*hidden;
	int unit,
	 limit,
	 rez;

	shown = (unsigned int *) SHOWN;
	hidden = (unsigned int *) HIDDEN;
	v_hide_c(handle);
	rez = Getrez();
	unit = (rez == 0 ? 4 : (2 / rez));
	limit = (rez == 2 ? 400 : 200);
	for (I = 0; I < limit; I++)
		for (J = 0; J < 80; J += unit)
			for (K = 0; K < unit; K++)
			{
				ying = *(hidden + (I + 1) * 80 - unit - J + K);
				*(shown + I * 80 + J + K) = ((ying & 1) << 15) +
					((ying & 2) << 13) + ((ying & 4) << 11) + ((ying & 8) << 9) +
					((ying & 16) << 7) + ((ying & 32) << 5) + ((ying & 64) << 3) +
					((ying & 128) << 1) + ((ying & 256) >> 1) + ((ying & 512) >> 3) +
					((ying & 1024) >> 5) + ((ying & 2048) >> 7) + ((ying & 4096) >> 9) +
					((ying & 8192) >> 11) + ((ying & 16384) >> 13) + ((ying & 32768) >> 15);
			}
	v_show_c(handle, 0);
}



quarter_screen()
{
	register int I,
	 J,
	 dots_per_row;
	register long *shown;
	int unit,
	 limit,
	 rez;

	v_hide_c(handle);
	shown = (long *) SHOWN;
	rez = Getrez();
	limit = (rez == 2 ? 400 : 200);
	dots_per_row = (rez == 0 ? 320 : 640);
	for (I = 0; I < limit; I += 2)
		for (J = 0; J < dots_per_row; J += 2)
		{
			a_putpixel(J / 2, I / 2, a_getpixel(J, I));
		}
	for (I = 4000; I < 8000; I++)
		*(shown + I) = 0L;
	for (I = 0; I < 4000; I += 8000 / limit)
		for (J = 4000 / limit; J < 8000 / limit; J++)
			*(shown + I + J) = 0L;
	for (I = 0; I < 3200; I += 32)
		for (J = 0; J < 16; J++)
		{
			cb_pal[KEY][I / 2 + J] = cb_pal[KEY][I + J];
		}
	stuff_current_cbpal(OLD);
	v_show_c(handle, 0);
}





check_keyboard(flag)
int flag;
{
	char c;

	if (Bconstat(2) == FALSE)
		return (FALSE);
	c = (char) (Bconin(2) & 127);
	if (c == 'c' || c == 'C' || c == 's' || c == 'S' || c == 'e' || c == 'E')
	{
		switch (flag)
		{
		case SAVE_S:
			copy_picture(SHOWN, HIDDEN);
			break;
		case SAVE_Z:
			copy_picture(SHOWN, MASK);
			set_pen();
			copy_picture(MASK, SHOWN);
			break;
		}
		return (TRUE);
	}
	return (FALSE);
}





clear_screen(destination)
register long *destination;
{
	register int N;

	v_hide_c(handle);
	for (N = 0; (N++) < 8000; *(destination++) = 0L) ;
	v_show_c(handle, 0);
}


copy_picture(source, destination)
register long *source,
*destination;
{
	register int N;

	v_hide_c(handle);
	for (N = 0; (N++) < 8000; *(destination++) = *(source++)) ;
	v_show_c(handle, 0);
}




zoom_edit()
{
	int I,
	 p[4],
	 button = NONE;

	v_hide_c(handle);
	graf_mouse(POINT_HAND, 0);
	while (button != NONE)
		vq_mouse(handle, &button, &p[2], &p[3]);
	while (button != BOTH)
	{
		vq_mouse(handle, &button, &p[2], &p[3]);
		if (check_keyboard(FALSE) == TRUE)
		{
			set_pen();
			zoom_it(p[0], p[1]);
		}
		if (button == NONE)
		{
			copy_picture(HIDDEN, SHOWN);
			v_hide_c(handle);
			while (button == NONE)
			{
				p[0] = p[2];
				p[1] = p[3];
				vq_mouse(handle, &button, &p[2], &p[3]);
				graf_movebox((work_out[0] + 1) / 10, 25, p[2], p[3], p[2], p[3]);
			}
			if (button == LEFT)
				zoom_it(p[2], p[3]);
			v_show_c(handle, 0);
		}
	}
	copy_picture(HIDDEN, SHOWN);
	return (TRUE);
}



zoom_it(x, y)
int x,
 y;
{
	int I,
	 k,
	 c,
	 xmax;

	copy_picture(HIDDEN, SHOWN);
	v_hide_c(handle);
	xmax = (work_out[0] + 1) / 10;
	if (x > (work_out[0]) * 9 / 10)
		x = (work_out[0]) * 9 / 10;
	if (y > 174)
		y = 174;
	for (I = 0; I < xmax; I++)
	{
		for (k = 0; k < 25; k++)
		{
			Setscreen(HIDDEN, SHOWN, -1);
			c = a_getpixel(x + I, y + k);
			Setscreen(SHOWN, SHOWN, -1);
			a_putpixel(I * 3, k * 3, c);
			a_putpixel(I * 3 + 1, k * 3, c);
			a_putpixel(I * 3 + 2, k * 3, c);
			a_putpixel(I * 3, k * 3 + 1, c);
			a_putpixel(I * 3 + 1, k * 3 + 1, c);
			a_putpixel(I * 3 + 2, k * 3 + 1, c);
			a_putpixel(I * 3, k * 3 + 2, c);
			a_putpixel(I * 3 + 1, k * 3 + 2, c);
			a_putpixel(I * 3 + 2, k * 3 + 2, c);
		}
	}
	Setscreen(SHOWN, SHOWN, -1);
	v_show_c(handle, 0);
	if (edit_zoom(x, y) == FALSE)
		zoom_it(x, y);
	copy_picture(HIDDEN, SHOWN);
}




edit_zoom(x0, y0)
int x0,
 y0;
{
	int x,
	 y,
	 I,
	 button,
	 c,
	 medmap[4],
	 lowmap[16];
	int attrib[10];

	medmap[0] = 0;
	medmap[1] = 3;
	medmap[2] = 1;
	medmap[3] = 2;
	lowmap[0] = 0;
	lowmap[1] = 15;
	lowmap[2] = 1;
	lowmap[3] = 2;
	lowmap[4] = 4;
	lowmap[5] = 6;
	lowmap[6] = 3;
	lowmap[7] = 5;
	lowmap[8] = 7;
	lowmap[9] = 8;
	lowmap[10] = 9;
	lowmap[11] = 10;
	lowmap[12] = 12;
	lowmap[13] = 14;
	lowmap[14] = 11;
	lowmap[15] = 13;
	vq_mouse(handle, &button, &x, &y);
	while (button != RIGHT)
	{
		check_keyboard(SAVE_Z);
		vqt_attributes(handle, attrib);
		c = (Getrez()? medmap[attrib[1]] : lowmap[attrib[1]]);
		vq_mouse(handle, &button, &x, &y);
		if (button == LEFT)
		{
			if (x > (work_out[0] + 1) * 3 / 10 || y > 75)
			{
				RING_BELL();
			} else
			{
				x = x / 3;
				y = y / 3;
				v_hide_c(handle);
				Setscreen(HIDDEN, SHOWN, -1);
				a_putpixel(x0 + x, y0 + y, c);
				Setscreen(SHOWN, SHOWN, -1);
				if ((x0 + x) > (work_out[0] + 1) * 3 / 10 || (y0 + y) > 75)
					a_putpixel(x0 + x, y0 + y, c);
				a_putpixel(x * 3, y * 3, c);
				a_putpixel(x * 3 + 1, y * 3, c);
				a_putpixel(x * 3 + 2, y * 3, c);
				a_putpixel(x * 3, y * 3 + 1, c);
				a_putpixel(x * 3 + 1, y * 3 + 1, c);
				a_putpixel(x * 3 + 2, y * 3 + 1, c);
				a_putpixel(x * 3, y * 3 + 2, c);
				a_putpixel(x * 3 + 1, y * 3 + 2, c);
				a_putpixel(x * 3 + 2, y * 3 + 2, c);
				v_show_c(handle, 0);
			}
		}
	}
	return (TRUE);
}




shadow_text(X, Y, text, color, mode)
int X,
 Y,
 color,
 mode;
char text[40];
{
	int attrib[10],
	 d;

	vqt_attributes(handle, attrib);
	vst_height(handle, 6, &d, &d, &d, &d);
	v_hide_c(handle);
	vswr_mode(handle, mode);			/* SET WRITE MODE    */
	vst_color(handle, 0);				/* SHADOW COLOR=0    */
	v_gtext(handle, X + 2, Y + 1, text);
	vst_color(handle, color);
	v_gtext(handle, X, Y, text);
	vst_color(handle, attrib[1]);		/* RESET OLD COLOR   */
	vswr_mode(handle, attrib[5]);		/* RESET OLD MODE    */
	vst_height(handle, text_h, &d, &d, &d, &d);
	v_show_c(handle, 1);
}


trade_buffers(key, bx, by, bw, bh)
int key,
 bx,
 by,
 bw,
 bh;
{
	int I;
	char s[90];

	key = (key / 0x100) - 0x003B;
	if (key <= max_num_screens)
	{
		form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
		form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
		HIDDEN = SCREEN[key];
		KEY = key;
		copy_picture(HIDDEN, SHOWN);
		stuff_current_cbpal(NEW);
		sprintf(s, "[0][ | Current screen  | set to Screen(%d)   | ][ok]", key + 1);
		form_alert(1, s);
		return (TRUE);
	} else
	{
		sprintf(s, "[1][ | Screen(%d) is not | available. | \
Last screen is %d   ][ Whoops ]", key + 1, max_num_screens + 1);
		form_alert(1, s);
		return (NO_REDRAW);
	}
}



stuff_current_cbpal(flag)
int flag;
{
	register int I,
	 J,
	 tempr,
	 tempg,
	 tempb;
	int temp;

	for (I = 0; I < 200; I++)			/* For STe use bits from 4th byte to */
		for (J = 0; J < 16; J++)		/* extend color range up to 30 (32)  */
		{
			tempr = ((cb_pal[KEY][I * 16 + J] & 0x4000) > 0 ? (0x1000 + (cb_pal[KEY][I * 16 + J]
																		 & 0xf00)) : (cb_pal[KEY][I * 16 + J] & 0xf00));
			tempg = ((cb_pal[KEY][I * 16 + J] & 0x2000) > 0 ? (0x0100 + (cb_pal[KEY][I * 16 + J]
																		 & 0x0f0)) : (cb_pal[KEY][I * 16 + J] & 0x0f0));
			tempb = ((cb_pal[KEY][I * 16 + J] & 0x1000) > 0 ? (0x0010 + (cb_pal[KEY][I * 16 + J]
																		 & 0x00f)) : (cb_pal[KEY][I * 16 + J] & 0x00f));
			cb1_palette[I * 16 + J] = 0x100 * STe_rgb[((tempr / 2) & 0xf00) / 0x100]
				+ 0x010 * STe_rgb[((tempg / 2) & 0x0f0) / 0x010] + STe_rgb[((tempb / 2) & 0x00f)];
			cb2_palette[I * 16 + J] = 0x100 * STe_rgb[(((0x100 + tempr) / 2) & 0xf00) / 0x100]
				+ 0x010 * STe_rgb[(((0x010 + tempg) / 2) & 0x0f0) / 0x010] + STe_rgb[(((1 + tempb) / 2) & 0x00f)];
			if ((I % 2) == 1)
			{							/* Swap definitions on odd lines */
				temp = cb2_palette[I * 16 + J];
				cb2_palette[I * 16 + J] = cb1_palette[I * 16 + J];
				cb1_palette[I * 16 + J] = temp;
			}
		}
	if (flag == NEW)
	{
		for (J = 0; J < 16; J++)
		{
			tempr = ((cb_pal[KEY][J] & 0x4000) > 0 ? (0x1000 + (cb_pal[KEY][J] & 0xf00)) : (cb_pal[KEY][J] & 0xf00));
			tempg = ((cb_pal[KEY][J] & 0x2000) > 0 ? (0x0100 + (cb_pal[KEY][J] & 0x0f0)) : (cb_pal[KEY][J] & 0x0f0));
			tempb = ((cb_pal[KEY][J] & 0x1000) > 0 ? (0x0010 + (cb_pal[KEY][J] & 0x00f)) : (cb_pal[KEY][J] & 0x00f));
			I = 0x100 * ((((0x100 + tempr) / 2) & 0xf00) / 0x100) + 0x010 *
				((((0x010 + tempg) / 2) & 0x0f0) / 0x010) + (((1 + tempb) / 2) & 0x00f);
			palette[J] = I;
			pic_palette[KEY][J] = cb_pal[KEY][J];
		}
		Setpal(palette);
	}
}




edit()
{
	int p[4],
	 button,
	 I,
	 temp;

	copy_picture(HIDDEN, SHOWN);
	graf_mouse(THIN_CROSS, 0);
	vq_mouse(handle, &button, &p[0], &p[1]);
	while (button != LEFT)
		vq_mouse(handle, &button, &p[0], &p[1]);
	if (rubber_box(p[0], p[1], &p[2], &p[3]) == TRUE)
	{
		if (p[2] < p[0])
		{
			temp = p[0];
			p[0] = p[2];
			p[2] = temp;
		}
		if (p[3] < p[1])
		{
			temp = p[1];
			p[1] = p[3];
			p[3] = temp;
		}
		p[2] = p[2] - p[0];
		p[3] = p[3] - p[1];
		clear_screen(SPRITE);
		cut_sprite(HIDDEN, SPRITE, p);
		copy_picture(SPRITE, SHOWN);
		for (I = 0; I < 75; I++)
			Vsync();
		copy_picture(HIDDEN, SHOWN);
		graf_mouse(ARROW, 0);
		return (TRUE);
	}
	return (FALSE);
}





cut_sprite(s_address, d_address, p)
long s_address,
 d_address;
int p[4];
{
	int J,
	 I,
	 sfdb[10],
	 dfdb[10],
	 xy[8];
	long *temp1,
	*temp2;

		/***** Set Memory Form Definition Blocks *****/
	J = ((I = Getrez()) == 0) ? 4 : 2;
	temp1 = (long *) &sfdb[0];
	temp2 = (long *) &dfdb[0];
	sfdb[2] = dfdb[2] = work_out[0] + 1;
	sfdb[3] = dfdb[3] = work_out[1] + 1;
	sfdb[4] = dfdb[4] = (work_out[0] + 1) / 16;
	sfdb[5] = dfdb[5] = 0;
	sfdb[6] = dfdb[6] = J;
	xy[0] = p[0];
	xy[1] = p[1];
	width = xy[6] = p[2];
	height = xy[7] = p[3];
	xy[2] = p[0] + p[2];
	xy[3] = p[1] + p[3];
	xy[4] = xy[5] = 0;
	*temp1 = s_address;
	*temp2 = d_address;
	vro_cpyfm(handle, 3, xy, sfdb, dfdb);
}




paste_sprite()
{
	if (height == 0)
		return (FALSE);
	copy_picture(HIDDEN, SHOWN);
	move_sprite();
	copy_picture(SHOWN, HIDDEN);		/* SAVE FOR FUTURE GENERATIONS */
	return (TRUE);
}



move_sprite()
{
	int sfdb[12],
	 dfdb[10],
	 xy[8],
	 planes,
	 but,
	 x,
	 y;
	long *temp1,
	*temp2;

		/***** Set Memory Form Definition Blocks *****/
	planes = 4 - 2 * Getrez();
	temp1 = (long *) &sfdb[0];
	temp2 = (long *) &dfdb[0];
	sfdb[2] = dfdb[2] = work_out[0] + 1;
	sfdb[3] = dfdb[3] = work_out[1] + 1;
	sfdb[4] = dfdb[4] = (work_out[0] + 1) / 16;
	sfdb[5] = dfdb[5] = 0;
	sfdb[6] = dfdb[6] = planes;
	but = NONE;
	copy_picture(HIDDEN, SHOWN);
	v_hide_c(handle);
	while (but != LEFT && but != RIGHT)
	{
		vq_mouse(handle, &but, &xy[4], &xy[5]);
		if (x != xy[4] || y != xy[5])
		{
			xy[0] = xy[4] = x;
			xy[1] = xy[5] = y;
			xy[2] = xy[6] = x + width;
			xy[3] = xy[7] = y + height;
			*temp1 = HIDDEN;
			*temp2 = SHOWN;
			Setscreen(HIDDEN, HIDDEN, -1);
			Vsync();
			vro_cpyfm(handle, 3, xy, sfdb, dfdb);
			vq_mouse(handle, &but, &xy[4], &xy[5]);
			*temp1 = SPRITE;
			*temp2 = SHOWN;
			xy[0] = xy[1] = 0;
			xy[2] = width;
			xy[3] = height;
			if (xy[4] > work_out[0] - width)
				xy[4] = work_out[0] - width;
			if (xy[5] > work_out[1] - height)
				xy[5] = work_out[1] - height;
			xy[6] = xy[4] + width;
			xy[7] = xy[5] + height;
			vro_cpyfm(handle, 3, xy, sfdb, dfdb);
			Setscreen(SHOWN, SHOWN, -1);
			Vsync();
		}
		x = xy[4];
		y = xy[5];
	}
	if (but == RIGHT)
		copy_picture(HIDDEN, SHOWN);
	return (TRUE);
}



select_picture(type, size, off)
int type;
unsigned *size;
long off[2];
{
	static char neo_alert_text[] = "[1][ Neochrome works |only in LOW resolution]\
[ ABORT|continue ]";

	*size = 32;
	switch (type)
	{
	case NEO:
	case S_NEO:
		if (Getrez() != LOW)
		{
			if (form_alert(1, neo_alert_text) == 1)
				return (FALSE);
		}
		off[0] = 4L;
		off[1] = 128L;
		if (chose_a_file("*.NEO", type) == FALSE)
			return (FALSE);
		break;
	case DEGAS:
	case S_DEGAS:
		off[0] = 2L;
		off[1] = 34L;
		switch (Getrez())
		{
		case LOW:
			if (chose_a_file("*.PI1", type) == FALSE)
				return (FALSE);
			break;
		case MEDIUM:
			if (chose_a_file("*.PI2", type) == FALSE)
				return (FALSE);
			break;
		}
		break;
	case PICTURES:
	case PICTUREL:
		off[0] = 0L;
		off[1] = 6400L;
		*size = 6400;
		if (chose_a_file("*.BST", type) == FALSE)
			return (FALSE);
		break;
	}
	return (TRUE);
}



chose_a_file(path, purpose)
char *path;
int purpose;
{
	int I,
	 J,
	 response;
	char directory[20],
	 name[20];

	if (purpose == PICTUREL || purpose == NEO || purpose == DEGAS)
		shadow_text(((work_out[0] / 3) - 7), 22, " Load which file ?", 2, TRANSPARENT);
	else if (purpose == PICTURES)
		shadow_text(((work_out[0] / 3) - 7), 22, " Save file as :", 2, TRANSPARENT);
	else if (purpose == PROGRAM)
		shadow_text(((work_out[0] / 3) - 7), 22, " Run which file ?", 2, TRANSPARENT);
	directory[0] = default_drive;
	directory[1] = ':';
	strcpy(&directory[2], path);
	strcpy(name, path);
	for (I = 0; I < 40; filename[I++] = '\0') ;
									   /*** Removes Junk characters ***/
	fsel_input(directory, name, &response);
	if (response == FALSE)
		return (FALSE);
	default_drive = directory[0];		/* Come Back To Last Directory */
	for (I = 0; directory[I] != '*'; filename[I++] = directory[I]) ;
	for (J = 0; name[J] != '\0'; filename[I++] = name[J++]) ;
	copy_picture(HIDDEN, SHOWN);
	shadow_text(((work_out[0] / 3) - 7), 22, filename, 2, TRANSPARENT);
	return (TRUE);
}






load_picture(size, off)
unsigned size;
long off[2];
{
	int filehandle,
	 I,
	 K,
	 J,
	 good,
	 r,
	 g,
	 b,
	 t;
	unsigned filesize;

	filehandle = open(filename, O_BINARY);
	if (filehandle == -1)
	{
		form_alert(1, "[1][ | Error opening file.     |    ][CANCEL]");
		return (FALSE);
	}
	v_hide_c(handle);
	lseek(filehandle, off[0], 0);		/* Skip header & load palette */
	if (size == 32)						/* Neo and Degas              */
	{
		good = read(filehandle, pic_palette[KEY], size);
		if (good == -1)
		{
			form_alert(1, "[1][ | Error loading palette.   |    ][CANCEL]");
			return (FALSE);
		}
		for (I = 0; I < 3200; I++)		/* deal with a Neo or Degas STe upgrade now */
		{
			t = 0xf00 & pic_palette[KEY][I % 16];
			r = (t > 0x700 ? 0x4000 + 2 * (t - 0x800) : 2 * t);
			t = 0x0f0 & pic_palette[KEY][I % 16];
			g = (t > 0x070 ? 0x2000 + 2 * (t - 0x080) : 2 * t);
			t = 0x00f & pic_palette[KEY][I % 16];
			b = (t > 0x007 ? 0x1000 + 2 * (t - 0x008) : 2 * t);
			cb_pal[KEY][I] = r + g + b;
		}
		stuff_current_cbpal(NEW);
	} else								/* ColorBurst II              */
	{
		read(filehandle, cb_pal[KEY], size);
		if (good == -1)
		{
			form_alert(1, "[1][ | Error loading palette.   |    ][CANCEL]");
			return (FALSE);
		}
	}
	lseek(filehandle, off[1], 0);		/*  Load screen  offset       */
	if (size == 32)
	{
		good = read(filehandle, HIDDEN, 32000);
	} else
	{
		filesize = (unsigned) (lseek(filehandle, 0L, 2) - off[1]);
		lseek(filehandle, off[1], 0);
		good = read(filehandle, MASK, filesize);
		if (good != -1)
			decompress_screen(HIDDEN, MASK);
	}
	if (good == -1)
	{
		form_alert(1, "[1][ | Error loading image.   |    ][CANCEL]");
		return (FALSE);
	}
	close(filehandle);
	v_show_c(handle, 0);
}




decompress_screen(screen, source)
register long screen[8000];
long source[8000];
{
	register int k,
	 m,
	 j = 0,
		off,
		*tempi;
	int cntrl_max,
	 I,
	 t,
	 r,
	 g,
	 b;
	long test;

	tempi = (int *) source;
	if (*tempi < 20 && STe_exists == TRUE)	/* Fix palette made by */
	{									/* ST versions         */
		for (I = 0; I < 3200; I++)
		{
			t = 0xf00 & cb_pal[KEY][I];
			r = (t > 0x700 ? 0x4000 + 2 * (t - 0x800) : 2 * t);
			t = 0x0f0 & cb_pal[KEY][I];
			g = (t > 0x070 ? 0x2000 + 2 * (t - 0x080) : 2 * t);
			t = 0x00f & cb_pal[KEY][I];
			b = (t > 0x007 ? 0x1000 + 2 * (t - 0x008) : 2 * t);
			cb_pal[KEY][I] = r + g + b;
		}
	}
	if (*tempi >= 30)
		return;							/* punt, if unknown version     */
	stuff_current_cbpal(NEW);			/* load new palette             */
	cntrl_max = *(tempi + 1);			/* get number of control words  */
	off = (cntrl_max - 2) / 2 + 1;
	for (k = 2; k < cntrl_max; k++)
	{
		my_control[k - 2] = *(tempi + k);
	}
	for (k = 0; k < cntrl_max - 2; k++)
	{
		if (my_control[k] < 0)			/* deal with compressed stuff */
		{
			for (m = 0; m < -1 * my_control[k]; m++)
				screen[j++] = source[off];
			off++;
		} else							/* deal with raw stuff */
		{
			for (m = 0; m < my_control[k]; m++)
				screen[j++] = source[off++];
		}
	}
}





save_picture(size, off)
unsigned size;
long off[2];
{
	int filehandle,
	 good,
	 header[2];
	unsigned n0,
	 n1;

	filehandle = open(filename, O_WRONLY | O_BINARY | O_CREAT);
	if (filehandle == -1)
	{
		form_alert(1, "[1][ | Error opening file.     |    ][CANCEL]");
		RING_BELL();
		return (FALSE);
	}
	if (size == 32)
	{
		n0 = off[0];
		header[0] = Getrez();			/* Degas required   */
		n1 = off[1] - 32 - off[0];		/* 0(D) or 92(neo)  */
		good = write(filehandle, header, n0);	/* write header     */
		good = write(filehandle, pic_palette[KEY], 32);	/* write palette    */
		good = write(filehandle, s, n1);	/* dummy neo scroll */
		if (good == -1)
		{
			form_alert(1, "[1][ | Error saving palette.   |    ][CANCEL]");
			return (FALSE);
		}
		good = write(filehandle, HIDDEN, 32000);	/* write image data */
		if (good == -1)
		{
			form_alert(1, "[1][ | Error saving image.   |    ][CANCEL]");
			return (FALSE);
		}
	} else
	{
		good = write(filehandle, cb_pal[KEY], 6400);
		if (good == -1)
		{
			form_alert(1, "[1][ | Error saving palette.   |    ][CANCEL]");
			return (FALSE);
		}
		copy_picture(HIDDEN, MASK);
		compress_screen(&size, MASK, MASK);
		good = write(filehandle, MASK, size);
		if (good == -1)
		{
			form_alert(1, "[1][ | Error saving image.   |    ][CANCEL]");
			return (FALSE);
		}
	}
	close(filehandle);
	RING_BELL();
}



compress_screen(size, screen, destination)
unsigned *size;
long screen[8000],
 destination[8100];
{
	register int k = 1,
		my_count = 0,
		m = 2,
		j = 0,
		*tempi;

	my_control[0] = 20 + Getrez();	  /*** Version*10 + Resolution ***/
	test = screen[0];
	while (k < 8000)
	{
		while (k < 8000 && test == screen[k])
		{
			k++;
			my_count++;
		}
		if (my_count > 0)
		{
			my_control[m++] = -1 * (my_count + 1);
			my_count = 0;
			my_buffer[j++] = test;
			if (k < 8000)
				test = screen[k];
			k++;
		}
		while (k < 8000 && test != screen[k])
		{
			my_count++;
			my_buffer[j++] = test;
			test = screen[k];
			k++;
		}
		if (my_count > 0)
		{
			my_control[m++] = my_count;
			my_count = 0;
		}
	}
	if ((m % 2) == 1)
		m += 1;							/* make my_count even */
	my_control[1] = m;					/* store number of control words */
	tempi = (int *) &destination[0];
	for (k = 0; k < m; k++)
		*(tempi + k) = my_control[k];
	for (k = 0; k < j; k++)
		destination[k + (m / 2)] = my_buffer[k];
	*size = (unsigned) (m * 2 + j * 4);
}





display_help()
{
	int message[8],
	 x,
	 y,
	 button,
	 state,
	 key,
	 n,
	 I,
	 bx,
	 by,
	 bw,
	 bh,
	 sx,
	 sy,
	 sw,
	 sh,
	 hold[16];

	Setpal(my_palette);
	form_center(help_1, &bx, &by, &bw, &bh);
	v_hide_c(handle);
	form_dial(FMD_GROW, 0, work_out[1] - 20, 10, 10, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	objc_draw(help_1, 0, 10, bx, by, bw, bh);
	v_show_c(handle, 0);
	evnt_multi(MU_BUTTON, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, message, 0, 0, &x, &y, &button, &state, &key, &n);
	v_hide_c(handle);
	form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
	form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
	if (objc_find(help_1, 0, 1, x, y) == EXIT1)
	{
		copy_picture(HIDDEN, SHOWN);
		Setpal(palette);
		v_show_c(handle, 0);
		return (TRUE);
	}
	form_center(help_2, &bx, &by, &bw, &bh);
	form_dial(FMD_GROW, 0, work_out[1] - 20, 10, 10, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	objc_draw(help_2, 0, 10, bx, by, bw, bh);
	v_show_c(handle, 0);
	evnt_multi(MU_BUTTON, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, message, 0, 0, &x, &y, &button, &state, &key, &n);
	v_hide_c(handle);
	form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
	form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
	if (objc_find(help_2, 0, 1, x, y) == EXIT2)
	{
		copy_picture(HIDDEN, SHOWN);
		Setpal(palette);
		v_show_c(handle, 0);
		return (TRUE);
	}
	form_center(help_3, &bx, &by, &bw, &bh);
	form_dial(FMD_GROW, 0, work_out[1] - 20, 10, 10, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	objc_draw(help_3, 0, 10, bx, by, bw, bh);
	v_show_c(handle, 0);
	evnt_multi(MU_BUTTON, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, message, 0, 0, &x, &y, &button, &state, &key, &n);
	v_hide_c(handle);
	form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
	form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
	if (objc_find(help_3, 0, 1, x, y) == EXIT3)
	{
		copy_picture(HIDDEN, SHOWN);
		Setpal(palette);
		v_show_c(handle, 0);
		return (TRUE);
	}
	form_center(help_4, &bx, &by, &bw, &bh);
	form_dial(FMD_GROW, 0, work_out[1] - 20, 10, 10, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	objc_draw(help_4, 0, 10, bx, by, bw, bh);
	v_show_c(handle, 0);
	evnt_multi(MU_BUTTON, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, message, 0, 0, &x, &y, &button, &state, &key, &n);
	v_hide_c(handle);
	form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
	form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
	copy_picture(HIDDEN, SHOWN);
	Setpal(palette);
	v_show_c(handle, 0);
	return (TRUE);
}



goto_colorburst_mode(duration)
int duration;
{
	v_hide_c(handle);
	insert_cb_interrupt();
	Xbtimer(1, 8, 1, hblank);
	vcount = 0;
	PAUSE(duration);
	Xbtimer(0, 0, 0, 0L);
	remove_cb_interrupt();
	v_show_c(handle, 0);
	Setpal(palette);
	return (TRUE);
}



insert_cb_interrupt()
{
	Vsync();
	Supexec(timer_setup);
}



remove_cb_interrupt()
{
	Supexec(clean_up);
}


asm
{
  hblank:
	movem.l A3, -(A7)					/* Get a Register to work with.  */
  movea.l temp_line, A3 setpal:
  move.l(A3), 0xff8240L move.l 4(A3), 0xff8244L move.l 8(A3), 0xff8248L move.l 12(A3), 0xff824cL move.l 16(A3), 0xff8250L move.l 20(A3), 0xff8254L move.l 24(A3), 0xff8258L move.l 28(A3), 0xff825cL bye:
	addi.
		l
#32,temp_line
		movem.l(A7) +, A3				/* Restore registers before leaving  */
		bclr.b
#0,0xfffa0b								/* clear interrupt with mfp 68901    */
		bclr.b
#0,0xfffa0f								/* clear interrupt pending on mfp    */
		rte								/* Return from the timer exception   */
}



asm
{										/* Timer B is used for hblanks.      */
  vblank:
	subq.b
#1,0xfffa13
		movem.l A3, -(A7)				/* Get some Registers to work with.  */
		cmpi.w
#1,pause_flag
		bne Go_on addi.w
#1,vcount								/* If we are 'Paused' increment the  */
		bra Go_on						/* 60 hz timing counter.             */
  Go_on:
	lea cb1_palette, A3					/* Use 1st set of palettes           */
		neg ptoggle cmpi.w
#1,ptoggle
		beq bye lea cb2_palette, A3		/* Use 2nd set of palettes           */
  bye:
	move.l A3, temp_line				/* Set palette pointer               */
		movem.l(A7) +, A3				/* Restore registers before leaving  */
		ori.b
#1,0xfffa13
		rts								/* return to process the rest of     */
}										/* the Vblank exception.             */



timer_setup()
{
	short stop_keyboard;

	stop_keyboard = 0x13;
	Ikbdws(1, &stop_keyboard);
	asm
	{
		movem.l A1 - A3, -(A7)			/* Get some Registers to work with.  */
	  store:
		move.b 0xfffa07, _1st_enable_register	/* Save old      */
			move.b 0xfffa09, _2nd_enable_register	/* values for    */
			move.b 0xfffa13, _1st_mask_register	/* later resto-  */
			/* ration.       */
	  set_up:
		lea vblank, A1					/* insert new vblank routine      */
			move.l A1, 0x4d2 andi.b
#0xdf,0xfffa09
			lea cb1_palette, A3			/* Set palette pointer            */
			move.l A3, temp_line movem.l(A7) +, A1 - A3	/* Reset registers before leaving */
	unlk A6 rts}
}



clean_up()
{
	short reset_keyboard;

	reset_keyboard = 0x11;
	Ikbdws(1, &reset_keyboard);
	asm
	{
	  restore:
		move.l
#0x0L,0x4d2
			move.b _1st_enable_register, 0xfffa07	/* Restore old     */
			move.b _2nd_enable_register, 0xfffa09	/* values to these */
			move.b _1st_mask_register, 0xfffa13	/* MFP 68901 reg.s */
	unlk A6 rts}
}



clear_vblank()
{
	asm
	{
		move.l
#0x0L,0x4d2								/* Delete extra vblank vector */
	unlk A6 rts}
}


Setpal(pal)
int pal[16];
{
	int I;

	for (I = 0; I < 16; I++)
		STe_color(I, pal[I]);				 /*** STe palette fix   ***/
}


STe_color(I, rgb)
int I,
 rgb;
{
	int r,
	 g,
	 b;

	r = (rgb & 0x0f00) / 0x0100;		/* isolate RED nibble   */
	g = (rgb & 0x00f0) / 0x0010;		/* isolate GREEN nibble */
	b = (rgb & 0x000f);					/* isolate BLUE nibble  */
	Setcolor(I, STe_rgb[r] * 0x100 + STe_rgb[g] * 0x010 + STe_rgb[b]);	/* set new version      */
}


rubber_spray(x, y, xf, yf)
int x,
 y,
*xf,
*yf;
{
	int hold,
	 d,
	 button = RIGHT;

	hold = lwidth;
	vsl_width(handle, 1);
	*xf = x + spray_xr;
	*yf = y + spray_yr;
	vswr_mode(handle, 3);
	v_ellarc(handle, x, y, *xf - x, *yf - y, 0, 3600);
	for (d = 0; d++ < 50;)
		Vsync();
	v_ellarc(handle, x, y, *xf - x, *yf - y, 0, 3600);
	v_hide_c(handle);
	while (button != NONE)
		vq_mouse(handle, &button, xf, yf);
	while (button == NONE)
	{
		vq_mouse(handle, &button, xf, yf);
		if (abs(*yf - y) > y / 2)
			*yf = y / 2;
		if (abs(*xf - x) > x / 2)
			*xf = x / 2;
		v_ellarc(handle, x, y, *xf - x, *yf - y, 0, 3600);
		Vsync();
		v_ellarc(handle, x, y, *xf - x, *yf - y, 0, 3600);
	}
	vswr_mode(handle, linewmode);
	vsl_width(handle, hold);
	v_show_c(handle, 0);
	if (button == LEFT)
	{
		while (button != NONE)
			vq_mouse(handle, &button, &d, &d);
		return (TRUE);
	}
	return (FALSE);
}



rubber_ellipse(x, y, xf, yf)
int x,
 y,
*xf,
*yf;
{
	int hold,
	 d,
	 button = RIGHT;

	hold = lwidth;
	vsl_width(handle, 1);
	*xf = x;
	*yf = y;
	vswr_mode(handle, 3);
	v_hide_c(handle);
	while (button != NONE)
		vq_mouse(handle, &button, xf, yf);
	while (button == NONE)
	{
		vq_mouse(handle, &button, xf, yf);
		v_ellarc(handle, x, y, *xf - x, *yf - y, 0, 3610);
		Vsync();
		v_ellarc(handle, x, y, *xf - x, *yf - y, 0, 3610);
	}
	vswr_mode(handle, linewmode);
	vsl_width(handle, hold);
	v_show_c(handle, 0);
	if (button == LEFT)
	{
		while (button != NONE)
			vq_mouse(handle, &button, &d, &d);
		return (TRUE);
	}
	return (FALSE);
}


rubber_box(x, y, xf, yf)
int x,
 y,
*xf,
*yf;
{
	int d,
	 box_array[10],
	 button = RIGHT;

	vsl_width(handle, 1);
	box_array[0] = box_array[6] = box_array[8] = x;
	box_array[1] = box_array[3] = box_array[9] = y;
	vswr_mode(handle, 3);
	v_hide_c(handle);
	while (button != NONE)
		vq_mouse(handle, &button, xf, yf);
	while (button == NONE)
	{
		vq_mouse(handle, &button, xf, yf);
		box_array[2] = box_array[4] = *xf;
		box_array[5] = box_array[7] = *yf;
		v_pline(handle, 5, box_array);
		Vsync();
		v_pline(handle, 5, box_array);
		Vsync();
	}
	vsl_width(handle, lwidth);
	vswr_mode(handle, linewmode);
	v_show_c(handle, 0);
	if (button == LEFT)
	{
		while (button != NONE)
			vq_mouse(handle, &button, &d, &d);
		return (TRUE);
	}
	return (FALSE);
}



set_line()
{
	int p[4],
	 temp;

	objc_offset(colorbox, SHOW_BOX, &p[0], &p[1]);
	p[2] = colorbox[SHOW_BOX].ob_width;
	p[3] = colorbox[SHOW_BOX].ob_height;
	v_hide_c(handle);
	objc_draw(colorbox, SHOW_BOX, 0, p[0], p[1], p[2], p[3]);
	p[1] = p[3] = p[1] + p[3] / 2;
	p[2] = p[2] - 1 + p[0];
	v_pline(handle, 2, p);
	Vsync();
	v_show_c(handle, 0);
}



set_text()
{
	int p[4],
	 temp;
	char s[4];

	objc_offset(colorbox, SHOW_BOX, &p[0], &p[1]);
	p[2] = colorbox[SHOW_BOX].ob_width;
	p[3] = colorbox[SHOW_BOX].ob_height;
	objc_draw(colorbox, SHOW_BOX, 0, p[0], p[1], p[2], p[3]);
	vswr_mode(handle, TRANSPARENT);
	sprintf(s, "%d", text_h);
	v_hide_c(handle);
	v_gtext(handle, p[0] + p[2] / 5, p[1] + p[3] - 4, s);
	Vsync();
	v_show_c(handle, 0);
	vswr_mode(handle, linewmode);
}



set_fill()
{
	int p[4],
	 temp;

	objc_offset(colorbox, SHOW_BOX, &p[0], &p[1]);
	p[2] = colorbox[SHOW_BOX].ob_width;
	p[3] = colorbox[SHOW_BOX].ob_height;
	objc_draw(colorbox, SHOW_BOX, 0, p[0], p[1], p[2], p[3]);
	temp = p[1];
	p[1] = p[1] + p[3] - 1;
	p[2] = p[2] - 1 + p[0];
	p[3] = temp;
	v_hide_c(handle);
	v_bar(handle, p);
	Vsync();
	v_show_c(handle, 0);
}



set_object(tree, item, state)
OBJECT *tree;
int item,
 state;
{
	int p[4];

	objc_offset(tree, item, &p[0], &p[1]);
	p[2] = tree[item].ob_width;
	p[3] = tree[item].ob_height;
	objc_change(tree, item, 0, p[0], p[1], p[2], p[3], state, 1);
}



advance_selection(set_flag)
int set_flag;
{
	int I,
	 k,
	 button,
	 x,
	 y,
	 attrib[6],
	 rez,
	 cw,
	 ch,
	 cellw,
	 cellh;

	rez = Getrez();
	switch (set_flag)
	{
	case SELECT_F:
		vqf_attributes(handle, attrib);
		k = attrib[0];
		I = attrib[2];
		if (k == 3 && I == 12)
		{
			k = 2;
			I = 1;
		}
		if (++I > 24)
		{
			I = 1;
			k = 3;
		}
		vsf_interior(handle, k);
		vsf_style(handle, I);
		set_fill();
		break;
	case LINE_W:
		if ((lwidth += 2 + rez * 2) > 29)
			lwidth = 1;
		vsl_width(handle, lwidth);
		set_line();
		if (lwidth != 1)
			set_object(colorbox, LINE_S, CROSSED);
		if (lwidth == 1)
			set_object(colorbox, LINE_S, NORMAL);
		break;
	case LINE_S:
		vql_attributes(handle, attrib);
		I = attrib[0];
		if (++I > 6)
			I = 1;
		vsl_type(handle, I);
		set_line();
		break;
	case SELECT_T:
		if (++text_h > 26)
			text_h = 4;
		vst_height(handle, text_h, &cw, &ch, &cellw, &cellh);
		set_text();
		break;
	}
	for (I = 0; I < 15; I++)
		Vsync();
}




decrement_selection(set_flag)
int set_flag;
{
	int I,
	 k,
	 button,
	 x,
	 y,
	 attrib[6],
	 rez,
	 cw,
	 ch,
	 cellw,
	 cellh;

	rez = Getrez();
	switch (set_flag)
	{
	case SELECT_F:
		vqf_attributes(handle, attrib);
		k = attrib[0];
		I = attrib[2];
		if (k == 3 && I == 1)
		{
			k = 2;
			I = 24;
		}
		if (--I < 1)
		{
			I = 12;
			k = 3;
		}
		vsf_interior(handle, k);
		vsf_style(handle, I);
		set_fill();
		break;
	case LINE_W:
		if ((lwidth -= 2 + rez * 2) < 1)
			lwidth = 29;
		vsl_width(handle, lwidth);
		set_line();
		if (lwidth != 1)
			set_object(colorbox, LINE_S, CROSSED);
		if (lwidth == 1)
			set_object(colorbox, LINE_S, NORMAL);
		break;
	case LINE_S:
		vql_attributes(handle, attrib);
		I = attrib[0];
		if (--I < 1)
			I = 3;
		vsl_type(handle, I);
		set_line();
		break;
	case SELECT_T:
		if (--text_h < 4)
			text_h = 26;
		vst_height(handle, text_h, &cw, &ch, &cellw, &cellh);
		set_text();
		break;
	}
	for (I = 0; I < 15; I++)
		Vsync();
}



call_other_program()
{
	int response;
	char *no_other = "[0][ |Error loading your program | ][ Sorry ]";

	clear_screen(SHOWN);
	Setpalette(desk_palette);
	response = chose_a_file("*.PRG", PROGRAM);
	if (response == FALSE)
	{
		Setpal(palette);
		copy_picture(HIDDEN, SHOWN);
		return;
	}
	response = Pexec(0, filename, "", 0L);
	copy_picture(HIDDEN, SHOWN);
	Setpal(palette);
	if (response < 0)
		form_alert(1, no_other);
}






set_rgb(tree, pen, item)
int pen,
 item;
OBJECT *tree;
{
	char text[4];
	int I,
	 color,
	 p[4],
	 r,
	 g,
	 b,
	 rgb,
	 k,
	 medmap[4],
	 lowmap[16];

	medmap[0] = 0;
	medmap[1] = 3;
	medmap[2] = 1;
	medmap[3] = 2;
	lowmap[0] = 0;
	lowmap[1] = 15;
	lowmap[2] = 1;
	lowmap[3] = 2;
	lowmap[4] = 4;
	lowmap[5] = 6;
	lowmap[6] = 3;
	lowmap[7] = 5;
	lowmap[8] = 7;
	lowmap[9] = 8;
	lowmap[10] = 9;
	lowmap[11] = 10;
	lowmap[12] = 12;
	lowmap[13] = 14;
	lowmap[14] = 11;
	lowmap[15] = 13;
	v_hide_c(handle);
	color = ((int) tree[pen].ob_spec) & 0x000f;
	active_pen = color;
	color = (Getrez() == 1) ? medmap[color] : lowmap[color];
	r = ((pic_palette[KEY][color] & 0xf00) / 0x100) + 16 * ((pic_palette[KEY][color] & 0x4000) / 0x4000);
	g = ((pic_palette[KEY][color] & 0x0f0) / 0x010) + 16 * ((pic_palette[KEY][color] & 0x2000) / 0x2000);
	b = (pic_palette[KEY][color] & 0xf) + 16 * ((pic_palette[KEY][color] & 0x1000) / 0x1000);
	switch (item)
	{
	case R_UP:
		if ((r += 1) > max_shade)
			r = max_shade;
		sprintf(text, "%02d", r);
		((TEDINFO *) tree[R_VALUE].ob_spec)->te_ptext = text;
		((TEDINFO *) tree[R_VALUE].ob_spec)->te_txtlen = 2;
		objc_offset(tree, R_VALUE, &p[0], &p[1]);
		p[2] = tree[R_VALUE].ob_width;
		p[3] = tree[R_VALUE].ob_height;
		objc_draw(tree, R_VALUE, 1, p[0], p[1], p[2], p[3]);
		break;
	case G_UP:
		if ((g += 1) > max_shade)
			g = max_shade;
		sprintf(text, "%02d", g);
		((TEDINFO *) tree[G_VALUE].ob_spec)->te_ptext = text;
		((TEDINFO *) tree[G_VALUE].ob_spec)->te_txtlen = 2;
		objc_offset(tree, G_VALUE, &p[0], &p[1]);
		p[2] = tree[G_VALUE].ob_width;
		p[3] = tree[G_VALUE].ob_height;
		objc_draw(tree, G_VALUE, 1, p[0], p[1], p[2], p[3]);
		break;
	case B_UP:
		if ((b += 1) > max_shade)
			b = max_shade;
		sprintf(text, "%02d", b);
		((TEDINFO *) tree[B_VALUE].ob_spec)->te_ptext = text;
		((TEDINFO *) tree[B_VALUE].ob_spec)->te_txtlen = 2;
		objc_offset(tree, B_VALUE, &p[0], &p[1]);
		p[2] = tree[B_VALUE].ob_width;
		p[3] = tree[B_VALUE].ob_height;
		objc_draw(tree, B_VALUE, 1, p[0], p[1], p[2], p[3]);
		break;
	case R_DOWN:
		if ((r -= 1) < 0)
			r = 0;
		sprintf(text, "%02d", r);
		((TEDINFO *) tree[R_VALUE].ob_spec)->te_ptext = text;
		((TEDINFO *) tree[R_VALUE].ob_spec)->te_txtlen = 2;
		objc_offset(tree, R_VALUE, &p[0], &p[1]);
		p[2] = tree[R_VALUE].ob_width;
		p[3] = tree[R_VALUE].ob_height;
		objc_draw(tree, R_VALUE, 1, p[0], p[1], p[2], p[3]);
		break;
	case G_DOWN:
		if ((g -= 1) < 0)
			g = 0;
		sprintf(text, "%02d", g);
		((TEDINFO *) tree[G_VALUE].ob_spec)->te_ptext = text;
		((TEDINFO *) tree[G_VALUE].ob_spec)->te_txtlen = 2;
		objc_offset(tree, G_VALUE, &p[0], &p[1]);
		p[2] = tree[G_VALUE].ob_width;
		p[3] = tree[G_VALUE].ob_height;
		objc_draw(tree, G_VALUE, 1, p[0], p[1], p[2], p[3]);
		break;
	case B_DOWN:
		if ((b -= 1) < 0)
			b = 0;
		sprintf(text, "%02d", b);
		((TEDINFO *) tree[B_VALUE].ob_spec)->te_ptext = text;
		((TEDINFO *) tree[B_VALUE].ob_spec)->te_txtlen = 2;
		objc_offset(tree, B_VALUE, &p[0], &p[1]);
		p[2] = tree[B_VALUE].ob_width;
		p[3] = tree[B_VALUE].ob_height;
		objc_draw(tree, B_VALUE, 1, p[0], p[1], p[2], p[3]);
		break;
	default:
		sprintf(text, "%02d", r);
		((TEDINFO *) tree[R_VALUE].ob_spec)->te_ptext = text;
		((TEDINFO *) tree[R_VALUE].ob_spec)->te_txtlen = 2;
		objc_offset(tree, R_VALUE, &p[0], &p[1]);
		p[2] = tree[R_VALUE].ob_width;
		p[3] = tree[R_VALUE].ob_height;
		objc_draw(tree, R_VALUE, 1, p[0], p[1], p[2], p[3]);
		sprintf(text, "%02d", g);
		((TEDINFO *) tree[G_VALUE].ob_spec)->te_ptext = text;
		((TEDINFO *) tree[G_VALUE].ob_spec)->te_txtlen = 2;
		objc_offset(tree, G_VALUE, &p[0], &p[1]);
		p[2] = tree[G_VALUE].ob_width;
		p[3] = tree[G_VALUE].ob_height;
		objc_draw(tree, G_VALUE, 1, p[0], p[1], p[2], p[3]);
		sprintf(text, "%02d", b);
		((TEDINFO *) tree[B_VALUE].ob_spec)->te_ptext = text;
		((TEDINFO *) tree[B_VALUE].ob_spec)->te_txtlen = 2;
		objc_offset(tree, B_VALUE, &p[0], &p[1]);
		p[2] = tree[B_VALUE].ob_width;
		p[3] = tree[B_VALUE].ob_height;
		objc_draw(tree, B_VALUE, 1, p[0], p[1], p[2], p[3]);
		v_show_c(handle, 0);
		return (TRUE);
		break;
	}
	r = (r > 15 ? (r - 16) * 0x100 + 0x4000 : r * 0x100);
	g = (g > 15 ? (g - 16) * 0x010 + 0x2000 : g * 0x010);
	b = (b > 15 ? (b - 16) + 0x1000 : b);
	rgb = r + b + g;
	pic_palette[KEY][color] = rgb;
	for (I = 0; I < 16; I++)
	{
		r = ((pic_palette[KEY][I] & 0xf00) / 0x100 + 16 * ((pic_palette[KEY][I] & 0x4000) / 0x4000)) / 2;
		g = ((pic_palette[KEY][I] & 0x0f0) / 0x010 + 16 * ((pic_palette[KEY][I] & 0x2000) / 0x2000)) / 2;
		b = ((pic_palette[KEY][I] & 0x00f) + 16 * ((pic_palette[KEY][I] & 0x1000) / 0x1000)) / 2;
		palette[I] = r * 0x100 + g * 0x010 + b;
	}
	Setpal(palette);
	v_show_c(handle, 0);
}



select_sprayer()
{
	int x,
	 y,
	 mid_row,
	 mid_col;

	mid_row = (Getrez() == 2 ? 200 : 100);
	mid_col = (Getrez() == 0 ? 160 : 320);
	if (rubber_spray(mid_col, mid_row, &x, &y) == TRUE)
	{
		if ((spray_xr = x - mid_col) < 0)
			spray_xr *= -1;
		if ((spray_yr = y - mid_row) < 0)
			spray_yr *= -1;
	}
	copy_picture(MASK, SHOWN);
}




reset_color(flag)
int flag;
{
	int I,
	 K,
	 color,
	 shade,
	 medmap[4],
	 lowmap[16];

	medmap[0] = 0;
	medmap[1] = 3;
	medmap[2] = 1;
	medmap[3] = 2;
	lowmap[0] = 0;
	lowmap[1] = 15;
	lowmap[2] = 1;
	lowmap[3] = 2;
	lowmap[4] = 4;
	lowmap[5] = 6;
	lowmap[6] = 3;
	lowmap[7] = 5;
	lowmap[8] = 7;
	lowmap[9] = 8;
	lowmap[10] = 9;
	lowmap[11] = 10;
	lowmap[12] = 12;
	lowmap[13] = 14;
	lowmap[14] = 11;
	lowmap[15] = 13;
	color = (Getrez() == 1) ? medmap[active_pen] : lowmap[active_pen];
	v_hide_c(handle);
	if (flag == NEW)
	{
		shade = pic_palette[KEY][color];	/* use new color    */
	} else
	{
		shade = old_shade[color];		/* use old color    */
	}
	old_shade[color] = pic_palette[KEY][color];	/* update old color */
	K = list[0];
	for (I = 1; I < 200; I++)
		list[I - 1] = list[I];			/* rotate colored   */
	list[199] = K;						/* region           */
	for (I = 0; I < 200; I++)
	{
		if (list[I] == 1 || locked == TRUE)
			cb_pal[KEY][I * 16 + color] = shade;
	}
	stuff_current_cbpal(OLD);
	v_show_c(handle, 0);
}


show_tos_version()
{
	int max_colors,
	 bx,
	 by,
	 bw,
	 bh,
	 K;
	unsigned short major,
	 minor,
	 I;
	char roms[18],
	 romcolor[23];

	RING_BELL();
	form_center(rom_vers, &bx, &by, &bw, &bh);
	v_hide_c(handle);
	form_dial(FMD_GROW, 0, work_out[1] - 20, 10, 10, bx, by, bw, bh);
	form_dial(FMD_START, 0, 0, 0, 0, bx, by, bw, bh);
	rom_version.tos_version = global[0];

	major = rom_version.part[0];
	minor = I = rom_version.part[1];
	while (minor > 10)
		minor /= 10;
	if (minor >= 6)
		for (K = 0; K < 16; K++)
			my_palette[K] *= 2;
	Setpal(my_palette);
	max_colors = (minor >= 6 ? 29791 : 3375);
	max_shade = (minor >= 6 ? 30 : 14);	/* Range: 0-14 ST or 0-30 STe */
	sprintf(roms, "version %d.%-2d ROMs", major, I);
	((TEDINFO *) rom_vers[ROMS].ob_spec)->te_ptext = roms;
	((TEDINFO *) rom_vers[ROMS].ob_spec)->te_txtlen = 17;
	sprintf(romcolor, "%5d colors available", max_colors);
	((TEDINFO *) rom_vers[ROMCOLOR].ob_spec)->te_ptext = romcolor;
	((TEDINFO *) rom_vers[ROMCOLOR].ob_spec)->te_txtlen = 22;
	objc_draw(rom_vers, 0, 10, bx, by, bw, bh);
	v_show_c(handle, 0);
	for (K = 0; K < 180; K++)
		Vsync();						/* wait 3 seconds */
	v_hide_c(handle);
	form_dial(FMD_SHRINK, 10, 10, 10, 10, bx, by, bw, bh);
	form_dial(FMD_FINISH, 0, 0, 0, 0, bx, by, bw, bh);
	if (minor < 6)
	{
		for (I = 0; I < 16; I++)
			STe_rgb[I] = I;				/* if ST, bypass STe fix */
		return (FALSE);
	}
	return (TRUE);
}
