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


#ifndef DISPLAY_H
#define DISPLAY_H


#define DISPLAY_LOADING_ANIMATE_TIME 250


void display_register_widgets(UIData *ui);

void display_freeze(void);
void display_thaw(void);

void display_playlist_row_insert(gint n);
void display_playlist_row_remove(gint n);
void display_playlist_row_update(gint n);
void display_playlist_refresh(void);

void display_presets_refresh(void);

void display_set_song_count(gint n);
void display_set_song_number(gint n);

void display_set_shuffle(void);
void display_set_repeat(void);
void display_set_repeat_1(void);
void display_set_loop_ab(gint value);
void display_set_playlist_window(void);
void display_set_remaining_mode(void);
void display_set_total_mode(void);

void display_set_position(gfloat value);
void display_set_volume(void);
void display_set_balance(void);

void display_set_loading(gint animate);
void display_set_status(gint delay, gint value);

void display_set_frame_count(gint n);
void display_set_cpu(gfloat percent);

void display_total_time_changed(void);
gint display_total_time_has_changed(void);
void display_set_time(gint count, gint remain, gint override);

void display_set_io_information(gint zero);
void display_set_song_text_info(gint n, gint current);
void display_set_title(const gchar *text);

void display_set_playlist(const gchar *text);

void display_set_song_info(void);


#endif



