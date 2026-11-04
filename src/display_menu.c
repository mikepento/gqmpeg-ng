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
#include "display_menu.h"

#include "btn_funcs.h"
#include "playlist.h"
#include "playlist-window.h"
#include "preferences.h"
#include "skin_default.h"
#include "ui2_main.h"
#include "ui2_menu.h"


#define DIVIDER_TEXT " ------"


static void display_menu_play_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_play_pressed();
}

static void display_menu_pause_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_pause_pressed();
}

static void display_menu_stop_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_stop_pressed();
}

static void display_menu_playlist_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	playlist_window_show();
}

static void display_menu_preset_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	load_preset(id);
}

static void display_menu_preset_playlist_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	set_preset_to_playlist();
}

static void display_menu_preset_song_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	set_preset_to_song();
}

static void display_menu_open_playlist_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_load_playlist_pressed();
}

static void display_menu_open_file_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_load_file_pressed();
}

static void display_menu_open_custom_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_load_custom_pressed();
}

static void display_menu_save_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_save_pressed();
}

static void display_menu_shuffle_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_shuffle_pressed();
}

static void display_menu_repeat_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_repeat_pressed();
}

static void display_menu_repeat1_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_repeat_1_pressed();
}

static void display_menu_total_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	time_toggle_pressed();
}

static void display_menu_remain_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	time_remaining_pressed();
}

static void display_menu_prefs_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	show_configuration_window();
}

static void display_menu_song_info_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	current_song_info_pressed();
}

static void display_menu_song_add_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	current_song_pl_add_pressed();
}

static void display_menu_song_remove_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	current_song_pl_rem_pressed();
}

static void display_menu_mixer_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	btn_mixer_pressed();
}

static void display_menu_edit_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	UIData *ui = data;

	btn_edit_skin_pressed(ui);
}

static void display_menu_help_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	help_window_show("documentation");
}

static void display_menu_about_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	show_about_info();
}

static void display_menu_iconify_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	ui_iconify(main_window);
}

static void display_menu_close_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	UIData *ui = data;

	if (ui != main_window) ui_close(ui);
}

static void display_menu_exit_cb(UIMenuData *md, const gchar *path, gint id, gpointer data)
{
	gqmpeg_schedule_exit();
}


gint display_menu_popup(const gchar *path, gint x, gint y, gint button, guint32 event_time, UIData *ui)
{
	UIMenuData *md;
	gint i;
	gint enabled;

	enabled = (current_song_get_data() != NULL);

	md = ui_menu_new("GQmpeg", "main_menu");
	ui_group_set_child(main_window, md->ui);
	ui_set_skin_callback(md->ui, skin_load_default_menu_cb, md);

	ui_menu_item_add_sensitive(md, _("Play"), "/play", 0, enabled,
				   display_menu_play_cb, NULL);
	ui_menu_item_add_sensitive(md, _("Pause"), "/pause", 0, enabled,
				   display_menu_pause_cb, NULL);
	ui_menu_item_add_sensitive(md, _("Stop"), "/stop", 0, enabled,
				   display_menu_stop_cb, NULL);

	ui_menu_item_add_sensitive(md, DIVIDER_TEXT, "/div1", 0, FALSE,
				   NULL, NULL);

	ui_menu_item_add(md, _("Playlist..."), "/playlist", 0,
			 display_menu_playlist_cb, NULL);

	ui_menu_item_add(md, _("Presets"), "/presets", 0,
			 NULL, NULL);

	for (i = 0; i < 10; i++)
		{
		gchar *buf;
		gchar *text;

		buf = g_strdup_printf("/presets/preset_%d", i);
		if (preset_name[i] && preset_file[i])
			{
			text = g_strdup_printf("%d - %s", i + 1, preset_name[i]);
			ui_menu_item_add_check(md, text, buf, i, preset_is_active(i),
					       display_menu_preset_cb, NULL);
			}
		else
			{
			text = g_strdup_printf(_("%d - (empty)"), i + 1);
			ui_menu_item_add(md, text, buf, i,
					 NULL, NULL);
			}
		g_free(text);
		g_free(buf);
		}

	ui_menu_item_add_sensitive(md, DIVIDER_TEXT, "/presets/div1", 0, FALSE,
				   NULL, NULL);

	ui_menu_item_add(md, _("Set preset to playlist..."), "/presets/to_playlist", 0,
			 display_menu_preset_playlist_cb, NULL);
	ui_menu_item_add(md, _("Set preset to song..."), "/presets/to_song", 0,
			 display_menu_preset_song_cb, NULL);

	ui_menu_item_add(md, _("Open"), "/open", 0,
			 NULL, NULL);
	ui_menu_item_add(md, _("Playlist..."), "/open/playlist", 0,
			 display_menu_open_playlist_cb, NULL);
	ui_menu_item_add(md, _("File..."), "/open/file", 0,
			 display_menu_open_file_cb, NULL);
	ui_menu_item_add(md, _("Custom..."), "/open/custom", 0,
			 display_menu_open_custom_cb, NULL);

	ui_menu_item_add_sensitive(md, _("Save playlist..."), "/save", 0, (playlist_get_count() > 0),
			 display_menu_save_cb, NULL);

	ui_menu_item_add_sensitive(md, DIVIDER_TEXT, "/div2", 0, FALSE,
				   NULL, NULL);

	ui_menu_item_add(md, _("Toggle"), "/toggle", 0,
			 NULL, NULL);
	ui_menu_item_add_check(md, _("Shuffle"), "/toggle/shuffle", 0, shuffle_mode,
			       display_menu_shuffle_cb, NULL);
	ui_menu_item_add_check(md, _("Repeat"), "/toggle/repeat", 0, repeat_mode,
			       display_menu_repeat_cb, NULL);
	ui_menu_item_add_check(md, _("Repeat 1"), "/toggle/repeat1", 0, repeat_1_enabled,
			       display_menu_repeat1_cb, NULL);
	ui_menu_item_add_check(md, _("Time total"), "/toggle/total", 0, show_total_time,
			       display_menu_total_cb, NULL);
	ui_menu_item_add_check(md, _("Time remaining"), "/toggle/remain", 0, show_remaining_time,
			       display_menu_remain_cb, NULL);

	ui_menu_item_add(md, _("Preferences..."), "/prefs", 0,
			 display_menu_prefs_cb, NULL);

	ui_menu_item_add_sensitive(md, DIVIDER_TEXT, "/div3", 0, FALSE,
				   NULL, NULL);

	ui_menu_item_add_sensitive(md, _("Current song"), "/current", 0, enabled,
				   NULL, NULL);
	ui_menu_item_add(md, _("Song Info..."), "/current/info", 0,
			 display_menu_song_info_cb, NULL);
	ui_menu_item_add(md, _("Add to playlist"), "/current/add", 0,
			 display_menu_song_add_cb, NULL);
	ui_menu_item_add(md, _("Remove from playlist"), "/current/remove", 0,
			 display_menu_song_remove_cb, NULL);

	ui_menu_item_add(md, _("Run mixer..."), "/mixer", 0,
			 display_menu_mixer_cb, NULL);

	ui_menu_item_add_sensitive(md, DIVIDER_TEXT, "/div4", 0, FALSE,
				   NULL, NULL);

	ui_menu_item_add(md, _("Skin Editor..."), "/editor", 0,
			 display_menu_edit_cb, ui);
	ui_menu_item_add(md, _("Help..."), "/help", 0,
			 display_menu_help_cb, NULL);
	ui_menu_item_add(md, _("About..."), "/about", 0,
			 display_menu_about_cb, NULL);

	ui_menu_item_add_sensitive(md, DIVIDER_TEXT, "/div5", 0, FALSE,
				   NULL, NULL);

	ui_menu_item_add(md, _("Iconify"), "/iconify", 0,
			 display_menu_iconify_cb, NULL);
	if (ui != main_window)
		{
		ui_menu_item_add(md, _("Close window"), "/close", 0,
				 display_menu_close_cb, ui);
		}
	ui_menu_item_add(md, _("Quit"), "/quit", 0,
			 display_menu_exit_cb, NULL);

	if (!ui_skin_load(md->ui, path, "skindata_menu"))
		{
		ui_skin_load(md->ui, NULL, "skindata_menu");
		}
	ui_menu_show(md, -1, -1, button, event_time);

	return TRUE;
}

