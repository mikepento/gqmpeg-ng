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

#include "ui2_includes.h"
#include "ui2_typedefs.h"

#include "ui2_list.h"
#include "ui2_list_edit.h"

#include "ui2_editor.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "ui_pixbuf_ops.h"
#include "ui_tabcomp.h"
#include "ui_utildlg.h"
#include "ui_tree_edit.h"

#include "ui2_button.h"
#include "ui2_slider.h"


enum {
	LIST_COLUMN_POINTER = 0,
	LIST_COLUMN_IMAGE,
	LIST_COLUMN_KEY,
	LIST_COLUMN_FLAG,
	LIST_COLUMN_COUNT
};

enum {
	COL_COLUMN_NUMBER = 0,
	COL_COLUMN_KEY,
	COL_COLUMN_WIDTH,
	COL_COLUMN_DYNAMIC,
	COL_COLUMN_JUSTIFY,
	COL_COLUMN_COUNT
};


typedef struct _ListDetail ListDetail;
struct _ListDetail
{
	ListData *ld;

	GtkWidget *image_entry;

	GtkWidget *border_left_spin;
	GtkWidget *border_right_spin;
	GtkWidget *border_top_spin;
	GtkWidget *border_bottom_spin;
	GtkWidget *border_stretch_button;
	GtkWidget *stretch_button;
	GtkWidget *sizeable_button;

	GtkWidget *row_image_entry;
	GtkWidget *row_border_left_spin;
	GtkWidget *row_border_right_spin;
	GtkWidget *row_stretch_button;
	GtkWidget *row_has_press_button;
	GtkWidget *row_has_prelight_button;

	GtkWidget *divider_image_entry;

	GtkWidget *text_image_entry;
	GtkWidget *text_extended_button;

	GtkWidget *text_font_entry;
	GtkWidget *text_font_r;
	GtkWidget *text_font_g;
	GtkWidget *text_font_b;
	GtkWidget *text_font_a;

	GtkWidget *flag_image_entry;
	GtkWidget *flag_sections_spin;
	GtkWidget *flag_column_spin;

	GtkWidget *width_spin;
	GtkWidget *height_spin;

	GtkWidget *column_count_spin;
	GtkWidget *column_list;
	GtkWidget *columns_right_toggle;

	WidgetData *wd;
	UIData *ui;
};

/* the name: yea, well I 'm keeping with the other
 * widget edit files namespace scheme :) */
typedef struct _ListListData ListListData;
struct _ListListData
{
	gchar *key;
	gchar *text_id;
	gchar *data;

	gchar *image;
	gint border_left;
	gint border_right;
	gint border_top;
	gint border_bottom;
	gint border_stretch;
	gint stretch;

	gchar *row_image;
	gint row_border_left;
	gint row_border_right;
	gint row_stretch;
	gint row_has_press;
	gint row_has_prelight;

	gchar *divider_image;

	gchar *text_image;
	gint text_extended;

	gchar *text_font;
	gint text_r;
	gint text_g;
	gint text_b;
	gint text_a;

	gchar *flag_image;
	gint flag_sections;
	gint flag_column;

	gint width;
	gint height;
	gint sizeable;

	gint column_count;
	gint *column_widths;		/* array of column widths */
	ListColumnFlags *column_flags;	/* column attributes (justify, etc. */
	gchar **column_keys;		/* column text keys */

	gint columns_right_justify;
};

typedef struct _ListPageColumn ListPageColumn;
struct _ListPageColumn
{
	gchar *key;
	gint width;
	ListColumnFlags flags;
};

typedef struct _ListPage ListPage;
struct _ListPage
{
	ListDetail ld;			/* no point in duplicating effort, the signals will be
					 * completely different though */

	GtkWidget *key_entry;
	GtkWidget *text_id_entry;
	GtkWidget *data_entry;

	GtkWidget *list;

	GList *column_list;		/* list of column attributes ListPageColumn pointers */

	EditData *ed;
};

static GdkPixbuf *list_get_pixbuf(gpointer widget)
{
	ListData *ld = widget;

	return ld->overlay;
}

static void list_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	ListData *ld = wd->widget;
	gchar *image;
	gchar *row_image;
	gchar *text_image;
	gchar *divider_image;
	gchar *flag_image;
	gint i;

	image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "image"),
					 dir, ld->overlay, "list_", wd->key);
	row_image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "row_image"),
					     dir, ld->row_overlay, "list_row_", wd->key);
	if (ld->font->overlay)
		{
		text_image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "text_image"),
						     dir, ld->font->overlay, "list_text_", wd->key);
		}
	else
		{
		text_image = NULL;
		}

	if (ld->divider_overlay)
		{
		divider_image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "divider_image"),
							 dir, ld->divider_overlay, "list_divider_", wd->key);
		}
	else
		{
		divider_image = NULL;
		}

	if (ld->flag_overlay)
		{
		flag_image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "flag_image"),
						      dir, ld->flag_overlay, "list_flag_", wd->key);
		}
	else
		{
		flag_image = NULL;
		}


	if (image) ui_edit_widget_set_path_key(wd, "image", dir, image);
	if (row_image) ui_edit_widget_set_path_key(wd, "row_image", dir, row_image);
	if (text_image) ui_edit_widget_set_path_key(wd, "text_image", dir, text_image);
	if (divider_image) ui_edit_widget_set_path_key(wd, "divider_image", dir, divider_image);
	if (flag_image) ui_edit_widget_set_path_key(wd, "flag_image", dir, flag_image);

	ui_edit_write_section(f, "list", wd->key);

	ui_edit_write_key_char(f, "image", image);

	ui_edit_write_key_int(f, "x", ld->x);
	ui_edit_write_key_int(f, "y", ld->y);
	ui_edit_write_key_int(f, "width", ld->width);
	ui_edit_write_key_int(f, "height", ld->height);
	ui_edit_write_key_bool(f, "sizeable", ld->sizeable);

	ui_edit_write_key_int(f, "columns", ld->column_count);
	ui_edit_write_key_bool(f, "columns_right_justify", ld->columns_right_justify);

	ui_edit_write_key_int(f, "border_top", ld->border_top);
	ui_edit_write_key_int(f, "border_right", ld->border_right);
	ui_edit_write_key_int(f, "border_bottom", ld->border_bottom);
	ui_edit_write_key_int(f, "border_left", ld->border_left);
	ui_edit_write_key_bool(f, "center_stretch", ld->stretch);

	ui_edit_write_key_char(f, "row_image", row_image);

	ui_edit_write_key_bool(f, "row_pressable", ld->row_has_press);
	ui_edit_write_key_bool(f, "row_prelight", ld->row_has_prelight);
	ui_edit_write_key_int(f, "row_border_left", ld->row_border_left);
	ui_edit_write_key_int(f, "row_border_right", ld->row_border_right);
	ui_edit_write_key_bool(f, "row_stretch", ld->row_stretch);

	if (text_image)
		{
		ui_edit_write_key_char(f, "text_image", text_image);
		ui_edit_write_key_bool(f, "text_extended", ld->font->extended);
		}
	if (ld->font->text_description)
		{
		ui_edit_write_key_char(f, "text_font", ld->font->text_description);
		ui_edit_write_key_int(f, "text_red", ld->text_r);
		ui_edit_write_key_int(f, "text_green", ld->text_g);
		ui_edit_write_key_int(f, "text_blue", ld->text_b);
		ui_edit_write_key_int(f, "text_alpha", ld->text_a);
		}

	if (divider_image) ui_edit_write_key_char(f, "divider_image", divider_image);
	if (flag_image)
		{
		ui_edit_write_key_char(f, "flag_image", flag_image);
		ui_edit_write_key_int(f, "flag_sections", ld->flag_sections);
		ui_edit_write_key_int(f, "flag_column", ld->flag_column);
		}

	/* write column attribs */
	for (i = 0; i < ld->column_count; i++)
		{
		gchar *buf;

		if (ld->column_keys[i] != NULL)
			{
			buf = g_strdup_printf("column_%d_key", i);
			ui_edit_write_key_char(f, buf, ld->column_keys[i]);
			g_free(buf);
			}

		buf = g_strdup_printf("column_%d_width", i);
		ui_edit_write_key_int(f, buf, ld->column_widths[i]);
		g_free(buf);

		buf = g_strdup_printf("column_%d_proportional", i);
		ui_edit_write_key_bool(f, buf, ld->column_flags[i] & UI_LIST_COLUMN_SIZE_PROPORTIONAL);
		g_free(buf);

		buf = g_strdup_printf("column_%d_right_justify", i);
		ui_edit_write_key_bool(f, buf, ld->column_flags[i] & UI_LIST_COLUMN_JUSTIFY_RIGHT);
		g_free(buf);
		}

	g_free(image);
	g_free(row_image);
	g_free(text_image);
	g_free(divider_image);
	g_free(flag_image);
}

static ListListData *list_edit_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while (work)
		{
		ListListData *ll = work->data;
		if (strcmp(image, ll->image) == 0) return ll;
		work = work->next;
		}
	return NULL;
}

static gpointer list_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	ListData *ld = wd->widget;
	ListListData *ll;
	const gchar *image;
	gint i;

	image = ui_widget_get_data(wd, "image");

	if (!image || list_edit_find(list, image)) return NULL;

	ll = g_new0(ListListData, 1);
	ll->image = g_strdup(ui_widget_get_data(wd, "image"));
	ll->border_left = ld->border_left;
	ll->border_right = ld->border_right;
	ll->border_top = ld->border_top;
	ll->border_bottom = ld->border_bottom;
	ll->border_stretch = ld->border_stretch;

	ll->row_image = g_strdup(ui_widget_get_data(wd, "row_image"));
	ll->row_border_left = ld->row_border_left;
	ll->row_border_right = ld->row_border_right;
	ll->row_stretch = ld->row_stretch;
	ll->row_has_press = ld->row_has_press;
	ll->row_has_prelight = ld->row_has_prelight;

	ll->divider_image = g_strdup(ui_widget_get_data(wd, "divider_image"));

	ll->text_image = g_strdup(ui_widget_get_data(wd, "text_image"));
	ll->text_extended = ld->font->extended;

	ll->text_font = g_strdup(ld->font->text_description);
	ll->text_r = ld->text_r;
	ll->text_g = ld->text_g;
	ll->text_b = ld->text_b;
	ll->text_a = ld->text_a;

	ll->flag_image = g_strdup(ui_widget_get_data(wd, "flag_image"));
	ll->flag_sections = ld->flag_sections;
	ll->flag_column = ld->flag_column;

	ll->width = ld->width;
	ll->height = ld->height;
	ll->sizeable = ld->sizeable;

	ll->column_count = ld->column_count;
	ll->columns_right_justify = ld->columns_right_justify;

	ll->column_widths = g_new0(gint, ll->column_count);
	memcpy(ll->column_widths, ld->column_widths, sizeof(gint) * ll->column_count);

	ll->column_flags = g_new0(ListColumnFlags, ll->column_count);
	memcpy(ll->column_flags, ld->column_flags, sizeof(ListColumnFlags) * ll->column_count);

	ll->column_keys = g_new0(gchar *, ll->column_count);
	for (i = 0; i < ll->column_count; i++) ll->column_keys[i] = g_strdup(ld->column_keys[i]);

	ll->key = g_strdup(wd->key);
	ll->data = g_strdup(ui_widget_get_data(wd, "data"));
	ll->text_id = g_strdup(wd->text_id);

	return ll;
}

static void list_edit_free(gpointer data)
{
	ListListData *ll = data;
	gint i;

	g_free(ll->image);
	g_free(ll->row_image);
	g_free(ll->divider_image);
	g_free(ll->text_image);
	g_free(ll->text_font);
	g_free(ll->flag_image);
	g_free(ll->column_widths);
	g_free(ll->column_flags);
	for (i = 0; i < ll->column_count; i++) g_free(ll->column_keys[i]);
	g_free(ll->column_keys);

	g_free(ll->key);
	g_free(ll->data);
	g_free(ll->text_id);

	g_free(ll);
}

static void list_details_pack(ListDetail *dt, GtkWidget *vbox, gint add_page)
{
	GtkWidget *notebook;
	GtkWidget *page;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *vbox2;
	GtkWidget *image;
	GtkListStore *store;

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 5);

	page = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(page), 5);

	if (add_page)
		{
		dt->image_entry = ui_edit_path_entry_new(page, _("Image:"), "SLIK_list_image");
		image = ui_edit_image_new(page);
		ui_edit_path_entry_connect_image(dt->image_entry, image);
		}

	hbox = ui_edit_frame_new(page, FALSE, NULL);

	dt->width_spin = ui_edit_spin_new(hbox, _("Width:"), 8, SKIN_SIZE_MAX, NULL);
	dt->height_spin = ui_edit_spin_new(hbox, _("Height:"), 8, SKIN_SIZE_MAX, NULL);

	dt->sizeable_button = ui_edit_toggle_new(page, _("Dynamic size"));

	vbox2 = ui_edit_vframe_new(page, TRUE, _("Border"));

	hbox = ui_edit_frame_new(vbox2, FALSE, NULL);
	dt->border_left_spin = ui_edit_spin_new(hbox, _("Left:"), 0, SKIN_SIZE_MAX, NULL);
	dt->border_right_spin = ui_edit_spin_new(hbox, _("Right:"), 0, SKIN_SIZE_MAX, NULL);
	hbox = ui_edit_frame_new(vbox2, FALSE, NULL);
	dt->border_top_spin = ui_edit_spin_new(hbox, _("Top:"), 0, SKIN_SIZE_MAX, NULL);
	dt->border_bottom_spin = ui_edit_spin_new(hbox, _("Bottom:"), 0, SKIN_SIZE_MAX, NULL);
	dt->border_stretch_button = ui_edit_toggle_new(vbox2, _("Stretch"));

	dt->stretch_button = ui_edit_toggle_new(page, _("Stretch center"));

	label = gtk_label_new(_("Background"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
	gtk_widget_show(page);

	page = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(page), 5);

	dt->row_image_entry = ui_edit_path_entry_new(page, _("Image:"), "SLIK_list_row_image");
	image = ui_edit_image_new(page);
	ui_edit_path_entry_connect_image(dt->row_image_entry, image);

	hbox = ui_edit_frame_new(page, TRUE, _("Border"));

	dt->row_border_left_spin = ui_edit_spin_new(hbox, _("Left:"), 0, SKIN_SIZE_MAX, NULL);
	dt->row_border_right_spin = ui_edit_spin_new(hbox, _("Right:"), 0, SKIN_SIZE_MAX, NULL);
	dt->row_stretch_button = ui_edit_toggle_new(hbox, _("Stretch center"));

	hbox = ui_edit_frame_new(page, FALSE, NULL);
	dt->row_has_press_button = ui_edit_toggle_new(hbox, _("Press"));
	dt->row_has_prelight_button = ui_edit_toggle_new(hbox, _("Prelight"));

	label = gtk_label_new(_("Row"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
	gtk_widget_show(page);

	page = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(page), 5);

	dt->text_font_entry = ui_edit_font_entry_new(page, _("Font:"));
	dt->text_image_entry = ui_edit_path_entry_new(page, _("Image:"), "SLIK_list_text_image");
	image = ui_edit_image_new(page);
	ui_edit_path_entry_connect_image(dt->text_image_entry, image);

	dt->text_extended_button = ui_edit_toggle_new(page, _("Extended"));

	hbox = ui_edit_frame_new(page, TRUE, _("Color:"));
	dt->text_font_r = ui_edit_spin_new(hbox, _("Red:"), 0, 255, NULL);
	dt->text_font_g = ui_edit_spin_new(hbox, _("Green:"), 0, 255, NULL);
	dt->text_font_b = ui_edit_spin_new(hbox, _("Blue:"), 0, 255, NULL);

	dt->text_font_a = ui_edit_spin_new(page, _("Alpha:"), 0, 255, NULL);

	label = gtk_label_new(_("Text"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
	gtk_widget_show(page);

	page = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(page), 5);

	dt->column_count_spin = ui_edit_spin_new(page, _("Columns:"), 1, 20, NULL);

	store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT,
				      G_TYPE_BOOLEAN, G_TYPE_STRING);
	dt->column_list = ui_edit_list_new(page, store, NULL, dt);
	g_object_unref(store);

	ui_edit_list_add_column(dt->column_list, _("#"), COL_COLUMN_NUMBER, FALSE, FALSE);
	ui_edit_list_add_column(dt->column_list, _("Key"), COL_COLUMN_KEY, FALSE, FALSE);
	ui_edit_list_add_column(dt->column_list, _("Width"), COL_COLUMN_WIDTH, FALSE, FALSE);
	ui_edit_list_add_column(dt->column_list, _("Dynamic"), COL_COLUMN_DYNAMIC, FALSE, FALSE);
	ui_edit_list_add_column(dt->column_list, _("Justify"), COL_COLUMN_JUSTIFY, FALSE, FALSE);

	dt->flag_column_spin = ui_edit_spin_new(page, _("Flag column:"), 0, 19, NULL);
	dt->columns_right_toggle = ui_edit_toggle_new(page, _("Right justify"));

	label = gtk_label_new(_("Columns"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
	gtk_widget_show(page);

	page = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(page), 5);

	hbox = ui_edit_vframe_new(page, TRUE, _("Column divider"));
	dt->divider_image_entry = ui_edit_path_entry_new(hbox, _("Image:"), "SLIK_list_divider_image");
	image = ui_edit_image_new(hbox);
	ui_edit_path_entry_connect_image(dt->divider_image_entry, image);

	hbox = ui_edit_vframe_new(page, TRUE, _("Flags"));
	dt->flag_image_entry = ui_edit_path_entry_new(hbox, _("Image:"), "SLIK_list_flag_image");
	image = ui_edit_image_new(hbox);
	ui_edit_path_entry_connect_image(dt->flag_image_entry, image);

	dt->flag_sections_spin = ui_edit_spin_new(hbox, _("Sections:"), 1, 100, NULL);

	label = gtk_label_new(_("Images"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, label);
	gtk_widget_show(page);

	gtk_widget_show(notebook);
}


static void list_props_list_update(ListDetail *dt, gint row)
{
	GtkTreeModel *store;
	GtkTreeIter iter;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(dt->column_list));
	if (row >= dt->ld->column_count ||
	    !gtk_tree_model_iter_nth_child(store, &iter, NULL, row)) return;

	gtk_list_store_set(GTK_LIST_STORE(store), &iter,
			   COL_COLUMN_NUMBER, row,
			   COL_COLUMN_KEY, dt->ld->column_keys[row],
			   COL_COLUMN_WIDTH, dt->ld->column_widths[row],
			   COL_COLUMN_DYNAMIC, (dt->ld->column_flags[row] & UI_LIST_COLUMN_SIZE_PROPORTIONAL),
			   COL_COLUMN_JUSTIFY, (dt->ld->column_flags[row] & UI_LIST_COLUMN_JUSTIFY_RIGHT) ? _("right") : _("left"), -1);
}

static void list_props_list_append(ListDetail *dt, gint row)
{
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (row >= dt->ld->column_count) return;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(dt->column_list));
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);

	list_props_list_update(dt, row);
}

static void list_props_list_populate(ListDetail *dt)
{
	GtkTreeModel *store;
	gint i;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(dt->column_list));
	gtk_list_store_clear(GTK_LIST_STORE(store));

	for (i = 0; i < dt->ld->column_count; i++) list_props_list_append(dt, i);
}

static void list_edit_props_sync(ListDetail *dt)
{
	ListData *ld = dt->ld;
	WidgetData *wd = dt->wd;

	ui_edit_spin_set_blocking(dt->width_spin, ld->width, dt);
	ui_edit_spin_set_blocking(dt->height_spin, ld->height, dt);
	ui_edit_toggle_set_blocking(dt->sizeable_button, ld->sizeable, dt);

	ui_edit_spin_set_blocking(dt->border_left_spin, ld->border_left, dt);
	ui_edit_spin_set_blocking(dt->border_right_spin, ld->border_right, dt);
	ui_edit_spin_set_blocking(dt->border_top_spin, ld->border_top, dt);
	ui_edit_spin_set_blocking(dt->border_bottom_spin, ld->border_bottom, dt);
	ui_edit_toggle_set_blocking(dt->border_stretch_button, ld->border_stretch, dt);

	ui_edit_toggle_set_blocking(dt->stretch_button, ld->stretch, dt);

	ui_edit_entry_set(dt->row_image_entry, ui_widget_get_data(wd, "row_image"));

	ui_edit_spin_set_blocking(dt->row_border_left_spin, ld->row_border_left, dt);
	ui_edit_spin_set_blocking(dt->row_border_right_spin, ld->row_border_right, dt);

	ui_edit_toggle_set_blocking(dt->row_stretch_button, ld->row_stretch, dt);
	ui_edit_toggle_set(dt->row_has_press_button, ld->row_has_press);
	ui_edit_toggle_set(dt->row_has_prelight_button, ld->row_has_prelight);

	ui_edit_entry_set(dt->divider_image_entry, ui_widget_get_data(wd, "divider_image"));

	ui_edit_entry_set(dt->text_image_entry, ui_widget_get_data(wd, "text_image"));
	ui_edit_toggle_set(dt->text_extended_button, ld->font->extended);

	ui_edit_entry_set(dt->text_font_entry, ld->font->text_description);

	ui_edit_spin_set_blocking(dt->text_font_r, ld->text_r, dt);
	ui_edit_spin_set_blocking(dt->text_font_g, ld->text_g, dt);
	ui_edit_spin_set_blocking(dt->text_font_b, ld->text_b, dt);
	ui_edit_spin_set_blocking(dt->text_font_a, ld->text_a, dt);

	ui_edit_entry_set(dt->flag_image_entry, ui_widget_get_data(wd, "flag_image"));
	ui_edit_spin_set(dt->flag_sections_spin, ld->flag_sections);
	ui_edit_toggle_set(dt->columns_right_toggle, ld->columns_right_justify);

	ui_edit_spin_set_blocking(dt->column_count_spin, ld->column_count, dt);
	ui_edit_spin_set_blocking(dt->flag_column_spin, ld->flag_column, dt);

	list_props_list_populate(dt);
}

static void list_props_clamp_borders(ListDetail *dt)
{
	ListData *ld = dt->ld;

	if (ld->border_left > ld->width - 1)
		{
		ld->border_left = ld->width - 1;
		ui_edit_spin_set_blocking(dt->border_left_spin, ld->border_left, dt);
		}
	if (ld->border_right > ld->width - 1 - ld->border_left)
		{
		ld->border_right = ld->width - 1 - ld->border_left;
		ui_edit_spin_set_blocking(dt->border_right_spin, ld->border_right, dt);
		}
	if (ld->border_top > ld->height - 1)
		{
		ld->border_top = ld->height - 1;
		ui_edit_spin_set_blocking(dt->border_top_spin, ld->border_top, dt);
		}
	if (ld->border_bottom > ld->height - 1 - ld->border_top)
		{
		ld->border_bottom = ld->height - 1 - ld->border_top;
		ui_edit_spin_set_blocking(dt->border_bottom_spin, ld->border_bottom, dt);
		}
	if (ld->row_border_left > ld->width - ld->border_left - ld->border_right - 1)
		{
		ld->row_border_left = ld->width - ld->border_left - ld->border_right - 1;
		ui_edit_spin_set_blocking(dt->row_border_left_spin, ld->row_border_left, dt);
		}
	if (ld->row_border_right > ld->width - ld->border_left - ld->border_right - 1 - ld->row_border_left)
		{
		ld->row_border_right = ld->width - ld->border_left - ld->border_right - 1 - ld->row_border_left;
		ui_edit_spin_set_blocking(dt->row_border_right_spin, ld->row_border_right, dt);
		}
}

static void list_props_geometry_cb(GtkObject *object, gpointer data)
{
	ListDetail *dt = data;
	gint old_w, old_h;
	gint w, h;

	old_w = dt->ld->width;
	old_h = dt->ld->height;

	w = ui_edit_spin_get(dt->width_spin);
	h = ui_edit_spin_get(dt->height_spin);

	if (w > dt->ui->skin->width)
		{
		w = dt->ui->skin->width;
		ui_edit_spin_set_blocking(dt->width_spin, w, dt);
		}
	if (dt->ld->x + w > dt->ui->skin->width)
		{
		ui_widget_set_coord(dt->ui, dt->wd, dt->ui->skin->width - w, dt->ld->y, TRUE);
		}
	if (h > dt->ui->skin->height)
		{
		h = dt->ui->skin->height;
		ui_edit_spin_set_blocking(dt->height_spin, h, dt);
		}
	if (dt->ld->y + h > dt->ui->skin->height)
		{
		ui_widget_set_coord(dt->ui, dt->wd, dt->ld->x, dt->ui->skin->height - h, TRUE);
		}

	if (old_w == w && old_h == h) return;

	list_props_clamp_borders(dt);

	ui_edit_widget_draw_highlight(dt->ui, dt->wd, FALSE);

	dt->ld->width = w;
	dt->ld->height = h;

	ui_edit_widget_resync(dt->ui, dt->wd, FALSE, old_w - w, old_h - h);
	ui_edit_widget_draw_highlight(dt->ui, dt->wd, TRUE);
}

static void list_props_sizeable_cb(GtkWidget *widget, gpointer data)
{
	ListDetail *dt = data;

	dt->ld->sizeable = ui_edit_toggle_get(dt->sizeable_button);
}

static void list_props_border_cb(GtkObject *object, gpointer data)
{
	ListDetail *dt = data;

	dt->ld->border_left = ui_edit_spin_get(dt->border_left_spin);
	dt->ld->border_right = ui_edit_spin_get(dt->border_right_spin);
	dt->ld->border_top = ui_edit_spin_get(dt->border_top_spin);
	dt->ld->border_bottom = ui_edit_spin_get(dt->border_bottom_spin);
	dt->ld->border_stretch = ui_edit_toggle_get(dt->border_stretch_button);
	dt->ld->stretch = ui_edit_toggle_get(dt->stretch_button);

	list_props_clamp_borders(dt);

	ui_edit_widget_resync(dt->ui, dt->wd, TRUE, 0, 0);
}

static void list_props_row_cb(GtkObject *object, gpointer data)
{
	ListDetail *dt = data;
	ListData *ld = dt->ld;

	ld->row_border_left = ui_edit_spin_get(dt->row_border_left_spin);
	ld->row_border_right = ui_edit_spin_get(dt->row_border_right_spin);
	ld->row_stretch = ui_edit_toggle_get(dt->row_stretch_button);

	ld->force_sync = TRUE;

	ui_edit_widget_resync(dt->ui, dt->wd, TRUE, 0, 0);
}

static void list_sync_column_count(ListData *ld, gint new_count)
{
	gint length;
	gint *old_column_widths;
        gint *old_column_real_widths;
        ListColumnFlags *old_column_flags;
        gchar **old_column_keys;
	gint i;
	GList *work;

	if (ld->column_count == new_count) return;

	old_column_widths = ld->column_widths;
	old_column_flags = ld->column_flags;
	old_column_real_widths = ld->column_real_widths;
	old_column_keys = ld->column_keys;

	ld->column_widths = g_new0(gint, new_count);
	ld->column_flags = g_new0(ListColumnFlags, new_count);
	ld->column_real_widths = g_new0(gint, new_count);
	ld->column_keys = g_new0(gchar *, new_count);

	length = MIN(new_count, ld->column_count);
	memcpy(ld->column_widths, old_column_widths, sizeof(gint) * length);
	memcpy(ld->column_flags, old_column_flags, sizeof(ListColumnFlags) * length);
	memcpy(ld->column_real_widths, old_column_real_widths, sizeof(gint) * length);
	memcpy(ld->column_keys, old_column_keys, sizeof(gchar *) * length);

	/* clean up old arrays, free data if necessary */
	for (i = new_count; i < ld->column_count; i++)
		{
		g_free(old_column_keys[i]);
		}
	g_free(old_column_widths);
	g_free(old_column_flags);
	g_free(old_column_real_widths);
	g_free(old_column_keys);

	/* fill new array space for any new columns */
	for (i = ld->column_count; i < new_count; i++)
		{
		ld->column_widths[i] = 1;
		ld->column_flags[i] = UI_LIST_COLUMN_SIZE_PROPORTIONAL;
		ld->column_real_widths[i] = 0;
		ld->column_keys[i] = NULL;
		}

	ld->column_count = new_count;

	/* now handle the row data */
	work = ld->row_list;
	while (work)
		{
		ListRowData *rd;
		gchar **old_text;

		rd = work->data;

		old_text = rd->text;
		rd->text = g_new0(gchar *, new_count);

		memcpy(rd->text, old_text, sizeof(gchar *) * MIN(rd->cells, new_count));

		for (i = new_count; i < rd->cells; i++)
			{
			g_free(old_text[i]);
			}
		g_free(old_text);

		rd->cells = new_count;

		work = work->next;
		}
}

static void list_props_column_count_cb(GtkAdjustment *adj, gpointer data)
{
	ListDetail *dt = data;

	list_sync_column_count(dt->ld, ui_edit_spin_get(dt->column_count_spin));
	list_props_list_populate(dt);

	dt->ld->force_sync = TRUE;

	ui_edit_widget_resync(dt->ui, dt->wd, TRUE, 0, 0);
}

static gint list_props_column_key_cb(TreeEditData *ted,
				     const gchar *old_text, const gchar *new_text, gpointer data)
{
	ListDetail *dt = data;
	ListData *ld = dt->ld;
	GList *work;
	gint row;

	row = tree_path_to_row(ted->path);
	if (row >= ld->column_count) return FALSE;
	if (strlen(new_text) == 0) new_text = NULL;

	g_free(ld->column_keys[row]);
	ld->column_keys[row] = g_strdup(new_text);

	/* clear all text for this column */
	work = ld->row_list;
	while (work)
		{
		ListRowData *rd = work->data;
		list_row_text_set(rd, row, NULL);
		work = work->next;
		}

	list_props_list_update(dt, row);
	ui_edit_widget_resync(dt->ui, dt->wd, TRUE, 0, 0);

	return FALSE;
}

static gint list_props_column_width_cb(TreeEditData *ted,
				       const gchar *old_text, const gchar *new_text, gpointer data)
{
	ListDetail *dt = data;
	ListData *ld = dt->ld;
	gint n;
	gint val;
	gint row;

	row = tree_path_to_row(ted->path);
	if (row >= ld->column_count ||
	    strlen(new_text) == 0) return FALSE;

	n = (ld->column_flags[row] & UI_LIST_COLUMN_SIZE_PROPORTIONAL) ? 100 : SKIN_SIZE_MAX;

	val = (gint)strtol(new_text, NULL, 10);
	if (val < 1) return FALSE;
	ld->column_widths[row] = MIN(val, n);

	list_props_list_update(dt, row);
	ld->force_sync = TRUE;
	ui_edit_widget_resync(dt->ui, dt->wd, TRUE, 0, 0);

	return FALSE;
}

static gint list_props_column_list_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	ListDetail *dt = data;
	GtkTreePath *tpath = NULL;
	GtkTreeViewColumn *tcolumn;
	gint row = -1;
	gint column = -1;
	gint update = FALSE;
	ListData *ld;
	gint n;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), event->x, event->y,
					  &tpath, &tcolumn, NULL, NULL))
		{
		GList *list;

		row = tree_path_to_row(tpath);

		list = gtk_tree_view_get_columns(GTK_TREE_VIEW(widget));
		column = g_list_index(list, tcolumn);
		g_list_free(list);
		}

	if (row == -1 || column == -1 ||
	    row >= dt->ld->column_count)
		{
		if (tpath) gtk_tree_path_free(tpath);
		return FALSE;
		}

	ld = dt->ld;

	switch (column)
		{
		case COL_COLUMN_KEY:
			if (event->button != 1) break;
			tree_edit_by_path(GTK_TREE_VIEW(dt->column_list), tpath, column, NULL,
					  list_props_column_key_cb, dt);
			break;
		case COL_COLUMN_WIDTH:
			n = (ld->column_flags[row] & UI_LIST_COLUMN_SIZE_PROPORTIONAL) ? 100 : SKIN_SIZE_MAX;
			if (event->button == 1)
				{
				tree_edit_by_path(GTK_TREE_VIEW(dt->column_list), tpath, column, NULL,
						  list_props_column_width_cb, dt);
				}
			else if (event->button == 2 || event->button == 5)
				{
				ld->force_sync = TRUE;
				update = TRUE;
				ld->column_widths[row] = CLAMP(ld->column_widths[row] - 1, 1, n);
				}
			else if (event->button == 3 || event->button == 4)
				{
				ld->force_sync = TRUE;
				update = TRUE;
				ld->column_widths[row] = CLAMP(ld->column_widths[row] + 1, 1, n);
				}
			break;
		case COL_COLUMN_DYNAMIC:
			if (event->button != 1) break;
			ld->force_sync = TRUE;
			update = TRUE;
			if (ld->column_flags[row] & UI_LIST_COLUMN_SIZE_PROPORTIONAL)
				{
				ld->column_flags[row] &= ~UI_LIST_COLUMN_SIZE_PROPORTIONAL;
				ld->column_flags[row] |= UI_LIST_COLUMN_SIZE_FIXED;
				}
			else
				{
				ld->column_flags[row] |= UI_LIST_COLUMN_SIZE_PROPORTIONAL;
				ld->column_flags[row] &= ~UI_LIST_COLUMN_SIZE_FIXED;
				ld->column_widths[row] = CLAMP(ld->column_widths[row], 1, 100);
				}
			break;
		case COL_COLUMN_JUSTIFY:
			if (event->button != 1) break;
			update = TRUE;
			if (ld->column_flags[row] & UI_LIST_COLUMN_JUSTIFY_RIGHT)
				ld->column_flags[row] &= ~UI_LIST_COLUMN_JUSTIFY_RIGHT;
			else
				ld->column_flags[row] |= UI_LIST_COLUMN_JUSTIFY_RIGHT;
			break;
		default:
			break;
		}

	if (tpath) gtk_tree_path_free(tpath);

	if (update)
		{
		list_props_list_update(dt, row);
		ui_edit_widget_resync(dt->ui, dt->wd, TRUE, 0, 0);
		return TRUE;
		}

	return FALSE;
}

static void list_props_text_color_cb(GtkAdjustment *adj, gpointer data)
{
	ListDetail *dt = data;

	dt->ld->text_r = ui_edit_spin_get(dt->text_font_r);
	dt->ld->text_g = ui_edit_spin_get(dt->text_font_g);
	dt->ld->text_b = ui_edit_spin_get(dt->text_font_b);
	dt->ld->text_a = ui_edit_spin_get(dt->text_font_a);

	if (!dt->ld->font->overlay) ui_edit_widget_resync(dt->ui, dt->wd, TRUE, 0, 0);
}

static void list_props_flag_column_cb(GtkAdjustment *adj, gpointer data)
{
	ListDetail *dt = data;

	dt->ld->flag_column = ui_edit_spin_get(dt->flag_column_spin);
	if (dt->ld->flag_column >= dt->ld->column_count)
		{
		dt->ld->flag_column = dt->ld->column_count - 1;
		ui_edit_spin_set_blocking(dt->flag_column_spin, dt->ld->flag_column, dt);
		}

	ui_edit_widget_resync(dt->ui, dt->wd, TRUE, 0, 0);
}

static void list_props_column_right_cb(GtkWidget *widget, gpointer data)
{
	ListDetail *dt = data;
	
	dt->ld->columns_right_justify = ui_edit_toggle_get(dt->columns_right_toggle);

	dt->ld->force_sync = TRUE;
	ui_edit_widget_resync(dt->ui, dt->wd, TRUE, 0, 0);
}

static void list_props_connect_spin(GtkWidget *spin, ListDetail *dt, GtkSignalFunc func)
{
	GtkAdjustment *adj;

	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	g_signal_connect(G_OBJECT(adj), "value_changed", func, dt);
}

static void list_props_signals(ListDetail *dt)
{
	list_props_connect_spin(dt->width_spin, dt, G_CALLBACK(list_props_geometry_cb));
	list_props_connect_spin(dt->height_spin, dt, G_CALLBACK(list_props_geometry_cb));
	g_signal_connect(G_OBJECT(dt->sizeable_button), "clicked",
			 G_CALLBACK(list_props_sizeable_cb) , dt);

	list_props_connect_spin(dt->border_top_spin, dt, G_CALLBACK(list_props_border_cb));
	list_props_connect_spin(dt->border_left_spin, dt, G_CALLBACK(list_props_border_cb));
	list_props_connect_spin(dt->border_right_spin, dt, G_CALLBACK(list_props_border_cb));
	list_props_connect_spin(dt->border_bottom_spin, dt, G_CALLBACK(list_props_border_cb));
	g_signal_connect(G_OBJECT(dt->border_stretch_button), "clicked",
			 G_CALLBACK(list_props_border_cb) , dt);

	g_signal_connect(G_OBJECT(dt->stretch_button), "clicked",
			 G_CALLBACK(list_props_border_cb) , dt);

	ui_edit_frame_sensitive(dt->row_image_entry, FALSE, TRUE);

	list_props_connect_spin(dt->row_border_left_spin, dt, G_CALLBACK(list_props_row_cb));
	list_props_connect_spin(dt->row_border_right_spin, dt, G_CALLBACK(list_props_row_cb));
	g_signal_connect(G_OBJECT(dt->row_stretch_button), "clicked",
			 G_CALLBACK(list_props_row_cb) , dt);

	ui_edit_frame_sensitive(dt->row_has_press_button, FALSE, TRUE);
	ui_edit_frame_sensitive(dt->row_has_prelight_button, FALSE, TRUE);

	ui_edit_frame_sensitive(dt->divider_image_entry, FALSE, FALSE);

	ui_edit_frame_sensitive(dt->text_image_entry, FALSE, TRUE);
	gtk_widget_set_sensitive(dt->text_extended_button, FALSE);

	ui_edit_frame_sensitive(dt->text_font_entry, FALSE, TRUE);

	list_props_connect_spin(dt->text_font_r, dt, G_CALLBACK(list_props_text_color_cb));
	list_props_connect_spin(dt->text_font_g, dt, G_CALLBACK(list_props_text_color_cb));
	list_props_connect_spin(dt->text_font_b, dt, G_CALLBACK(list_props_text_color_cb));
	list_props_connect_spin(dt->text_font_a, dt, G_CALLBACK(list_props_text_color_cb));

	ui_edit_frame_sensitive(dt->flag_image_entry, FALSE, FALSE);
	ui_edit_frame_sensitive(dt->flag_sections_spin, FALSE, TRUE);

	list_props_connect_spin(dt->column_count_spin, dt, G_CALLBACK(list_props_column_count_cb));

	g_signal_connect(G_OBJECT(dt->column_list), "button_press_event",
			 G_CALLBACK(list_props_column_list_cb), dt);

	list_props_connect_spin(dt->flag_column_spin, dt, G_CALLBACK(list_props_flag_column_cb));

	g_signal_connect(G_OBJECT(dt->columns_right_toggle), "clicked",
			 G_CALLBACK(list_props_column_right_cb), dt);
}

static gpointer list_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail)
{
	ListData *ld = wd->widget;
	ListDetail *dt = detail;

	if (!dt)
		{
		dt = g_new0(ListDetail, 1);

		list_details_pack(dt, vbox, FALSE);
		list_props_signals(dt);
		}

	dt->ld = ld;
	dt->wd = wd;
	dt->ui = ui;

	list_edit_props_sync(dt);

	return dt;
}

static void list_edit_page_add_cb(GtkWidget *widget, gpointer data)
{
	ListPage *pd = data;
	ListDetail *dt;
	const gchar *key;
	const gchar *text_id;
	const gchar *image;
	const gchar *row_image;
	const gchar *text_image;
	const gchar *font_desc;
	const gchar *divider_image;
	const gchar *flag_image;
	ListData *ld;

	dt = (ListDetail *)pd;

	key = ui_edit_entry_get(pd->key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	image = ui_edit_entry_get(dt->image_entry);
	row_image = ui_edit_entry_get(dt->row_image_entry);
	text_image = ui_edit_entry_get(dt->text_image_entry);
	font_desc = ui_edit_entry_get(dt->text_font_entry);
	divider_image = ui_edit_entry_get(dt->divider_image_entry);
	flag_image = ui_edit_entry_get(dt->flag_image_entry);

	if (!key || !image || !isfile(image))
		{
		warning_dialog(_("List error"), _("List must contain a key and valid image."));
		return;
		}
	if (!row_image || !isfile(row_image))
		{
		warning_dialog(_("List error"), _("Invalid or missing row image."));
		return;
		}
	if (text_image && !isfile(text_image))
		{
		warning_dialog(_("List error"), _("Invalid or missing text image."));
		return;
		}
	else if (!font_desc && !text_image)
		{
		warning_dialog(_("List error"), _("Must specify valid font description or image."));
		return;
		}

	ld = list_new(util_pixbuf_new_from_file(image),
		      0, 0, ui_edit_spin_get(dt->width_spin), ui_edit_spin_get(dt->height_spin),
		      ui_edit_toggle_get(dt->sizeable_button), ui_edit_spin_get(dt->column_count_spin),
		      ui_edit_spin_get(dt->border_top_spin), ui_edit_spin_get(dt->border_right_spin),
		      ui_edit_spin_get(dt->border_bottom_spin), ui_edit_spin_get(dt->border_left_spin),
		      ui_edit_toggle_get(dt->stretch_button));
	if (ld)
		{
		GdkPixbuf *pb = NULL;
		gint n;
		GList *work;
		WidgetData *wd;
		guint8 r, g, b, a;

		if (divider_image) pb = util_pixbuf_new_from_file(divider_image);

		list_image_row(ld, util_pixbuf_new_from_file(row_image),
			       ui_edit_toggle_get(dt->row_has_press_button),
			       ui_edit_toggle_get(dt->row_has_prelight_button),
			       ui_edit_spin_get(dt->row_border_left_spin),
			       ui_edit_spin_get(dt->row_border_right_spin),
			       ui_edit_toggle_get(dt->row_stretch_button), pb);

		r = ui_edit_spin_get(dt->text_font_r);
		g = ui_edit_spin_get(dt->text_font_g);
		b = ui_edit_spin_get(dt->text_font_b);
		a = ui_edit_spin_get(dt->text_font_a);

		if (font_desc)
			{
			list_set_font(ld, font_desc, NULL, FALSE, r, g, b, a);
			}
		else if (text_image)
			{
			list_set_font(ld, NULL, util_pixbuf_new_from_file(text_image), 
				      ui_edit_toggle_get(dt->text_extended_button), r, g, b, a);
			}

		if (!ld->font)
			{
			warning_dialog(_("List error"), _("Failed to initialize font with specified parameters."));
			list_free(ld);
			return;
			}

		list_set_column_justify(ld, ui_edit_toggle_get(dt->columns_right_toggle));

		pb = NULL;
		if (flag_image) pb = util_pixbuf_new_from_file(flag_image);

		list_image_row_flag(ld, pb, ui_edit_spin_get(dt->flag_sections_spin),
				    ui_edit_spin_get(dt->flag_column_spin));

		n = 0;
		work = pd->column_list;
		while (work)
			{
			ListPageColumn *c = work->data;
			list_set_column_attributes(ld, n, c->width, c->flags, c->key);
			work = work->next;
			n++;
			}

		wd = list_register(pd->ed->ui->skin, ld, key, text_id);
		ui_widget_set_data(wd, "row_image", row_image);
		if (!font_desc) ui_widget_set_data(wd, "text_image", text_image);
		ui_widget_set_data(wd, "divider_image", divider_image);
		ui_widget_set_data(wd, "flag_image", flag_image);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	tab_completion_append_to_history(dt->image_entry, image);
	tab_completion_append_to_history(dt->row_image_entry, row_image);
	tab_completion_append_to_history(dt->text_image_entry, text_image);
	tab_completion_append_to_history(dt->divider_image_entry, divider_image);
	tab_completion_append_to_history(dt->flag_image_entry, flag_image);
}

static ListPageColumn *page_column_new(const gchar *key, gint width, ListColumnFlags flags)
{
	ListPageColumn *c;

	c = g_new(ListPageColumn, 1);
	c->key = g_strdup(key);
	c->width = width;
	c->flags = flags;

	return c;
}

static void page_column_free(ListPageColumn *c)
{
	if (!c) return;

	g_free(c->key);
	g_free(c);
}

static void list_edit_page_free_columns(ListPage *pd)
{
	GList *work;
	work = pd->column_list;
	while(work)
		{
		ListPageColumn *c = work->data;
		page_column_free(c);
		work = work->next;
		}
	g_list_free(pd->column_list);
	pd->column_list = NULL;
}

static void list_edit_page_list_sync(ListPage *pd)
{
	ListDetail *dt;
	GtkListStore *store;
	GList *work;
	gint n;

	dt = (ListDetail *)pd;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(dt->column_list)));
	gtk_list_store_clear(store);

	n = 0;
	
	work = pd->column_list;
	while (work)
		{
		ListPageColumn *c = work->data;
		GtkTreeIter iter;

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   COL_COLUMN_NUMBER, n,
				   COL_COLUMN_KEY, c->key,
				   COL_COLUMN_WIDTH, c->width,
				   COL_COLUMN_DYNAMIC, (c->flags & UI_LIST_COLUMN_SIZE_PROPORTIONAL),
				   COL_COLUMN_JUSTIFY, (c->flags * UI_LIST_COLUMN_JUSTIFY_RIGHT) ? _("right") : _("left"), -1 );

		work = work->next;
		n++;
		}
}

static void list_edit_page_sync(ListPage *pd, ListListData *ll)
{
	ListDetail *dt;
	gint i;

	if (!ll) return;
	dt = (ListDetail *)pd;

	ui_edit_entry_set(pd->key_entry, ll->key);
	ui_edit_entry_set(pd->text_id_entry, ll->text_id);
	ui_edit_entry_set(pd->data_entry, ll->data);

	/* the notebook */

	ui_edit_entry_set(dt->image_entry, ll->image);

	ui_edit_spin_set(dt->width_spin, ll->width);
	ui_edit_spin_set(dt->height_spin, ll->height);
	ui_edit_toggle_set(dt->sizeable_button, ll->sizeable);

	ui_edit_spin_set(dt->border_left_spin, ll->border_left);
	ui_edit_spin_set(dt->border_right_spin, ll->border_right);
	ui_edit_spin_set(dt->border_top_spin, ll->border_top);
	ui_edit_spin_set(dt->border_bottom_spin, ll->border_bottom);
	ui_edit_toggle_set(dt->border_stretch_button, ll->border_stretch);

	ui_edit_toggle_set(dt->stretch_button, ll->stretch);

	ui_edit_entry_set(dt->row_image_entry, ll->row_image);

	ui_edit_spin_set(dt->row_border_left_spin, ll->row_border_left);
	ui_edit_spin_set(dt->row_border_right_spin, ll->row_border_right);

	ui_edit_toggle_set(dt->row_stretch_button, ll->row_stretch);
	ui_edit_toggle_set(dt->row_has_press_button, ll->row_has_press);
	ui_edit_toggle_set(dt->row_has_prelight_button, ll->row_has_prelight);

	ui_edit_entry_set(dt->divider_image_entry, ll->divider_image);

	ui_edit_entry_set(dt->text_image_entry, ll->text_image);
	ui_edit_toggle_set(dt->text_extended_button, ll->text_extended);

	ui_edit_entry_set(dt->text_font_entry, ll->text_font);

	ui_edit_spin_set(dt->text_font_r, ll->text_r);
	ui_edit_spin_set(dt->text_font_g, ll->text_g);
	ui_edit_spin_set(dt->text_font_b, ll->text_b);
	ui_edit_spin_set(dt->text_font_a, ll->text_a);

	ui_edit_entry_set(dt->flag_image_entry, ll->flag_image);
	ui_edit_spin_set(dt->flag_sections_spin, ll->flag_sections);

	ui_edit_spin_set_blocking(dt->column_count_spin, ll->column_count, pd);
	ui_edit_spin_set(dt->flag_column_spin, ll->flag_column);
	ui_edit_toggle_set(dt->columns_right_toggle, ll->columns_right_justify);

	/* the column list */
	list_edit_page_free_columns(pd);
	for (i = 0; i < ll->column_count; i++)
		{
		pd->column_list = g_list_append(pd->column_list,
			page_column_new(ll->column_keys[i], ll->column_widths[i], ll->column_flags[i]));
		}

	list_edit_page_list_sync(pd);
}

static void list_edit_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	ListPage *pd = data;
	ListListData *ll;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, LIST_COLUMN_POINTER, &ll, -1);
	list_edit_page_sync(pd, ll);
}

static void list_edit_column_count_cb(GtkObject *object, gpointer data)
{
	ListPage *pd = data;
	ListDetail *dt;
	gint n;
	gint l;

	dt = (ListDetail *)pd;
	n = ui_edit_spin_get(dt->column_count_spin);
	l = g_list_length(pd->column_list);

	if (n > l)
		{
		gint i;
		for (i = l; i < n; i++)
			{
			gchar *buf = g_strdup_printf("column_%d", i);
			pd->column_list = g_list_append(pd->column_list,
				page_column_new(buf, 10, UI_LIST_COLUMN_SIZE_FIXED));
			g_free(buf);
			}
		}
	else if (n < l)
		{
		GList *work;

		work = g_list_last(pd->column_list);
		if (work)
			{
			ListPageColumn *c = work->data;
			pd->column_list = g_list_remove(pd->column_list, c);
			page_column_free(c);
			}
		}

	list_edit_page_list_sync(pd);
}

static void list_edit_font_change_cb(GtkWidget *w, gpointer data)
{
	ListPage *pd = data;
	ListDetail *dt;
	gint sensitive;

	dt = (ListDetail *)pd;

	sensitive = (strlen(gtk_entry_get_text(GTK_ENTRY(dt->text_font_entry))) == 0);

	ui_edit_frame_sensitive(dt->text_image_entry, sensitive, TRUE);
	gtk_widget_set_sensitive(dt->text_extended_button, sensitive);

	ui_edit_frame_sensitive(dt->text_font_r, !sensitive, FALSE);
	ui_edit_frame_sensitive(dt->text_font_a, !sensitive, TRUE);
}

static void list_edit_page_destroy_cb(GtkWidget *widget, gpointer data)
{
	ListPage *pd = data;

	list_edit_page_free_columns(pd);
	g_free(pd);
}

static GtkWidget *list_edit_page_new(EditData *ed)
{
	ListDetail *dt;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkAdjustment *adj;
	ListPage *pd;
	GtkListStore *store;

	pd = g_new0(ListPage, 1);
	pd->ed = ed;

	dt = (ListDetail *)pd;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	g_object_set_data(G_OBJECT(hbox), "page", pd);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(list_edit_page_destroy_cb), pd);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	pd->key_entry = ui_edit_key_entry_new(vbox, ed->ui, list_type_id());
	pd->data_entry = ui_edit_entry_new(vbox, _("Data:"));
	pd->text_id_entry = ui_edit_entry_new(vbox, _("Text id:"));

	list_details_pack(dt, vbox, TRUE);

	g_signal_connect(G_OBJECT(dt->text_font_entry), "changed",
			 G_CALLBACK(list_edit_font_change_cb), pd);

	ui_edit_entry_set(dt->text_font_entry, SLIK_DEFAULT_FONT);
	ui_edit_spin_set(dt->text_font_r, 0);
	ui_edit_spin_set(dt->text_font_g, 0);
	ui_edit_spin_set(dt->text_font_b, 0);
	ui_edit_spin_set(dt->text_font_a, 255);

	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(dt->column_count_spin));
        g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(list_edit_column_count_cb), pd);

	button = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(list_edit_page_add_cb), pd);
	gtk_widget_show(button);

	store = gtk_list_store_new(4, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING,
				      G_TYPE_BOOLEAN);
	pd->list = ui_edit_list_new(hbox, store,
				    G_CALLBACK(list_edit_page_list_cb), pd);
	g_object_unref(store);

	ui_edit_list_add_column(pd->list, _("Image"), LIST_COLUMN_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(pd->list, _("Key"), LIST_COLUMN_KEY, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Flag"), LIST_COLUMN_FLAG, FALSE, FALSE);
	
	pd->column_list = g_list_append(pd->column_list,
					page_column_new("column_1", 10, UI_LIST_COLUMN_SIZE_FIXED));
	list_edit_page_list_sync(pd);

	gtk_widget_show(hbox);

	return hbox;
}

static void list_edit_page_add(GtkWidget *widget, gpointer data)
{
	ListListData *ll = data;
	ListPage *pd;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	pd = g_object_get_data(G_OBJECT(widget), "page");
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->list)));

	pixbuf = ui_edit_list_pixbuf_from_file(ll->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   LIST_COLUMN_POINTER, ll,
			   LIST_COLUMN_IMAGE, pixbuf,
			   LIST_COLUMN_KEY, ll->key,
			   LIST_COLUMN_FLAG, (ll->flag_image != NULL), -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

void list_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = list_get_pixbuf;

	od->func_edit_write = list_edit_write;

	od->func_edit_read = list_edit_read;
	od->func_edit_free = list_edit_free;

	od->func_edit_props = list_edit_props;

	od->func_edit_page_new = list_edit_page_new;
	od->func_edit_page_add = list_edit_page_add;
}
