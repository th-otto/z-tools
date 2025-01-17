#include "general.h"
#include "prefs.h"
#include "plugins.h"
#include "close_modal.h"
#include "ztext.h"
#include "pic_load.h"
#include "catalog/catalog.h"
#include "file/file.h"
#include "pic_save.h"
#include "menu.h"
#include "plugins/common/zvplugin.h"


/* local variable */
static int last_choice = -1;
static const char *source_file;

#define MAX_TYPENAME_LEN 10


unsigned int get_option_mask(CODEC *codec)
{
	unsigned int have_options = 0;

	switch (codec->type)
	{
	case CODEC_LDG:
		{
			struct _ldg_funcs *funcs;

			funcs = codec->c.ldg.funcs;
			if (funcs->set_jpg_option)
				have_options |= (1 << OPTION_QUALITY) | (1 << OPTION_COLOR_SPACE) | (1 << OPTION_PROGRESSIVE);
			if (funcs->set_tiff_option)
				have_options |= (1 << OPTION_QUALITY) | (1 << OPTION_COMPRESSION);
			if (funcs->set_webp_option)
				have_options |= (1 << OPTION_QUALITY) | (1 << OPTION_COMPRLEVEL);
		}
		break;
	case CODEC_SLB:
		{
			SLB *slb;
			int i;

			slb = &codec->c.slb;
			for (i = OPTION_QUALITY; i < 32; i++)
				if (plugin_get_option(slb, i) >= 0)
					have_options |= 1 << i;
		}
		break;
	}
	return have_options;
}

static boolean encoder_plugin_setup(WINDOW *win, int encoder_selected, const char *name)
{
	CODEC *codec = codecs[encoder_selected];
	OBJECT *tree = get_tree(SAVE_DIAL);

	curr_output_plugin = NULL;

	if (!plugin_ref(codec))
		return FALSE;
	switch (codec->type)
	{
	case CODEC_LDG:
		{
			struct _ldg_funcs *funcs;

			funcs = codec->c.ldg.funcs;
			if (!funcs->encoder_init || !funcs->encoder_write || !funcs->encoder_quit)
			{
				errshow(codec->name, LDG_ERR_BASE + LDG_NO_FUNC);
				return FALSE;
			}
			curr_output_plugin = codec;
		}
		break;
	case CODEC_SLB:
		if (!(codec->capabilities & CAN_ENCODE))
		{
			errshow(codec->extensions, -ENOSYS);
			return FALSE;
		}
		curr_output_plugin = codec;
		break;
	default:
		return FALSE;
	}

	strcpy(tree[SAVE_DIAL_FORMAT].ob_spec.free_string, name);

	if (win)
	{
		if (get_option_mask(codec))
			ObjcChange(OC_FORM, win, SAVE_DIAL_OPTIONS, NORMAL, 0);
		else
			ObjcChange(OC_FORM, win, SAVE_DIAL_OPTIONS, DISABLED, 0);
	   	ObjcDraw(OC_FORM, win, SAVE_DIAL_FORMAT, 1);
   		ObjcDraw(OC_FORM, win, SAVE_DIAL_OPTIONS, 1);
	} else
	{
		if (get_option_mask(codec))
			tree[SAVE_DIAL_OPTIONS].ob_state &= ~OS_DISABLED;
		else
			tree[SAVE_DIAL_OPTIONS].ob_state |= OS_DISABLED;
	}

	return TRUE;
}


static void format_type(CODEC *codec, char *str)
{
	int len;

	/* limited by array, and button width in resource file */
	len = strlen(codec->extensions);
	memset(str, ' ', MAX_TYPENAME_LEN - 1);
	if (len >= MAX_TYPENAME_LEN)
		len = MAX_TYPENAME_LEN - 1;
	memcpy(str, codec->extensions, len);
	str[MAX_TYPENAME_LEN - 1] = '\0';
}


static void format_popup(WINDOW *win, int obj_index)
{
	char items[MAX_CODECS][MAX_TYPENAME_LEN];
	char *items_ptr[MAX_CODECS];
	int16 i, x, y;
	int choice;

	for (i = 0; i < encoder_plugins_nbr; i++)
	{
		format_type(codecs[i], items[i]);
		items_ptr[i] = items[i];
	}

	objc_offset(FORM(win), obj_index, &x, &y);

	choice = MenuPopUp(items_ptr, x, y, encoder_plugins_nbr, -1, last_choice, P_LIST | P_WNDW | P_CHCK);

	if (choice > 0)
	{
		if (last_choice != choice)
		{
			plugin_unref(curr_output_plugin);
			encoder_plugin_setup(win, choice - 1, items_ptr[choice - 1]);
		}

		if (curr_output_plugin)
			last_choice = choice;

		if (curr_output_plugin)
			ObjcChange(OC_FORM, win, SAVE_DIAL_SAVE, NORMAL, TRUE);
		else
			ObjcChange(OC_FORM, win, SAVE_DIAL_SAVE, DISABLED, TRUE);
	}
}


static void __CDECL save_dialog_event(WINDOW *win EVNT_BUFF_PARAM)
{
	char target_file[MAX_PATH+MAXNAMLEN];
	char target_file_path[MAX_PATH];
	char target_file_name[MAXNAMLEN];
	char extension[MAXNAMLEN];
	char file_mask[MAXNAMLEN];
	int16 object = EVNT_BUFF[4];
	int16 source_file_len;
	int16 source_path_len;
	char *dot;

	switch (object)
	{
		case SAVE_DIAL_FORMAT:
			format_popup(win, object);
			ObjcChange(OC_FORM, win, object, NORMAL, TRUE);
			break;

		case SAVE_DIAL_OPTIONS:
			ObjcChange(OC_FORM, win, object, NORMAL, TRUE);
			save_option_dialog(source_file, codecs[last_choice - 1]);
			break;

		case SAVE_DIAL_SAVE:
			source_file_len = (int16)strlen(source_file);

			source_path_len = source_file_len;

			while(source_path_len > 0 && source_file[source_path_len] != '/' && source_file[source_path_len] != '\\')
				source_path_len--;

			strcpy(target_file_path, source_file);

			target_file_path[source_path_len] = '\0';

			source_path_len++;

			strcpy(target_file_name, source_file + source_path_len);

			/* copy the source's name in the target's name with the new extension for exemple ("toto.gif" to "toto.jpg") */
			zstrncpy(extension, codecs[last_choice - 1]->extensions, 4);
			str2lower(extension);
			dot = strrchr(target_file_name, '.');
			if (dot)
				strcpy(dot + 1, extension);

			/* The file mask (for exemple *.jpg) */
			strcpy(file_mask, "*.");
			strcat(file_mask, extension);

			ObjcChange(OC_FORM, win, object, NORMAL, FALSE);
			close_modal(win EVNT_BUFF_NULL);

			if (FselInput(target_file_path, target_file_name, file_mask, get_string(SAVE_TITLE), NULL, NULL))
			{
				strcpy(target_file, target_file_path);

				strcat(target_file, target_file_name);

				/* the source can't be the same that the destination */
				if (strcasecmp(source_file, target_file) == 0)
				{
					errshow(NULL, SOURCE_TARGET_SAME);
					break;
				}

				if (pic_save(source_file, target_file))
				{
					/* if the new file is added in the same dir that win_catalog, we refresh it */
					if (win_catalog)
					{
						WINDICON *wicones = (WINDICON *)DataSearch(win_catalog, WD_ICON);

						if (strcasecmp(target_file_path, wicones->directory) == 0)
						{
							(void)scan_dir(win_catalog, wicones->directory);
							WinCatalog_filelist_redraw();
							ObjcDraw(OC_TOOLBAR, win_catalog, TOOLBAR_DELETE, 1);
							ObjcDraw(OC_TOOLBAR, win_catalog, TOOLBAR_INFO, 1);
							ObjcDraw(OC_TOOLBAR, win_catalog, TOOLBAR_SAVE, 1);
							menu_ienable(get_tree(MENU_BAR), MENU_BAR_DELETE, 0);
							menu_ienable(get_tree(MENU_BAR), MENU_BAR_INFORMATION, 0);
							menu_ienable(get_tree(MENU_BAR), MENU_BAR_SAVE, 0);
						}
					}
				}
		    }
			plugin_unref(curr_input_plugin);
		    plugin_unref(curr_output_plugin);
			break;

		case SAVE_DIAL_ABORT:
			ObjcChange(OC_FORM, win, object, NORMAL, FALSE);
			close_win(win);
			plugin_unref(curr_input_plugin);
		    plugin_unref(curr_output_plugin);
			break;
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
void save_dialog(const char *fullfilename)
{
	WINDOW 		*win_save_dialog;
	OBJECT *tree;
	int16 i;
	CODEC *input;

	if (!encoder_plugins_nbr)
		return;

	source_file = fullfilename;

	tree = get_tree(SAVE_DIAL);

	if (last_choice <= 0 && (input = get_codec(source_file)) != NULL)
	{
		for (i = 0; i < encoder_plugins_nbr; i++)
			if (input == codecs[i])
			{
				char name[MAX_TYPENAME_LEN];
				format_type(input, name);
				if (!encoder_plugin_setup(NULL, i, name))
					return;
				last_choice = i + 1;
				tree[SAVE_DIAL_SAVE].ob_state &= ~OS_DISABLED;
				break;
			}
	}

	win_save_dialog = FormCreate(tree, NAME|MOVER, save_dialog_event, get_string(SAVE_TITLE), NULL, TRUE, FALSE);

	/* Make this window modal */
	WindSet(win_save_dialog, WF_BEVENT, BEVENT_MODAL, 0, 0, 0);

	EvntAttach(win_save_dialog, WM_CLOSED, close_modal);

	MenuDisable();
}
