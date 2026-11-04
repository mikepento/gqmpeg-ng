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
#include "display.h"

#include "btn_funcs.h"
#include "mixer.h"
#include "playlist.h"
#include "playlist-window.h"
#include "preferences.h"
#include "ui2_button.h"
#include "ui2_dial.h"
#include "ui2_item.h"
#include "ui2_list.h"
#include "ui2_main.h"
#include "ui2_number.h"
#include "ui2_slider.h"
#include "ui2_text.h"
#include "ui2_widget.h"


static gint dsp_frozen = FALSE;
static gint dsp_h = 0;
static gint dsp_m = 0;
static gint dsp_s = 0;
static gint dsp_song = 0;
static gint dsp_total_h = 0;
static gint dsp_total_m = 0;
static gint dsp_total_s = 0;
static gint dsp_song_total = 0;
static gint dsp_frame = 0;
static gint dsp_song_minute = 0;
static gint dsp_song_second = 0;

static gfloat dsp_position = 0.0;
static gfloat dsp_cpu = 0.0;


/*
 *-----------------------------------------------------------------------------
 * utility functions (private)
 *-----------------------------------------------------------------------------
 */

static void calc_times(gint *h, gint *m, gint *s, gint t, gint hour_exists)
{
	*h = t / 3600;
	*m = (t - (*h * 3600)) / 60;

	if (!hour_exists && *h > 0)
		{
		*s = *m;
		*m = *h;
		}
	else
		{
		*s = t - ((*h *3600) + (*m * 60));
		}
}

/*
 *-----------------------------------------------------------------------------
 * display widgets, cbs, blah blah
 *-----------------------------------------------------------------------------
 */

/* buttons w/light */

static gint button_play_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return (status == STATUS_PLAY || status == STATUS_NEXT);
}

static void button_play_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_play_pressed();
}

static gint button_stop_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return (status == STATUS_STOP);
}

static void button_stop_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_stop_pressed();
}

static gint button_pause_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return (status == STATUS_PAUSE);
}

static void button_pause_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_pause_pressed();
}

static gint button_repeat_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return repeat_mode;
}

static void button_repeat_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_repeat_pressed();
}

static gint button_repeat_1_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return repeat_1_enabled;
}

static void button_repeat_1_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_repeat_1_pressed();
}

static gint button_shuffle_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return shuffle_mode;
}

static void button_shuffle_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_shuffle_pressed();
}

static gint button_loop_ab_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return loop_ab_enabled;
}

static void button_loop_ab_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_loop_ab_pressed();
}

static gint button_time_remaining_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return show_remaining_time;
}

static void button_time_remaining_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	time_remaining_pressed();
}

static gint button_time_total_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return show_total_time;
}

static void button_time_total_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	time_toggle_pressed();
}

static gint button_playlist_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	return (playlist_window != NULL);
}

static void button_playlist_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	playlist_window_toggle();
}


/* buttons normal */

static void button_next_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_next_pressed();
}

static void button_next_press_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_next_down();
}

static void button_next_release_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_next_up();
}

static void button_prev_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_prev_pressed();
}

static void button_prev_press_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_prev_down();
}

static void button_prev_release_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_prev_up();
}

static void button_ff_press_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_ff_down();
}

static void button_ff_release_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_ff_up();
}

static void button_rw_press_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_rw_down();
}

static void button_rw_release_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_rw_up();
}

static void button_volume_up_press_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_volume_up_pressed();
}

static void button_volume_down_press_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_volume_down_pressed();
}

static void button_volume_release_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_volume_released();
}

static void button_balance_left_press_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_balance_left_pressed();
}

static void button_balance_right_press_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_balance_right_pressed();
}

static void button_balance_release_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_balance_released();
}

static void button_config_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	show_configuration_window();
}

static void button_exit_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	gqmpeg_exit();
}

static void button_iconify_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_iconify_pressed();
}

static void button_mixer_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_mixer_pressed();
}

static void button_open_file_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_load_file_pressed();
}

static void button_open_playlist_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_load_playlist_pressed();
}

static void button_open_custom_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_load_custom_pressed();
}

static void button_save_playlist_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	btn_save_pressed();
}

static void button_preset_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	gint n;

	n = GPOINTER_TO_INT(data);
	load_preset(n);
}


/* numbers */

static gint number_hour_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_h;
}

static gint number_minute_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_m;
}

static gint number_second_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_s;
}

static gint number_song_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_song;
}

static gint number_song_total_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return playlist_get_count();
}

static gint number_frame_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_frame;
}

static gint number_frame_total_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return frames + frames_remaining;
}

static gint number_input_rate_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return input_bitrate;
}

static gint number_input_hz_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return input_hz / 1000;
}

static gint number_output_bits_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return output_bits;
}

static gint number_output_hz_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return output_hz / 1000;
}

static gint number_cpu_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return (gint)dsp_cpu;
}

static gint number_song_minute_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_song_minute;
}

static gint number_song_second_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_song_second;
}

static gint number_playlist_hour_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_total_h;
}

static gint number_playlist_minute_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_total_m;
}

static gint number_playlist_second_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return dsp_total_s;
}

static gint number_track_status_cb(NumberData *number, const gchar *key, gpointer data)
{
	return current_song_get_track();
}

/* items */

static gint item_balance_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return item_section_by_percent(item, (gfloat)get_balance() / 100.0);
}

static gint item_load_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return 0;
}

static gint item_loop_ab_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return loop_ab_enabled;
}

static gint item_minus_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return show_remaining_time;
}

static gint item_mpeglayer_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return mpeg_layer;
}

static gint item_mpegmode_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return mpeg_mode;
}

static gint item_mpegversion_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return mpeg_version;
}

static gint item_position_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return item_section_by_percent(item, dsp_position);
}

static gint item_repeat_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return repeat_mode;
}

static gint item_repeat_1_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return repeat_1_enabled;
}

static gint item_shuffle_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return shuffle_mode;
}

static gint item_status_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return (status == STATUS_NEXT ? STATUS_PLAY : status);
}

static gint item_stereo_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return output_channels;
}

static gint item_time_mode_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	SongData *sd = current_song_get_data();

	if (sd && sd->live)
		{
		return 2;
		}

	return (show_total_time) ? 1 : 0;
}

static gint item_volume_status_cb(ItemData *item, const gchar *key, gpointer data)
{
	return item_section_by_percent(item, (gfloat)get_volume() / 100.0);
}

/* text */

static const gchar *text_title_status_cb(TextData *text, const gchar *key, gpointer data)
{
	text->autoscroll = title_scrolls_always;
	return current_song_get_title();
}

static const gchar *text_artist_status_cb(TextData *text, const gchar *key, gpointer data)
{
	return current_song_get_artist();
}

static const gchar *text_album_status_cb(TextData *text, const gchar *key, gpointer data)
{
	return current_song_get_album();
}

static const gchar *text_genre_status_cb(TextData *text, const gchar *key, gpointer data)
{
	return current_song_get_genre();
}

static const gchar *text_year_status_cb(TextData *text, const gchar *key, gpointer data)
{
	return current_song_get_year();
}

static const gchar *text_comment_status_cb(TextData *text, const gchar *key, gpointer data)
{
	return current_song_get_comment();
}

static const gchar *text_playlist_status_cb(TextData *text, const gchar *key, gpointer data)
{
	return playlist_filename;
}

/* sliders */

static gfloat slider_position_status_cb(SliderData *slider, const gchar *key, gpointer data)
{
	return dsp_position;
}

static void slider_position_release_cb(SliderData *slider, const gchar *key, gfloat value, gpointer data)
{
	position_slider_released(value);
}

static void slider_position_drag_cb(SliderData *slider, const gchar *key, gfloat value, gpointer data)
{
	position_slider_moved(value);
}

static gfloat slider_volume_status_cb(SliderData *slider, const gchar *key, gpointer data)
{
	return (gfloat)get_volume() / 100.0;
}

static void slider_volume_drag_cb(SliderData *slider, const gchar *key, gfloat value, gpointer data)
{
	volume_slider_moved(value);
}

static gfloat slider_balance_status_cb(SliderData *slider, const gchar *key, gpointer data)
{
	return (gfloat)get_balance() / 100.0;
}

static void slider_balance_drag_cb(SliderData *slider, const gchar *key, gfloat value, gpointer data)
{
	balance_slider_moved(value);
}

static gfloat dial_position_status_cb(DialData *dial, const gchar *key, gpointer data)
{
	return dsp_position;
}

static void dial_position_release_cb(DialData *dial, const gchar *key, gfloat value, gpointer data)
{
	position_slider_released(value);
}

static void dial_position_drag_cb(DialData *dial, const gchar *key, gfloat value, gpointer data)
{
	position_slider_moved(value);
}

static gfloat dial_volume_status_cb(DialData *dial, const gchar *key, gpointer data)
{
	return (gfloat)get_volume() / 100.0;
}

static void dial_volume_drag_cb(DialData *dial, const gchar *key, gfloat value, gpointer data)
{
	volume_slider_moved(value);
}

static gfloat dial_balance_status_cb(DialData *dial, const gchar *key, gpointer data)
{
	return (gfloat)get_balance() / 100.0;
}

static void dial_balance_drag_cb(DialData *dial, const gchar *key, gfloat value, gpointer data)
{
	balance_slider_moved(value);
}

/* lists */

static gint list_playlist_length_request_cb(ListData *list, const gchar *key, gpointer data)
{
	return playlist_get_count();
}

static gint flag_convert(SongData *sd)
{
	SongFlags flags;

	flags = sd->flags;

	if (flags & SONG_FLAG_PLAYING) return 0;
	if (flags & SONG_FLAG_NOT_FOUND) return 3;
	if (flags & SONG_FLAG_UNKNOWN_TYPE) return 2;
	if (flags & SONG_FLAG_PLAY_FAIL) return 1;

	return -1;
}

static gint list_playlist_row_info_cb(ListData *list, const gchar *key,
				      gint row, ListRowData *rd, gpointer data)
{
	SongData *sd;
	gchar *buf;

	sd = playlist_get_data(row);
	if (!sd) return FALSE;

	list_row_column_set_text(list, rd, "title", playlist_data_get_title(sd));
	buf = playlist_data_get_time(sd);
	list_row_column_set_text(list, rd, "time", buf);
	g_free(buf);
	buf = g_strdup_printf("%d.", row + 1);
	list_row_column_set_text(list, rd, "number", buf);
	g_free(buf);

	list_row_set_flag(rd, flag_convert(sd));

	playlist_update_background(sd);

	return TRUE;
}

static void list_playlist_select_cb(ListData *list, const gchar *key, gint row, ListRowData *rd, gpointer data)
{
	if (row != current_song_get_number() || status != STATUS_PLAY) current_song_set_and_play(row, NULL);
}

static void list_playlist_click_cb(ListData *list, const gchar *key, gint row, ListRowData *rd, gint button, gpointer data)
{
	if (row != current_song_get_number()) current_song_set(row, NULL);
}

static gint list_presets_length_request_cb(ListData *list, const gchar *key, gpointer data)
{
	return 10;
}

static gint list_presets_row_info_cb(ListData *list, const gchar *key,
				     gint row, ListRowData *rd, gpointer data)
{
	gchar *buf;

	if (row > 9 || row < 0) return FALSE;

	list_row_column_set_text(list, rd, "description", preset_name[row]);
	buf = g_strdup_printf("%d.", row + 1);
	list_row_column_set_text(list, rd, "number", buf);
	g_free(buf);

	list_row_set_flag(rd, (preset_is_active(row)) ? 0 : -1);

	return TRUE;
}

static void list_presets_click_cb(ListData *list, const gchar *key, gint row, ListRowData *rd, gint button, gpointer data)
{
	load_preset(row);
}

void display_register_widgets(UIData *ui)
{
	gint i;

	/* buttons w/light */

	button_register_key("play", ui,
			    button_play_status_cb, NULL,
			    button_play_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("stop", ui,
			    button_stop_status_cb, NULL,
			    button_stop_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("pause", ui,
			    button_pause_status_cb, NULL,
			    button_pause_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("repeat", ui,
			    button_repeat_status_cb, NULL,
			    button_repeat_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("repeat_1", ui,
			    button_repeat_1_status_cb, NULL,
			    button_repeat_1_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("shuffle", ui,
			    button_shuffle_status_cb, NULL,
			    button_shuffle_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("loop_ab", ui,
			    button_loop_ab_status_cb, NULL,
			    button_loop_ab_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("time_remaining", ui,
			    button_time_remaining_status_cb, NULL,
			    button_time_remaining_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("time_total", ui,
			    button_time_total_status_cb, NULL,
			    button_time_total_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("playlist", ui,
			    button_playlist_status_cb, NULL,
			    button_playlist_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	/* buttons normal */

	button_register_key("next", ui,
			    NULL, NULL,
			    button_next_click_cb, NULL,
			    button_next_press_cb, NULL,
			    button_next_release_cb, NULL);

	button_register_key("prev", ui,
			    NULL, NULL,
			    button_prev_click_cb, NULL,
			    button_prev_press_cb, NULL,
			    button_prev_release_cb, NULL);

	button_register_key("ff", ui,
			    NULL, NULL,
			    NULL, NULL,
			    button_ff_press_cb, NULL,
			    button_ff_release_cb, NULL);

	button_register_key("rw", ui,
			    NULL, NULL,
			    NULL, NULL,
			    button_rw_press_cb, NULL,
			    button_rw_release_cb, NULL);

	button_register_key("volume_up", ui,
			    NULL, NULL,
			    NULL, NULL,
			    button_volume_up_press_cb, NULL,
			    button_volume_release_cb, NULL);

	button_register_key("volume_down", ui,
			    NULL, NULL,
			    NULL, NULL,
			    button_volume_down_press_cb, NULL,
			    button_volume_release_cb, NULL);

	button_register_key("balance_left", ui,
			    NULL, NULL,
			    NULL, NULL,
			    button_balance_left_press_cb, NULL,
			    button_balance_release_cb, NULL);

	button_register_key("balance_right", ui,
			    NULL, NULL,
			    NULL, NULL,
			    button_balance_right_press_cb, NULL,
			    button_balance_release_cb, NULL);

	button_register_key("config", ui,
			    NULL, NULL,
			    button_config_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("exit", ui,
			    NULL, NULL,
			    button_exit_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("iconify", ui,
			    NULL, NULL,
			    button_iconify_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("mixer", ui,
			    NULL, NULL,
			    button_mixer_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("open_file", ui,
			    NULL, NULL,
			    button_open_file_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("open_playlist", ui,
			    NULL, NULL,
			    button_open_playlist_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("open_custom", ui,
			    NULL, NULL,
			    button_open_custom_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	button_register_key("save_playlist", ui,
			    NULL, NULL,
			    button_save_playlist_click_cb, NULL,
			    NULL, NULL,
			    NULL, NULL);

	for (i = 0; i < 10; i++)
		{
		gchar *buf;

		buf = g_strdup_printf("preset_%d", i + 1);
		button_register_key(buf, ui,
				    NULL, NULL,
				    button_preset_click_cb, GINT_TO_POINTER(i),
				    NULL, NULL,
				    NULL, NULL);
		g_free(buf);
		}

	/* numbers */

	number_register_key("hour", ui, number_hour_status_cb, NULL);
	number_register_key("minute", ui, number_minute_status_cb, NULL);
	number_register_key("second", ui, number_second_status_cb, NULL);

	number_register_key("song", ui, number_song_status_cb, NULL);
	number_register_key("song_total", ui, number_song_total_status_cb, NULL);

	number_register_key("frame", ui, number_frame_status_cb, NULL);
	number_register_key("frame_total", ui, number_frame_total_status_cb, NULL);

	number_register_key("input_rate", ui, number_input_rate_status_cb, NULL);
	number_register_key("input_hz", ui, number_input_hz_status_cb, NULL);
	number_register_key("output_bits", ui, number_output_bits_status_cb, NULL);
	number_register_key("output_hz", ui, number_output_hz_status_cb, NULL);

	number_register_key("cpu", ui, number_cpu_status_cb, NULL);

	number_register_key("song_minute", ui, number_song_minute_status_cb, NULL);
	number_register_key("song_second", ui, number_song_second_status_cb, NULL);

	number_register_key("playlist_hour", ui, number_playlist_hour_status_cb, NULL);
	number_register_key("playlist_minute", ui, number_playlist_minute_status_cb, NULL);
	number_register_key("playlist_second", ui, number_playlist_second_status_cb, NULL);

	number_register_key("track", ui, number_track_status_cb, NULL);

	/* items */

	item_register_key("balance", ui, item_balance_status_cb, NULL);
	item_register_key("load", ui, item_load_status_cb, NULL);
	item_register_key("loop_ab", ui, item_loop_ab_status_cb, NULL);
	item_register_key("minus", ui, item_minus_status_cb, NULL);
	item_register_key("mpeglayer", ui, item_mpeglayer_status_cb, NULL);
	item_register_key("mpegmode", ui, item_mpegmode_status_cb, NULL);
	item_register_key("mpegversion", ui, item_mpegversion_status_cb, NULL);
	item_register_key("position", ui, item_position_status_cb, NULL);
	item_register_key("repeat", ui, item_repeat_status_cb, NULL);
	item_register_key("repeat_1", ui, item_repeat_1_status_cb, NULL);
	item_register_key("shuffle", ui, item_shuffle_status_cb, NULL);
	item_register_key("status", ui, item_status_status_cb, NULL);
	item_register_key("stereo", ui, item_stereo_status_cb, NULL);
	item_register_key("time_mode", ui, item_time_mode_status_cb, NULL);
	item_register_key("volume", ui, item_volume_status_cb, NULL);

	/* text */

	text_register_key("title", ui, text_title_status_cb, NULL);
	text_register_key("artist", ui, text_artist_status_cb, NULL);
	text_register_key("album", ui, text_album_status_cb, NULL);
	text_register_key("genre", ui, text_genre_status_cb, NULL);
	text_register_key("year", ui, text_year_status_cb, NULL);
	text_register_key("comment", ui, text_comment_status_cb, NULL);

	text_register_key("playlist", ui, text_playlist_status_cb, NULL);

	/* sliders */

	slider_register_key("position", ui,
			    slider_position_status_cb, NULL,
			    slider_position_drag_cb, NULL,
			    slider_position_release_cb, NULL,
			    slider_position_drag_cb, NULL);

	slider_register_key("volume", ui,
			    slider_volume_status_cb, NULL,
			    slider_volume_drag_cb, NULL,
			    NULL, NULL,
			    slider_volume_drag_cb, NULL);

	slider_register_key("balance", ui,
			    slider_balance_status_cb, NULL,
			    slider_balance_drag_cb, NULL,
			    NULL, NULL,
			    slider_balance_drag_cb, NULL);

	/* dials */

	dial_register_key("position", ui,
			  dial_position_status_cb, NULL,
			  dial_position_drag_cb, NULL,
			  dial_position_release_cb, NULL,
			  dial_position_drag_cb, NULL);

	dial_register_key("volume", ui,
			  dial_volume_status_cb, NULL,
			  dial_volume_drag_cb, NULL,
			  NULL, NULL,
			  dial_volume_drag_cb, NULL);

	dial_register_key("balance", ui,
			  dial_balance_status_cb, NULL,
			  dial_balance_drag_cb, NULL,
			  NULL, NULL,
			  dial_balance_drag_cb, NULL);

	/* lists */

	list_register_key("playlist", ui,
			  list_playlist_length_request_cb, NULL,
			  list_playlist_row_info_cb, NULL,
			  list_playlist_click_cb, NULL,
			  list_playlist_select_cb, NULL,
			  NULL, NULL);

	list_register_key("presets", ui,
			  list_presets_length_request_cb, NULL,
			  list_presets_row_info_cb, NULL,
			  list_presets_click_cb, NULL,
			  NULL, NULL,
			  NULL, NULL);

}

/*
 *-----------------------------------------------------------------------------
 * display widget update functions (public)
 *-----------------------------------------------------------------------------
 */


void display_freeze(void)
{
	dsp_frozen = TRUE;
}

void display_thaw(void)
{
	dsp_frozen = FALSE;
}

void display_playlist_row_insert(gint n)
{
	if (!main_window || dsp_frozen) return;

	list_row_insert("playlist", main_window, n);
}

void display_playlist_row_remove(gint n)
{
	if (!main_window || dsp_frozen) return;

	list_row_remove("playlist", main_window, n);
}

void display_playlist_row_update(gint n)
{
	if (!main_window || dsp_frozen) return;

	list_row_update("playlist", main_window, n);
}

void display_playlist_refresh(void)
{
	if (!main_window || dsp_frozen) return;

	list_refresh("playlist", main_window);
}

void display_presets_refresh(void)
{
	if (!main_window) return;

	list_refresh("presets", main_window);
}

void display_set_song_count(gint n)
{
	dsp_song_total = n;

	if (!main_window || dsp_frozen) return;

	calc_times(&dsp_total_h, &dsp_total_m, &dsp_total_s, playlist_get_length(),
		   ui_widget_exists(main_window, "playlist_hour", number_type_id()));

	number_value_set("song_total", main_window, dsp_song_total);
	number_value_set("playlist_hour", main_window, dsp_total_h);
	number_value_set("playlist_minute", main_window, dsp_total_m);
	number_value_set("playlist_second", main_window, dsp_total_s);

	playlist_window_length_update();
}

void display_set_song_number(gint n)
{
	dsp_song = n + 1;

	if (!main_window || dsp_frozen) return;

	number_value_set("song", main_window, dsp_song);
}

void display_set_shuffle(void)
{
	if (!main_window) return;

	item_section_set("shuffle", main_window, shuffle_mode);
	button_state_set("shuffle", main_window, shuffle_mode);
}

void display_set_repeat(void)
{
	if (!main_window) return;

	item_section_set("repeat", main_window, repeat_mode);
	button_state_set("repeat", main_window, repeat_mode);
}

void display_set_repeat_1(void)
{
	if (!main_window) return;

	item_section_set("repeat_1", main_window, repeat_1_enabled);
	button_state_set("repeat_1", main_window, repeat_1_enabled);
}

void display_set_loop_ab(gint value)
{
	if (!main_window) return;

	button_state_set("loop_ab", main_window, value);
	item_section_set("loop_ab", main_window, value);
}

void display_set_playlist_window(void)
{
	if (!main_window) return;

	button_state_set("playlist", main_window, playlist_window != NULL);
}

void display_set_remaining_mode(void)
{
	if (!main_window) return;

	item_section_set("minus", main_window, show_remaining_time ? 1 : 0);
	button_state_set("time_remaining", main_window, show_remaining_time);
}

void display_set_total_mode(void)
{
	SongData *sd = current_song_get_data();

	if (!main_window) return;

	if (sd && sd->live)
		{
		item_section_set("time_mode", main_window, 2);
		}
	else
		{
		item_section_set("time_mode", main_window, show_total_time ? 1 : 0);
		}
	button_state_set("time_total", main_window, show_total_time);
}

void display_set_position(gfloat value)
{
	gint total;

	total = seconds_remaining + seconds;

	if (value >= 0.0)
		{
		dsp_position = value;
		}
	else if (total == 0)
		{
		dsp_position = 0.0;
		}
	else
		{
		dsp_position = (float)seconds / total;
		}

	if (!main_window) return;

	item_section_set_by_percent("position", main_window, dsp_position);
	dial_value_set("position", main_window, dsp_position);
	slider_value_set("position", main_window, dsp_position);
	if (total > 0)
		{
		slider_step_size_set("position", main_window, (1.0 / (gfloat)total), 10);
		}
}

void display_set_volume(void)
{
	gfloat val;

	if (!main_window) return;

	val = (float)get_volume() / 100.0;

	item_section_set_by_percent("volume", main_window, val);
	dial_value_set("volume", main_window, val);
	slider_value_set("volume", main_window, val);
}

void display_set_balance(void)
{
	gfloat val;

	if (!main_window) return;

	val = (float)get_balance() / 100.0;

	item_section_set_by_percent("balance", main_window, val);
	dial_value_set("balance", main_window, val);
	slider_value_set("balance", main_window, val);
}

void display_set_loading(gint animate)
{
	if (!main_window) return;

	item_animation_step("load", main_window, FALSE, !animate);
}

void display_set_status(gint delay, gint value)
{
	if (!main_window) return;

	if (delay)
		{
		button_state_set("play", main_window, value);
		button_state_set("pause", main_window, value);
		item_section_set("status", main_window, value ? STATUS_PLAY : STATUS_PAUSE);
		return;
		}

	button_state_set("play", main_window, status == STATUS_PLAY || status == STATUS_NEXT);
	button_state_set("stop", main_window, status == STATUS_STOP);
	button_state_set("pause", main_window, status == STATUS_PAUSE);
	item_section_set("status", main_window, status == STATUS_NEXT ? STATUS_PLAY : status);
}

void display_set_frame_count(gint n)
{
	dsp_frame = n;

	if (!main_window) return;

	number_value_set("frame", main_window, dsp_frame);
}

void display_set_cpu(gfloat percent)
{
	dsp_cpu = percent;

	if (!main_window) return;

	number_value_set("cpu", main_window, dsp_cpu);
}

static gint total_time_amount = 0;
static gint total_time_changed = TRUE;

static void display_total_time_recalc(void)
{
	SongData *sd;

	sd = current_song_get_data();

	if (sd && sd->live)
		{
		total_time_amount = 0;
		}
	else if (show_total_time)
		{
		gint n = current_song_get_number();
		if (n == -1)
			total_time_amount = 0;
		else if (show_remaining_time)
			total_time_amount = playlist_get_length_remaining(n);
		else
			total_time_amount = playlist_get_length_accumulated(n);
		}
	else
		{
		total_time_amount = 0;
		}

	total_time_changed = FALSE;
}

void display_total_time_changed(void)
{
	total_time_changed = TRUE;
}

gint display_total_time_has_changed(void)
{
	return total_time_changed;
}

void display_set_time(gint count, gint remain, gint override)
{
	gint n;

	if (!main_window) return;

	if (override)
		{
		n = count;
		}
	else
		{
		if (total_time_changed) display_total_time_recalc();

		if (show_remaining_time)
			{
			if (total_time_amount)
				n = total_time_amount - count;
			else
				n = remain;
			}
		else
			{
			n = total_time_amount + count;
			}
		}

	calc_times(&dsp_h, &dsp_m, &dsp_s, n, ui_widget_exists(main_window, "hour", number_type_id()));

	number_value_set("hour", main_window, dsp_h);
	number_value_set("minute", main_window, dsp_m);
	number_value_set("second", main_window, dsp_s);
}

void display_set_io_information(gint zero)
{
	gint h;

	if (!main_window) return;

	if (zero)
		{
		SongData *sd;

		sd = current_song_get_data();

		input_bitrate = (sd) ? sd->bit_rate : 0;
		input_hz = (sd) ? sd->khz_rate : 0;
#if 0
		frames = frames_remaining = 0;
#endif
		seconds =  0;
		seconds_remaining = (sd) ? sd->length : 0;

		output_channels = 0;
		mpeg_version = 0;
		mpeg_layer = 0;
		mpeg_mode = 0;
		}

	number_value_set("output_bits", main_window, output_bits);
	number_value_set("output_hz", main_window, output_hz / 1000);
	number_value_set("input_rate", main_window, input_bitrate);
	number_value_set("input_hz", main_window, input_hz / 1000);

	number_value_set("frame", main_window, frames);
	number_value_set("frame_total", main_window, frames + frames_remaining);
	calc_times(&h, &dsp_song_minute, &dsp_song_second, seconds + seconds_remaining, FALSE);
	number_value_set("song_minute", main_window, dsp_song_minute);
	number_value_set("song_second", main_window, dsp_song_second);

	item_section_set("stereo", main_window, output_channels);
	item_section_set("mpegversion", main_window, mpeg_version);
	item_section_set("mpeglayer", main_window, mpeg_layer);
	item_section_set("mpegmode", main_window, mpeg_mode);
}

void display_set_song_text_info(gint n, gint current)
{
	if (!main_window) return;

	if (current)
		{
		text_set_text("artist", main_window, current_song_get_artist());
		text_set_text("album", main_window, current_song_get_album());
		text_set_text("genre", main_window, current_song_get_genre());
		text_set_text("title", main_window, current_song_get_title());
		text_set_text("year", main_window, current_song_get_year());
		text_set_text("comment", main_window, current_song_get_comment());
		number_value_set("track", main_window, current_song_get_track());
		}
	else
		{
		text_set_text("artist", main_window, playlist_get_artist(n));
		text_set_text("album", main_window, playlist_get_album(n));
		text_set_text("genre", main_window, playlist_get_genre(n));
		text_set_text("title", main_window, playlist_get_title(n));
		text_set_text("year", main_window, playlist_get_year(n));
		text_set_text("comment", main_window, playlist_get_comment(n));
		number_value_set("track", main_window, playlist_get_track(n));
		}

	text_set_autoscroll("title", main_window, title_scrolls_always);

	/* remember speed was 8 with old version */
}

void display_set_title(const gchar *text)
{
	if (!main_window) return;

	text_set_text("title", main_window, text);
	text_set_autoscroll("title", main_window, title_scrolls_always);
}

void display_set_playlist(const gchar *text)
{
	if (!main_window) return;

	text_set_text("playlist", main_window, text);
}

/*
 *-----------------------------------------------------------------------------
 * display utility functions (public)
 *-----------------------------------------------------------------------------
 */

/*
 * this function assumes redraw of current song only,
 * and is mainly for initial song changes
 */
void display_set_song_info(void)
{
	if (!main_window) return;

	display_set_song_text_info(-1, TRUE);
	display_set_io_information(TRUE);
	display_set_total_mode();
	display_set_song_number(current_song_get_number());
}

