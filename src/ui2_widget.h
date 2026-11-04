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

#ifndef UI2_WIDGET_H
#define UI2_WIDGET_H


#define UI_WIDGET_PRIORITY_NORMAL 0
#define UI_WIDGET_PRIORITY_HIGH   1


void ui_widget_free(WidgetData *wd);
WidgetData *ui_widget_new(const gchar *key, const gchar *text_id, WidgetType type, gpointer widget);


void ui_widget_motion(UIData *ui, WidgetData *wd, gint x, gint y);
gint ui_widget_press(UIData *ui, WidgetData *wd, gint x, gint y);
void ui_widget_release(UIData *ui, WidgetData *wd, gint x, gint y);

void ui_widget_draw(UIData *ui, WidgetData *wd, gint update, gint force);
void ui_widget_reset(UIData *ui, WidgetData *wd);
void ui_widget_sync_back(UIData *ui, WidgetData *wd);

void ui_widget_hide(UIData *ui, WidgetData *wd);
void ui_widget_show(UIData *ui, WidgetData *wd);

gint ui_widget_focus_draw(UIData *ui, WidgetData *wd, gint x, gint y, gint w, gint h);
gint ui_widget_focus_key_event(UIData *ui, WidgetData *wd, GdkEventKey *event);

gint ui_widget_can_focus(WidgetData *wd);

void ui_widget_hide_by_key(UIData *ui, const gchar *key, WidgetType type);
void ui_widget_show_by_key(UIData *ui, const gchar *key, WidgetType type);

gint ui_widget_get_geometry(WidgetData *wd, gint *x, gint *y, gint *w, gint *h);
void ui_widget_set_coord(UIData *ui, WidgetData *wd, gint x, gint y, gint redraw);
void ui_widget_set_size(UIData *ui, WidgetData *wd, gint dev_w, gint dev_h, gint redraw);
void ui_widget_set_anchor(WidgetData *wd, gint right, gint bottom);

gint ui_widget_for_each_key(UIData *ui, const gchar *key, WidgetType type,
                            void (*func)(WidgetData *wd, gpointer data, UIData *ui),
                            gpointer data);
/* this one stays within the ui, (no parent/child propogation) */
gint ui_widget_for_each_key_one(UIData *ui, const gchar *key, WidgetType type,
			        void (*func)(WidgetData *wd, gpointer data, UIData *ui),
			        gpointer data);

WidgetData *ui_widget_find_by_coord(UIData *ui, gint x, gint y);
gint ui_widget_contacts_area(WidgetData *wd, gint x, gint y, gint w, gint h);

WidgetData *ui_widget_get_by_widget(UIData *ui, gpointer widget);

void ui_widget_set_data(WidgetData *wd, const gchar *key, const gchar *data);
const gchar *ui_widget_get_data(WidgetData *wd, const gchar *key);

const gchar *ui_widget_get_data_by_widget(UIData *ui, gpointer widget);

WidgetObjectData *ui_widget_type_new(const gchar *description);
WidgetObjectData *ui_widget_object_by_type(WidgetType type);
WidgetObjectData *ui_widget_object_by_text(const gchar *description);
const gchar *ui_widget_type_to_text(WidgetType type);
WidgetObjectData *ui_widget_object_copy(WidgetObjectData *src);

#endif


