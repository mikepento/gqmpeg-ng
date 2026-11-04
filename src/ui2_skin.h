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

#ifndef UI_SKIN_H
#define UI_SKIN_H


SkinData *skin_new(void);
void skin_free(SkinData *s);

/* returns pixmap and pixbuf scratch buffers for use by widgets,
 * size may be larger than requested width and height
 * pixbuf does not have an alpha channel
 */
void skin_get_scratch(SkinData *skin, gint w, gint h, GdkPixbuf **pixbuf, GdkPixmap **pixmap);

GdkPixmap *skin_get_buffer(SkinData *s);
GdkPixbuf *skin_get_pixbuf(SkinData *s);
UIData *skin_get_ui(SkinData *skin);

void skin_sync_back(SkinData *s, UIData *ui);
void skin_set_underlay(SkinData *skin, UIData *ui, GdkPixbuf *pb);
void skin_sync_buffer(SkinData *si, UIData *ui);

/* key stuff */
WidgetData *skin_register_widget(SkinData *skin, const gchar *key, const gchar *text_id, WidgetType type, gpointer widget);
void skin_register_free_all(SkinData *skin);
WidgetData *skin_widget_get_by_key(SkinData *skin, const gchar *key, WidgetType type);
WidgetData *skin_widget_get_by_text_id(SkinData *skin, const gchar *text_id);
gint skin_widget_for_each_key(UIData *ui, const gchar *key, WidgetType type,
			      void (*func)(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui),
			      gpointer data);

void skin_debug_print_registered_keys(SkinData *skin);

void skin_widget_check_bounds(SkinData *skin, WidgetData *wd);
void skin_check_bounds(SkinData *skin);

void skin_resize(UIData *ui, gint w, gint h);

void skin_widgets_init(SkinData *skin, UIData *ui);

void skin_finalize(SkinData *skin);


#endif






