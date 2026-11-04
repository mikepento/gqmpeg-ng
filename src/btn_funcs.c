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
#include "btn_funcs.h"

#include "display.h"
#include "mixer.h"
#include "players.h"
#include "playlist.h"
#include "playlist-dlg.h"
#include "preset-dlg.h"
#include "ui2_button.h"
#include "ui2_editor.h"
#include "ui2_main.h"
#include "ui2_skin.h"
#include "ui2_slider.h"
#include "ui2_text.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "songinfo.h"


/*
 *-----------------------------------------------------------------------------
 * static vars
 *-----------------------------------------------------------------------------
 */

static gint time_seek = 0;
static gint time_seek_pos = 0;
static gint time_seek_max = 0;

static gint song_seek = 0;
static gint song_seek_pos = -1;

static gint vol_btn_down = 0;
static gint bal_btn_down = 0;

/*
 *-----------------------------------------------------------------------------
 * static funcs
 *-----------------------------------------------------------------------------
 */

static void adjust_volume_by(gint n);
static void adjust_balance_by(gint n);

/*
 *-----------------------------------------------------------------------------
 * main window widget time dependant routine
 *-----------------------------------------------------------------------------
 */

gint main_window_widgets_update(void)
{
	static gint old_song_seek = 0;

	if (time_seek != 0) return TRUE;

	if (song_seek != 0 && song_seek_pos >= 0)
		{
		static int trip_cnt;
		if (song_seek < 8 && song_seek > -8)
			{
			if (song_seek > 0)
				{
				song_seek++;
				}
			else
				{
				song_seek--;
				}
			trip_cnt = 0;
			}
		else
			{
			if (song_seek < 32 && song_seek > -32)
				{
				trip_cnt++;
				if (trip_cnt > 2) trip_cnt = 0;
				if (song_seek > 0)
					{
					song_seek++;
					}
				else
					{
					song_seek--;
					}
				}
			else
				{
				trip_cnt = 0;
				}
			if (trip_cnt == 0)
				{
				gint p;
				if (song_seek > 0)
					{
					p = playlist_get_next(song_seek_pos);
					}
				else
					{
					p = playlist_get_prev(song_seek_pos);
					}
				if (p >= 0)
					{
					song_seek_pos = p;
					display_set_song_number(song_seek_pos);
					display_set_title(playlist_get_title(song_seek_pos));
					}
				}
			}
		old_song_seek = song_seek;
		}
	else if (old_song_seek != song_seek)
		{
		old_song_seek = song_seek;
		if (song_seek_pos != current_song_get_number())
                        {
                        display_set_song_number(current_song_get_number());
			display_set_title(current_song_get_title());
			}
		}

	if (vol_btn_down !=0)
		{
		if (vol_btn_down == 1)
			adjust_volume_by(3);
		else
			adjust_volume_by(-3);
		}

	if (bal_btn_down !=0)
		{
		if (bal_btn_down == 1)
			adjust_balance_by(-3);
		else
			adjust_balance_by(3);
		}

	return FALSE;
}

/*
 *-----------------------------------------------------------------------------
 * main play function button callbacks
 *-----------------------------------------------------------------------------
 */

void btn_play_pressed(void)
{
	if (status == STATUS_PLAY)
		{
		playback_exec_command(EXEC_PAUSE, current_song_get_data(), 0);
		}
	else if (status == STATUS_PAUSE)
		{
		playback_exec_command(EXEC_CONTINUE, current_song_get_data(), 0);
		}
	else
		{
		playback_exec_command(EXEC_PLAY, current_song_get_data(), 0);
		}
}

void btn_stop_pressed(void)
{
	if (status == STATUS_STOP)
		{
		/* already stopped, now reset to top of playlist */
		if (current_song_is_in_playlist() && current_song_get_number() != playlist_get_first())
			{
			current_song_set(playlist_get_first(), NULL);
			}
		return;
		}
	playback_exec_command(EXEC_STOP, current_song_get_data(), 0);
}

void btn_pause_pressed(void)
{
	if (status == STATUS_STOP) return;
	if (status == STATUS_NEXT)
		{
		playback_exec_command(EXEC_PLAY, current_song_get_data(), 0);
		}
	else if (status == STATUS_PLAY)
		{
		playback_exec_command(EXEC_PAUSE, current_song_get_data(), 0);
		}
	else
		{
		playback_exec_command(EXEC_CONTINUE, current_song_get_data(), 0);
		}
}

/*
 *-----------------------------------------------------------------------------
 * song positioning button callbacks
 *-----------------------------------------------------------------------------
 */

void btn_next_pressed(void)
{
	song_seek = 0;
	if (song_seek_pos == -1)
		{
		if (!repeat_mode) return;
		song_seek_pos = playlist_get_first();
		}
	current_song_set(song_seek_pos, NULL);
}

void btn_prev_pressed(void)
{
	btn_next_pressed();
}

void btn_next_down(void)
{
	if (playlist_get_count() > 0)
		{
		song_seek = 1;
		}
	else
		{
		song_seek = 0;
		song_seek_pos = -1;
		return;
		}
	song_seek_pos = playlist_get_next(current_song_get_number());
	if (song_seek_pos >= 0)
		{
		display_set_song_number(song_seek_pos);
		display_set_title(playlist_get_title(song_seek_pos));
		}
}

void btn_next_up(void)
{
	song_seek = 0;
}

void btn_prev_down(void)
{
	if (playlist_get_count() > 0)
		{
		song_seek = -1;
		}
	else
		{
		song_seek = 0;
		song_seek_pos = -1;
		return;
		}
	song_seek_pos = current_song_get_number();
	if (!((status == STATUS_PLAY || status == STATUS_PAUSE) && prev_includes_current && seconds > 4))
		{
		gint p;
		p = playlist_get_prev(current_song_get_number());
		if (p >= 0)
			{
			song_seek_pos = p;
			display_set_song_number(song_seek_pos);
			display_set_title(playlist_get_title(song_seek_pos));
			}
		}
}

void btn_prev_up(void)
{
	song_seek = 0;
}

/*
 *-----------------------------------------------------------------------------
 * time positioning slider callbacks
 *-----------------------------------------------------------------------------
 */

void position_slider_moved(float pos)
{
	gint secs;
	gint frms;

	pos_slider_in_drag = TRUE;

	if (seconds_remaining + seconds == 0 || status == STATUS_STOP) return;

	secs = (float)(seconds_remaining + seconds) * pos;
	frms = (float)(frames_remaining + frames) * pos;
	display_set_frame_count(frms);
	display_set_time(secs, seconds_remaining + seconds - secs, FALSE);
}

void position_slider_released(float pos)
{
	gint secs;

	pos_slider_in_drag = FALSE;

	if (seconds_remaining + seconds == 0 || status == STATUS_STOP) return;

	secs = (float)pos * (seconds_remaining + seconds);
	playback_exec_command(EXEC_SEEK, current_song_get_data(), secs);
}

/*
 *-----------------------------------------------------------------------------
 * time positioning button callbacks
 *-----------------------------------------------------------------------------
 */

static gint time_seeking_callback(gpointer data)
{
	gint fp;

	if (time_seek == 0) return FALSE;

	/* fast forwarding */
	if (time_seek > 0)
		{
		if (time_seek < 10)
			{
			time_seek_pos += 1;
			time_seek++;
			}
		else
			{
			time_seek_pos += 5;
			}
		if (time_seek_pos > time_seek_max - 2) time_seek_pos = 0;
		}
	/* rewinding */
	if (time_seek < 0)
		{
		if (time_seek > -10)
			{
			time_seek_pos -= 1;
			time_seek--;
			}
		else
			{
			time_seek_pos -= 5;
			}
		if (time_seek_pos < 0) time_seek_pos = time_seek_max - 2;
		}
	display_set_time(time_seek_pos, time_seek_max - time_seek_pos, FALSE);
	display_set_position((float)time_seek_pos / time_seek_max);

	fp = (float) time_seek_pos / time_seek_max * (frames + frames_remaining);
	display_set_frame_count(fp);
	return TRUE;
}

static void time_seeking_start(void)
{
	time_seek_max = seconds + seconds_remaining;
	if (time_seek_max == 0) time_seek_max = 1; /* avoid div by zero */
	time_seek_pos = seconds;
	gtk_timeout_add(200, time_seeking_callback, NULL);
}

static void time_seeking_stop(void)
{
	if (time_seek == 0) return;
	time_seek = 0;
	playback_exec_command(EXEC_SEEK, current_song_get_data(), time_seek_pos);
}

void btn_rw_pressed(void)
{
	if (time_seek != 0) return;
	if (status != STATUS_PLAY && status != STATUS_PAUSE) return;
	time_seek = -1;
	time_seeking_start();
}

void btn_ff_pressed(void)
{
	if (time_seek != 0) return;
	if (status != STATUS_PLAY && status != STATUS_PAUSE) return;
	time_seek = 1;
	time_seeking_start();
}

void btn_rw_down(void)
{
	btn_rw_pressed();
}

void btn_rw_up(void)
{
	time_seeking_stop();
}

void btn_ff_down(void)
{
	btn_ff_pressed();
}

void btn_ff_up(void)
{
	time_seeking_stop();
}

/*
 *-----------------------------------------------------------------------------
 * volume callbacks
 *-----------------------------------------------------------------------------
 */

static void adjust_volume_by(gint n)
{
	gint vol = get_volume();
	vol += n;
	if (vol < 0) vol = 0;
	if (vol > 100) vol = 100;
	set_volume(vol);
	display_set_volume();
}

void volume_slider_moved(float pos)
{
	set_volume((gint)(pos * 100));
}

void btn_volume_up_pressed(void)
{
	vol_btn_down = 1;
	adjust_volume_by(3);
}

void btn_volume_down_pressed(void)
{
	vol_btn_down = 2;
	adjust_volume_by(-3);
}

void btn_volume_released(void)
{
	vol_btn_down = 0;
}

/*
 *-----------------------------------------------------------------------------
 * balance callbacks
 *-----------------------------------------------------------------------------
 */

static void adjust_balance_by(gint n)
{
	gint bal = get_balance();
	bal += n;
	if (bal < 0) bal = 0;
	if (bal > 100) bal = 100;
	if (bal > 50 - abs(n) && bal < 50 + abs(n)) bal = 50; /* basic balance auto-centering */
	set_balance(bal);
	display_set_balance();
}

void balance_slider_moved(float pos)
{
	gint p = pos * 100;
	if (p > 44 && p < 56) p = 50; /* to help balance 'lock' on center */
	set_balance(p);
}

void btn_balance_left_pressed(void)
{
	bal_btn_down = 1;
	adjust_balance_by(-3);
}

void btn_balance_right_pressed(void)
{
	bal_btn_down = 2;
	adjust_balance_by(3);
}

void btn_balance_released(void)
{
	bal_btn_down = 0;
}

/*
 *-----------------------------------------------------------------------------
 * misc callbacks
 *-----------------------------------------------------------------------------
 */

void btn_loop_ab_pressed(void)
{
	if (loop_ab_enabled)
		{
		if (loop_ab_end == 0 && loop_ab_start < seconds)
			{
			loop_ab_end = seconds;
			display_set_loop_ab(TRUE);
			playback_exec_command(EXEC_SEEK, current_song_get_data(), loop_ab_start);
			}
		else
			{
			loop_ab_enabled = FALSE;
			display_set_loop_ab(FALSE);
			}
		}
	else if (status == STATUS_PLAY || status == STATUS_PAUSE)
		{
		SongData *sd;

		/* check and ignore live sources (can't seek) */
		sd = current_song_get_data();
		if (!sd || sd->live) return;

		loop_ab_enabled = TRUE;
		loop_ab_start = seconds;
		loop_ab_end = 0;
		display_set_loop_ab(TRUE);
		}
}

void btn_shuffle_pressed(void)
{
	if (shuffle_mode)
		{
		shuffle_mode = FALSE;
		shuffle_list_destroy();
		}
	else
		{
		shuffle_mode = TRUE;
		shuffle_list_create(TRUE);
		}
	display_set_shuffle();
	display_total_time_changed();
}

void btn_repeat_pressed(void)
{
	repeat_mode = !repeat_mode;
	display_set_repeat();
}

void btn_repeat_1_pressed(void)
{
	repeat_1_enabled = !repeat_1_enabled;
	display_set_repeat_1();
}

void time_toggle_pressed(void)
{
	show_total_time = !show_total_time;
	if (status == STATUS_PLAY || status == STATUS_PAUSE)
		{
		display_set_time(seconds, seconds_remaining, FALSE);
		}
	display_set_total_mode();
	display_total_time_changed();
}

void time_remaining_pressed(void)
{
	show_remaining_time = !show_remaining_time;
	if (status == STATUS_PLAY || status == STATUS_PAUSE)
		{
		display_set_time(seconds, seconds_remaining, FALSE);
		}
	display_set_remaining_mode();
	display_total_time_changed();
}

void btn_iconify_pressed(void)
{
	ui_iconify(main_window);
}

void btn_mixer_pressed(void)
{
	if (mixer_command)
		{
		gchar *command = g_strconcat(mixer_command, " &", NULL);
		system(command);
		g_free(command);
		}
}

void current_song_info_pressed(void)
{
	view_song_info(current_song_get_path());
}

void current_song_pl_add_pressed(void)
{
	const gchar *path = current_song_get_path();

	if (path) playlist_add(path, TRUE);
}

void current_song_pl_rem_pressed(void)
{
	gint n = current_song_get_number();

	if (n > -1) playlist_remove(NULL, n, FALSE);
}

/*
 *-----------------------------------------------------------------------------
 * load callbacks
 *-----------------------------------------------------------------------------
 */

void btn_load_playlist_pressed(void)
{
	playlist_dialog_load(NULL);
}

void btn_load_file_pressed(void)
{
	playlist_dialog_load_file(NULL, FALSE);
}

void btn_load_custom_pressed(void)
{
	playlist_dialog_add_custom_type(FALSE);
}

void btn_save_pressed(void)
{
	playlist_dialog_save_as(playlist_pathname);
}

/*
 *-----------------------------------------------------------------------------
 * preset callbacks
 *-----------------------------------------------------------------------------
 */

void preset_1_pressed(void)
{
	load_preset(0);
}

void preset_2_pressed(void)
{
	load_preset(1);
}

void preset_3_pressed(void)
{
	load_preset(2);
}

void preset_4_pressed(void)
{
	load_preset(3);
}

void preset_5_pressed(void)
{
	load_preset(4);
}

void preset_6_pressed(void)
{
	load_preset(5);
}

void preset_7_pressed(void)
{
	load_preset(6);
}

void preset_8_pressed(void)
{
	load_preset(7);
}

void preset_9_pressed(void)
{
	load_preset(8);
}

void preset_10_pressed(void)
{
	load_preset(9);
}

void set_preset_to_playlist(void)
{
	preset_dialog(playlist_pathname, TRUE);
}

void set_preset_to_song(void)
{
	preset_dialog(current_song_get_path(), FALSE);
}

void btn_mode_pressed(void)
{
	WidgetData *wd;

	/* there can be many, but simply use the first one */
	wd = skin_widget_get_by_key(main_window->skin, "skin_toggle", button_type_id());
	if (wd)
		{
		const gchar *mode_key;
		mode_key = ui_widget_get_data(wd, "data");
		ui_skin_mode_set(main_window, mode_key);
		}
}

void btn_edit_skin_pressed(UIData *ui)
{
	EditData *ed;
	GList *list = NULL;

	ed = ui_edit_new(ui, "GQmpeg", "skin_editor", _("Skin editor - GQmpeg"));

	/* ed is NULL if an edit window is already active for the ui */
	if (!ed) return;

	ui_edit_set_help_path(ed, GQMPEG_SKIN_HELP_PATH);

	list = g_list_append(list, "skindata");
	list = g_list_append(list, "skindata_menu");
	list = g_list_append(list, "skindata_flyby");
	list = g_list_append(list, "skindata_alt");

	ui_edit_set_skindata_list(ed, list);

	g_list_free(list);
}

