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


#ifndef PLAYLIST_H
#define PLAYLIST_H


GList *path_list_sort(GList *list);
GList *path_list_filter(GList *list, gint is_dir_list);

void shuffle_list_create(gint start_with_current);
void shuffle_list_destroy(void);

gint playlist_load_start(const gchar *fn, gint append, gint strict, gint parse_background, gint info_background);
gint playlist_load(const gchar *fn, gint append, gint strict);
gint playlist_load_from_file(const gchar *path, gint append, gint strict, gint show_warnings);
gint playlist_save(const gchar *fn);

void playlist_append_from_dir(const gchar *path, gint recursive);
gint playlist_insert_from_dir(const gchar *path, gint row, gint recursive);

SongData *playlist_data_new(const gchar *path, gint read_info);
void playlist_data_free(SongData *sd);

const gchar *playlist_data_get_path(SongData *sd);
const gchar *playlist_data_get_title(SongData *sd);
const gchar *playlist_data_get_artist(SongData *sd);
const gchar *playlist_data_get_album(SongData *sd);
const gchar *playlist_data_get_genre(SongData *sd);
const gchar *playlist_data_get_year(SongData *sd);
const gchar *playlist_data_get_comment(SongData *sd);
gint playlist_data_get_track(SongData *sd);
gchar *playlist_data_get_time(SongData *sd);

SongData *playlist_get_data(gint n);

const gchar *playlist_get_item(gint n);
const gchar *playlist_get_title(gint n);
const gchar *playlist_get_artist(gint n);
const gchar *playlist_get_album(gint n);
const gchar *playlist_get_genre(gint n);
const gchar *playlist_get_year(gint n);
const gchar *playlist_get_comment(gint n);
gint playlist_get_track(gint n);
gchar *playlist_get_time(gint n);

gint playlist_item_is_live(gint n);
gint playlist_item_is_custom(gint n);
gint playlist_get_index(const gchar *path);
gint playlist_get_index_by_data(SongData *sd);
gint playlist_get_count(void);
gint playlist_get_length(void);
gint playlist_get_length_accumulated(gint n);
gint playlist_get_length_remaining(gint n);
gint playlist_get_next(gint n);
gint playlist_get_prev(gint n);
gint playlist_get_first(void);
gint playlist_get_last(void);

void playlist_update_all_info(void);
void playlist_sort_by_func(GCompareFunc sort_func);

SongData *playlist_add(const gchar *path, gint read_info);
SongData *playlist_insert(const gchar *path, gint n, gint read_info);
gint playlist_move(gint s, gint t);
gint playlist_remove(const gchar *path, gint n, gint all);
void playlist_update_by_data_full(SongData *sd, gint force);
void playlist_update_by_data(SongData *sd);
void playlist_update_background(SongData *sd);
void playlist_update(gint n, const gchar *path);
void playlist_update_by_path(const gchar *path);
void playlist_replace(const gchar *old_path, const gchar *new_path);
void playlist_clear(void);

gint is_playlist(const gchar *path);
void playlist_randomize(void);

void playlist_data_customize_title(SongData *sd, const gchar *title);
void playlist_data_customize_comment(SongData *sd, const gchar *comment);

SongFlags playlist_get_flags(gint n);
void playlist_set_flags(gint n, SongFlags flags);
void playlist_unset_flags(gint n, SongFlags flags);


#endif

