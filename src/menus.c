/*
 * GQmpeg
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "gqmpeg.h"
#include "menus.h"

#include "btn_funcs.h"
#include "playlist.h"
#include "playlist-window.h"
#include "preferences.h"
#include "ui_help.h"
#include "ui2_main.h"
#include "ui_menu.h"


/*
 *----------------
 * the menus
 *----------------
 */

static void load_preset_from_menu(GtkWidget *widget, gpointer data)
{
	gint n = GPOINTER_TO_INT(data);

	load_preset(n);
}

static GtkWidget *menu_preset(void)
{
	GtkWidget *menu;
	gint i;

	menu = gtk_menu_new();
	for (i=0; i<10; i++)
		{
		gchar *s;

		if (preset_name[i] && preset_file[i])
			{
			s = g_strdup_printf("%d - %s", i + 1, preset_name[i]);
			menu_item_add_check(menu, s, preset_is_active(i),
					    G_CALLBACK(load_preset_from_menu), GINT_TO_POINTER(i));
			g_free(s);
			}
		else
			{
			s = g_strdup_printf(_("%d - (empty)"), i + 1);
			menu_item_add(menu, s, NULL, NULL);
			g_free(s);
			}
                }

	menu_item_add_divider(menu);
	menu_item_add(menu, _("Set preset to _playlist..."), G_CALLBACK(set_preset_to_playlist), preset_file[i]);
	menu_item_add(menu, _("Set preset to _song..."), G_CALLBACK(set_preset_to_song), preset_file[i]);

	return menu;
}

static void menu_editor_cb(GtkWidget *widget, gpointer data)
{
	UIData *ui = data;

	btn_edit_skin_pressed(ui);
}

static void show_help(GtkWidget *widget, gpointer data)
{
	help_window_show("documentation");
}

static void close_window(GtkWidget *widget, gpointer data)
{
	UIData *ui = data;

	if (ui != main_window) ui_close(ui);
}

GtkWidget *menu_main(UIData *ui)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *submenu;
	gint enabled;

	enabled = (current_song_get_data() != NULL);

	menu = popup_menu_short_lived();

	item = menu_item_add_sensitive(menu, _("_Play"), enabled, G_CALLBACK(btn_play_pressed), NULL);
	item = menu_item_add_sensitive(menu, _("Pause"), enabled, G_CALLBACK(btn_pause_pressed), NULL);
	item = menu_item_add_sensitive(menu, _("_Stop"), enabled, G_CALLBACK(btn_stop_pressed), NULL);

	menu_item_add_divider(menu);

	menu_item_add_stock(menu, _("Play_list..."), GTK_STOCK_INDEX,
			G_CALLBACK(playlist_window_show), NULL);

	item = menu_item_add(menu, _("P_resets"), NULL, NULL);
	submenu = menu_preset();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	item = menu_item_add_stock(menu, _("_Open"), GTK_STOCK_OPEN, NULL, NULL);
	submenu = gtk_menu_new();
	menu_item_add(submenu, _("_Playlist..."), G_CALLBACK(btn_load_playlist_pressed), NULL);
	menu_item_add(submenu, _("_File..."), G_CALLBACK(btn_load_file_pressed), NULL);
	menu_item_add(submenu, _("_Custom..."), G_CALLBACK(btn_load_custom_pressed), NULL);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	item = menu_item_add_stock_sensitive(menu, _("Sa_ve playlist..."), GTK_STOCK_SAVE_AS,
			(playlist_get_count() > 0),
			G_CALLBACK(btn_save_pressed), NULL);

	menu_item_add_divider(menu);

	item = menu_item_add(menu, _("_Toggle"), NULL, NULL);
	submenu = gtk_menu_new();
	menu_item_add_check(submenu, _("_Shuffle"), shuffle_mode, btn_shuffle_pressed, NULL);
	menu_item_add_check(submenu, _("_Repeat"), repeat_mode, btn_repeat_pressed, NULL);
	menu_item_add_check(submenu, _("Repeat _1"), repeat_1_enabled, btn_repeat_1_pressed, NULL);
	menu_item_add_check(submenu, _("_Time total"), show_total_time, time_toggle_pressed, NULL);
	menu_item_add_check(submenu, _("T_ime remaining"), show_remaining_time, time_remaining_pressed, NULL);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	menu_item_add_stock(menu, _("Pre_ferences..."), GTK_STOCK_PREFERENCES,
			G_CALLBACK(show_configuration_window), NULL);

	menu_item_add_divider(menu);

	item = menu_item_add_sensitive(menu, _("_Current song"), enabled, NULL, NULL);
	submenu = gtk_menu_new();
	menu_item_add_stock(submenu, _("Song _Info..."), GTK_STOCK_PROPERTIES,
			G_CALLBACK(current_song_info_pressed), NULL);
	menu_item_add_stock(submenu, _("_Add to playlist"), GTK_STOCK_ADD,
			G_CALLBACK(current_song_pl_add_pressed), NULL);
	menu_item_add_stock(submenu, _("_Remove from playlist"), GTK_STOCK_REMOVE,
			G_CALLBACK(current_song_pl_rem_pressed), NULL);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	menu_item_add_stock(menu, _("Run _mixer..."), GTK_STOCK_EXECUTE,
			G_CALLBACK(btn_mixer_pressed), NULL);

	menu_item_add_divider(menu);
	menu_item_add(menu, _("Skin _Editor..."), G_CALLBACK(menu_editor_cb), ui);
	menu_item_add_stock(menu, _("_Help..."), GTK_STOCK_HELP,
			G_CALLBACK(show_help), NULL);
	menu_item_add(menu, _("_About..."), G_CALLBACK(show_about_info), NULL);
	menu_item_add_divider(menu);
	menu_item_add(menu, _("_Iconify"), G_CALLBACK(btn_iconify_pressed), NULL);
	if (ui != main_window)
		{
		menu_item_add_stock(menu, _("Close _window"), GTK_STOCK_CLOSE,
				G_CALLBACK(close_window), ui);
		}
	menu_item_add_stock(menu, _("_Quit"), GTK_STOCK_QUIT,
			G_CALLBACK(gqmpeg_exit), NULL);

	return menu;
}

GtkWidget *menu_filelist_dir(void)
{
	GtkWidget *menu;

	menu = popup_menu_short_lived();

	menu_item_add(menu, _("_Add"), G_CALLBACK(plw_add_dir_pressed), NULL);
	menu_item_add(menu, _("Add _Recursive"), G_CALLBACK(plw_add_dir_recursive_pressed), NULL);

	return menu;
}

GtkWidget *menu_filelist_file(void)
{
	GtkWidget *menu;

	menu = popup_menu_short_lived();

	menu_item_add_stock(menu, _("_Add"), GTK_STOCK_ADD,
			 G_CALLBACK(plw_filelist_add_pressed), NULL);
	menu_item_add(menu, _("_Insert"), G_CALLBACK(plw_filelist_insert_pressed), NULL);
	menu_item_add_divider(menu);
	menu_item_add(menu, _("_Play"), G_CALLBACK(plw_filelist_play_pressed), NULL);
	menu_item_add(menu, _("Add and P_lay"), G_CALLBACK(plw_filelist_add_play_pressed), NULL);
	menu_item_add_stock(menu, _("_Song Info..."), GTK_STOCK_PROPERTIES,
			G_CALLBACK(plw_filelist_songinfo_pressed), NULL);
	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("_Copy"), GTK_STOCK_COPY,
			G_CALLBACK(plw_filelist_copy_pressed), NULL);
	menu_item_add(menu, _("_Move"), G_CALLBACK(plw_filelist_move_pressed), NULL);
	menu_item_add(menu, _("_Rename"), G_CALLBACK(plw_filelist_rename_pressed), NULL);
	menu_item_add_stock(menu, _("_Delete"), GTK_STOCK_DELETE,
			G_CALLBACK(plw_filelist_delete_pressed), NULL);

	return menu;
}

GtkWidget *menu_filelist_playlist(void)
{
	GtkWidget *menu;

	menu = popup_menu_short_lived();

	menu_item_add_stock(menu, _("_Open"), GTK_STOCK_OPEN,
			G_CALLBACK(plw_load_pressed), NULL);
	menu_item_add(menu, _("_Append"), G_CALLBACK(plw_append_pressed), NULL);
	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("_Copy"), GTK_STOCK_COPY,
			G_CALLBACK(plw_filelist_copy_pressed), NULL);
	menu_item_add(menu, _("_Move"), G_CALLBACK(plw_filelist_move_pressed), NULL);
	menu_item_add(menu, _("_Rename"), G_CALLBACK(plw_filelist_rename_pressed), NULL);
	menu_item_add_stock(menu, _("_Delete"), GTK_STOCK_DELETE,
			G_CALLBACK(plw_filelist_delete_pressed), NULL);

	return menu;
}

static void menu_playlist_add_defaults(GtkWidget *menu)
{
	menu_item_add(menu, _("_Play"), G_CALLBACK(plw_playlist_play_pressed), NULL);
	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("_Remove"), GTK_STOCK_REMOVE,
			G_CALLBACK(plw_playlist_remove_pressed), NULL);
	menu_item_add_stock(menu, _("_Song Info..."), GTK_STOCK_PROPERTIES,
			G_CALLBACK(plw_playlist_songinfo_pressed), NULL);
	menu_item_add(menu, _("Edit _title"), G_CALLBACK(plw_playlist_edit_title_pressed), NULL);
	menu_item_add(menu, _("Edit c_omment"), G_CALLBACK(plw_playlist_edit_comment_pressed), NULL);
	menu_item_add_divider(menu);
}

static void menu_playlist_add_shuffle(GtkWidget *menu)
{
	menu_item_add_divider(menu);
	menu_item_add(menu, _("Randomize _list"), G_CALLBACK(plw_playlist_randomize_pressed), NULL);
}

GtkWidget *menu_playlist(void)
{
	GtkWidget *menu;

	menu = popup_menu_short_lived();

	menu_playlist_add_defaults(menu);

	menu_item_add_stock(menu, _("_Copy"), GTK_STOCK_COPY,
			G_CALLBACK(plw_playlist_copy_pressed), NULL);
	menu_item_add(menu, _("_Move"), G_CALLBACK(plw_playlist_move_pressed), NULL);
	menu_item_add(menu, _("Re_name"), G_CALLBACK(plw_playlist_rename_pressed), NULL);
	menu_item_add_stock(menu, _("_Delete"), GTK_STOCK_DELETE,
			G_CALLBACK(plw_playlist_delete_pressed), NULL);

	menu_playlist_add_shuffle(menu);

	return menu;
}

GtkWidget *menu_playlist_custom(void)
{
	GtkWidget *menu;

	menu = popup_menu_short_lived();

	menu_playlist_add_defaults(menu);

	menu_item_add(menu, _("_Edit..."), G_CALLBACK(plw_playlist_edit_pressed), NULL);

	menu_playlist_add_shuffle(menu);

	return menu;
}

