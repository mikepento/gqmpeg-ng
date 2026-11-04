/*
 * GQmpeg
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "gqmpeg.h"

#include "ui_fileops.h"

#include "icon.xpm"

	/* main window widgets */
UIData 		*main_window = NULL;
GtkWidget	*playlist_window = NULL;

	/* file lists */
GList *filelist = NULL;
GList *dirlist = NULL;

	/* filtering */
GList *filter = NULL;
gint disable_filtering = FALSE;
gint show_dot_files = FALSE;

	/* playlist info */
gchar *playlist_pathname = NULL;
gchar *playlist_filename = NULL;

	/* song time info */
gint seconds = 0;
gint seconds_remaining = 0;
gint frames = 0;
gint frames_remaining = 0;

	/* input info */
gint input_channels = 0;
gint input_hz = 0;
gint input_bitrate = 0;

	/* output info */
gint output_channels = 0;
gint output_hz = 0;
gint output_bits = 0;

	/* mp3 specifics */
gint mpeg_version = 0;
gint mpeg_layer = 0;
gint mpeg_mode = 0;

	/* options */
gint		initial_playlist_mode = PLAYLIST_MODE_SESSION;
gchar		*initial_playlist_pathname = NULL;
gint		initial_playlist_position = 0;
gint		initial_directory_enable = FALSE;
gchar		*initial_directory_path = NULL;
gint		show_playlist_on_startup = FALSE;
gint		start_playing_on_startup = FALSE;
gint		read_file_information = TRUE;
gint		window_pos_x = 0;
gint		window_pos_y = 0;
gint		window_list_x = 0;
gint		window_list_y = 0;
gint		window_list_w = 700;
gint		window_list_h = 421;
gint		window_list_div_dir = 170;
gint		window_list_div_mid = 305;
gchar		*window_list_columns = NULL;
gint		wm_decorations = FALSE;
gint		prev_includes_current = FALSE;

	/* miscellaneous */
gchar		*current_path = NULL;
gchar 		*mixer_command = NULL;
gint		mixer_device_id = 0;

gint		status = STATUS_STOP;
gint		repeat_mode = FALSE;
gint		shuffle_mode = FALSE;
gint		save_mode_in_playlist = FALSE;
gint		obey_mode_in_playlist = FALSE;

gchar		*preset_name[10];
gchar		*preset_file[10];
gint		play_presets = TRUE;
gint		show_remaining_time = FALSE;
gint		show_total_time = FALSE;

int		pid;

gchar		*skin_default_path = NULL;
gchar		*skin_mode_key = NULL;

gint		pos_slider_in_drag = FALSE;
gint		new_song = FALSE;

gint		show_extra_winamp_info = TRUE;

gint		title_scrolls_always = FALSE;
gint		title_show_extension = FALSE;
gint		title_convert_underscores = TRUE;

gchar		**gqmpeg_icon_xpm = icon_xpm;

gint		allow_ipc = TRUE;

gint		dock_mode = FALSE;

gint		drop_single_to_playlist = FALSE;
gint		play_next_on_error = FALSE;

gint		loop_ab_enabled = FALSE;
gint		loop_ab_start = 0;
gint		loop_ab_end = 0;

gint		repeat_1_enabled = FALSE;

gint		change_delay = 0;

gint		flyby_enabled = FALSE;
gint		flyby_location = 3;

gint		skinned_menus_enable = FALSE;

/* end of globals */

void init_vars(void)
{
	gint i;

	playlist_pathname = g_strdup("");
	playlist_filename = g_strdup(_("Untitled.gqmpeg"));
	current_path = get_current_dir();

#if 0
	skin_default_path = g_strconcat(GQMPEG_SKINDIR, "/mrclean", NULL);
#endif

	for (i=0; i<10; i++)
		{
		preset_name[i] = NULL;
		preset_file[i] = NULL;
		}

	window_list_columns = g_strdup("");
}

