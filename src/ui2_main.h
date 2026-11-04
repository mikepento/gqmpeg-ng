/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#ifndef UI2_MAIN_H
#define UI2_MAIN_H


/* do not call this to close a ui! use ui_close() instead */
void ui_free(UIData *ui);

/* key is used for the window's subclass */
UIData *ui_new(const gchar *class, const gchar *key, gint decorations, const gchar *title);
void ui_title_set(UIData *ui, const gchar *text);
void ui_moveable_set(UIData *ui, gint moveable);
void ui_focus_set(UIData *ui, gint enable);

UIData *ui_new_dialog(const gchar *class, const gchar *subclass, gint decorations, const gchar *title);
UIData *ui_new_into_container(const gchar *class, const gchar *key, GtkWidget *widget);

UIData *ui_spawn_child(UIData *parent, const gchar *key, const gchar *title, const gchar *mode_key);

void ui_close(UIData *ui);

void ui_set_mouse_callback(UIData *ui, void (*func)(UIData *ui, gint button, guint32 time, gpointer data), gpointer data);
void ui_set_skin_callback(UIData *ui, SkinData *(*func)(UIData *ui, const gchar *key, gpointer data), gpointer data);
void ui_set_back_callback(UIData *ui, gint (*func)(UIData *ui, GdkPixbuf *pixbuf, gpointer data), gpointer data);
void ui_set_new_window_callback(UIData *ui, void (*func)(UIData *ui, const gchar *key, gpointer data), gpointer data);


gint ui_geometry_get(UIData *ui, gint *x, gint *y, gint *w, gint *h);

RegisterData *ui_register_key(UIData *ui, const gchar *key, WidgetType type, gpointer callbacks, guint length);
void ui_register_free_all(UIData *ui);

/* the private funcs are only for internal widget signals (ex: slider signal for list scrolling) */
RegisterData *ui_register_key_private(UIData *ui, const gchar *key, WidgetType type,
				      gpointer callbacks, guint length, gpointer widget);
void ui_register_free_private(UIData *ui);
void ui_unregister_key_private_for_widget(UIData *ui, gpointer widget);

gpointer ui_get_registered_callbacks(UIData *ui, const gchar *key, WidgetType type);

gint ui_registered_key_exists(UIData *ui, const gchar *key, WidgetType type);

gint ui_widget_exists(UIData *ui, const gchar *key, WidgetType type);

gint ui_registered_key_is_private(UIData *ui, const gchar *key, WidgetType type);

void ui_skin_set(UIData *ui, SkinData *skin, const gchar *path, const gchar *mode_key);
SkinData *ui_skin_load_default(UIData *ui, const gchar *key);
gint ui_skin_load(UIData *ui, const gchar *path, const gchar *mode_key);
gint ui_skin_mode_set(UIData *ui, const gchar *mode_key);
void ui_set_underlay(UIData *ui, GdkPixbuf *pb);
void ui_sync_states(void);

void ui_debug_print_registered_keys(UIData *ui);
void ui_debug_print_all_keys(UIData *ui);

void window_set_icon(GtkWidget *window, const char **icon, const gchar *file, GdkPixbuf *pixbuf);
void ui_set_icon(UIData *ui, const char **icon, const gchar *file, GdkPixbuf *pixbuf);
void ui_iconify(UIData *ui);

UIData *ui_find_by_key(const gchar *key);

void ui_group_set_child(UIData *parent, UIData *child);
void ui_group_unset_child(UIData *child);
UIData *ui_group_get_parent(UIData *ui);

GdkPixbuf *ui_slik_logo(void);
GtkWidget *ui_slik_credit(void);


#endif





