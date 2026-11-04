/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#include "ui2_includes.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "ui2_typedefs.h"
#include "ui2_editor.h"

#include "ui2_button.h"
#include "ui2_display.h"
#include "ui2_main.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_help.h"
#include "ui_utildlg.h"
#include "ui_fileops.h"
#include "ui_pixbuf_ops.h"
#include "ui_pixbuf_save.h"
#include "ui_tabcomp.h"
#include "ui_tree_edit.h"


#define HELP_WINDOW_WIDTH 600
#define HELP_WINDOW_HEIGHT 350

#define HELP_WINDOW_FONT "-adobe-courier-medium-r-normal--*-120-*-*-*-*-*-*"

#define LIST_IMG_MAX_W 64
#define LIST_IMG_MAX_H 32


enum {
	EDITOR_COLUMN_BACK_POINTER = 0,
	EDITOR_COLUMN_BACK_IMAGE,
	EDITOR_COLUMN_BACK_WIDTH,
	EDITOR_COLUMN_BACK_HEIGHT,
	EDITOR_COLUMN_BACK_SIZEABLE,
	EDITOR_COLUMN_BACK_BORDER,
	EDITOR_COLUMN_BACK_TRANSPARENT,
	EDITOR_COLUMN_BACK_COUNT
};

enum {
	EDITOR_COLUMN_FOCUS_POINTER = 0,
	EDITOR_COLUMN_FOCUS_IMAGE,
	EDITOR_COLUMN_FOCUS_BORDER,
	EDITOR_COLUMN_FOCUS_COUNT
};

enum {
	EDITOR_COLUMN_WIDGET_POINTER = 0,
	EDITOR_COLUMN_WIDGET_KEY,
	EDITOR_COLUMN_WIDGET_TYPE,
	EDITOR_COLUMN_WIDGET_DATA,
	EDITOR_COLUMN_WIDGET_COUNT
};

typedef struct _EditTypeData EditTypeData;
struct _EditTypeData
{
	WidgetType type;
	GList *list;		/* list for read in widgets (data from ui_edit_widget_read) */
	GtkWidget *page;	/* base parent widget for adding new widget (inserted to notebook) */
};


static GList *editor_resource_list = NULL;

static gint dummy_type_id = -1;	/* widget type id of the dummy widget */


static void widget_list_set_widget(EditData *ed, WidgetData *wd);
static void widget_list_update(EditData *ed, WidgetData *wd, gint col, const gchar *text);
static void widget_list_reset(EditData *ed);

static void edit_widget_set_coord(EditData *ed, WidgetData *wd, gint x, gint y);

static void prop_update(EditData *ed, WidgetData *wd, gint force);

void ui_edit_widget_draw_highlight(UIData *ui, WidgetData *wd, gint enable);
static void ui_edit_widget_active_set(EditData *ed, WidgetData *wd);

static void help_window_scroll(EditData *ed, WidgetType type);

static void comment_window_push_build(EditData *ed);
static void comment_window_push(EditData *ed);
static void comment_window_pull(EditData *ed);


/*
 *-----------------------------------------------------------------------------
 * widget editor utils
 *-----------------------------------------------------------------------------
 */

void ui_edit_frame_sensitive(GtkWidget *widget, gint state, gint stop_at_box)
{
	GtkWidget *parent;

	parent = widget;
	while(parent)
		{
		if ((stop_at_box && GTK_IS_BOX(parent) && !GTK_IS_COMBO(parent)) ||
		    (!stop_at_box && GTK_IS_FRAME(parent)) )
			{
			gtk_widget_set_sensitive(parent, state);
			return;
			}
		parent = parent->parent;
		}
}

static GtkWidget *ui_edit_real_frame_new(GtkWidget *box, gint has_frame, const gchar *text, gint vert)
{
	GtkWidget *hbox;
	GtkWidget *frame = NULL;

	if (has_frame)
		{
		frame = gtk_frame_new(text);
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
		gtk_widget_show(frame);
		}

	if (vert)
		{
		hbox = gtk_vbox_new(FALSE, 0);
		}
	else
		{
		hbox = gtk_hbox_new(FALSE, 5);
		}
	if (frame)
		{
		gtk_container_add(GTK_CONTAINER(frame), hbox);
		}
	else
		{
		gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
		}
	gtk_widget_show(hbox);

	return hbox;
}

GtkWidget *ui_edit_frame_new(GtkWidget *box, gint has_frame, const gchar *text)
{
	return ui_edit_real_frame_new(box, has_frame, text, FALSE);
}

GtkWidget *ui_edit_vframe_new(GtkWidget *box, gint has_frame, const gchar *text)
{
	return ui_edit_real_frame_new(box, has_frame, text, TRUE);
}

GtkWidget *ui_edit_label_new(GtkWidget *box, const gchar *text)
{
	GtkWidget *label;

	label = gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	return label;
}

GtkWidget *ui_edit_image_new(GtkWidget *box)
{
	GtkWidget *scrolled;
	GtkWidget *image;

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
	gtk_widget_show(scrolled);

	image = gtk_image_new();
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), image);
	gtk_widget_show(image);

	return image;
}

void ui_edit_image_blank(GtkWidget *image)
{
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
	gdk_pixbuf_unref(pixbuf);
}

GtkWidget *ui_edit_toggle_new(GtkWidget *box, const gchar *text)
{
	GtkWidget *button;

	button = gtk_check_button_new_with_label(text);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
        gtk_widget_show(button);

	return button;
}

void ui_edit_toggle_set(GtkWidget *button, gint active)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), active);
}

void ui_edit_toggle_set_blocking(GtkWidget *button, gint active, gpointer block_data)
{
	g_signal_handlers_block_matched(G_OBJECT(button), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, block_data);
	ui_edit_toggle_set(button, active);
	g_signal_handlers_unblock_matched(G_OBJECT(button), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, block_data);
}

gint ui_edit_toggle_get(GtkWidget *button)
{
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}

static gint ui_edit_font_entry_dialog_delete_cb(GtkWidget *font_dialog, GdkEventAny *event, gpointer data)
{
	gtk_widget_hide(font_dialog);
	return TRUE;
}

static void ui_edit_font_entry_dialog_cancel_cb(GtkWidget *button, gpointer data)
{
	GtkWidget *font_dialog = data;

	gtk_widget_hide(font_dialog);
}

static void ui_edit_font_entry_dialog_ok_cb(GtkWidget *button, gpointer data)
{
	GtkWidget *font_dialog = data;
	gchar *font_name;

	font_name = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(font_dialog));
	if (font_name)
		{
		GtkWidget *entry;

		entry = g_object_get_data(G_OBJECT(font_dialog), "font_entry");
		gtk_entry_set_text(GTK_ENTRY(entry), font_name);
		g_free(font_name);
		}
	gtk_widget_hide(font_dialog);
}

static void ui_edit_font_entry_button_cb(GtkWidget *button, gpointer data)
{
	GtkWidget *entry = data;
	GtkWidget *font_dialog;

	font_dialog = g_object_get_data(G_OBJECT(button), "font_dialog");

	if (font_dialog)
		{
		if (GTK_WIDGET_VISIBLE(font_dialog))
			{
			gdk_window_raise(font_dialog->window);
			return;
			}
		}
	else
		{
		font_dialog = gtk_font_selection_dialog_new(_("Select font"));
		g_signal_connect(G_OBJECT(font_dialog), "delete_event",
				 G_CALLBACK(ui_edit_font_entry_dialog_delete_cb), NULL);

		g_signal_connect(G_OBJECT(GTK_FONT_SELECTION_DIALOG(font_dialog)->cancel_button), "clicked",
				 G_CALLBACK(ui_edit_font_entry_dialog_cancel_cb), font_dialog);
		g_object_set_data(G_OBJECT(font_dialog), "font_entry", entry);
		g_signal_connect(G_OBJECT(GTK_FONT_SELECTION_DIALOG(font_dialog)->ok_button), "clicked",
				 G_CALLBACK(ui_edit_font_entry_dialog_ok_cb), font_dialog);

		g_object_set_data(G_OBJECT(button), "font_dialog", font_dialog);
		}

	gtk_font_selection_set_font_name(GTK_FONT_SELECTION(GTK_FONT_SELECTION_DIALOG(font_dialog)->fontsel),
					 gtk_entry_get_text(GTK_ENTRY(entry)));

	gtk_widget_show(font_dialog);
}

static void ui_edit_font_entry_destroy_cb(GtkWidget *button, gpointer data)
{
	GtkWidget *font_dialog;

	font_dialog = g_object_get_data(G_OBJECT(button), "font_dialog");

	if (font_dialog)
		{
		gtk_widget_destroy(font_dialog);
		}	
}

GtkWidget *ui_edit_font_entry_new(GtkWidget *box, const gchar *text)
{
	GtkWidget *hbox;
	GtkWidget *entry;
	GtkWidget *button;

	hbox = ui_edit_frame_new(box, FALSE, NULL);
	ui_edit_label_new(hbox, text);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
        gtk_widget_show(entry);

	button = gtk_button_new_with_label("...");
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(ui_edit_font_entry_button_cb), entry);
	g_signal_connect(G_OBJECT(button), "destroy",
			 G_CALLBACK(ui_edit_font_entry_destroy_cb), NULL);
        gtk_widget_show(button);

	return entry;
}

GtkWidget *ui_edit_path_entry_new(GtkWidget *box, const gchar *text, const gchar *history_key)
{
	GtkWidget *hbox;
	GtkWidget *combo;
	GtkWidget *entry;

	hbox = ui_edit_frame_new(box, FALSE, NULL);
	ui_edit_label_new(hbox, text);

	combo = tab_completion_new_with_history(&entry, NULL,
						history_key, 16, NULL, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
        gtk_widget_show(combo);

	gtk_entry_set_text(GTK_ENTRY(entry), "");

	return entry;
}

static void ui_edit_path_image_cb(GtkWidget *entry, gpointer data)
{
	GtkWidget *image = data;
	const gchar *path;

	path = gtk_entry_get_text(GTK_ENTRY(entry));

	if (isfile(path))
		{
		GdkPixbuf *pb;

		pb = util_pixbuf_new_from_file(path);
		if (pb)
			{
			gtk_image_set_from_pixbuf(GTK_IMAGE(image), pb);
			gdk_pixbuf_unref(pb);
			return;
			}
		}

	ui_edit_image_blank(image);
}

void ui_edit_path_entry_connect_image(GtkWidget *entry, GtkWidget *image)
{
	g_signal_connect(G_OBJECT(entry), "changed",
			 G_CALLBACK(ui_edit_path_image_cb), image);
}

static gint key_entry_sort_cb(gconstpointer a, gconstpointer b)
{
	return strcmp((gchar *)a, (gchar *)b);
}

static void ui_edit_key_entry_set_keys(GtkWidget *combo, UIData *ui, WidgetType type, gpointer block_data)
{
	GList *list = NULL;
	GList *work;
	gchar *buf;

	buf = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));

	work = ui->register_list;
	while(work && type >= 0)
		{
		RegisterData *rd = work->data;
		if (rd->type == type) list = g_list_append(list, rd->key);
		work = work->next;
		}

	list = g_list_sort(list, key_entry_sort_cb);

	if (block_data) g_signal_handlers_block_matched(G_OBJECT(GTK_COMBO(combo)->entry),
							G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, block_data);

	if (!list) list = g_list_append(list, "");
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), buf);

	if (block_data) g_signal_handlers_unblock_matched(G_OBJECT(GTK_COMBO(combo)->entry),
							  G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, block_data);

	g_free(buf);
	g_list_free(list);
}

GtkWidget *ui_edit_key_entry_new(GtkWidget *box, UIData *ui, WidgetType type)
{
	GtkWidget *hbox;
	GtkWidget *combo;

	hbox = ui_edit_frame_new(box, FALSE, NULL);
	ui_edit_label_new(hbox, _("Key:"));

	combo = gtk_combo_new();
	ui_edit_key_entry_set_keys(combo, ui, type, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
        gtk_widget_show(combo);

	return GTK_COMBO(combo)->entry;
}

GtkWidget *ui_edit_entry_new(GtkWidget *box, const gchar *text)
{
	GtkWidget *hbox;
	GtkWidget *entry;

	hbox = ui_edit_frame_new(box, FALSE, NULL);
	ui_edit_label_new(hbox, text);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
        gtk_widget_show(entry);

	return entry;
}

void ui_edit_entry_set(GtkWidget *entry, const gchar *text)
{
	gchar *buf;

	buf = g_strdup(text);
	gtk_entry_set_text(GTK_ENTRY(entry), buf ? buf : "");
	g_free(buf);
}

void ui_edit_entry_set_blocking(GtkWidget *entry, const gchar *text, gpointer block_data)
{
	g_signal_handlers_block_matched(G_OBJECT(entry), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, block_data);
	ui_edit_entry_set(entry, text);
	g_signal_handlers_unblock_matched(G_OBJECT(entry), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, block_data);
}

const gchar *ui_edit_entry_get(GtkWidget *entry)
{
	const gchar *buf;

	buf = gtk_entry_get_text(GTK_ENTRY(entry));
	if (strlen(buf) > 0) return buf;
	return NULL;
}

GtkWidget *ui_edit_spin_new(GtkWidget *box, const gchar *text, gint lower, gint upper, GtkObject **adjustment)
{
	GtkObject *adj;
	GtkWidget *spin;
	GtkWidget *hbox;

	hbox = ui_edit_frame_new(box, FALSE, NULL);
	ui_edit_label_new(hbox, text);

	adj = gtk_adjustment_new(lower > 0.0 ? (gfloat)lower : 0.0, (gfloat)lower, (gfloat)upper, 1.0, 1.0, 1.0);
	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_ALWAYS);
	gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
	gtk_widget_show(spin);

	if (adjustment) *adjustment = adj;

	return spin;
}

GtkWidget *ui_edit_spin_new_with_scale(GtkWidget *box, const gchar *text,
				       gint lower, gint upper, GtkObject **adjustment)
{
	GtkWidget *spin;
	GtkObject *adj;
	GtkWidget *hscale;
	GtkWidget *hbox;
	gint expand;

	expand = GTK_IS_HBOX(box);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, expand, expand, 0);
	gtk_widget_show(hbox);

	if (text) ui_edit_label_new(hbox, text);

	hscale = gtk_hscale_new_with_range((gdouble)lower, (gdouble)upper, 1.0);

	gtk_scale_set_draw_value(GTK_SCALE(hscale), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), hscale, TRUE, TRUE, 0);
	gtk_widget_show(hscale);

	adj = GTK_OBJECT(gtk_range_get_adjustment(GTK_RANGE(hscale)));
	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
	gtk_widget_show(spin);

	if (adjustment) *adjustment = adj;

	return spin;
}

void ui_edit_spin_set(GtkWidget *spin, gint value)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), (gfloat)value);
}

void ui_edit_spin_set_blocking(GtkWidget *spin, gint val, gpointer block_data)
{
	GtkAdjustment *adj;

	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	g_signal_handlers_block_matched(G_OBJECT(adj), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, block_data);
	gtk_adjustment_set_value(adj, (gfloat)val);
	g_signal_handlers_unblock_matched(G_OBJECT(adj), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, block_data);
}

gint ui_edit_spin_get(GtkWidget *spin)
{
	return gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
}

/*
 *-----------------------------------------------------------------------------
 * widget editor clist utils
 *-----------------------------------------------------------------------------
 */

GtkWidget *ui_edit_list_new(GtkWidget *box, GtkListStore *store,
			    GtkSignalFunc changed_func, gpointer changed_data)
{
	GtkWidget *treeview;
	GtkWidget *scrolled;

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
	gtk_widget_show(scrolled);

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_container_add(GTK_CONTAINER(scrolled), treeview);
	gtk_widget_show(treeview);

	if (changed_func)
		{
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		g_signal_connect(G_OBJECT(selection), "changed",
				 G_CALLBACK(changed_func), changed_data);
		}

	return treeview;
}


void ui_edit_list_add_column(GtkWidget *treeview, const gchar *title, gint column_id,
			     gint image, gint right_justify)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, title);

	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);

	if (!image)
		{
		gtk_tree_view_column_set_resizable(column, TRUE);
		renderer = gtk_cell_renderer_text_new();
		if (right_justify) g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", column_id);
		}
	else
		{
		renderer = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", column_id);
		}

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
}

static gint ui_edit_list_find_data(GtkWidget *treeview, GtkTreeModel **store, GtkTreeIter *iter, gpointer data)
{
	GtkTreeModel *model;
	gint valid;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	if (store) *store = model;

	valid = gtk_tree_model_get_iter_first(model, iter);
	while (valid)
		{
		gpointer needle;

		gtk_tree_model_get(model, iter, 0, &needle, -1);
		if (needle == data) return TRUE;
		valid = gtk_tree_model_iter_next(model, iter);
		}

	return FALSE;
}

GdkPixbuf *ui_edit_list_pixbuf_from_file(const gchar *image)
{
	GdkPixbuf *pb;
	gint w, h;

	if (!image) return NULL;

	pb = util_pixbuf_new_from_file(image);
	if (!pb) return NULL;

	w = gdk_pixbuf_get_width(pb);
	h = gdk_pixbuf_get_height(pb);
	if (w > LIST_IMG_MAX_W || h > LIST_IMG_MAX_H)
		{
		gfloat scale;
		GdkPixbuf *t;

		if ((float)LIST_IMG_MAX_W / w < (float)LIST_IMG_MAX_H / h)
			{
			scale = (float)LIST_IMG_MAX_W / w;
			}
		else
			{
			scale = (float)LIST_IMG_MAX_H / h;
			}
		w = (float)w * scale;
		h = (float)h * scale;
		if (w < 1) w = 1;
		if (h < 1) h = 1;

		t = pb;
		pb = gdk_pixbuf_scale_simple(t, w, h, GDK_INTERP_BILINEAR);
		gdk_pixbuf_unref(t);
		}

	return pb;
}

gchar *ui_edit_bool_to_text(gint enable)
{
	return enable ? "X" : "";
}

gchar *ui_edit_int_to_text(gint value)
{
	return g_strdup_printf("%d", value);
}

/*
 *-----------------------------------------------------------------------------
 * editor widget funcs
 *-----------------------------------------------------------------------------
 */

static GdkPixbuf *ui_edit_widget_get_pixbuf(WidgetData *wd)
{
	if (wd->od && wd->od->func_get_pixbuf)
		{
		return wd->od->func_get_pixbuf(wd->widget);
		}
	return NULL;
}

static void ui_edit_widget_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	if (wd->od && wd->od->func_edit_write)
		{
		const gchar *data;

		/* this starts the section and writes widget specific data */
		wd->od->func_edit_write(f, wd, skin, dir);

		/* now write the general widget data */
		data = ui_widget_get_data(wd, "data");

		if (wd->text_id) ui_edit_write_key_char(f, "text_id", wd->text_id);
		if (data) ui_edit_write_key_char(f, "data", data);

		if (wd->anchor_right) ui_edit_write_key_bool(f, "anchor_right", wd->anchor_right);
		if (wd->anchor_bottom) ui_edit_write_key_bool(f, "anchor_bottom", wd->anchor_bottom);
		}
}

static gpointer ui_edit_widget_read(UIData *ui, WidgetData *wd, GList *list)
{
	if (!wd || !wd->od || !wd->od->func_edit_read) return NULL;

	return wd->od->func_edit_read(ui, wd, list);
}

static gint ui_edit_widget_is_removable(WidgetData *wd)
{
	if (!wd || !wd->od || !wd->od->func_edit_removable) return TRUE;

	return wd->od->func_edit_removable(wd->widget);
}

void ui_edit_widget_add_finish(EditData *ed, WidgetData *wd, const gchar *image, const gchar *data)
{
	ui_widget_set_data(wd, "data", data);
	ui_widget_set_data(wd, "image", image);

	if (wd->od && wd->od->func_init)
		{
		wd->od->func_init(wd->widget, wd->key, ed->ui);
		}
	if (wd->od && wd->od->is_visible)
		{
		gint x, y, w, h, dmy;

		if (ui_widget_get_geometry(wd, &dmy, &dmy, &w, &h))
			{
			if (w > ed->ui->skin->width || h > ed->ui->skin->height)
				{
				ed->ui->skin->widget_list = g_list_remove(ed->ui->skin->widget_list, wd);
				ui_widget_free(wd);
				warning_dialog(_("Widget too big"), _("The widget is too big, the width or height exceeds the background size."));
				return;
				}

			x = ((ed->ui->skin->width - w) / 2);
			y = ((ed->ui->skin->height - h) / 2);

			ui_widget_set_coord(ed->ui, wd, x, y, TRUE);
			ui_widget_draw(ed->ui, wd, TRUE, FALSE);
			}
		}

	widget_list_reset(ed);
	widget_list_set_widget(ed, wd);
}

/* This will completely redraw a widget, updating it's back and redraw all widgets that overlap it
 * add_w and add_h are for extending the redraw region, useful when sizing a widget down - to update
 * the old area it no longer inhabits.
 */
void ui_edit_widget_resync(UIData *ui, WidgetData *wd, gint highlight, gint add_w, gint add_h)
{
	gint x, y, w, h;

	if (!wd || !(ui_widget_get_geometry(wd, &x, &y, &w, &h))) return;

	/* do not allow negative add's */
	add_w = MAX(0, add_w);
	add_h = MAX(0, add_h);

	if (highlight) ui_edit_widget_draw_highlight(ui, wd, FALSE);
	pixbuf_copy_area(ui->skin->overlay, x, y,
                         ui->skin->pixbuf, x, y, w + add_w, h + add_h, FALSE);
	ui_widget_sync_back(ui, wd);
	ui_display_redraw_area(ui, x, y, w + add_w, h + add_h, TRUE);
	if (highlight) ui_edit_widget_draw_highlight(ui, wd, TRUE);
}

/*
 *-----------------------------------------------------------------------------
 * skin comments reader
 *-----------------------------------------------------------------------------
 */

static void ui_edit_comments_reset(EditData *ed)
{
	g_free(ed->comment_title);
	g_free(ed->comment_version);
	g_free(ed->comment_author);
	g_free(ed->comment_email);
	g_free(ed->comment_url);
	g_free(ed->comment_license);
	g_free(ed->comment_misc);
	g_free(ed->comment_stamp);

	ed->comment_title = NULL;
	ed->comment_version = NULL;
	ed->comment_author = NULL;
	ed->comment_email = NULL;
	ed->comment_url = NULL;
	ed->comment_license = NULL;
	ed->comment_misc = NULL;
	ed->comment_stamp = NULL;

	ed->comment_build = 0;
}

static void ui_edit_comments_read(EditData *ed, const gchar *path)
{
	FILE *f = NULL;
	char buf[1024];
	gchar *key;
	gchar *text;

	ui_edit_comments_reset(ed);

	if (path)
		{
		gchar *pathl = path_from_utf8(path);
		f = fopen(pathl, "r");
		g_free(pathl);
		}
	if (!f)
		{
		comment_window_push(ed);
		return;
		}

	key = NULL;
	text = NULL;
	while (fgets(buf, sizeof(buf), f))
		{
		if (buf[0]!='#')
			{
			if (key && strcasecmp(key, "Comments") == 0 &&
			    text && !ed->comment_misc)
				{
				ed->comment_misc = text;
				text = NULL;
				}
			g_free(key);
			g_free(text);
			key = NULL;
			text = NULL;
			continue;
			}
		else
			{
			/* comment, remove newlines */
			gchar *ptr;

			ptr = buf;
			while (*ptr != '\0')
				{
				if (*ptr == '\n') *ptr = '\0';
				ptr++;
				}
			}

		if (!key)
			{
			gchar *begin;
			gchar *ptr;
			gint c = 0;

			ptr = buf;
			ptr++;
			begin = ptr;

			while (*ptr != ':' && *ptr != '\n' && *ptr != '\0')
				{
				c++;
				ptr++;
				}
			if (*ptr == ':')
				{
				/* found a comment key */
				key = g_strndup(begin, c);
				ptr++;
				while (*ptr == ' ') ptr++;
				text = g_strdup(ptr);
				}
			}
		else
			{
			/* key must be extended comment */
			gchar *ptr;
			gchar *tmp;

			ptr = buf;
			ptr++;
			while (*ptr == ' ') ptr++;

			tmp = g_strconcat(text ? text : "", text ? "\n" : "",  ptr, NULL);
			g_free(text);
			text = tmp;
			}
		if (key && strcasecmp(key, "Comments") != 0)
			{
			if (!ed->comment_title &&
			    strcasecmp(key, "Title") == 0)
				{
				ed->comment_title = text;
				}
			else if (!ed->comment_version &&
				 strcasecmp(key, "Version") == 0)
				{
				ed->comment_version = text;
				}
			else if (!ed->comment_stamp &&
				 strcasecmp(key, "Date stamp") == 0)
				{
				ed->comment_stamp = text;
				}
			else if (strcasecmp(key, "Build") == 0)
				{
				ed->comment_build = (gint)strtol(text, NULL, 10);
				g_free(text);
				}
			else if (!ed->comment_author &&
				 strcasecmp(key, "Author") == 0)
				{
				ed->comment_author = text;
				}
			else if (!ed->comment_email &&
				 strcasecmp(key, "Email") == 0)
				{
				ed->comment_email = text;
				}
			else if (!ed->comment_url &&
				 strcasecmp(key, "URL") == 0)
				{
				ed->comment_url = text;
				}
			else if (!ed->comment_license &&
				 strcasecmp(key, "License") == 0)
				{
				ed->comment_license = text;
				}
			else
				{
				printf("Unknown comment key: %s\n", key);
				g_free(text);
				}
			g_free(key);
			key = NULL;
			text = NULL;
			}
		}

	fclose(f);
	g_free(text);

	comment_window_push(ed);
}

/*
 *-----------------------------------------------------------------------------
 * skin saving utils
 *-----------------------------------------------------------------------------
 */

gchar *ui_edit_copy_unique_file(const gchar *src, const gchar *dir, GdkPixbuf *pb,
				const gchar *hint1, const gchar *hint2)
{
	gchar *dest;
	gchar *name;

	if (!dir || !isdir(dir)) return NULL;

	if (!src && !pb) return NULL;

	name = NULL;

	if (!src)
		{
		/* create one from the pixbuf */
		gchar *buf;
		gchar *new;

		buf = g_strconcat(dir, "/", hint1, hint2, NULL);
		new = unique_filename(buf, ".png", "_", FALSE);
		g_free(buf);

		if (new)
			{
			if (debug_mode) printf("Save file is %s\n", new);

			if (!pixbuf_to_file_as_png(pb, new))
				{
				printf("Failed to write png \"%s\"\n", new);
				}

			name = g_strdup(filename_from_path(new));

			g_free(new);
			}

		return name;
		}

	if (!isfile(src)) return NULL;

	dest = g_strconcat(dir, "/", filename_from_path(src), NULL);

	if (strcmp(src, dest) == 0)
		{
		/* same file */
		name = g_strdup(filename_from_path(dest));
		}
	else if (isfile(dest))
		{
		/* dest exists */

		if (filesize(src) == filesize(dest) && checksum_simple(src) == checksum_simple(dest))
			{
			/* identical files (i hope) */
			name = g_strdup(filename_from_path(dest));
			}
		else
			{
			gchar *new;
			new = unique_filename(dest, NULL, "_", FALSE);
			if (new)
				{
				if (copy_file(src, new))
					{
					name = g_strdup(filename_from_path(new));
					}
				else
					{
					printf("failed to copy \"%s\" to \"%s\"\n", src, dest);
					}
				g_free(new);
				}
			}
		}
	else if (copy_file(src, dest))
		{
		name = g_strdup(filename_from_path(dest));
		}
	else
		{
		printf("failed to copy \"%s\" to \"%s\"\n", src, dest);
		}

	g_free(dest);

	return name;
}

void ui_edit_write_section(FILE *f, const gchar *section, const gchar *subsection)
{
	if (subsection)
		fprintf(f, "\n[%s_%s]\n", section, subsection);
	else
		fprintf(f, "\n[%s]\n", section);
}

void ui_edit_write_key_char(FILE *f, const gchar *key, const gchar *value)
{
	fprintf(f, "%s = %s\n", key, value ? value : "");
}

void ui_edit_write_key_int(FILE *f, const gchar *key, gint value)
{
	fprintf(f, "%s = %d\n", key, value);
}

void ui_edit_write_key_bool(FILE *f, const gchar *key, gint value)
{
	fprintf(f, "%s = %s\n", key, value ? "TRUE" : "FALSE");
}

void ui_edit_widget_set_path_key(WidgetData *wd, const gchar *key, const gchar *base, const gchar *name)
{
	gchar *buf;

	buf = g_strconcat(base, "/", name, NULL);
	ui_widget_set_data(wd, key, buf);
	g_free(buf);
}

static void ui_edit_write_comment(FILE *f, const gchar *key, const gchar *text)
{
	fprintf(f, "#%s:%s%s\n", key, text ? " " : "", text ? text : "");
}

static void ui_edit_write_comment_lines(FILE *f, const gchar *key, const gchar *text)
{
	gint offset;
	gchar *spc;
	const gchar *ptr;
	gint line = 0;
	gint i;

	if (!text)
		{
		ui_edit_write_comment(f, key, text);
		return;
		}

	offset = strlen(key) + 1;	/* "key" + ":" */

	spc = g_strdup_printf("%s: ", key);
	i = 0;
	while (spc[i] != '\0')
		{
		spc[i] = ' ';
		i++;
		}

	ptr = text;
	while (*ptr != '\0')
		{
		const gchar *begin;
		gchar *buf;
		gint c = 0;

		begin = ptr;
		while (*ptr != '\0' && *ptr != '\n')
			{
			ptr++;
			c++;
			}

		buf = g_strndup(begin, c);

		if (line == 0)
			{
			fprintf(f, "#%s: %s\n", key, buf);
			}
		else
			{
			fprintf(f, "#%s%s\n", spc, buf);
			}

		g_free(buf);

		if (*ptr == '\n') ptr++;
		line++;
		}

	g_free(spc);
}

gint ui_edit_save(EditData *ed, const gchar *skin_dir, const gchar *data_filename)
{
	SkinData *skin = ed->ui->skin;
	gchar *data_path;
	gchar *pathl;
	FILE *f;
	gchar *image;
	gchar *mask = NULL;
	GList *work;
	time_t ts;
	gchar *buf;
	const char *time_buf;

	if (!skin_dir) return FALSE;

	comment_window_pull(ed);

	if (!data_filename) data_filename = "skindata";

	/* ugly, but anchored widgets need to properly save coordinates */
	skin_resize(ed->ui, skin->width_def, skin->height_def);

	/* try to create skin dir, if it doesn't exist */
	if (!isdir(skin_dir))
		{
		printf("creating dir \"%s\"...\n", skin_dir);
		if (!mkdir_utf8(skin_dir, 0755))
			{
			printf("fail to create dir \"%s\"\n", skin_dir);
			return FALSE;
			}
		}

	/* create / open skindata */

	data_path = g_strconcat(skin_dir, "/", data_filename, NULL);
	if (isfile(data_path))
		{
		gchar *bak;
		gchar *new;

		printf("backing up data \"%s\"...\n", data_path);
		bak = g_strconcat(data_path, ".bak", NULL);

		new = unique_filename(bak, NULL, NULL, TRUE);
		if (new)
			{
			if (!move_file(data_path, new))
				{
				printf("Failed to backup \"%s\" to \"%s\"\n", data_path, new);
				}
			g_free(new);
			}

		g_free(bak);
		}

	pathl = path_from_utf8(data_path);
	f = fopen(pathl, "w");
	g_free(pathl);
	if (!f)
		{
		printf("Failed to open \"%s\"\n", data_path);
		g_free(data_path);
		return FALSE;
		}

	fprintf(f, "#SLIK skin data file\n\n");
	fprintf(f, "#file created by %s %s (SLIK %s)\n\n", PACKAGE, VERSION, SLIK_VERSION);

	ts = time(NULL);
	g_free(ed->comment_stamp);
	time_buf = ctime(&ts);
	ed->comment_stamp = g_strndup(time_buf, strlen(time_buf) - 1);
	ed->comment_build++;

	ui_edit_write_comment(f, "Title", ed->comment_title);
	ui_edit_write_comment(f, "Version", ed->comment_version);
	ui_edit_write_comment(f, "Date stamp", ed->comment_stamp);
	buf = g_strdup_printf("%d", ed->comment_build);
	ui_edit_write_comment(f, "Build", buf);
	g_free(buf);
	ui_edit_write_comment(f, "Author", ed->comment_author);
	ui_edit_write_comment(f, "Email", ed->comment_email);
	ui_edit_write_comment(f, "URL", ed->comment_url);
	ui_edit_write_comment(f, "License", ed->comment_license);
	ui_edit_write_comment_lines(f, "Comments", ed->comment_misc);

	ui_edit_write_section(f, "main", NULL);

	image = ui_edit_copy_unique_file(skin->background_filename, skin_dir,
					 skin->real_overlay ? skin->real_overlay : skin->overlay,
					 "background", NULL);
	if (skin->background_mask_filename)
		{
		mask = ui_edit_copy_unique_file(ed->ui->skin->background_mask_filename,
						skin_dir, NULL, "background_mask", NULL);
		}

	ui_edit_write_key_char(f, "image", image);
	ui_edit_write_key_bool(f, "transparent", skin->transparent);

	if (mask) ui_edit_write_key_char(f, "mask", mask);

	/* extra skin stuff */
	ui_edit_write_key_bool(f, "border", skin->has_border);
	if (skin->has_border)
		{
		ui_edit_write_key_int(f, "border_left", skin->border_left);
		ui_edit_write_key_int(f, "border_right", skin->border_right);
		ui_edit_write_key_int(f, "border_top", skin->border_top);
		ui_edit_write_key_int(f, "border_bottom", skin->border_bottom);

		ui_edit_write_key_bool(f, "border_left_stretch", skin->border_left_stretch);
		ui_edit_write_key_bool(f, "border_right_stretch", skin->border_right_stretch);
		ui_edit_write_key_bool(f, "border_top_stretch", skin->border_top_stretch);
		ui_edit_write_key_bool(f, "border_bottom_stretch", skin->border_bottom_stretch);
		}

	if (skin->sizeable) ui_edit_write_key_bool(f, "sizeable", skin->sizeable);
	if (skin->stretch) ui_edit_write_key_bool(f, "stretch", skin->stretch);

	if (skin->sizeable || skin->stretch || skin->has_border ||
	    skin->width_def != gdk_pixbuf_get_width(skin->real_overlay) ||
	    skin->height_def != gdk_pixbuf_get_height(skin->real_overlay) )
		{
		ui_edit_write_key_int(f, "width", skin->width_def);
		ui_edit_write_key_int(f, "height", skin->height_def);
		}

	if (skin->sizeable)
		{
		ui_edit_write_key_int(f, "width_min", skin->width_min);
		ui_edit_write_key_int(f, "width_max", skin->width_max);
		ui_edit_write_key_int(f, "width_increment", skin->width_inc);

		ui_edit_write_key_int(f, "height_min", skin->height_min);
		ui_edit_write_key_int(f, "height_max", skin->height_max);
		ui_edit_write_key_int(f, "height_increment", skin->height_inc);
		}

	/* focus stuff */

	if (skin->focus_overlay)
		{
		gchar *focus;
		focus = ui_edit_copy_unique_file(skin->focus_filename, skin_dir,
						 skin->focus_overlay, "focus", NULL);
		ui_edit_write_key_char(f, "focus", focus);
		g_free(focus);
		}

	ui_edit_write_key_bool(f, "focus_stretch", skin->focus_stretch);
	ui_edit_write_key_bool(f, "focus_has_border", skin->focus_has_border);
	if (skin->focus_has_border)
		{
		ui_edit_write_key_int(f, "focus_border_left", skin->focus_border_left);
		ui_edit_write_key_int(f, "focus_border_right", skin->focus_border_right);
		ui_edit_write_key_int(f, "focus_border_top", skin->focus_border_top);
		ui_edit_write_key_int(f, "focus_border_bottom", skin->focus_border_bottom);
		}

	ui_edit_write_key_bool(f, "focus_anchor_right", skin->focus_anchor_right);
	ui_edit_write_key_bool(f, "focus_anchor_bottom", skin->focus_anchor_bottom);

	ui_edit_write_key_bool(f, "focus_filled", skin->focus_box_filled);

	ui_edit_write_key_int(f, "focus_red", (gint)skin->focus_box_r);
	ui_edit_write_key_int(f, "focus_green", (gint)skin->focus_box_g);
	ui_edit_write_key_int(f, "focus_blue", (gint)skin->focus_box_b);
	ui_edit_write_key_int(f, "focus_alpha", (gint)skin->focus_box_alpha);

	/* we must save digits first, so that numbers can always find them,
	 * for this we simply save all non-visible widgets first
	 */
	work = ed->ui->skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;

		if (wd->od && !wd->od->is_visible) ui_edit_widget_write(f, wd, ed->ui->skin, skin_dir);
		work = work->next;
		}

	/* now do everything else */
	work = ed->ui->skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;

		if (wd->od && wd->od->is_visible) ui_edit_widget_write(f, wd, ed->ui->skin, skin_dir);
		work = work->next;
		}

	fclose(f);

	g_free(image);
	g_free(mask);
	g_free(data_path);

	comment_window_push(ed);

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * widget id list utils
 *-----------------------------------------------------------------------------
 */

static GList *ui_edit_widget_id_list_new(void)
{
	WidgetObjectData *od;
	WidgetType t = 0;
	GList *list = NULL;

	while ((od = ui_widget_object_by_type(t)) != NULL)
		{
		EditTypeData *td;

		td = g_new0(EditTypeData, 1);
		td->type = t;
		td->list = NULL;

		list = g_list_append(list, td);

		t++;
		}
	return list;
}

static void ui_edit_widget_id_list_free(GList *list)
{
	GList *work;
	work = list;
	while (work)
		{
		EditTypeData *td;
		WidgetObjectData *od;

		td = work->data;
		work = work->next;

		od = ui_widget_object_by_type(td->type);

		if (od && od->func_edit_free)
			{
			GList *w = td->list;
			while(w)
				{
				od->func_edit_free(w->data);
				w = w->next;
				}
			}

		g_list_free(td->list);
		g_free(td);
		}

	g_list_free(list);
}

static void ui_edit_widget_id_list_read(UIData *ui, WidgetData *wd, GList *list)
{
	GList *work;
	work = list;
	while (work)
		{
		EditTypeData *td = work->data;

		if (wd->type == td->type)
			{
			gpointer data;

			data = ui_edit_widget_read(ui, wd, td->list);
			if (data)
				{
				WidgetObjectData *od;
				od = ui_widget_object_by_type(td->type);
				if (td->page && od && od->func_edit_page_add)
					{
					od->func_edit_page_add(td->page, data);
					}
				td->list = g_list_append(td->list, data);
				}
			return;
			}

		work = work->next;
		}
}

/* this is a very hacky function for use by widgets that know what they are doing (for numbers.) */
void ui_edit_widget_id_list_set_page(GList *list, WidgetType type, GtkWidget *page)
{
	GList *work;
	work = list;
	while (work)
		{
		EditTypeData *td = work->data;

		if (td->type == type)
			{
			td->page = page;
			return;
			}
		work = work->next;
		}
}

/*
 *-----------------------------------------------------------------------------
 * skin background/image (main) data similar to contents of a ui2_xxx_edit.c
 *-----------------------------------------------------------------------------
 */


typedef struct _MainListData MainListData;
struct _MainListData
{
	gchar *image;
	gint transparent;
	gchar *mask;

	gint stretch;

	gint has_border;
	gint border_left;
	gint border_right;
	gint border_top;
	gint border_bottom;
	gint border_left_stretch;
	gint border_right_stretch;
	gint border_top_stretch;
	gint border_bottom_stretch;

	gint sizeable;
	gint width_def;
	gint width_min;
	gint width_max;
	gint width_inc;
	gint height_def;
	gint height_min;
	gint height_max;
	gint height_inc;
};

typedef struct _MainPage MainPage;
struct _MainPage
{
	GtkWidget *image_entry;
	GtkWidget *mask_entry;
	GtkWidget *transparent_button;

	GtkWidget *stretch_button;

	GtkWidget *has_border_button;
	GtkWidget *border_left_spin;
	GtkWidget *border_right_spin;
	GtkWidget *border_top_spin;
	GtkWidget *border_bottom_spin;
	GtkWidget *border_left_stretch_button;
	GtkWidget *border_right_stretch_button;
	GtkWidget *border_top_stretch_button;
	GtkWidget *border_bottom_stretch_button;

	GtkWidget *sizeable_button;
	GtkWidget *width_def_spin;
	GtkWidget *width_min_spin;
	GtkWidget *width_max_spin;
	GtkWidget *width_inc_spin;
	GtkWidget *height_def_spin;
	GtkWidget *height_min_spin;
	GtkWidget *height_max_spin;
	GtkWidget *height_inc_spin;

	GtkWidget *reset_size_button;

	GtkWidget *list;
	GtkWidget *image;

	/* focus */

	GtkWidget *focus_image_entry;

	GtkWidget *focus_stretch_toggle;
	GtkWidget *focus_border_toggle;

	GtkWidget *focus_left_spin;
	GtkWidget *focus_right_spin;
	GtkWidget *focus_top_spin;
	GtkWidget *focus_bottom_spin;

	GtkWidget *focus_anchor_right_toggle;
	GtkWidget *focus_anchor_bottom_toggle;

	GtkWidget *focus_filled_toggle;

	GtkWidget *focus_r_spin;
	GtkWidget *focus_g_spin;
	GtkWidget *focus_b_spin;
	GtkWidget *focus_a_spin;

	GtkWidget *focus_list;
	GtkWidget *focus_image;

	UIData *ui;
};

typedef struct _FocusListData FocusListData;
struct _FocusListData
{
	gchar *image;

	gint stretch;
	gint border;

	gint border_left;
	gint border_right;
	gint border_top;
	gint border_bottom;

	gint anchor_right;
	gint anchor_bottom;

	gint filled;

	guint8 r;
	guint8 g;
	guint8 b;
	guint8 a;
};

/*
 *-----------------------------------------------------------------------------
 * focus portion
 *-----------------------------------------------------------------------------
 */

static void edit_focus_page_add(EditData *ed, FocusListData *fd);

static FocusListData *edit_focus_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while (work)
		{
		FocusListData *fd = work->data;
		if (strcmp(image, fd->image) == 0) return fd;
		work = work->next;
		}
	return NULL;
}

static void edit_focus_read(EditData *ed, SkinData *skin)
{
	FocusListData *fd;

	if (!skin->focus_filename) return;
	if (edit_focus_find(ed->focus_list, skin->focus_filename)) return;

	fd = g_new0(FocusListData, 1);
	fd->image = g_strdup(skin->focus_filename);

	fd->stretch = skin->focus_stretch;

	fd->border = skin->focus_has_border;
	fd->border_left = skin->focus_border_left;
	fd->border_right = skin->focus_border_right;
	fd->border_top = skin->focus_border_top;
	fd->border_bottom = skin->focus_border_bottom;

	fd->anchor_right = skin->focus_anchor_right;
	fd->anchor_bottom = skin->focus_anchor_bottom;

	fd->r = skin->focus_box_r;
	fd->g = skin->focus_box_g;
	fd->b = skin->focus_box_b;
	fd->a = skin->focus_box_alpha;

	ed->focus_list = g_list_append(ed->focus_list, fd);
	edit_focus_page_add(ed, fd);
}

static void edit_focus_list_free(GList *list)
{
	GList *work;
	work = list;
	while (work)
		{
		FocusListData *fd = work->data;
		work = work->next;

		g_free(fd->image);
		g_free(fd);
		}
	g_list_free(list);
}

static void edit_focus_page_border_sync(MainPage *mp)
{
	gint enabled;

	enabled = ui_edit_toggle_get(mp->focus_border_toggle);
	ui_edit_frame_sensitive(mp->focus_left_spin, enabled, TRUE);
	ui_edit_frame_sensitive(mp->focus_right_spin, enabled, TRUE);
	ui_edit_frame_sensitive(mp->focus_top_spin, enabled, TRUE);
	ui_edit_frame_sensitive(mp->focus_bottom_spin, enabled, TRUE);

	ui_edit_frame_sensitive(mp->focus_anchor_right_toggle, (!enabled), FALSE);
}

static void edit_focus_page_sync(MainPage *mp, FocusListData *fd)
{
	if (!fd) return;

	ui_edit_toggle_set(mp->focus_stretch_toggle, fd->stretch);
	ui_edit_toggle_set(mp->focus_border_toggle, fd->border);

	ui_edit_spin_set(mp->focus_left_spin, fd->border_left);
	ui_edit_spin_set(mp->focus_right_spin, fd->border_right);
	ui_edit_spin_set(mp->focus_top_spin, fd->border_top);
	ui_edit_spin_set(mp->focus_bottom_spin, fd->border_bottom);

	edit_focus_page_border_sync(mp);

	ui_edit_toggle_set(mp->focus_anchor_right_toggle, fd->anchor_right);
	ui_edit_toggle_set(mp->focus_anchor_bottom_toggle, fd->anchor_bottom);

	ui_edit_toggle_set(mp->focus_filled_toggle, fd->filled);

	ui_edit_spin_set(mp->focus_r_spin, (gint)fd->r);
	ui_edit_spin_set(mp->focus_g_spin, (gint)fd->g);
	ui_edit_spin_set(mp->focus_b_spin, (gint)fd->b);
	ui_edit_spin_set(mp->focus_a_spin, (gint)fd->a);

	ui_edit_entry_set(mp->focus_image_entry, fd->image);
}

static void edit_focus_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	EditData *ed = data;
	MainPage *mp;
	FocusListData *fd;
	GtkTreeModel *store;
	GtkTreeIter iter;

	mp = ed->main_page;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, EDITOR_COLUMN_FOCUS_POINTER, &fd, -1);
	edit_focus_page_sync(mp, fd);
}

static void edit_focus_page_add(EditData *ed, FocusListData *fd)
{
	MainPage *mp = ed->main_page;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(mp->focus_list)));

	pixbuf = ui_edit_list_pixbuf_from_file(fd->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   EDITOR_COLUMN_FOCUS_POINTER, fd,
			   EDITOR_COLUMN_FOCUS_IMAGE, pixbuf,
			   EDITOR_COLUMN_FOCUS_BORDER, fd->border, -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

static void edit_focus_page_add_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	MainPage *mp;
	const gchar *image;
	GdkPixbuf *pb = NULL;
	SkinData *skin;

	mp = ed->main_page;

	image = ui_edit_entry_get(mp->focus_image_entry);

	if (image) pb = util_pixbuf_new_from_file(image);

	skin = ed->ui->skin;

	if (skin->focus_overlay) gdk_pixbuf_unref(skin->focus_overlay);
	skin->focus_overlay = pb;

	skin->focus_stretch = ui_edit_toggle_get(mp->focus_stretch_toggle);
	skin->focus_has_border = ui_edit_toggle_get(mp->focus_border_toggle);

	skin->focus_border_left = ui_edit_spin_get(mp->focus_left_spin);
	skin->focus_border_right = ui_edit_spin_get(mp->focus_right_spin);
	skin->focus_border_top = ui_edit_spin_get(mp->focus_top_spin);
	skin->focus_border_bottom = ui_edit_spin_get(mp->focus_bottom_spin);

	skin->focus_anchor_right = ui_edit_toggle_get(mp->focus_anchor_right_toggle);
	skin->focus_anchor_bottom = ui_edit_toggle_get(mp->focus_anchor_bottom_toggle);

	skin->focus_box_filled = ui_edit_toggle_get(mp->focus_filled_toggle);

	skin->focus_box_r = (guint8)ui_edit_spin_get(mp->focus_r_spin);
	skin->focus_box_g = (guint8)ui_edit_spin_get(mp->focus_g_spin);
	skin->focus_box_b = (guint8)ui_edit_spin_get(mp->focus_b_spin);
	skin->focus_box_alpha = (guint8)ui_edit_spin_get(mp->focus_a_spin);

	g_free(skin->focus_filename);
	skin->focus_filename = g_strdup(image);

	prop_update(ed, NULL, TRUE);

	tab_completion_append_to_history(mp->focus_image_entry, image);
}

static void edit_focus_page_border_cb(GtkWidget *widget, gpointer data)
{
	MainPage *mp = data;
	edit_focus_page_border_sync(mp);
}

static void edit_focus_page_pack(MainPage *mp, GtkWidget *vbox)
{
	GtkWidget *hbox2;
	GtkWidget *frame;

	frame = ui_edit_vframe_new(vbox, TRUE, _("Image"));

	mp->focus_border_toggle = ui_edit_toggle_new(frame, _("Has border"));
	g_signal_connect(G_OBJECT(mp->focus_border_toggle), "clicked",
			 G_CALLBACK(edit_focus_page_border_cb), mp);

	hbox2 = ui_edit_frame_new(frame, FALSE, NULL);

	mp->focus_left_spin = ui_edit_spin_new(hbox2, _("Left:"), 1, 200, NULL);
	mp->focus_right_spin = ui_edit_spin_new(hbox2, _("Right:"), 1, 200, NULL);

	hbox2 = ui_edit_frame_new(frame, FALSE, NULL);

	mp->focus_top_spin = ui_edit_spin_new(hbox2, _("Top:"), 1, 200, NULL);
	mp->focus_bottom_spin = ui_edit_spin_new(hbox2, _("Bottom:"), 1, 200, NULL);

	mp->focus_stretch_toggle = ui_edit_toggle_new(frame, _("Stretch"));
	gtk_widget_set_sensitive(mp->focus_stretch_toggle, FALSE);

	hbox2 = ui_edit_frame_new(frame, TRUE, _("Anchor"));
	mp->focus_anchor_right_toggle = ui_edit_toggle_new(hbox2, _("Right"));
	mp->focus_anchor_bottom_toggle = ui_edit_toggle_new(hbox2, _("Bottom"));

	frame = ui_edit_vframe_new(vbox, TRUE, _("Color"));
	hbox2 = ui_edit_frame_new(frame, FALSE, NULL);
	mp->focus_r_spin = ui_edit_spin_new(hbox2, _("Red:"), 0, 255, NULL);
	mp->focus_g_spin = ui_edit_spin_new(hbox2, _("Green:"), 0, 255, NULL);
	mp->focus_b_spin = ui_edit_spin_new(hbox2, _("Blue:"), 0, 255, NULL);
	hbox2 = ui_edit_frame_new(frame, FALSE, NULL);
	mp->focus_filled_toggle = ui_edit_toggle_new(frame, _("Filled"));

	mp->focus_a_spin = ui_edit_spin_new(vbox, _("Alpha:"), 0, 255, NULL);

	/* sane default (visible) */
	ui_edit_spin_set(mp->focus_r_spin, 255);
	ui_edit_spin_set(mp->focus_a_spin, 255);
}

static GtkWidget *edit_focus_page_new(EditData *ed)
{
	MainPage *mp;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkListStore *store;

	mp = ed->main_page;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	edit_focus_page_pack(mp, vbox);

	mp->focus_image_entry = ui_edit_path_entry_new(vbox, _("Image:"), "SLIK_focus_image");
	mp->focus_image = ui_edit_image_new(vbox);
	ui_edit_path_entry_connect_image(mp->focus_image_entry, mp->focus_image);

	button = gtk_button_new_with_label(_("Change"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(edit_focus_page_add_cb), ed);
	gtk_widget_show(button);

	store = gtk_list_store_new(3, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN);
	mp->focus_list = ui_edit_list_new(hbox, store,
					  G_CALLBACK(edit_focus_page_list_cb), ed);
	g_object_unref(store);

	ui_edit_list_add_column(mp->focus_list, _("Image"), EDITOR_COLUMN_FOCUS_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(mp->focus_list, _("Border"), EDITOR_COLUMN_FOCUS_BORDER, FALSE, FALSE);

	gtk_widget_show(hbox);

	return hbox;
}

static void edit_focus_props_bs_cb(GtkWidget *widget, gpointer data)
{
	MainPage *mp = data;
	SkinData *skin;

	skin = mp->ui->skin;

	skin->focus_stretch = ui_edit_toggle_get(mp->focus_stretch_toggle);
	skin->focus_has_border = ui_edit_toggle_get(mp->focus_border_toggle);
	skin->focus_anchor_right = ui_edit_toggle_get(mp->focus_anchor_right_toggle);
	skin->focus_anchor_bottom = ui_edit_toggle_get(mp->focus_anchor_bottom_toggle);
	skin->focus_box_filled = ui_edit_toggle_get(mp->focus_filled_toggle);
}

static void edit_focus_props_spin_cb(GtkAdjustment *adj, gpointer data)
{
	MainPage *mp = data;
	SkinData *skin;

	skin = mp->ui->skin;

	skin->focus_border_left = ui_edit_spin_get(mp->focus_left_spin);
	skin->focus_border_right = ui_edit_spin_get(mp->focus_right_spin);
	skin->focus_border_top = ui_edit_spin_get(mp->focus_top_spin);
	skin->focus_border_bottom = ui_edit_spin_get(mp->focus_bottom_spin);

	skin->focus_box_r = (guint8)ui_edit_spin_get(mp->focus_r_spin);
	skin->focus_box_g = (guint8)ui_edit_spin_get(mp->focus_g_spin);
	skin->focus_box_b = (guint8)ui_edit_spin_get(mp->focus_b_spin);
	skin->focus_box_alpha = (guint8)ui_edit_spin_get(mp->focus_a_spin);
}

static void edit_focus_connect_spin(GtkWidget *spin, MainPage *mp)
{
	GtkAdjustment *adj;

	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(edit_focus_props_spin_cb), mp);
}

/*
 *-----------------------------------------------------------------------------
 * window
 *-----------------------------------------------------------------------------
 */

static void edit_main_page_add(EditData *ed, MainListData *md);

static MainListData *edit_main_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while (work)
		{
		MainListData *md = work->data;
		if (strcmp(image, md->image) == 0) return md;
		work = work->next;
		}
	return NULL;
}

static void edit_main_read(EditData *ed, SkinData *skin)
{
	MainListData *md;

	edit_focus_read(ed, skin);

	if (!skin->background_filename) return;
	if (edit_main_find(ed->main_list, skin->background_filename)) return;

	md = g_new0(MainListData, 1);
	md->image = g_strdup(skin->background_filename);
	md->transparent = skin->transparent;
	md->mask = g_strdup(skin->background_mask_filename);

	md->stretch = skin->stretch;

	md->has_border = skin->has_border;
	md->border_left = skin->border_left;
	md->border_right = skin->border_right;
	md->border_top = skin->border_top;
	md->border_bottom = skin->border_bottom;
	md->border_left_stretch = skin->border_left_stretch;
	md->border_right_stretch = skin->border_right_stretch;
	md->border_top_stretch = skin->border_top_stretch;
	md->border_bottom_stretch = skin->border_bottom_stretch;

	md->sizeable = skin->sizeable;
	md->width_def = skin->width_def;
	md->width_min = skin->width_min;
	md->width_max = skin->width_max;
	md->width_inc = skin->width_inc;
	md->height_def = skin->height_def;
	md->height_min = skin->height_min;
	md->height_max = skin->height_max;
	md->height_inc = skin->height_inc;

	ed->main_list = g_list_append(ed->main_list, md);
	edit_main_page_add(ed, md);
}

static void edit_main_list_free(GList *list)
{
	GList *work;
	work = list;
	while(work)
		{
		MainListData *md = work->data;
		work = work->next;

		g_free(md->image);
		g_free(md->mask);
		g_free(md);
		}
	g_list_free(list);
}

static void edit_main_page_add_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	MainPage *mp;
	const gchar *image;
	const gchar *mask;
	GdkPixbuf *pb = NULL;
	SkinData *skin;
	gint width, height;
	GList *work;

	mp = ed->main_page;

	image = ui_edit_entry_get(mp->image_entry);
	mask = ui_edit_entry_get(mp->mask_entry);

	if (mask && !isfile(mask))
		{
		warning_dialog(_("Background error"), _("Mask is not valid."));
		return;
		}

	if (image) pb = util_pixbuf_new_from_file(image);

	if (!pb)
		{
		warning_dialog(_("Background error"), _("Image is not valid."));
		return;
		}

	width = gdk_pixbuf_get_width(pb);
	height = gdk_pixbuf_get_height(pb);

	skin = ed->ui->skin;

	skin->stretch = ui_edit_toggle_get(mp->stretch_button);

	skin->has_border = ui_edit_toggle_get(mp->has_border_button);
	skin->border_left = ui_edit_spin_get(mp->border_left_spin);
	skin->border_right = ui_edit_spin_get(mp->border_right_spin);
	skin->border_top = ui_edit_spin_get(mp->border_top_spin);
	skin->border_bottom = ui_edit_spin_get(mp->border_bottom_spin);
	skin->border_left_stretch = ui_edit_toggle_get(mp->border_left_stretch_button);
	skin->border_right_stretch = ui_edit_toggle_get(mp->border_right_stretch_button);
	skin->border_top_stretch = ui_edit_toggle_get(mp->border_top_stretch_button);
	skin->border_bottom_stretch = ui_edit_toggle_get(mp->border_bottom_stretch_button);

	skin->sizeable = ui_edit_toggle_get(mp->sizeable_button);
	skin->width_def = ui_edit_spin_get(mp->width_def_spin);
	skin->width_min = ui_edit_spin_get(mp->width_min_spin);
	skin->width_max = ui_edit_spin_get(mp->width_max_spin);
	skin->width_inc = ui_edit_spin_get(mp->width_inc_spin);
	skin->height_def = ui_edit_spin_get(mp->height_def_spin);
	skin->height_min = ui_edit_spin_get(mp->height_min_spin);
	skin->height_max = ui_edit_spin_get(mp->height_max_spin);
	skin->height_inc = ui_edit_spin_get(mp->height_inc_spin);

	if (skin->overlay) gdk_pixbuf_unref(skin->overlay);
	if (skin->real_overlay) gdk_pixbuf_unref(skin->real_overlay);
	skin->overlay = NULL;
	skin->real_overlay = pb;

	if (skin->has_border || skin->sizeable || skin->stretch)
		{
		skin->width = skin->width_def;
		skin->height = skin->height_def;
		}
	else
		{
		skin->width = width;
		skin->height = height;
		}

	g_free(skin->background_filename);
	skin->background_filename = g_strdup(image);

	if (skin->mask) gdk_pixbuf_unref(skin->mask);
	skin->mask = NULL;
	if (mask) skin->mask = util_pixbuf_new_from_file(mask);

	g_free(skin->background_mask_filename);
	skin->background_mask_filename = g_strdup(mask);

	skin->transparent = ui_edit_toggle_get(mp->transparent_button);

	work = skin->widget_list;
	while (work)
		{
		WidgetData *wd = work->data;
		gint x, y, w, h;

		if (wd->od && wd->od->is_visible && ui_widget_get_geometry(wd, &x, &y, &w, &h))
			{
			gint nx, ny;

			nx = x;
			ny = y;
			if (wd->anchor_right && nx > skin->width - w) nx = MAX(0, skin->width - w);
			if (wd->anchor_bottom && ny > skin->height - h) ny = MAX(0, skin->height - h);
			if (nx != x || ny != y) ui_widget_set_coord(ed->ui, wd, nx, ny, FALSE);
			}
		work = work->next;
		}

	ui_display_sync_all(ed->ui);
	prop_update(ed, NULL, TRUE);

	tab_completion_append_to_history(mp->image_entry, image);
	tab_completion_append_to_history(mp->mask_entry, mask);
}

static void edit_main_page_sync(MainPage *mp, MainListData *md)
{
	if (!md) return;

	ui_edit_toggle_set(mp->stretch_button, md->stretch);

	ui_edit_toggle_set(mp->has_border_button, md->has_border);
	ui_edit_spin_set(mp->border_left_spin, md->border_left);
	ui_edit_spin_set(mp->border_right_spin, md->border_right);
	ui_edit_spin_set(mp->border_top_spin, md->border_top);
	ui_edit_spin_set(mp->border_bottom_spin, md->border_bottom);
	ui_edit_toggle_set(mp->border_left_stretch_button, md->border_left_stretch);
	ui_edit_toggle_set(mp->border_right_stretch_button, md->border_right_stretch);
	ui_edit_toggle_set(mp->border_top_stretch_button, md->border_top_stretch);
	ui_edit_toggle_set(mp->border_bottom_stretch_button, md->border_bottom_stretch);

	ui_edit_toggle_set(mp->sizeable_button, md->sizeable);
	ui_edit_spin_set(mp->width_def_spin, md->width_def);
	ui_edit_spin_set(mp->width_min_spin, md->width_min);
	ui_edit_spin_set(mp->width_max_spin, md->width_max);
	ui_edit_spin_set(mp->width_inc_spin, md->width_inc);
	ui_edit_spin_set(mp->height_def_spin, md->height_def);
	ui_edit_spin_set(mp->height_min_spin, md->height_min);
	ui_edit_spin_set(mp->height_max_spin, md->height_max);
	ui_edit_spin_set(mp->height_inc_spin, md->height_inc);

	ui_edit_entry_set(mp->image_entry, md->image);
	ui_edit_toggle_set(mp->transparent_button, md->transparent);
	ui_edit_entry_set(mp->mask_entry, md->mask);
}

static void edit_main_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	EditData *ed = data;
	MainPage *mp;
	MainListData *md;
	GtkTreeModel *store;
	GtkTreeIter iter;

	mp = ed->main_page;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, EDITOR_COLUMN_BACK_POINTER, &md, -1);
	edit_main_page_sync(mp, md);
}

static void edit_main_page_add(EditData *ed, MainListData *md)
{
	MainPage *mp = ed->main_page;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(mp->list)));

	pixbuf = ui_edit_list_pixbuf_from_file(md->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   EDITOR_COLUMN_BACK_POINTER, md,
			   EDITOR_COLUMN_BACK_IMAGE, pixbuf,
			   EDITOR_COLUMN_BACK_WIDTH, md->width_def,
			   EDITOR_COLUMN_BACK_HEIGHT, md->height_def,
			   EDITOR_COLUMN_BACK_SIZEABLE, md->sizeable,
			   EDITOR_COLUMN_BACK_BORDER, md->has_border,
			   EDITOR_COLUMN_BACK_TRANSPARENT, md->transparent, -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

static void edit_main_page_sizer_cb(GtkWidget *widget, gpointer data)
{
	MainPage *mp = data;
	gint stretch;
	gint size;
	gint state;

	stretch = ui_edit_toggle_get(mp->stretch_button);
	size = ui_edit_toggle_get(mp->sizeable_button);
	state = ui_edit_toggle_get(mp->has_border_button);

	ui_edit_frame_sensitive(mp->border_left_stretch_button, state, FALSE);
	ui_edit_frame_sensitive(mp->border_right_stretch_button, state, FALSE);
	ui_edit_frame_sensitive(mp->border_top_stretch_button, state, FALSE);
	ui_edit_frame_sensitive(mp->border_bottom_stretch_button, state, FALSE);

#if 0
	/* we now allow sizing at any time */
	ui_edit_frame_sensitive(mp->width_def_spin, size || stretch || state, FALSE);
#endif
	ui_edit_frame_sensitive(mp->width_min_spin, size, FALSE);
	ui_edit_frame_sensitive(mp->height_min_spin, size, FALSE);
}

static void edit_main_page_pack(MainPage *mp, GtkWidget *vbox)
{
	GtkWidget *hbox2;
	GtkWidget *frame;

	mp->has_border_button = ui_edit_toggle_new(vbox, _("Has border"));

	hbox2 = ui_edit_frame_new(vbox, FALSE, NULL);

	frame = ui_edit_frame_new(hbox2, TRUE, _("Left"));
	mp->border_left_spin = ui_edit_spin_new(frame, _("Size:"), 1, 200, NULL);
	mp->border_left_stretch_button = ui_edit_toggle_new(frame, _("Stretch"));

	frame = ui_edit_frame_new(hbox2, TRUE, _("Right"));
	mp->border_right_spin = ui_edit_spin_new(frame, _("Size:"), 1, 200, NULL);
	mp->border_right_stretch_button = ui_edit_toggle_new(frame, _("Stretch"));

	hbox2 = ui_edit_frame_new(vbox, FALSE, NULL);

	frame = ui_edit_frame_new(hbox2, TRUE, _("Top"));
	mp->border_top_spin = ui_edit_spin_new(frame, _("Size:"), 1, 200, NULL);
	mp->border_top_stretch_button = ui_edit_toggle_new(frame, _("Stretch"));

	frame = ui_edit_frame_new(hbox2, TRUE, _("Bottom"));
	mp->border_bottom_spin = ui_edit_spin_new(frame, _("Size:"), 1, 200, NULL);
	mp->border_bottom_stretch_button = ui_edit_toggle_new(frame, _("Stretch"));

	frame = ui_edit_frame_new(vbox, FALSE, NULL);

	mp->stretch_button = ui_edit_toggle_new(frame, _("Stretch background"));
	mp->sizeable_button = ui_edit_toggle_new(frame, _("Allow resize"));

	frame = ui_edit_frame_new(vbox, TRUE, _("Default size"));
	mp->width_def_spin = ui_edit_spin_new(frame, _("Width:"), 8, 2000, NULL);
	mp->height_def_spin = ui_edit_spin_new(frame, _("Height:"), 8, 2000, NULL);

	mp->reset_size_button = gtk_button_new_with_label(_("Reset to image"));
	gtk_box_pack_end(GTK_BOX(frame), mp->reset_size_button, FALSE, FALSE, 0);
	gtk_widget_show(mp->reset_size_button);

	frame = ui_edit_frame_new(vbox, TRUE, _("Width attributes"));
	mp->width_min_spin = ui_edit_spin_new(frame, _("Min:"), 8, 2000, NULL);
	mp->width_max_spin = ui_edit_spin_new(frame, _("Max:"), 8, 2000, NULL);
	mp->width_inc_spin = ui_edit_spin_new(frame, _("Increment:"), 1, 2000, NULL);

	frame = ui_edit_frame_new(vbox, TRUE, _("Height attributes"));
	mp->height_min_spin = ui_edit_spin_new(frame, _("Min:"), 8, 2000, NULL);
	mp->height_max_spin = ui_edit_spin_new(frame, _("Max:"), 8, 2000, NULL);
	mp->height_inc_spin = ui_edit_spin_new(frame, _("Increment:"), 1, 2000, NULL);
}

static GtkWidget *edit_main_page_new(EditData *ed)
{
	GtkListStore *store;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *button;
	MainPage *mp;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	mp = g_new0(MainPage, 1);

	edit_main_page_pack(mp, vbox);

	g_signal_connect(G_OBJECT(mp->has_border_button), "clicked",
			 G_CALLBACK(edit_main_page_sizer_cb), mp);
	g_signal_connect(G_OBJECT(mp->stretch_button), "clicked",
			 G_CALLBACK(edit_main_page_sizer_cb), mp);
	g_signal_connect(G_OBJECT(mp->sizeable_button), "clicked",
			 G_CALLBACK(edit_main_page_sizer_cb), mp);

	gtk_widget_set_sensitive(mp->reset_size_button, FALSE);

	mp->image_entry = ui_edit_path_entry_new(vbox, _("Image:"), "SLIK_main_image");
	mp->image = ui_edit_image_new(vbox);
	ui_edit_path_entry_connect_image(mp->image_entry, mp->image);
	mp->mask_entry = ui_edit_path_entry_new(vbox, _("Mask:"), "SLIK_main_mask_image");
	mp->transparent_button = ui_edit_toggle_new(vbox, _("Transparent"));

	button = gtk_button_new_with_label(_("Change"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(edit_main_page_add_cb), ed);
	gtk_widget_show(button);

	store = gtk_list_store_new(7, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_INT,
				      G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
				      G_TYPE_BOOLEAN);
	mp->list = ui_edit_list_new(hbox, store,
				    G_CALLBACK(edit_main_page_list_cb), ed);
	g_object_unref(store);

	ui_edit_list_add_column(mp->list, _("Image"), EDITOR_COLUMN_BACK_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(mp->list, _("Width"), EDITOR_COLUMN_BACK_WIDTH, FALSE, FALSE);
	ui_edit_list_add_column(mp->list, _("Height"), EDITOR_COLUMN_BACK_HEIGHT, FALSE, FALSE);
	ui_edit_list_add_column(mp->list, _("Sizeable"), EDITOR_COLUMN_BACK_SIZEABLE, FALSE, FALSE);
	ui_edit_list_add_column(mp->list, _("Border"), EDITOR_COLUMN_BACK_BORDER, FALSE, FALSE);
	ui_edit_list_add_column(mp->list, _("Transparent"), EDITOR_COLUMN_BACK_TRANSPARENT, FALSE, FALSE);

	edit_main_page_sizer_cb(NULL, mp);

	gtk_widget_show(hbox);
	ed->main_page = mp;

	return hbox;
}

static void edit_main_page_free(MainPage *mp)
{
	g_free(mp);
}

static void edit_main_set_spin(MainPage *mp)
{
	SkinData *skin = mp->ui->skin;

	ui_edit_spin_set_blocking(mp->border_left_spin, skin->border_left, mp);
	ui_edit_spin_set_blocking(mp->border_right_spin, skin->border_right, mp);
	ui_edit_spin_set_blocking(mp->border_top_spin, skin->border_top, mp);
	ui_edit_spin_set_blocking(mp->border_bottom_spin, skin->border_bottom, mp);

	ui_edit_spin_set_blocking(mp->width_def_spin, skin->width_def, mp);
	ui_edit_spin_set_blocking(mp->width_min_spin, skin->width_min, mp);
	ui_edit_spin_set_blocking(mp->width_max_spin, skin->width_max, mp);
	ui_edit_spin_set_blocking(mp->width_inc_spin, skin->width_inc, mp);
	ui_edit_spin_set_blocking(mp->height_def_spin, skin->height_def, mp);
	ui_edit_spin_set_blocking(mp->height_min_spin, skin->height_min, mp);
	ui_edit_spin_set_blocking(mp->height_max_spin, skin->height_max, mp);
	ui_edit_spin_set_blocking(mp->height_inc_spin, skin->height_inc, mp);
}

static void edit_main_props_force_redraw(MainPage *mp)
{
	SkinData *skin = mp->ui->skin;
	gint s;

	/* We must make it look sizeable so that toggling stretch/borders
	 * will still update the overlay.
         */

	s = skin->sizeable;
	skin->sizeable = TRUE;
	ui_display_sync_all(mp->ui);
	skin->sizeable = s;
}

static void edit_main_props_border_cb(GtkWidget *widget, gpointer data)
{
	MainPage *mp = data;
	SkinData *skin;

	skin = mp->ui->skin;

	skin->has_border = ui_edit_toggle_get(mp->has_border_button);

	edit_main_page_sizer_cb(NULL, mp);
	edit_main_props_force_redraw(mp);
}

static void edit_main_props_stretch_cb(GtkWidget *widget, gpointer data)
{
	MainPage *mp = data;
	SkinData *skin;

	skin = mp->ui->skin;

	skin->stretch = ui_edit_toggle_get(mp->stretch_button);

	edit_main_page_sizer_cb(NULL, mp);
	edit_main_props_force_redraw(mp);
}

static void edit_main_props_sizeable_cb(GtkWidget *widget, gpointer data)
{
	MainPage *mp = data;
	SkinData *skin;

	skin = mp->ui->skin;

	skin->sizeable = ui_edit_toggle_get(mp->sizeable_button);
	if (!skin->sizeable)
		{
		skin->width_def = skin->width;
		skin->height_def = skin->height;
		edit_main_set_spin(mp);
		}

	edit_main_page_sizer_cb(NULL, mp);
}

static void edit_main_props_bs_cb(GtkWidget *widget, gpointer data)
{
	MainPage *mp = data;
	SkinData *skin;

	skin = mp->ui->skin;

	skin->border_left_stretch = ui_edit_toggle_get(mp->border_left_stretch_button);
	skin->border_right_stretch = ui_edit_toggle_get(mp->border_right_stretch_button);
	skin->border_top_stretch = ui_edit_toggle_get(mp->border_top_stretch_button);
	skin->border_bottom_stretch = ui_edit_toggle_get(mp->border_bottom_stretch_button);

	ui_display_sync_all(mp->ui);
}

static void edit_main_props_spin_cb(GtkAdjustment *adj, gpointer data)
{
	MainPage *mp = data;
	SkinData *skin;
	gint w, h;

	skin = mp->ui->skin;

	skin->border_left = ui_edit_spin_get(mp->border_left_spin);
	skin->border_right = ui_edit_spin_get(mp->border_right_spin);
	skin->border_top = ui_edit_spin_get(mp->border_top_spin);
	skin->border_bottom = ui_edit_spin_get(mp->border_bottom_spin);

	skin->width_def = ui_edit_spin_get(mp->width_def_spin);
	skin->width_min = ui_edit_spin_get(mp->width_min_spin);
	skin->width_max = ui_edit_spin_get(mp->width_max_spin);
	skin->width_inc = ui_edit_spin_get(mp->width_inc_spin);
	skin->height_def = ui_edit_spin_get(mp->height_def_spin);
	skin->height_min = ui_edit_spin_get(mp->height_min_spin);
	skin->height_max = ui_edit_spin_get(mp->height_max_spin);
	skin->height_inc = ui_edit_spin_get(mp->height_inc_spin);

	w = gdk_pixbuf_get_width(skin->real_overlay) - 1;
	h = gdk_pixbuf_get_height(skin->real_overlay) - 1;

	if (adj == GTK_SPIN_BUTTON(mp->border_left_spin)->adjustment)
		{
		if (skin->border_right > w - skin->border_left) skin->border_right = w - skin->border_left;
		ui_display_sync_all(mp->ui);
		}
	if (adj == GTK_SPIN_BUTTON(mp->border_right_spin)->adjustment)
		{
		if (skin->border_left > w - skin->border_right) skin->border_left = w - skin->border_right;
		ui_display_sync_all(mp->ui);
		}
	if (adj == GTK_SPIN_BUTTON(mp->border_top_spin)->adjustment)
		{
		if (skin->border_bottom > h - skin->border_top) skin->border_bottom = h - skin->border_top;
		ui_display_sync_all(mp->ui);
		}
	if (adj == GTK_SPIN_BUTTON(mp->border_bottom_spin)->adjustment)
		{
		if (skin->border_top > h - skin->border_bottom) skin->border_top = h - skin->border_bottom;
		ui_display_sync_all(mp->ui);
		}

	if (adj == GTK_SPIN_BUTTON(mp->width_def_spin)->adjustment)
		{
		if (skin->width_min > skin->width_def) skin->width_min = skin->width_def;
		if (skin->width_max < skin->width_def) skin->width_max = skin->width_def;
		skin_resize(mp->ui, skin->width_def, skin->height_def);
		}
	if (adj == GTK_SPIN_BUTTON(mp->height_def_spin)->adjustment)
		{
		if (skin->height_min > skin->height_def) skin->height_min = skin->height_def;
		if (skin->height_max < skin->height_def) skin->height_max = skin->height_def;
		skin_resize(mp->ui, skin->width_def, skin->height_def);
		}

	if (adj == GTK_SPIN_BUTTON(mp->width_min_spin)->adjustment)
		{
		if (skin->width_def < skin->width_min) skin->width_def = skin->width_min;
		if (skin->width_max < skin->width_min) skin->width_max = skin->width_min;
		skin_resize(mp->ui, skin->width_min, skin->height_min);
		}
	if (adj == GTK_SPIN_BUTTON(mp->height_min_spin)->adjustment)
		{
		if (skin->height_def < skin->height_min) skin->height_def = skin->height_min;
		if (skin->height_max < skin->height_min) skin->height_max = skin->height_min;
		skin_resize(mp->ui, skin->width_min, skin->height_min);
		}

	if (adj == GTK_SPIN_BUTTON(mp->width_max_spin)->adjustment)
		{
		if (skin->width_def > skin->width_max) skin->width_def = skin->width_max;
		if (skin->width_min > skin->width_max) skin->width_min = skin->width_max;
		skin_resize(mp->ui, skin->width_max, skin->height_max);
		}
	if (adj == GTK_SPIN_BUTTON(mp->height_max_spin)->adjustment)
		{
		if (skin->height_def > skin->height_max) skin->height_def = skin->height_max;
		if (skin->height_min > skin->height_max) skin->height_min = skin->height_max;
		skin_resize(mp->ui, skin->width_max, skin->height_max);
		}

	edit_main_set_spin(mp);
}

static void edit_main_connect_spin(GtkWidget *spin, MainPage *mp)
{
	GtkAdjustment *adj;

	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(edit_main_props_spin_cb), mp);
}

static void edit_main_props_rest_cb(GtkWidget *widget, gpointer data)
{
	MainPage *mp = data;
	SkinData *skin;

	skin = mp->ui->skin;

	ui_edit_spin_set(mp->width_def_spin, gdk_pixbuf_get_width(skin->real_overlay));
	ui_edit_spin_set(mp->height_def_spin, gdk_pixbuf_get_height(skin->real_overlay));
}

static gpointer edit_main_props(UIData *ui, GtkWidget *vbox, gpointer detail)
{
	MainPage *mp = detail;
	SkinData *skin;

	skin = ui->skin;

	if (!mp)
                {
		GtkWidget *notebook;
		GtkWidget *page;
		GtkWidget *label;
		GtkAdjustment *adj;
		gint w, h;

		mp = g_new0(MainPage, 1);
		mp->ui = ui;

		notebook = gtk_notebook_new();
		gtk_box_pack_start(GTK_BOX(vbox), notebook, FALSE, FALSE, 0);
		gtk_widget_show(notebook);

		page = gtk_vbox_new(FALSE, 0);
		label = gtk_label_new(_("window"));
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);

		edit_main_page_pack(mp, page);

		w = gdk_pixbuf_get_width(skin->real_overlay) - 2;
		h = gdk_pixbuf_get_height(skin->real_overlay) - 2;

		adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(mp->border_left_spin));
		adj->upper = (gfloat)w;

		adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(mp->border_right_spin));
		adj->upper = (gfloat)w;

		adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(mp->border_top_spin));
		adj->upper = (gfloat)h;

		adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(mp->border_bottom_spin));
		adj->upper = (gfloat)h;

		g_signal_connect(G_OBJECT(mp->has_border_button), "clicked",
				 G_CALLBACK(edit_main_props_border_cb), mp);
		g_signal_connect(G_OBJECT(mp->stretch_button), "clicked",
				 G_CALLBACK(edit_main_props_stretch_cb), mp);
		g_signal_connect(G_OBJECT(mp->sizeable_button), "clicked",
				 G_CALLBACK(edit_main_props_sizeable_cb), mp);

		g_signal_connect(G_OBJECT(mp->border_left_stretch_button), "clicked",
				 G_CALLBACK(edit_main_props_bs_cb), mp);
		g_signal_connect(G_OBJECT(mp->border_right_stretch_button), "clicked",
				 G_CALLBACK(edit_main_props_bs_cb), mp);
		g_signal_connect(G_OBJECT(mp->border_top_stretch_button), "clicked",
				 G_CALLBACK(edit_main_props_bs_cb), mp);
		g_signal_connect(G_OBJECT(mp->border_bottom_stretch_button), "clicked",
				 G_CALLBACK(edit_main_props_bs_cb), mp);

		edit_main_connect_spin(mp->border_left_spin, mp);
		edit_main_connect_spin(mp->border_right_spin, mp);
		edit_main_connect_spin(mp->border_top_spin, mp);
		edit_main_connect_spin(mp->border_bottom_spin, mp);

		edit_main_connect_spin(mp->width_def_spin, mp);
		edit_main_connect_spin(mp->width_min_spin, mp);
		edit_main_connect_spin(mp->width_max_spin, mp);
		edit_main_connect_spin(mp->width_inc_spin, mp);
		edit_main_connect_spin(mp->height_def_spin, mp);
		edit_main_connect_spin(mp->height_min_spin, mp);
		edit_main_connect_spin(mp->height_max_spin, mp);
		edit_main_connect_spin(mp->height_inc_spin, mp);

		g_signal_connect(G_OBJECT(mp->reset_size_button), "clicked",
				 G_CALLBACK(edit_main_props_rest_cb), mp);

		gtk_widget_show(page);

		page = gtk_vbox_new(FALSE, 0);
		label = gtk_label_new(_("focus"));
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);

		edit_focus_page_pack(mp, page);

		g_signal_connect(G_OBJECT(mp->focus_stretch_toggle), "clicked",
				 G_CALLBACK(edit_focus_props_bs_cb), mp);
		g_signal_connect(G_OBJECT(mp->focus_border_toggle), "clicked",
				 G_CALLBACK(edit_focus_props_bs_cb), mp);
		g_signal_connect(G_OBJECT(mp->focus_anchor_right_toggle), "clicked",
				 G_CALLBACK(edit_focus_props_bs_cb), mp);
		g_signal_connect(G_OBJECT(mp->focus_anchor_bottom_toggle), "clicked",
				 G_CALLBACK(edit_focus_props_bs_cb), mp);
		g_signal_connect(G_OBJECT(mp->focus_filled_toggle), "clicked",
				 G_CALLBACK(edit_focus_props_bs_cb), mp);

		edit_focus_connect_spin(mp->focus_left_spin, mp);
		edit_focus_connect_spin(mp->focus_right_spin, mp);
		edit_focus_connect_spin(mp->focus_top_spin, mp);
		edit_focus_connect_spin(mp->focus_bottom_spin, mp);

		edit_focus_connect_spin(mp->focus_r_spin, mp);
		edit_focus_connect_spin(mp->focus_g_spin, mp);
		edit_focus_connect_spin(mp->focus_b_spin, mp);
		edit_focus_connect_spin(mp->focus_a_spin, mp);

		gtk_widget_show(page);
                }

	ui_edit_toggle_set_blocking(mp->stretch_button, skin->stretch, mp);

	ui_edit_toggle_set_blocking(mp->has_border_button, skin->has_border, mp);
	ui_edit_toggle_set_blocking(mp->border_left_stretch_button, skin->border_left_stretch, mp);
	ui_edit_toggle_set_blocking(mp->border_right_stretch_button, skin->border_right_stretch, mp);
	ui_edit_toggle_set_blocking(mp->border_top_stretch_button, skin->border_top_stretch, mp);
	ui_edit_toggle_set_blocking(mp->border_bottom_stretch_button, skin->border_bottom_stretch, mp);

	ui_edit_toggle_set_blocking(mp->sizeable_button, skin->sizeable, mp);

	edit_main_set_spin(mp);

	edit_main_page_sizer_cb(NULL, mp);

	/* focus */

	ui_edit_toggle_set_blocking(mp->focus_border_toggle, skin->focus_has_border, mp);
	ui_edit_toggle_set_blocking(mp->focus_stretch_toggle, skin->focus_stretch, mp);
	ui_edit_spin_set_blocking(mp->focus_left_spin, skin->focus_border_left, mp);
	ui_edit_spin_set_blocking(mp->focus_right_spin, skin->focus_border_right, mp);
	ui_edit_spin_set_blocking(mp->focus_top_spin, skin->focus_border_top, mp);
	ui_edit_spin_set_blocking(mp->focus_bottom_spin, skin->focus_border_bottom, mp);

	edit_focus_page_border_sync(mp);

	ui_edit_toggle_set_blocking(mp->focus_anchor_right_toggle, skin->focus_anchor_right, mp);
	ui_edit_toggle_set_blocking(mp->focus_anchor_bottom_toggle, skin->focus_anchor_bottom, mp);

	ui_edit_toggle_set_blocking(mp->focus_filled_toggle, skin->focus_box_filled, mp);
	ui_edit_spin_set_blocking(mp->focus_r_spin, (gint)skin->focus_box_r, mp);
	ui_edit_spin_set_blocking(mp->focus_g_spin, (gint)skin->focus_box_g, mp);
	ui_edit_spin_set_blocking(mp->focus_b_spin, (gint)skin->focus_box_b, mp);
	ui_edit_spin_set_blocking(mp->focus_a_spin, (gint)skin->focus_box_alpha, mp);

	return mp;
}

/*
 *-----------------------------------------------------------------------------
 * widget read/import utils
 *-----------------------------------------------------------------------------
 */

static gint ui_edit_import_skin_keys(EditData *ed, const gchar *path)
{
	SkinData *skin;
	GList *work;
	gchar *dir;

	if (debug_mode) printf("Editor parsing skin \"%s\"...\n", path);

	dir = remove_level_from_path(path);
	skin = skin_parse(dir, path, TRUE);
	g_free(dir);

	if (!skin) return FALSE;

	work = skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;
		ui_edit_widget_id_list_read(ed->ui, wd, ed->widget_id_list);
		work = work->next;
		}

	edit_main_read(ed, skin);

	skin_free(skin);

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * widget properties interface
 *-----------------------------------------------------------------------------
 */

/* old_wd is used to check that is is an update of the background props */
static void prop_sync(EditData *ed, WidgetData *wd, WidgetData *old_wd)
{
	if (ed->detail_data && ((!wd && old_wd != NULL) || (wd && ed->detail_type != wd->type)) )
		{
		/* hide the details, some widget types do not update
		 * immediately, but will upon losing focus (spin buttons)
		 */
		if (ed->details) gtk_widget_hide(ed->details);

		g_free(ed->detail_data);
		ed->detail_data = NULL;
		ed->detail_type = -1;
		}

	if (!ed->detail_data)
		{
		if (ed->details) gtk_widget_destroy(ed->details);
		ed->details = NULL;
		}

	if (!ed->details)
		{
		ed->details = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(ed->detail_vbox), ed->details, FALSE, FALSE, 0);
		}

	if (wd && wd->od && wd->od->func_edit_props)
		{
		ed->detail_data = wd->od->func_edit_props(ed->ui, wd, ed->details, ed->detail_data);
		ed->detail_type = wd->type;
		}
	else if (!wd)
		{
		ed->detail_data = edit_main_props(ed->ui, ed->details, ed->detail_data);
		ed->detail_type = -1;
		}

	if (!GTK_WIDGET_VISIBLE(ed->details)) gtk_widget_show(ed->details);
}

static void prop_set_coord(EditData *ed, gint x, gint y, gint max_x, gint max_y)
{
	GtkAdjustment *xa;
	GtkAdjustment *ya;

	xa = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(ed->spin_x));
	ya = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(ed->spin_y));

	g_signal_handlers_block_matched(G_OBJECT(xa), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, ed);
	xa->upper = (gfloat)max_x;
	xa->value = (gfloat)x;
	gtk_adjustment_value_changed(xa);
	g_signal_handlers_unblock_matched(G_OBJECT(xa), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, ed);

	g_signal_handlers_block_matched(G_OBJECT(ya), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, ed);
	ya->upper = (gfloat)max_y;
	ya->value = (gfloat)y;
	gtk_adjustment_value_changed(ya);
	g_signal_handlers_unblock_matched(G_OBJECT(ya), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, ed);
}

static void prop_set_sensitive(EditData *ed, gint state)
{
	ui_edit_frame_sensitive(ed->key_combo_entry, state, TRUE);
	ui_edit_frame_sensitive(ed->data_entry, state, TRUE);
	ui_edit_frame_sensitive(ed->text_id_entry, state, TRUE);

	ui_edit_frame_sensitive(ed->spin_x, state, TRUE);
	ui_edit_frame_sensitive(ed->spin_y, state, TRUE);
	ui_edit_frame_sensitive(ed->anchor_right_button, state, FALSE);
}

static void prop_update(EditData *ed, WidgetData *wd, gint force)
{
	gchar *buf;
	const gchar *ckey;
	const gchar *cdata;
	WidgetType ctype;
	const gchar *cimage;
	const gchar *cmask;
	gint car, cab;
	gint x, y, w, h;
	WidgetData *old_wd;
	GdkPixbuf *pb;

	if (wd)
		{
		if (!ui_widget_get_geometry(wd, &x, &y, &w, &h))
			{
			x = y = w = h = 0;
			}
		}
	else
		{
		x = y = 0;
		w = ed->ui->skin->width;
		h = ed->ui->skin->height;
		}

	buf = g_strdup_printf(_("%-12s w: %-3d h: %-3d"), wd ? ui_widget_type_to_text(wd->type) : _("skin size"), w, h);
	gtk_label_set_text(GTK_LABEL(ed->description), buf);
	g_free(buf);

	prop_set_coord(ed, x, y, ed->ui->skin->width - w, ed->ui->skin->height - h);

	old_wd = ed->info_widget;

	ed->info_widget = wd;

	prop_sync(ed, wd, old_wd);

	if (old_wd == ed->info_widget && !force) return;

	if (old_wd != ed->info_widget &&
	    ( ((old_wd == NULL) != (ed->info_widget == NULL)) || (old_wd && ed->info_widget && old_wd->type != ed->info_widget->type)) )
		help_window_scroll(ed, ed->info_widget ? ed->info_widget->type : -2);

	if (wd)
		{
		pb = ui_edit_widget_get_pixbuf(wd);

		ckey = wd->key;
		cdata = ui_widget_get_data(wd, "data");
		ctype = wd->type;
		cimage = ui_widget_get_data(wd, "image");
		cmask = ui_widget_get_data(wd, "clip_mask");

		car = wd->anchor_right;
		cab = wd->anchor_bottom;
		}
	else
		{
		pb = ed->ui->skin->real_overlay;

		ckey = "";
		cdata = NULL;
		ctype = -1;
		cimage = ed->ui->skin->background_filename;
		cmask = ed->ui->skin->background_mask_filename;

		car = FALSE;
		cab = FALSE;
		}

	if (pb)
		{
		gtk_image_set_from_pixbuf(GTK_IMAGE(ed->widget_image), pb);
		}
	else
		{
		ui_edit_image_blank(ed->widget_image);
		}

	ui_edit_entry_set_blocking(GTK_COMBO(ed->key_combo_entry)->entry, ckey, ed);
	ui_edit_entry_set_blocking(ed->data_entry, cdata, ed);
	ui_edit_entry_set_blocking(ed->text_id_entry, (wd && wd->text_id) ? wd->text_id : "", ed);
	ui_edit_key_entry_set_keys(ed->key_combo_entry, ed->ui, ctype, ed);

	ui_edit_entry_set(ed->image_entry, cimage);
	ui_edit_entry_set(ed->clip_mask_entry, cmask);

	ui_edit_toggle_set(ed->anchor_right_button, car);
	ui_edit_toggle_set(ed->anchor_bottom_button, cab);

	prop_set_sensitive(ed, (wd != NULL));
}

static void prop_clear(EditData *ed)
{
	ed->info_widget = NULL;

	/* hide the details, some widget types do not update
	 * immediately, but will upon losing focus (spin buttons)
	 */
	if (ed->details) gtk_widget_hide(ed->details);

	if (ed->detail_data)
		{
		g_free(ed->detail_data);
		ed->detail_data = NULL;
		ed->detail_type = -1;
		}

	if (ed->details)
		{
		gtk_widget_destroy(ed->details);
		ed->details = NULL;
		}
}


/*
 *-----------------------------------------------------------------------------
 * widget properties callbacks
 *-----------------------------------------------------------------------------
 */

static void prop_spin_x_cb(GtkAdjustment *adjustment, gpointer data)
{
	EditData *ed = data;
	gint dmy, ox, x, y;

	if (!ed->info_widget || !ed->info_widget->od || !ed->info_widget->od->is_visible ||
	    !ui_widget_get_geometry(ed->info_widget, &ox, &y, &dmy, &dmy)) return;

	x = (gint)GTK_ADJUSTMENT(adjustment)->value;
	if (ox == x) return;

	ui_edit_widget_draw_highlight(ed->ui, ed->active_widget, FALSE);
	edit_widget_set_coord(ed, ed->info_widget, x, y);
	ui_edit_widget_draw_highlight(ed->ui, ed->active_widget, TRUE);
}

static void prop_spin_y_cb(GtkAdjustment *adjustment, gpointer data)
{
	EditData *ed = data;
	gint dmy, x, oy, y;

	if (!ed->info_widget || !ed->info_widget->od || !ed->info_widget->od->is_visible ||
	    !ui_widget_get_geometry(ed->info_widget, &x, &oy, &dmy, &dmy)) return;

	y = (gint)GTK_ADJUSTMENT(adjustment)->value;
	if (oy == y) return;

	ui_edit_widget_draw_highlight(ed->ui, ed->active_widget, FALSE);
	edit_widget_set_coord(ed, ed->info_widget, x, y);
	ui_edit_widget_draw_highlight(ed->ui, ed->active_widget, TRUE);
}

static void prop_key_entry_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	WidgetData *wd;
	const gchar *buf;

	if (!ed->info_widget) return;

	wd = ed->info_widget;

	buf = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(ed->key_combo_entry)->entry));
	if (buf && strlen(buf) > 0 && strcmp (buf, wd->key) != 0)
		{
		/* reset the od to original */
		wd->od = ui_widget_object_by_type(wd->type);

		/* update any privately registered keys
		 * (assumes standard format <desc>_<key>_<action>)
		 */
		if (wd->od && wd->od->description)
			{
			GList *work;
			gchar *needle;
			gint nl;

			needle = g_strdup_printf("%s_%s_", wd->od->description, wd->key);
			nl = strlen(needle);
			work = ed->ui->register_list;
			while(work)
				{
				RegisterData *rd = work->data;

				if (rd->private && rd->private_widget == wd->widget &&
				    strncmp(rd->key, needle, nl) == 0)
					{
					gchar *t = g_strdup_printf("%s_%s_%s", wd->od->description,
								   buf, rd->key + nl);
					g_free(rd->key);
					rd->key = t;
					}

				work = work->next;
				}
			g_free(needle);
			}

		/* set new key */
		g_free(wd->key);
		wd->key = g_strdup(buf);

		/* re-finalize, widget may now need it */
		skin_finalize(ed->ui->skin);

		widget_list_update(ed, ed->info_widget, EDITOR_COLUMN_WIDGET_KEY, ed->info_widget->key);
		ui_widget_draw(ed->ui, ed->info_widget, TRUE, FALSE);
		}
}

static void prop_data_entry_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	const gchar *buf;
	const gchar *d;

	if (!ed->info_widget) return;

	buf = gtk_entry_get_text(GTK_ENTRY(ed->data_entry));
	d = ui_widget_get_data(ed->info_widget, "data");

	if ((buf && strlen(buf) == 0 && !d) ||
	    (buf && d && strcmp(buf, d) == 0))
		{
		return;
		}

	if (buf && strlen(buf) > 0)
		{
		ui_widget_set_data(ed->info_widget, "data", buf);
		widget_list_update(ed, ed->info_widget, EDITOR_COLUMN_WIDGET_DATA, buf);
		}
	else
		{
		ui_widget_set_data(ed->info_widget, "data", NULL);
		widget_list_update(ed, ed->info_widget, EDITOR_COLUMN_WIDGET_DATA, NULL);
		}

	ui_widget_draw(ed->ui, ed->info_widget, TRUE, FALSE);
}

static void prop_text_id_entry_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	const gchar *buf;
	WidgetData *wd;

	if (!ed->info_widget) return;
	wd = ed->info_widget;

	buf = gtk_entry_get_text(GTK_ENTRY(ed->text_id_entry));

	g_free(wd->text_id);
	wd->text_id = NULL;
	if (buf && strlen(buf) > 0)
		{
		wd->text_id = g_strdup(buf);
		}
}

static void prop_anchor_right_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;

	if (!ed->info_widget) return;

	ed->info_widget->anchor_right = ui_edit_toggle_get(ed->anchor_right_button);
	ui_edit_widget_draw_highlight(ed->ui, ed->info_widget, TRUE);
}

static void prop_anchor_bottom_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;

	if (!ed->info_widget) return;

	ed->info_widget->anchor_bottom = ui_edit_toggle_get(ed->anchor_bottom_button);
	ui_edit_widget_draw_highlight(ed->ui, ed->info_widget, TRUE);
}

/*
 *-----------------------------------------------------------------------------
 * widget list stuff
 *-----------------------------------------------------------------------------
 */

static void widget_list_set_widget(EditData *ed, WidgetData *wd)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	if (!wd) wd = skin_widget_get_by_key(ed->ui->skin, "background", dummy_type_id);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ed->widget_list));

	if (!ui_edit_list_find_data(ed->widget_list, &store, &iter, wd))
		{
		gtk_tree_selection_unselect_all(selection);
		}
	else
		{
		gtk_tree_selection_select_iter(selection, &iter);
		tree_view_row_make_visible(GTK_TREE_VIEW(ed->widget_list), &iter, TRUE);
		}
}

static void widget_list_update(EditData *ed, WidgetData *wd, gint col, const gchar *text)
{
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (ui_edit_list_find_data(ed->widget_list, &store, &iter, wd))
		{
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, col, text, -1);
		}
}

static void widget_list_reset(EditData *ed)
{
	GtkListStore *store;
	GList *work;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ed->widget_list)));
	gtk_list_store_clear(store);

	work = ed->ui->skin->widget_list;
	while (work)
		{
		WidgetData *wd;
		GtkTreeIter iter;

		wd = work->data;
		work = work->next;

#if 0
		/* do not display the dummy widget (so it can not be removed) */
		if (wd->type == dummy_type_id) continue;
#endif

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   EDITOR_COLUMN_WIDGET_POINTER, wd,
				   EDITOR_COLUMN_WIDGET_KEY, wd->key,
				   EDITOR_COLUMN_WIDGET_TYPE, ui_widget_type_to_text(wd->type),
				   EDITOR_COLUMN_WIDGET_DATA, ui_widget_get_data(wd, "data"), -1);
		}
}

static void widget_list_select_cb(GtkTreeSelection *selection, gpointer data)
{
	EditData *ed = data;
	WidgetData *wd;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, EDITOR_COLUMN_WIDGET_POINTER, &wd, -1);
	if (wd->type == dummy_type_id) wd = NULL;

	ui_edit_widget_active_set(ed, wd);
	prop_update(ed, wd, FALSE);
}

static void widget_list_row_move_cb(GtkTreeModel *store, GtkTreePath *path,
				    GtkTreeIter *iter, gpointer data)
{
	EditData *ed = data;
	WidgetData *wd;
	gint row;
	gint old_row;

	/* egads, finding out that dnd changed the row order is a pain! */

	gtk_tree_model_get(store, iter, EDITOR_COLUMN_WIDGET_POINTER, &wd, -1);
	if (!wd) return;

	row = tree_path_to_row(path);
	old_row = g_list_index(ed->ui->skin->widget_list, wd);

	if (row > old_row) row--;

	if (row != old_row)
		{
		ed->ui->skin->widget_list = g_list_remove(ed->ui->skin->widget_list, wd);
		ed->ui->skin->widget_list = g_list_insert(ed->ui->skin->widget_list, wd, row);
		}
}

/*
 *-----------------------------------------------------------------------------
 * active widget stuff
 *-----------------------------------------------------------------------------
 */

void ui_edit_widget_draw_highlight(UIData *ui, WidgetData *wd, gint enable)
{
	gint x, y, w, h;
	gint wx, wy, ww, wh;

	if (!wd ||
	    !wd->od ||
	    !wd->od->is_visible ||
	    !ui_widget_get_geometry(wd, &wx, &wy, &ww, &wh) )
		{
		return;
		}

	x = wx - 2;
	y = wy - 2;
	w = ww + 4;
	h = wh + 4;

	if (x < 0)
		{
		w += x;
		x = 0;
		}
	if (y < 0)
		{
		h += y;
		y = 0;
		}
	if (x + w > ui->skin->width) w = ui->skin->width - x;
	if (y + h > ui->skin->height) h = ui->skin->height - y;
	if (w < 1  || h < 1) return;

	if (enable)
		{
		gint ax, ay, bx, by;

		/* bounding box */
		pixbuf_draw_rect_fill(ui->skin->pixbuf,
				      x, y, w, 2,
				      255, 0, 0, 255);
		pixbuf_draw_rect_fill(ui->skin->pixbuf,
				      x, y + h - 2, w, 2,
				      255, 0, 0, 255);
		pixbuf_draw_rect_fill(ui->skin->pixbuf,
				      x, y, 2, h,
				      255, 0, 0, 255);
		pixbuf_draw_rect_fill(ui->skin->pixbuf,
				      x + w - 2, y, 2, h,
				      255, 0, 0, 255);

		/* anchor indicator */
		ax = (wd->anchor_right) ? (x + w - 6) : (x);
		if (ax + 6 > ui->skin->width) ax = ui->skin->width - 6;
		if (ax < 0) ax = 0;

		ay = (wd->anchor_bottom) ? (y + h - 6) : (y);
		if (ay + 6 > ui->skin->height) ay = ui->skin->height - 6;
		if (ay < 0) ay = 0;

		bx = ax;
		by = ay;

		if (wd->anchor_right) ax += 4;
		if (wd->anchor_bottom) by += 4;

		pixbuf_draw_rect_fill(ui->skin->pixbuf,
				      ax, ay, 2, 6,
				      255, 255, 0, 255);
		pixbuf_draw_rect_fill(ui->skin->pixbuf,
				      bx, by, 6, 2,
				      255, 255, 0, 255);
		}
	else
		{
		pixbuf_copy_area(ui->skin->overlay, x, y,
			 	 ui->skin->pixbuf, x, y, w, h, FALSE);
		ui_display_redraw_area(ui, x, y, w, h, FALSE);
		}
	ui_display_render_area(ui, x, y, w, h, NULL);
}

static void ui_edit_widget_active_set(EditData *ed, WidgetData *wd)
{
	if (ed->active_widget == wd) return;

	if (ed->active_widget)
		{
		ui_edit_widget_draw_highlight(ed->ui, ed->active_widget, FALSE);
		}
	
	ed->active_widget = wd;

	if (ed->active_widget)
		{
		ui_edit_widget_draw_highlight(ed->ui, ed->active_widget, TRUE);
		}

	widget_list_set_widget(ed, ed->active_widget);
	if (!ed->active_widget) prop_update(ed, NULL, FALSE);

}

static void edit_widget_set_coord(EditData *ed, WidgetData *wd, gint x, gint y)
{
	ui_widget_set_coord(ed->ui, wd, x, y, TRUE);

	if (wd == ed->active_widget) prop_update(ed, wd, FALSE);
}

/*
 *-----------------------------------------------------------------------------
 * mouse handling (mirrored from ui2_display)
 *-----------------------------------------------------------------------------
 */

static gint edit_motion(GtkWidget *w, GdkEventMotion *event, gpointer data)
{
	EditData *ed = data;
	gint x = (gint)event->x;
	gint y = (gint)event->y;

	if (ed->ui->active_widget) return FALSE;

	if (ed->in_move && ed->active_widget)
		{
		/* widget drag */
		gint px, py;
		gint wx, wy, ww, wh;

		px = x - ed->press_x;
		py = y - ed->press_y;
		if (ed->active_widget->od && ed->active_widget->od->is_visible &&
		    ui_widget_get_geometry(ed->active_widget, &wx, &wy, &ww, &wh))
			{
			px += ed->press_widget_x;
			py += ed->press_widget_y;
			if (px < 0) px = 0;
			if (py < 0) py = 0;
			if (px > ed->ui->skin->width - ww) px = ed->ui->skin->width - ww;
			if (py > ed->ui->skin->height - wh) py = ed->ui->skin->height - wh;
			if (wx != px || wy != py) edit_widget_set_coord(ed, ed->active_widget, px, py);
			}
		return TRUE;
		}

	if (!ed->click_to_focus) ui_edit_widget_active_set(ed, ui_widget_find_by_coord(ed->ui, x, y));

	return FALSE;
}

static gint edit_pressed(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	EditData *ed = data;
	gint x = (gint)event->x;
	gint y = (gint)event->y;
	WidgetData *old_wd;

	if (ed->ui->active_widget) return FALSE;

	if (debug_mode) printf("pressed:%d x %d\n", x, y);

	if (event->button != 2 && event->button != 3) return FALSE;

	old_wd = ed->active_widget;

	if (ed->click_to_focus)
		{
		if (!(ed->active_widget &&
		      event->button == 2 &&
		      ui_widget_contacts_area(ed->active_widget, event->x, event->y, 1, 1)))
			{
			ui_edit_widget_active_set(ed, ui_widget_find_by_coord(ed->ui, x, y));
			}
		}

	if ((!ed->click_to_focus && ed->active_widget) ||
	    (ed->click_to_focus && ed->active_widget && ed->active_widget == old_wd) )
		{
		gint dmy;
		/* widget movement */
		ed->in_move = TRUE;
		ed->press_x = x;
		ed->press_y = y;
		if (!ui_widget_get_geometry(ed->active_widget, &ed->press_widget_x, &ed->press_widget_y, &dmy, &dmy))
			{
			ed->press_widget_x = ed->press_widget_y = 0;
			}
		ui_edit_widget_draw_highlight(ed->ui, ed->active_widget, FALSE);
		}

	ed->in_press = TRUE;

	return TRUE;
}

static gint edit_released(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	EditData *ed = data;

	if (ed->ui->active_widget) return FALSE;

	if (ed->active_widget)
		{
		ui_edit_widget_draw_highlight(ed->ui, ed->active_widget, TRUE);
		if (debug_mode) printf("edit releasing widget: \"%s\" (%d)\n", ed->active_widget->key, ed->active_widget->type);
		}

	ed->in_press = FALSE;
	ed->in_move = FALSE;

	return TRUE;
}

static void edit_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	EditData *ed = data;

	if (ed->active_widget)
		{
		/* do nothing ? */
		}
}

static void edit_events_init(EditData *ed)
{
	g_signal_connect(G_OBJECT(ed->ui->display),"motion_notify_event",
			 G_CALLBACK(edit_motion), ed);
	g_signal_connect(G_OBJECT(ed->ui->display),"button_press_event",
			 G_CALLBACK(edit_pressed), ed);
	g_signal_connect(G_OBJECT(ed->ui->display),"button_release_event",
			 G_CALLBACK(edit_released), ed);
	g_signal_connect(G_OBJECT(ed->ui->display),"leave_notify_event",
			 G_CALLBACK(edit_leave), ed);

	ed->active_widget = NULL;
	ed->in_press = FALSE;
	ed->in_move = FALSE;
}

/*
 *-----------------------------------------------------------------------------
 * overlay
 *-----------------------------------------------------------------------------
 */

static void edit_overlay_press_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	GList *work;
	SkinData *skin;

	skin = ed->ui->skin;

	if (ed->info_widget) ui_edit_widget_draw_highlight(ed->ui, ed->info_widget, FALSE);

	work = skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;
		gint x, y, w, h;
		if (wd->od && wd->od->is_visible && ui_widget_get_geometry(wd, &x, &y, &w, &h))
			{
			if (wd->in_bounds)
				{
				pixbuf_draw_rect_fill(skin->pixbuf,
						      x, y, w, h,
						      255, 0, 0, 128);
				}
			else
				{
				if (x + w > skin->width) w = skin->width - x;
				if (x > skin->width - 2)
					{
					w = 2;
					x = skin->width - 2;
					}
				if (x < 0)
					{
					w += x;
					x = 0;
					}
				if (y + h > skin->height) h = skin->height - y;
				if (y > skin->height - 2)
					{
					h = 2;
					y = skin->height - 2;
					}
				if (y < 0)
					{
					h+= y;
					y = 0;
					}
				if (w < 2) w = 2;
				if (h < 2) h = 2;
				pixbuf_draw_rect_fill(skin->pixbuf,
						      x, y, w, h,
						      255, 0, 255, 128);
				}
			if (wd->anchor_right) x += w - 2;
			if (wd->anchor_bottom) y += h - 2;
			pixbuf_draw_rect_fill(skin->pixbuf,
					      x, y, 2, 2,
					      255, 255, 0, 255);
			}
		work = work->next;
		}
	ui_display_render_area(ed->ui, 0, 0, skin->width, skin->height, NULL);
}

static void edit_overlay_release_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;

	ui_display_redraw_area(ed->ui, 0, 0, ed->ui->skin->width, ed->ui->skin->height, FALSE);
	if (ed->info_widget) ui_edit_widget_draw_highlight(ed->ui, ed->info_widget, TRUE);
}

/*
 *-----------------------------------------------------------------------------
 * misc. callbacks
 *-----------------------------------------------------------------------------
 */

static void edit_redraw_cb(GtkWidget *button, gpointer data)
{
	EditData *ed = data;
	ui_display_sync_all(ed->ui);
	ui_edit_widget_draw_highlight(ed->ui, ed->active_widget, TRUE);
}

static void edit_load_cb(GtkWidget *button, gpointer data)
{
	EditData *ed = data;
	gchar *skin_path;
	const gchar *skin_key;
	SkinData *skin;
	gchar *buf;

	skin_path = remove_trailing_slash(gtk_entry_get_text(GTK_ENTRY(ed->save_path_entry)));
	skin_key = gtk_entry_get_text(GTK_ENTRY(ed->save_key_entry));

	if (!skin_key || strlen(skin_key) == 0) skin_key = NULL;

	buf = g_strconcat(skin_path, "/", skin_key ? skin_key : "skindata", NULL);
	skin = skin_parse(skin_path, buf, TRUE);
	if (skin) ui_edit_comments_read(ed, buf);
	g_free(buf);

	if (skin)
		{
		ui_skin_set(ed->ui, skin, skin_path, skin_key);

		ed->active_widget = NULL;
		prop_clear(ed);
		widget_list_reset(ed);
		prop_update(ed, NULL, TRUE);
		ui_display_sync_all(ed->ui);

		tab_completion_append_to_history(ed->save_path_entry, skin_path);
		}
	else
		{
		warning_dialog(_("Load error"), _("Failed to load skin from path and key specified."));
		}

	g_free(skin_path);
}

static void edit_click_focus_cb(GtkWidget *button, gpointer data)
{
	EditData *ed = data;
	ed->click_to_focus = GTK_TOGGLE_BUTTON(button)->active;
}

static void edit_delete_cb(GtkWidget *button, gpointer data)
{
	EditData *ed = data;
	GList *work;
	WidgetData *wd;
	WidgetData *new_wd;

	if (!ed->active_widget) return;

	wd = ed->active_widget;

	if (!ui_edit_widget_is_removable(wd))
		{
		warning_dialog(_("Widget still referenced"), _("This widget can not be removed,\nother widgets still reference it."));
		return;
		}

	work = g_list_find(ed->ui->skin->widget_list, wd);
	if (!work) return;

	if (work->next)
		{
		new_wd = work->next->data;
		}
	else if (work->prev)
		{
		new_wd = work->prev->data;
		}
	else
		{
		new_wd = NULL;
		}

	ui_edit_widget_active_set(ed, new_wd);
	prop_update(ed, new_wd, TRUE);

	ui_unregister_key_private_for_widget(ed->ui, wd->widget);

	ed->ui->skin->widget_list = g_list_remove(ed->ui->skin->widget_list, wd);
	if (ed->ui->focus_widget == wd) ed->ui->focus_widget = NULL;

	ui_edit_widget_draw_highlight(ed->ui, wd, FALSE);

	widget_list_reset(ed);
	widget_list_set_widget(ed, new_wd);

	ui_widget_free(wd);
}

static void edit_save_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	gchar *skin_path;
	const gchar *skin_key;

	skin_path = remove_trailing_slash(gtk_entry_get_text(GTK_ENTRY(ed->save_path_entry)));
	skin_key = gtk_entry_get_text(GTK_ENTRY(ed->save_key_entry));

	if (!skin_key || strlen(skin_key) == 0) skin_key = NULL;

	if (ui_edit_save(ed, skin_path, skin_key))
		{
		gchar *buf;

		tab_completion_append_to_history(ed->save_path_entry, skin_path);

		buf = g_strdup_printf(_("Skin successfully saved to \"%s\"\nwith data file \"%s\".\n"
				        "Build %d\nUse the comment button to edit release information."),
				      skin_path, skin_key ? skin_key : "skindata", ed->comment_build);
		warning_dialog(_("Skin saved"), buf);
		g_free(buf);
		}
	else
		{
		gchar *buf;

		buf = g_strdup_printf(_("Failed to save skin to \"%s\"\nwith data file \"%s\"."),
				      skin_path, skin_key ? skin_key : "skindata");
		warning_dialog(_("Skin save failed"), buf);
		g_free(buf);
		}

	g_free(skin_path);
}

/*
 *-----------------------------------------------------------------------------
 * copy data from the 'parent'
 *-----------------------------------------------------------------------------
 */

static void ui_edit_copy_registry(UIData *ui, UIData *source)
{
	GList *work;

	work = source->register_list;
	while(work)
		{
		RegisterData *srd = work->data;
		RegisterData *rd;

		if (srd->private == FALSE)
			{
			rd = g_new(RegisterData, 1);
			rd->type = srd->type;
			rd->key = g_strdup(srd->key);
			rd->private = FALSE;
			if (srd->callbacks_l == 0 ||
			    (rd->type == button_type_id() &&
				(strcmp(srd->key, "exit") == 0 ||	/* disable keys that should 	*/
				strcmp(srd->key, "iconify") == 0 ||	/* not work in the editor	*/
				strcmp(srd->key, "skin_toggle") == 0 ||
				strcmp(srd->key, "skin_open") == 0 ||
				strcmp(srd->key, "skin_close") == 0 ||
				strcmp(srd->key, "skin_iconify") == 0 ||
				strcmp(srd->key, "skin_sticky") == 0) ) )
				{
				rd->callbacks = NULL;
				rd->callbacks_l = 0;
				}
			else
				{
				rd->callbacks = g_memdup(srd->callbacks, srd->callbacks_l);
				rd->callbacks_l = srd->callbacks_l;
				}
			if (debug_mode) printf("copying callbackdata for \"%s\", size = %d\n", rd->key, rd->callbacks_l);

			ui->register_list = g_list_prepend(ui->register_list, rd);
			}

		work = work->next;
		}

	ui->register_list = g_list_reverse(ui->register_list);
}

static SkinData *ui_edit_copy_skin(UIData *source, EditData *ed)
{
	/* I'm an idiot, I thought this was gonna be hard, but knowing the skin_path,
	 * just load it instead of painfully trying to go through skin->widget_list
	 * and copying everything.
	 */
	SkinData *skin;

	if (source->skin_path)
		{
		gchar *datafile;

		datafile = g_strconcat(source->skin_path, "/", source->skin_mode_key ? source->skin_mode_key : "skindata", NULL);
		skin = skin_parse(source->skin_path, datafile, TRUE);
		if (skin) ui_edit_comments_read(ed, datafile);
		g_free(datafile);
		}
	else
		{
		skin = ui_skin_load_default(source, source->skin_mode_key);
		ui_edit_comments_read(ed, NULL);
		}

	if (!skin)
		{
		skin = skin_new();
		ui_edit_comments_read(ed, NULL);
		}

	return skin;
}

static void ui_edit_copy_ui(UIData *ui, UIData *source, UIData *skin_source, EditData *ed)
{
	if (!ui || !source || ui == source) return;

	ui_register_free_all(ui);
	ui_edit_copy_registry(ui, source);

	ui_skin_set(ui, ui_edit_copy_skin(skin_source, ed),
		    skin_source->skin_path, skin_source->skin_mode_key);
}

/*
 *-----------------------------------------------------------------------------
 * 'help' window
 *-----------------------------------------------------------------------------
 */

static void help_window_scroll(EditData *ed, WidgetType type)
{
	const gchar *key;

	if (!ed->help_window ||
	    !ed->help_follow ||
	    !GTK_WIDGET_VISIBLE(ed->help_window)) return;

	key = ui_widget_type_to_text(type);

	if (type == -2) key = "main";

	if (!key) return;

	help_window_set_key(ed->help_window, key);
}

static void help_window_follow_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;

	ed->help_follow = ui_edit_toggle_get(widget);
	if (ed->help_follow)
		{
		help_window_scroll(ed, ed->info_widget ? ed->info_widget->type : -1);
		}
}

static void help_window_destroy_cb(GtkWidget *window, gpointer data)
{
	EditData *ed = data;

	ed->help_window = NULL;
}

static void help_window_setup(EditData *ed, const gchar *path)
{
	GtkWidget *vbox;
	GtkWidget *button;

	if (ed->help_window) return;

	ed->help_window = help_window_new(_("Help - SLIK"), "SLIK", "editor_help",
					  path, NULL);
	g_signal_connect(G_OBJECT(ed->help_window), "destroy",
			 G_CALLBACK(help_window_destroy_cb), ed);

	/* extra */
	vbox = help_window_get_box(ed->help_window);

	button = ui_edit_toggle_new(vbox, _("Scroll to follow active widget"));
	ed->help_follow = FALSE;
	ui_edit_toggle_set(button, ed->help_follow);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(help_window_follow_cb), ed);

	window_set_icon(ed->help_window, NULL, NULL, NULL);
}

static void help_window_show_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;

	if (ed->help_window)
		{
		gdk_window_raise(ed->help_window->window);
		}
	else
		{
		help_window_setup(ed, ed->help_path);
		}
}

/*
 *-----------------------------------------------------------------------------
 * 'comment' window
 *-----------------------------------------------------------------------------
 */

static void comment_window_push_build(EditData *ed)
{
	if (!ed->comment_window) return;

	ui_edit_spin_set(ed->comment_build_spin, ed->comment_build);
}

static void comment_window_push(EditData *ed)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	
	if (!ed->comment_window) return;

	ui_edit_entry_set(ed->comment_title_entry, ed->comment_title);
	ui_edit_entry_set(ed->comment_version_entry, ed->comment_version);
	ui_edit_entry_set(ed->comment_author_entry, ed->comment_author);
	ui_edit_entry_set(ed->comment_email_entry, ed->comment_email);
	ui_edit_entry_set(ed->comment_url_entry, ed->comment_url);
	ui_edit_entry_set(ed->comment_license_entry, ed->comment_license);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ed->comment_misc_entry));
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	gtk_text_buffer_delete(buffer, &start, &end);
	if (ed->comment_misc)
		{
		if (!g_utf8_validate(ed->comment_misc, strlen(ed->comment_misc), NULL))
			{
			gchar *buf;

			buf = g_locale_to_utf8(ed->comment_misc, strlen(ed->comment_misc), NULL, NULL, NULL);
			gtk_text_buffer_set_text(buffer, buf, strlen(buf));
			g_free(buf);
			}
		else
			{
			gtk_text_buffer_set_text(buffer, ed->comment_misc, strlen(ed->comment_misc));
			}
		}

	ui_edit_entry_set(ed->comment_stamp_label, ed->comment_stamp);
	ui_edit_spin_set(ed->comment_build_spin, ed->comment_build);
}

static void comment_window_pull(EditData *ed)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;

	if (!ed->comment_window) return;

	g_free(ed->comment_title);
	g_free(ed->comment_version);
	g_free(ed->comment_author);
	g_free(ed->comment_email);
	g_free(ed->comment_url);
	g_free(ed->comment_license);
	g_free(ed->comment_misc);

	ed->comment_title = g_strdup(ui_edit_entry_get(ed->comment_title_entry));
	ed->comment_version = g_strdup(ui_edit_entry_get(ed->comment_version_entry));
	ed->comment_author = g_strdup(ui_edit_entry_get(ed->comment_author_entry));
	ed->comment_email = g_strdup(ui_edit_entry_get(ed->comment_email_entry));
	ed->comment_url = g_strdup(ui_edit_entry_get(ed->comment_url_entry));
	ed->comment_license = g_strdup(ui_edit_entry_get(ed->comment_license_entry));

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ed->comment_misc_entry));
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	ed->comment_misc = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	if (ed->comment_misc && strlen(ed->comment_misc) == 0)
		{
		g_free(ed->comment_misc);
		ed->comment_misc = NULL;
		}

	ed->comment_build = ui_edit_spin_get(ed->comment_build_spin);
}

static void comment_build_reset_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;

	ed->comment_build = 0;
	comment_window_push_build(ed);
}

static gint comment_window_delete_cb(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	EditData *ed = data;

	if (GTK_WIDGET_VISIBLE(ed->comment_window)) gtk_widget_hide(ed->comment_window);

	return TRUE;
}

static void comment_window_show(EditData *ed)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *scrolled;

	if (ed->comment_window)
		{
		if (GTK_WIDGET_VISIBLE(ed->comment_window))
			{
			gdk_window_raise(ed->comment_window->window);
			}
		else
			{
			gtk_widget_show(ed->comment_window);
			}
		return;
		}

	ed->comment_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(ed->comment_window), HELP_WINDOW_WIDTH / 2, -1);
	gtk_window_set_resizable(GTK_WINDOW(ed->comment_window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(ed->comment_window), 5);
	gtk_window_set_wmclass(GTK_WINDOW(ed->comment_window), "editor_comments", "SLIK");

	gtk_window_set_title(GTK_WINDOW(ed->comment_window), _("Edit comments - SLIK"));

	g_signal_connect(G_OBJECT(ed->comment_window), "delete_event",
			 G_CALLBACK(comment_window_delete_cb), ed);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(ed->comment_window), vbox);
	gtk_widget_show(vbox);

	ed->comment_title_entry = ui_edit_entry_new(vbox, _("Title:"));
	ed->comment_version_entry = ui_edit_entry_new(vbox, _("Version:"));
	ed->comment_author_entry = ui_edit_entry_new(vbox, _("Author:"));
	ed->comment_email_entry = ui_edit_entry_new(vbox, _("Email:"));
	ed->comment_url_entry = ui_edit_entry_new(vbox, _("URL:"));
	ed->comment_license_entry = ui_edit_entry_new(vbox, _("License:"));

	ed->comment_stamp_label = ui_edit_entry_new(vbox, _("Time stamp:"));
	gtk_widget_set_sensitive(ed->comment_stamp_label, FALSE);

	hbox = ui_edit_frame_new(vbox, FALSE, NULL);
	ed->comment_build_spin = ui_edit_spin_new(hbox, _("Build:"), 0, 100000, NULL);
	button = gtk_button_new_with_label(_("Reset"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(comment_build_reset_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	hbox = ui_edit_frame_new(vbox, TRUE, _("Comments"));

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 0);
	gtk_widget_show(scrolled);

	ed->comment_misc_entry = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(ed->comment_misc_entry), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled), ed->comment_misc_entry);
	gtk_widget_show(ed->comment_misc_entry);

	comment_window_push(ed);

	gtk_widget_show(ed->comment_window);
}

static void edit_comments_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;

	comment_window_show(ed);
}

/*
 *-----------------------------------------------------------------------------
 * 'add' window
 *-----------------------------------------------------------------------------
 */

static void widget_window_add_skin(EditData *ed, const gchar *skin_dir, GtkWidget *entry)
{
	GList *list;
	GList *work;

	if (!path_list(skin_dir, &list, NULL)) return;

	work = list;
	while (work)
		{
		gchar *path;
		const gchar *name;

		path = work->data;
		work = work->next;

		name = filename_from_path(path);

		if (strncmp(name, "skindata", 8) == 0 &&
		    strstr(name, ".bak") == NULL) /* ignore backup files */
			{
			if (entry) gtk_entry_set_text(GTK_ENTRY(entry), path);
			while(g_main_context_iteration(NULL, FALSE));
			ui_edit_import_skin_keys(ed, path);
			}
		}

	path_list_free(list);
}

#if 0
static void widget_window_add_dir(EditData *ed, const gchar *path, GtkWidget *entry)
{
	GList *dir_list = NULL;
	GList *work;

	if (!path_list(path, NULL, &dir_list)) return;

	work = dir_list;
	while (work)
		{
		gchar *skin_dir = work->data;
		widget_window_add_skin(ed, skin_dir, entry);
		work = work->next;
		}

	path_list_free(dir_list);
}
#endif

static void widget_window_extract_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *progress;
	GtkWidget *entry;
	GList *complete_list;
	GList *work;
	gint l;
	gint c;

	gtk_widget_set_sensitive(widget, FALSE);

	if (editor_resource_list == NULL)
		{
		printf("editor resource list is empty\n");
		return;
		}

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_default_size(GTK_WINDOW(window), HELP_WINDOW_WIDTH, -1);
	gtk_window_set_title(GTK_WINDOW(window), _("Reading skins - SLIK"));

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show(vbox);

	ui_edit_label_new(vbox, _("Extracting widgets, please wait..."));

	entry = gtk_entry_new();
	gtk_widget_set_sensitive(entry, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show(entry);

	progress = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), progress, FALSE, FALSE, 0);
	gtk_widget_show(progress);

	window_set_icon(window, NULL, NULL, NULL);

	gtk_widget_show(window);

	complete_list = NULL;
	work = editor_resource_list;
	while (work)
		{
		GList *tmp = NULL;
		gchar *path = work->data;

		path_list(path, NULL, &tmp);
		complete_list = g_list_concat(complete_list, tmp);

		work = work->next;
		}

	l = g_list_length(complete_list);
	c = 0;
	work = complete_list;
	while (work)
		{
		gchar *path = work->data;
		work = work->next;

		widget_window_add_skin(ed, path, entry);
		c++;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), (gfloat)c / l);
		}

	path_list_free(complete_list);

	gtk_widget_destroy(window);
}

static void widget_window_import_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	gchar *path;

	path = remove_trailing_slash(ui_edit_entry_get(ed->import_path_entry));

	if (isdir(path))
		{
		widget_window_add_skin(ed, path, NULL);
		tab_completion_append_to_history(ed->import_path_entry, path);
		}
	else if (ui_edit_import_skin_keys(ed, path))
		{
		tab_completion_append_to_history(ed->import_path_entry, path);
		}

	g_free(path);
}

static void widget_window_setup_notebook(EditData *ed)
{
	GtkWidget *page;
	GtkWidget *label;
	GList *work;

	page = edit_main_page_new(ed);
	label = gtk_label_new(_("background"));
	gtk_notebook_append_page(GTK_NOTEBOOK(ed->notebook), page, label);

	page = edit_focus_page_new(ed);
	label = gtk_label_new(_("focus"));
	gtk_notebook_append_page(GTK_NOTEBOOK(ed->notebook), page, label);

	work = ed->widget_id_list;
	while(work)
		{
		WidgetObjectData *od;
		EditTypeData *td;

		td = work->data;
		work = work->next;

		od = ui_widget_object_by_type(td->type);
		if (od && od->func_edit_page_new)
			{
			td->page = od->func_edit_page_new(ed);
			label = gtk_label_new(ui_widget_type_to_text(td->type));
			gtk_notebook_append_page(GTK_NOTEBOOK(ed->notebook), td->page, label);
			}
		}
}

static gint widget_window_delete_cb(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	EditData *ed = data;
	if (GTK_WIDGET_VISIBLE(ed->widget_window))
		{
		gtk_widget_hide(ed->widget_window);
		}

	return TRUE;
}

static void widget_window_show_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;

	if (GTK_WIDGET_VISIBLE(ed->widget_window))
		{
		gdk_window_raise(ed->widget_window->window);
		}
	else
		{
		gtk_widget_show(ed->widget_window);
		}
}

static void widget_window_setup(EditData *ed)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *combo;
	GtkWidget *sep;

	if (ed->widget_window) return;

	/* window */

	ed->widget_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(ed->widget_window), TRUE);
	gtk_container_set_border_width (GTK_CONTAINER(ed->widget_window), 5);
	gtk_window_set_wmclass(GTK_WINDOW(ed->widget_window), "editor_add_widget", "SLIK");

	gtk_window_set_title(GTK_WINDOW(ed->widget_window), _("Add widget - SLIK"));

	g_signal_connect(G_OBJECT(ed->widget_window), "delete_event",
			 G_CALLBACK(widget_window_delete_cb), ed);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(ed->widget_window), vbox);
	gtk_widget_show(vbox);

	/* new widget notebook */

	ed->notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), ed->notebook, TRUE, TRUE, 0);
	gtk_widget_show(ed->notebook);
	widget_window_setup_notebook(ed);

	hbox = ui_edit_frame_new(vbox, FALSE, NULL);

	combo = tab_completion_new_with_history(&ed->import_path_entry, NULL, "SLIK_import", 16, NULL, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);

	button = gtk_button_new_with_label(_("import"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(widget_window_import_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	sep = gtk_vseparator_new();
	gtk_box_pack_start(GTK_BOX(hbox), sep, FALSE, FALSE, 0);
	gtk_widget_show(sep);

	button = gtk_button_new_with_label(_("generate list from installed skins"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(widget_window_extract_cb), ed);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	window_set_icon(ed->widget_window, NULL, NULL, NULL);
}

/*
 *-----------------------------------------------------------------------------
 * dummy widget
 *-----------------------------------------------------------------------------
 */

static void editor_dummy_back_set(gpointer widget, GdkPixbuf *pb)
{
	EditData *ed = widget;

	/* this is used to update widget properties 
	 * (size may have changed on dynamic widgets,
	 *  and this signal is definitely after any size updates)
	 */

	prop_update(ed, ed->info_widget, FALSE);
}

/* we actually need the editor to register a unique type (for the dummy widget) */
static void editor_type_init(void)
{
	WidgetObjectData *od;

	if (dummy_type_id != -1) return;

	od = ui_widget_type_new("window");
        dummy_type_id = od->type;

	od->func_back = editor_dummy_back_set;
}

/*
 *-----------------------------------------------------------------------------
 * edit window new / close
 *-----------------------------------------------------------------------------
 */

void ui_edit_set_skindata_list(EditData *ed, GList *list)
{
	GtkWidget *combo;

	if (!ed->save_key_entry) return;

	combo = ed->save_key_entry->parent;

	if (GTK_IS_COMBO(combo))
		{
		gchar *text;

		text = g_strdup(gtk_entry_get_text(GTK_ENTRY(ed->save_key_entry)));
		gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
		gtk_entry_set_text(GTK_ENTRY(ed->save_key_entry), text);
		g_free(text);
		}
}

void ui_edit_set_help_path(EditData *ed, const gchar *path)
{
	g_free(ed->help_path);
	ed->help_path = g_strdup(path);
}

static void ui_edit_parent_destroy_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;
	ui_edit_close(ed);
}

static void ui_edit_destroy_cb(GtkWidget *widget, gpointer data)
{
	EditData *ed = data;

	if (ed->widget_window) gtk_widget_destroy(ed->widget_window);
	if (ed->help_window) gtk_widget_destroy(ed->help_window);
	if (ed->comment_window) gtk_widget_destroy(ed->comment_window);

	ui_edit_widget_id_list_free(ed->widget_id_list);
	edit_main_list_free(ed->main_list);
	edit_focus_list_free(ed->focus_list);
	edit_main_page_free(ed->main_page);

	if (ed->parent) ed->parent->edit = NULL;

	ed->ui->destroyed = TRUE;
	ui_free(ed->ui);

	g_free(ed->detail_data);
	g_free(ed->help_path);

	ui_edit_comments_reset(ed);

	g_free(ed);
}

void ui_edit_close(EditData *ed)
{
	if (!ed) return;

	gtk_widget_destroy(ed->window);
}

static gint ui_edit_delete_cb(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	EditData *ed = data;
	ui_edit_close(ed);

	return TRUE;
}

EditData *ui_edit_new(UIData *parent, const gchar *class, const gchar *subclass, const gchar *title)
{
	EditData *ed;
	UIData *skin_source;
	GtkWidget *main_vbox;
	GtkWidget *sep;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkObject *adj;
	GtkWidget *button;
	GtkWidget *combo;
	GtkWidget *hpane;
	GtkWidget *vpane;
	GtkWidget *scrolled;
	GList *list;
	GtkListStore *store;

	if (slik_transparency_force || slik_colorshift_on || slik_double_size)
		{
		warning_dialog(_("Editor warning"), _("The skin editor requires that the transparency, color tint,\nand double size options be off."));
		return NULL;
		}

	if (!parent) return NULL;

	skin_source = parent;
	if (parent->parent) parent = parent->parent;

	if (parent->edit) return NULL;

	ed = g_new0(EditData, 1);

	ed->click_to_focus = TRUE;

	ed->active_widget = NULL;
	ed->info_widget = NULL;

	ed->details = NULL;
	ed->detail_data = NULL;

	ed->widget_id_list = ui_edit_widget_id_list_new();
	ed->main_list = NULL;

	ed->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(ed->window), TRUE);
	gtk_container_set_border_width (GTK_CONTAINER(ed->window), 5);
	gtk_window_set_wmclass(GTK_WINDOW(ed->window), subclass, class);

	if (title)
		{
		gtk_window_set_title(GTK_WINDOW(ed->window), title);
		}
	else
		{
		gtk_window_set_title(GTK_WINDOW(ed->window), _("Skin editor - SLIK"));
		}

	g_signal_connect(G_OBJECT(ed->window), "delete_event",
			 G_CALLBACK(ui_edit_delete_cb), ed);
	g_signal_connect(G_OBJECT(ed->window), "destroy",
			 G_CALLBACK(ui_edit_destroy_cb), ed);

	main_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(ed->window), main_vbox);
	gtk_widget_show(main_vbox);

	hpane = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), hpane, TRUE, TRUE, 0);
	gtk_widget_show(hpane);

	vpane = gtk_vpaned_new();
	gtk_paned_pack1(GTK_PANED(hpane), vpane, TRUE, TRUE);
	gtk_widget_show(vpane);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack1(GTK_PANED(vpane), scrolled, TRUE, TRUE);
	gtk_widget_show(scrolled);

	ed->table = gtk_table_new(1, 1, TRUE);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), ed->table);
	gtk_widget_show(ed->table);

	/* utils */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2(GTK_PANED(vpane), vbox, TRUE, TRUE);
	gtk_widget_show(vbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
	gtk_widget_show(hbox);

	button = gtk_button_new_with_label(_("help"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(help_window_show_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("overlay"));
	g_signal_connect(G_OBJECT(button), "pressed",
			 G_CALLBACK(edit_overlay_press_cb), ed);
	g_signal_connect(G_OBJECT(button), "released",
			 G_CALLBACK(edit_overlay_release_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("redraw"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(edit_redraw_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = gtk_check_button_new_with_label(_("click to focus"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), ed->click_to_focus);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(edit_click_focus_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("comments"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(edit_comments_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("add..."));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(widget_window_show_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("remove"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(edit_delete_cb), ed);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	/* widget list */

	store = gtk_list_store_new(4, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	ed->widget_list = ui_edit_list_new(vbox, store,
					   G_CALLBACK(widget_list_select_cb), ed);
	g_object_unref(store);

	ui_edit_list_add_column(ed->widget_list, _("Key"), EDITOR_COLUMN_WIDGET_KEY, FALSE, FALSE);
	ui_edit_list_add_column(ed->widget_list, _("Type"), EDITOR_COLUMN_WIDGET_TYPE, FALSE, FALSE);
	ui_edit_list_add_column(ed->widget_list, _("Data"), EDITOR_COLUMN_WIDGET_DATA, FALSE, FALSE);

	g_signal_connect(G_OBJECT(store), "row_changed",
			 G_CALLBACK(widget_list_row_move_cb), ed);

	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(ed->widget_list), TRUE);

	/* set up edit ui */

	ed->ui = g_new0(UIData, 1);

	ed->ui->decorations = FALSE;
	ed->ui->root_win_idle = -1;
	ed->ui->edit = NULL;
	ed->ui->skin = NULL;
	ed->ui->allow_move = FALSE;

	ed->ui->window = NULL;

	ed->ui->display = gtk_drawing_area_new();
	ui_display_events_init(ed->ui);
	gtk_table_attach(GTK_TABLE(ed->table), ed->ui->display, 0, 1, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
	gtk_widget_show(ed->ui->display);

	edit_events_init(ed);

        gtk_widget_realize(ed->ui->display);

	parent->edit = ed->ui;
	ed->parent = parent;

	g_signal_connect(G_OBJECT(parent->window), "destroy",
			 G_CALLBACK(ui_edit_parent_destroy_cb), ed);

	/* copy ui data from parent */

	ui_edit_copy_ui(ed->ui, parent, skin_source, ed);

	/* register a dummy widget so that the editor info pane also updates on a resize */
	editor_type_init();
	skin_register_widget(ed->ui->skin, "background", NULL, dummy_type_id, ed);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack2(GTK_PANED(hpane), vbox, FALSE, FALSE);
	gtk_widget_show(vbox);

	/* widget property area */

	hbox = ui_edit_frame_new(vbox, FALSE, NULL);

	ed->description = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), ed->description, TRUE, TRUE, 0);
	gtk_widget_show(ed->description);

	ed->spin_x = ui_edit_spin_new(hbox, _("x:"), 0, 0, &adj);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(prop_spin_x_cb), ed);

	ed->spin_y = ui_edit_spin_new(hbox, _("y:"), 0, 0, &adj);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(prop_spin_y_cb), ed);

	hbox = ui_edit_frame_new(vbox, FALSE, NULL);
	ui_edit_label_new(hbox, _("Key:"));

	ed->key_combo_entry = gtk_combo_new();
	g_signal_connect(G_OBJECT(GTK_COMBO(ed->key_combo_entry)->entry), "changed",
			 G_CALLBACK(prop_key_entry_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), ed->key_combo_entry, TRUE, TRUE, 0);
	gtk_widget_show(ed->key_combo_entry);

	ed->data_entry = ui_edit_entry_new(vbox, _("Data:"));
	g_signal_connect(G_OBJECT(ed->data_entry), "changed",
			 G_CALLBACK(prop_data_entry_cb), ed);

	hbox = ui_edit_frame_new(vbox, FALSE, NULL);

	ed->text_id_entry = ui_edit_entry_new(vbox, _("Text id:"));
	g_signal_connect(G_OBJECT(ed->text_id_entry), "changed",
			 G_CALLBACK(prop_text_id_entry_cb), ed);

	ed->detail_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), ed->detail_vbox, FALSE, FALSE, 0);
	gtk_widget_show(ed->detail_vbox);

	ed->widget_image = ui_edit_image_new(vbox);

	ed->image_entry = ui_edit_entry_new(vbox, _("Image:"));
	ui_edit_frame_sensitive(ed->image_entry, FALSE, TRUE);

	ed->clip_mask_entry = ui_edit_entry_new(vbox, _("Clip mask:"));
	ui_edit_frame_sensitive(ed->clip_mask_entry, FALSE, TRUE);

	hbox = ui_edit_frame_new(vbox, TRUE, _("Coordinate anchor"));

	ed->anchor_right_button = ui_edit_toggle_new(hbox, _("Right"));
	g_signal_connect(G_OBJECT(ed->anchor_right_button), "clicked",
			 G_CALLBACK(prop_anchor_right_cb), ed);

	ed->anchor_bottom_button = ui_edit_toggle_new(hbox, _("Bottom"));
	g_signal_connect(G_OBJECT(ed->anchor_bottom_button), "clicked",
			 G_CALLBACK(prop_anchor_bottom_cb), ed);

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), sep, FALSE, FALSE, 0);
	gtk_widget_show(sep);

	/* path, save */

	hbox = ui_edit_frame_new(main_vbox, FALSE, NULL);

	button = gtk_button_new_with_label(_("Load"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(edit_load_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	label = gtk_label_new(_("Skin path:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	combo = tab_completion_new_with_history(&ed->save_path_entry, ed->ui->skin_path,
						"SLIK_save", 16, NULL, NULL);
	if (!ed->ui->skin_path && editor_resource_list)
		{
		gtk_entry_set_text(GTK_ENTRY(ed->save_path_entry), (gchar *)editor_resource_list->data);
		}
	gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);

	label = gtk_label_new(_("Data filename:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	combo = gtk_combo_new();
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
	gtk_widget_show(combo);
	ed->save_key_entry = GTK_COMBO(combo)->entry;

	/* example skindata keys */
	list = g_list_append(NULL, "skindata");
	list = g_list_append(list, "skindata_alt");
	list = g_list_append(list, "skindata_1");
	list = g_list_append(list, "skindata_2");
	list = g_list_append(list, "skindata_3");

	gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
	g_list_free(list);

	gtk_entry_set_text(GTK_ENTRY(ed->save_key_entry), ed->ui->skin_mode_key ? ed->ui->skin_mode_key : "skindata");
	
	button = gtk_button_new_with_label(_("Save"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(edit_save_cb), ed);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	/* finish up */
	widget_window_setup(ed);
	ed->help_window = NULL;

	widget_list_reset(ed);
	prop_update(ed, NULL, TRUE);

	window_set_icon(ed->window, NULL, NULL, NULL);
	gtk_widget_show(ed->window);

	return ed;
}

/*
 *-----------------------------------------------------------------------------
 * distort fun
 *-----------------------------------------------------------------------------
 */

static gint distort_cb(gpointer data)
{
	UIData *ui = data;
	GList *work;

	if (!ui->skin) return FALSE;

	work = g_list_nth(ui->skin->widget_list, (float)rand() / (float)RAND_MAX * (float)g_list_length(ui->skin->widget_list));

	if (work)
		{
		gint x, y, w, h;
		gint ox, oy;

		WidgetData *wd = work->data;

		if (!wd->od || !wd->od->is_visible) return TRUE;

		if (!ui_widget_get_geometry(wd, &x, &y, &w, &h)) return TRUE;

		ox = x;
		oy = y;

		x += (rand() > RAND_MAX / 2 ? 1 : -1);
		y += (rand() > RAND_MAX / 2 ? 1 : -1);

		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x + w > ui->skin->width) x = ui->skin->width - w;
		if (y + h > ui->skin->height) y = ui->skin->height - h;

		ui_widget_set_coord(ui, wd, x, y, TRUE);

		pixbuf_copy_area(ui->skin->overlay, ox, oy,
				 ui->skin->pixbuf, ox, oy, w, h, FALSE);
		ui_widget_sync_back(ui, wd);

		ui_display_redraw_area(ui, ox, oy, w, h, FALSE);
		ui_widget_draw(ui, wd, FALSE, TRUE);
		ui_display_render_area(ui, ox, oy, w, h, NULL);
		}

	return TRUE;
}

void ui_distort(UIData *ui)
{
	g_timeout_add_full(G_PRIORITY_LOW, 5, distort_cb, ui, NULL);
}

/*
 *-----------------------------------------------------------------------------
 * register system and user skin directories here (for use by editor)
 *-----------------------------------------------------------------------------
 */

void ui_edit_add_skin_resource(const gchar *path)
{
	GList *work;
	if (!path) return;
	work = editor_resource_list;
	while(work)
		{
		gchar *t = work->data;
		work = work->next;
		if (strcmp(t, path) == 0) return;
		}
	editor_resource_list = g_list_append(editor_resource_list, g_strdup(path));
}
