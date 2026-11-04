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


#ifndef TYPES_H
#define TYPES_H


gint file_is_hidden(const gchar *name);

gint file_is_in_filter(const gchar *name);
void add_to_filter(const gchar *text, const gchar *description, gint module_id);
void remove_from_filter(const gchar *text);
gint filter_determine_module_id(const gchar *path);
gchar *filter_get_text_list(void);
gchar *filter_get_text_list_by_id(gint module_id);
const gchar *filter_get_description_by_id(gint module_id);

void add_to_typelist(const gchar *format, const gchar *description, gint module_id, gint live,
		     gint (*is_type_func)(const gchar *),
		     GtkWidget *(*entry_setup_func)(const gchar *),
		     GtkWidget *(*edit_func)(SongData *),
		     gchar *(*get_path_func)(GtkWidget *));
gint typelist_determine_module_id(const gchar *path);
gint typelist_determine_type_id(const gchar *path);
void typelist_determine_ids(const gchar *path, gint *module_id, gint *custom, gint *custom_type, gint *live);
GtkWidget *typelist_get_entry_widget_by_id(gint id, const gchar *path);
GtkWidget *typelist_get_edit_widget_by_id(gint id, SongData *sd);
gchar *typelist_get_path_from_widget(gint id, GtkWidget *widget);
GList *typelist_get_description_list(void);


#endif
