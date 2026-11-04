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


#ifndef PLAYLIST_DLG_H
#define PLAYLIST_DLG_H


void playlist_dialog_save_as(gchar *path);
void playlist_dialog_load(gchar *path);
void playlist_dialog_append(gchar *path);
void playlist_dialog_import(gchar *path);

void playlist_dialog_add_custom_type(gint to_playlist);
void playlist_dialog_edit_custom_type(SongData *sd);

void playlist_dialog_load_file(const gchar *path, gint to_playlist);


#endif
