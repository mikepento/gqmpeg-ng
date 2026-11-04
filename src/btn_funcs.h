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


#ifndef BTN_FUNCS_H
#define BTN_FUNCS_H


gint main_window_widgets_update(void);
void btn_play_pressed(void);
void btn_stop_pressed(void);
void btn_pause_pressed(void);

void btn_next_pressed(void);
void btn_prev_pressed(void);
void btn_next_down(void);
void btn_next_up(void);
void btn_prev_down(void);
void btn_prev_up(void);

void position_slider_moved(float pos);
void position_slider_released(float pos);
void btn_rw_pressed(void);
void btn_ff_pressed(void);
void btn_rw_down(void);
void btn_rw_up(void);
void btn_ff_down(void);
void btn_ff_up(void);

void volume_slider_moved(float pos);
void btn_volume_up_pressed(void);
void btn_volume_down_pressed(void);
void btn_volume_released(void);

void balance_slider_moved(float pos);
void btn_balance_left_pressed(void);
void btn_balance_right_pressed(void);
void btn_balance_released(void);

void btn_loop_ab_pressed(void);
void btn_shuffle_pressed(void);
void btn_repeat_pressed(void);
void btn_repeat_1_pressed(void);
void time_toggle_pressed(void);
void time_remaining_pressed(void);
void btn_iconify_pressed(void);
void btn_mixer_pressed(void);
void current_song_info_pressed(void);
void current_song_pl_add_pressed(void);
void current_song_pl_rem_pressed(void);

void btn_load_playlist_pressed(void);
void btn_load_file_pressed(void);
void btn_load_custom_pressed(void);

void btn_save_pressed(void);

void preset_1_pressed(void);
void preset_2_pressed(void);
void preset_3_pressed(void);
void preset_4_pressed(void);
void preset_5_pressed(void);
void preset_6_pressed(void);
void preset_7_pressed(void);
void preset_8_pressed(void);
void preset_9_pressed(void);
void preset_10_pressed(void);
void set_preset_to_playlist(void);
void set_preset_to_song(void);

void btn_mode_pressed(void);
void btn_edit_skin_pressed(UIData *ui);


#endif

