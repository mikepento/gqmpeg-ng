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

#ifndef UI2_EDITOR_H
#define UI2_EDITOR_H


void ui_edit_frame_sensitive(GtkWidget *widget, gint state, gint stop_at_box);
GtkWidget *ui_edit_frame_new(GtkWidget *box, gint has_frame, const gchar *text);
GtkWidget *ui_edit_vframe_new(GtkWidget *box, gint has_frame, const gchar *text);

GtkWidget *ui_edit_label_new(GtkWidget *box, const gchar *text);

GtkWidget *ui_edit_image_new(GtkWidget *box);
void ui_edit_image_blank(GtkWidget *image);

GtkWidget *ui_edit_toggle_new(GtkWidget *box, const gchar *text);
void ui_edit_toggle_set(GtkWidget *button, gint active);
void ui_edit_toggle_set_blocking(GtkWidget *button, gint active, gpointer block_data);
gint ui_edit_toggle_get(GtkWidget *button);

GtkWidget *ui_edit_font_entry_new(GtkWidget *box, const gchar *text);

GtkWidget *ui_edit_path_entry_new(GtkWidget *box, const gchar *text, const gchar *history_key);
void ui_edit_path_entry_connect_image(GtkWidget *entry, GtkWidget *image);

GtkWidget *ui_edit_key_entry_new(GtkWidget *box, UIData *ui, WidgetType type);

GtkWidget *ui_edit_entry_new(GtkWidget *box, const gchar *text);
void ui_edit_entry_set(GtkWidget *entry, const gchar *text);
void ui_edit_entry_set_blocking(GtkWidget *entry, const gchar *text, gpointer block_data);
const gchar *ui_edit_entry_get(GtkWidget *entry);

GtkWidget *ui_edit_spin_new(GtkWidget *box, const gchar *text, gint lower, gint upper, GtkObject **adjustment);
GtkWidget *ui_edit_spin_new_with_scale(GtkWidget *box, const gchar *text,
				       gint lower, gint upper, GtkObject **adjustment);
void ui_edit_spin_set(GtkWidget *spin, gint value);
void ui_edit_spin_set_blocking(GtkWidget *spin, gint val, gpointer block_data);
gint ui_edit_spin_get(GtkWidget *spin);

GtkWidget *ui_edit_list_new(GtkWidget *box, GtkListStore *store,
                            GtkSignalFunc changed_func, gpointer changed_data);
void ui_edit_list_add_column(GtkWidget *treeview, const gchar *title, gint column_id,
			     gint image, gint right_justify);
GdkPixbuf *ui_edit_list_pixbuf_from_file(const gchar *image);

gchar *ui_edit_bool_to_text(gint enable);
gchar *ui_edit_int_to_text(gint value);

void ui_edit_widget_add_finish(EditData *ed, WidgetData *wd, const gchar *image, const gchar *data);
void ui_edit_widget_resync(UIData *ui, WidgetData *wd, gint highlight, gint add_w, gint add_h);

gchar *ui_edit_copy_unique_file(const gchar *src, const gchar *dir, GdkPixbuf *pb,
				const gchar *hint1, const gchar *hint2);

void ui_edit_write_section(FILE *f, const gchar *section, const gchar *subsection);
void ui_edit_write_key_char(FILE *f, const gchar *key, const gchar *value);
void ui_edit_write_key_int(FILE *f, const gchar *key, gint value);
void ui_edit_write_key_bool(FILE *f, const gchar *key, gint value);

void ui_edit_widget_set_path_key(WidgetData *wd, const gchar *key, const gchar *base, const gchar *name);
gint ui_edit_save(EditData *ed, const gchar *skin_dir, const gchar *data_filename);

void ui_edit_widget_id_list_set_page(GList *list, WidgetType type, GtkWidget *page);

void ui_edit_widget_draw_highlight(UIData *ui, WidgetData *wd, gint enable);

void ui_edit_set_help_path(EditData *ed, const gchar *path);
void ui_edit_set_skindata_list(EditData *ed, GList *list);

void ui_edit_close(EditData *ed);

EditData *ui_edit_new(UIData *parent, const gchar *class, const gchar *subclass, const gchar *title);

void ui_edit_add_skin_resource(const gchar *path);

void ui_distort(UIData *ui);


#endif

