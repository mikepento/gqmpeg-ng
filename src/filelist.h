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


#ifndef FILELIST_H
#define FILELIST_H


gint filelist_count(void);
void filelist_add_item(const gchar *path);
void filelist_insert_item(const gchar *path, gint n);
void filelist_remove_item(gint n);
void filelist_swap_items(gint n1, gint n2);
const gchar *filelist_get_item(gint n);
gchar *filelist_get_path(gint n);
gint dirlist_count(void);
void dirlist_add_item(const gchar *path);
void dirlist_insert_item(const gchar *path, gint n);
void dirlist_remove_item(gint n);
void dirlist_swap_items(gint n1, gint n2);
const gchar *dirlist_get_item(gint n);
gchar *dirlist_get_path(gint n);
void filelist_refresh(void);
void change_to_path(const gchar *path);


#endif
