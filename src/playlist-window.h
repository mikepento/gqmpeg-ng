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


#ifndef PLAYLIST_WINDOW_H
#define PLAYLIST_WINDOW_H


void playlist_window_update_song_icon_by_flags(gint n, SongFlags flags);
void playlist_window_update_song_flags(gint n);

void playlist_window_append(gint n);
void playlist_window_insert(gint n);
void playlist_window_move(gint s, gint t);
void playlist_window_remove(gint n);
void playlist_window_update_item(gint n);
void playlist_window_clear(void);
void playlist_window_populate(void);

void plw_add_dir_pressed(void);
void plw_add_dir_recursive_pressed(void);

void plw_filelist_songinfo_pressed(void);
void plw_filelist_move_pressed(void);
void plw_filelist_copy_pressed(void);
void plw_filelist_rename_pressed(void);
void plw_filelist_delete_pressed(void);
void plw_filelist_add_play_pressed(void);
void plw_filelist_play_pressed(void);
void plw_filelist_add_pressed(void);
void plw_filelist_insert_pressed(void);
void plw_load_pressed(void);
void plw_append_pressed(void);

void plw_playlist_songinfo_pressed(void);
void plw_playlist_edit_pressed(void);
void plw_playlist_edit_title_pressed(void);
void plw_playlist_edit_comment_pressed(void);
void plw_playlist_songinfo_pressed(void);
void plw_playlist_edit_pressed(void);
void plw_playlist_songinfo_pressed(void);
void plw_playlist_edit_pressed(void);
void plw_playlist_songinfo_pressed(void);
void plw_playlist_edit_pressed(void);
void plw_playlist_move_pressed(void);
void plw_playlist_copy_pressed(void);
void plw_playlist_rename_pressed(void);
void plw_playlist_delete_pressed(void);
void plw_playlist_remove_pressed(void);
void plw_playlist_play_pressed(void);
void plw_playlist_randomize_pressed(void);

void playlist_window_length_update(void);
void playlist_window_update_titles(void);

void playlist_window_filelist_update(void);

void playlist_window_toggle(void);
void playlist_window_show(void);
void playlist_window_hide(void);


#endif
