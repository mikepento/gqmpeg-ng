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

#include "ui2_slider.h"
#include "ui2_slider_edit.h"

#include "ui2_display.h"
#include "ui2_editor.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "ui_pixbuf_ops.h"
#include "ui_tabcomp.h"
#include "ui_utildlg.h"


enum {
	SLIDER_COLUMN_POINTER = 0,
	SLIDER_COLUMN_IMAGE,
	SLIDER_COLUMN_KEY,
	SLIDER_COLUMN_VERTICAL,
	SLIDER_COLUMN_PRESS,
	SLIDER_COLUMN_PRELIGHT,
	SLIDER_COLUMN_COUNT
};


typedef struct _SliderDetail SliderDetail;
struct _SliderDetail
{
	SliderData *slider;
	GtkWidget *vertical_button;
	GtkWidget *reverse_button;
	GtkWidget *item_key_entry;
	GtkWidget *has_press_button;
	GtkWidget *prelight_button;

	GtkWidget *real_length_spin;
	GtkWidget *border_1_spin;
	GtkWidget *border_2_spin;
	GtkWidget *stretch_button;

	GtkWidget *sizeable_button;

	WidgetData *wd;
	UIData *ui;
};

typedef struct _SliderListData SliderListData;
struct _SliderListData
{
	gchar *key;
	gchar *text_id;
	gchar *data;

	gchar *image;
	gint length;
	gint vertical;
	gint has_press;
	gint has_prelight;
	gint reversed;

	gint real_length;
	gint border_1;
	gint border_2;
	gint stretch;

	gint sizeable;

	gchar *item_key;
};

typedef struct _SliderPage SliderPage;
struct _SliderPage
{
	GtkWidget *key_entry;
	GtkWidget *text_id_entry;
	GtkWidget *data_entry;

	GtkWidget *image_entry;
	GtkWidget *length_spin;

	GtkWidget *vertical_button;
	GtkWidget *reverse_button;
	GtkWidget *has_press_button;
	GtkWidget *has_prelight_button;

	GtkWidget *real_length_spin;
	GtkWidget *border_1_spin;
	GtkWidget *border_2_spin;
	GtkWidget *stretch_button;

	GtkWidget *sizeable_button;

	GtkWidget *item_key_entry;

	GtkWidget *list;
	GtkWidget *image;

	EditData *ed;
};


static GdkPixbuf *slider_get_pixbuf(gpointer data)
{
	SliderData *slider = data;
	return slider->overlay;
}

static void slider_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	SliderData *slider = wd->widget;
	gchar *image;

	image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "image"),
					 dir, slider->overlay, "slider_", wd->key);

	if (image) ui_edit_widget_set_path_key(wd, "image", dir, image);

	ui_edit_write_section(f, "slider", wd->key);

	ui_edit_write_key_char(f, "image", image);
	ui_edit_write_key_int(f, "x", slider->x);
	ui_edit_write_key_int(f, "y", slider->y);
	ui_edit_write_key_int(f, "length", slider->length);

	ui_edit_write_key_bool(f, "pressable", slider->has_press);
	ui_edit_write_key_bool(f, "prelight", slider->has_prelight);
	ui_edit_write_key_bool(f, "reversed", slider->reversed);
	ui_edit_write_key_bool(f, "vertical", slider->vertical);

	ui_edit_write_key_int(f, "trough_length", slider->real_length);
	ui_edit_write_key_int(f, "border_1", slider->border_1);
	ui_edit_write_key_int(f, "border_2", slider->border_2);
	ui_edit_write_key_bool(f, "stretch", slider->stretch);

	ui_edit_write_key_bool(f, "sizeable", slider->sizeable);

	if (slider->item_key) ui_edit_write_key_char(f, "item_link", slider->item_key);

	g_free(image);
}

static SliderListData *slider_edit_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while(work)
		{
		SliderListData *sd = work->data;
		if (strcmp(image, sd->image) == 0) return sd;
		work = work->next;
		}
	return NULL;
}

static gpointer slider_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	SliderData *slider = wd->widget;
	SliderListData *sd;
	const gchar *image;

	image = ui_widget_get_data(wd, "image");

	if (!image || slider_edit_find(list, image)) return NULL;

	sd = g_new0(SliderListData, 1);
	sd->image = g_strdup(image);

	sd->length = slider->length;

	sd->vertical = slider->vertical;
	sd->reversed = slider->reversed;

	sd->has_press = slider->has_press;
	sd->has_prelight = slider->has_prelight;

	sd->real_length = slider->real_length;
	sd->border_1 = slider->border_1;
	sd->border_2 = slider->border_2;
	sd->stretch = slider->stretch;

	sd->sizeable = slider->sizeable;

	sd->item_key = g_strdup(slider->item_key);

	sd->key = g_strdup(wd->key);
	sd->data = g_strdup(ui_widget_get_data(wd, "data"));
	sd->text_id = g_strdup(wd->text_id);

	return sd;
}

static void slider_edit_free(gpointer data)
{
	SliderListData *sd = data;

	g_free(sd->key);
	g_free(sd->text_id);
	g_free(sd->data);
	g_free(sd->image);
	g_free(sd->item_key);
	g_free(sd);
}

static void slider_edit_props_real_length_cb(GtkObject *adj, gpointer data)
{
	SliderDetail *sd = data;
	gint read_l, old_l;
	gint l;

	old_l = sd->slider->real_length;
	l = read_l = ui_edit_spin_get(sd->real_length_spin);

	if (l < sd->slider->handle_size) l = sd->slider->handle_size;
	if (l < sd->slider->border_1 + sd->slider->border_2 + 1) l = sd->slider->border_1 + sd->slider->border_2 + 1;

	if (sd->slider->vertical)
		{
		if (l > sd->ui->skin->height) l = sd->ui->skin->height;
		if (sd->ui->skin->height - sd->slider->y < l)
			ui_widget_set_coord(sd->ui, sd->wd, sd->slider->x, sd->ui->skin->width - l, TRUE);
		}
	else
		{
		if (l > sd->ui->skin->width) l = sd->ui->skin->width;
		if (sd->ui->skin->width - sd->slider->x < l)
			ui_widget_set_coord(sd->ui, sd->wd, sd->ui->skin->height - l, sd->slider->y, TRUE);
		}

	if (l != read_l) ui_edit_spin_set_blocking(sd->real_length_spin, l, sd);

	if (old_l == l) return;

	ui_edit_widget_draw_highlight(sd->ui, sd->wd, FALSE);

	sd->slider->real_length = l;
	if (sd->slider->vertical)
		{
		sd->slider->height = l;
		}
	else
		{
		sd->slider->width = l;
		}

	slider_sync_overlay(sd->slider);
	ui_edit_widget_resync(sd->ui, sd->wd, TRUE,
			      sd->slider->vertical ? 0 : MAX(0, old_l - l),
			      sd->slider->vertical ? MAX(0, old_l - l) : 0);
}

static void slider_edit_props_border_1_cb(GtkObject *adj, gpointer data)
{
	SliderDetail *sd = data;
	gint b;

	b = ui_edit_spin_get(sd->border_1_spin);
	if (b > sd->slider->length - 1)
		{
		b = sd->slider->length - 1;
		ui_edit_spin_set_blocking(sd->border_1_spin, b, sd);
		}
	if (sd->slider->border_2 > sd->slider->length - b - 1)
		{
		sd->slider->border_2 = sd->slider->length - b - 1;
		ui_edit_spin_set_blocking(sd->border_2_spin, sd->slider->border_2, sd);
		}

	sd->slider->border_1 = b;
	ui_edit_widget_resync(sd->ui, sd->wd, FALSE, 0, 0);
}

static void slider_edit_props_border_2_cb(GtkObject *adj, gpointer data)
{
	SliderDetail *sd = data;
	gint b;

	b = ui_edit_spin_get(sd->border_2_spin);
	if (b > sd->slider->length - 1)
		{
		b = sd->slider->length - 1;
		ui_edit_spin_set_blocking(sd->border_2_spin, b, sd);
		}
	if (sd->slider->border_1 > sd->slider->length - b - 1)
		{
		sd->slider->border_1 = sd->slider->length - b - 1;
		ui_edit_spin_set_blocking(sd->border_1_spin, sd->slider->border_1, sd);
		}

	sd->slider->border_2 = b;
	ui_edit_widget_resync(sd->ui, sd->wd, FALSE, 0, 0);
}

static void slider_edit_props_stretch_cb(GtkWidget *button, gpointer data)
{
	SliderDetail *sd = data;

	sd->slider->stretch = ui_edit_toggle_get(sd->stretch_button);
	ui_edit_widget_resync(sd->ui, sd->wd, FALSE, 0, 0);
}

static void slider_edit_props_sizeable_cb(GtkWidget *button, gpointer data)
{
	SliderDetail *sd = data;

	sd->slider->sizeable = ui_edit_toggle_get(sd->sizeable_button);
}

static void slider_edit_props_reversed_cb(GtkWidget *button, gpointer data)
{
	SliderDetail *sd = data;

	sd->slider->reversed = GTK_TOGGLE_BUTTON(button)->active;

	ui_widget_draw(sd->ui, sd->wd, TRUE, TRUE);
}

static void slider_edit_props_item_key_cb(GtkWidget *entry, gpointer data)
{
	SliderDetail *sd = data;
	const gchar *buf;

	g_free(sd->slider->item_key);
	buf = gtk_entry_get_text(GTK_ENTRY(entry));
	if (buf && strlen(buf) > 0)
		{
		sd->slider->item_key = g_strdup(buf);
		}
	else
		{
		sd->slider->item_key = NULL;
		}

	ui_widget_draw(sd->ui, sd->wd, TRUE, TRUE);
}

static gpointer slider_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail)
{
	SliderData *slider = wd->widget;
	SliderDetail *sd = detail;

	if (!sd)
		{
		GtkWidget *hbox;
		GtkWidget *vbox2;
		GtkObject *adj;

		sd = g_new0(SliderDetail, 1);

		hbox = ui_edit_frame_new(vbox, FALSE, NULL);

		sd->vertical_button = ui_edit_toggle_new(hbox, _("Vertical"));
		gtk_widget_set_sensitive(sd->vertical_button, FALSE);

		sd->has_press_button = ui_edit_toggle_new(hbox, _("Press"));
		gtk_widget_set_sensitive(sd->has_press_button, FALSE);

		sd->prelight_button = ui_edit_toggle_new(hbox, _("Prelight"));
		gtk_widget_set_sensitive(sd->prelight_button, FALSE);

		vbox2 = ui_edit_vframe_new(vbox, TRUE, _("Trough"));

		hbox = ui_edit_frame_new(vbox2, FALSE, NULL);

		sd->real_length_spin = ui_edit_spin_new(hbox, _("Length:"), 1, SKIN_SIZE_MAX, &adj);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(slider_edit_props_real_length_cb), sd);

		sd->stretch_button = ui_edit_toggle_new(hbox, _("Stretch"));
		g_signal_connect(G_OBJECT(sd->stretch_button), "clicked",
				 G_CALLBACK(slider_edit_props_stretch_cb), sd);

		hbox = ui_edit_frame_new(vbox2, FALSE, NULL);

		sd->border_1_spin = ui_edit_spin_new(hbox, _("Border 1:"), 0, SKIN_SIZE_MAX, &adj);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(slider_edit_props_border_1_cb), sd);

		sd->border_2_spin = ui_edit_spin_new(hbox, _("Border 2:"), 0, SKIN_SIZE_MAX, &adj);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(slider_edit_props_border_2_cb), sd);

		sd->sizeable_button = ui_edit_toggle_new(vbox, _("Dynamic size"));
		g_signal_connect(G_OBJECT(sd->sizeable_button), "clicked",
				 G_CALLBACK(slider_edit_props_sizeable_cb), sd);

		sd->reverse_button = ui_edit_toggle_new(vbox, _("Reverse"));
		g_signal_connect(G_OBJECT(sd->reverse_button), "clicked",
				 G_CALLBACK(slider_edit_props_reversed_cb), sd);

		sd->item_key_entry = ui_edit_entry_new(vbox, _("Item link key:") );
		g_signal_connect(G_OBJECT(sd->item_key_entry), "changed",
				 G_CALLBACK(slider_edit_props_item_key_cb), sd);
		}

	sd->slider = slider;
	sd->wd = wd;
	sd->ui = ui;

	ui_edit_toggle_set_blocking(sd->reverse_button, slider->reversed, sd);

	ui_edit_toggle_set(sd->vertical_button, slider->vertical);
	ui_edit_toggle_set(sd->has_press_button, slider->has_press);
	ui_edit_toggle_set(sd->prelight_button, slider->has_prelight);

	ui_edit_spin_set_blocking(sd->real_length_spin, slider->real_length, sd);
	ui_edit_spin_set_blocking(sd->border_1_spin, slider->border_1, sd);
	ui_edit_spin_set_blocking(sd->border_2_spin, slider->border_2, sd);
	ui_edit_toggle_set_blocking(sd->stretch_button, slider->stretch, sd);

	ui_edit_toggle_set_blocking(sd->sizeable_button, slider->sizeable, sd);

	ui_edit_entry_set_blocking(sd->item_key_entry, slider->item_key, sd);

	return sd;
}

static void slider_edit_page_add_cb(GtkWidget *widget, gpointer data)
{
	SliderPage *pd = data;
	SliderData *slider;
	const gchar *key;
	const gchar *text_id;
	const gchar *image;
	const gchar *item_key;

	key = ui_edit_entry_get(pd->key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	image = ui_edit_entry_get(pd->image_entry);
	item_key = ui_edit_entry_get(pd->item_key_entry);

	if (!key || !image || !isfile(image))
		{
		warning_dialog(_("Slider error"), _("Slider must contain a key and valid image."));
		return;
		}

	slider = slider_new_from_file(image, 0, 0,
				      ui_edit_spin_get(pd->length_spin),
				      ui_edit_toggle_get(pd->vertical_button),
				      ui_edit_toggle_get(pd->reverse_button),
				      ui_edit_toggle_get(pd->has_press_button),
				      ui_edit_toggle_get(pd->has_prelight_button),
				      item_key);
	slider_set_attributes(slider, ui_edit_toggle_get(pd->sizeable_button),
			      ui_edit_spin_get(pd->real_length_spin),
			      ui_edit_spin_get(pd->border_1_spin),
			      ui_edit_spin_get(pd->border_2_spin),
			      ui_edit_toggle_get(pd->stretch_button));

	if (slider)
		{
		WidgetData *wd;
		wd = slider_register(pd->ed->ui->skin, slider, key, text_id);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	tab_completion_append_to_history(pd->image_entry, image);
}

static void slider_edit_page_sync(SliderPage *pd, SliderListData *sd)
{
	if (!sd) return;

	ui_edit_entry_set(pd->key_entry, sd->key);
	ui_edit_entry_set(pd->text_id_entry, sd->text_id);
	ui_edit_entry_set(pd->data_entry, sd->data);

	ui_edit_entry_set(pd->image_entry, sd->image);

	ui_edit_toggle_set(pd->vertical_button, sd->vertical);
	ui_edit_toggle_set(pd->reverse_button, sd->reversed);
	ui_edit_toggle_set(pd->has_press_button, sd->has_press);
	ui_edit_toggle_set(pd->has_prelight_button, sd->has_prelight);

	ui_edit_spin_set(pd->length_spin, sd->length);

	ui_edit_spin_set(pd->real_length_spin, sd->real_length);
	ui_edit_spin_set(pd->border_1_spin, sd->border_1);
	ui_edit_spin_set(pd->border_2_spin, sd->border_2);
	ui_edit_toggle_set(pd->stretch_button, sd->stretch);

	ui_edit_toggle_set(pd->sizeable_button, sd->sizeable);

	ui_edit_entry_set(pd->item_key_entry, sd->item_key);
}

static void slider_edit_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	SliderPage *pd = data;
	SliderListData *sd;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, SLIDER_COLUMN_POINTER, &sd, -1);
	slider_edit_page_sync(pd, sd);
}

static void slider_edit_page_destroy_cb(GtkWidget *widget, gpointer data)
{
	SliderPage *pd = data;

	g_free(pd);
}

static GtkWidget *slider_edit_page_new(EditData *ed)
{
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *button;
	SliderPage *pd;
	GtkListStore *store;

	pd = g_new0(SliderPage, 1);
	pd->ed = ed;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	g_object_set_data(G_OBJECT(hbox), "page", pd);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(slider_edit_page_destroy_cb), pd);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	pd->key_entry = ui_edit_key_entry_new(vbox, ed->ui, slider_type_id());
	pd->data_entry = ui_edit_entry_new(vbox, _("Data:"));
	pd->text_id_entry = ui_edit_entry_new(vbox, _("Text id:"));

	hbox2 = ui_edit_frame_new(vbox, FALSE, NULL);
	pd->vertical_button = ui_edit_toggle_new(hbox2, _("Vertical"));
	pd->has_press_button = ui_edit_toggle_new(hbox2, _("Press"));
	pd->has_prelight_button = ui_edit_toggle_new(hbox2, _("Prelight"));

	vbox2 = ui_edit_vframe_new(vbox, TRUE, _("Trough"));
	hbox2 = ui_edit_frame_new(vbox2, FALSE, NULL);
	pd->real_length_spin = ui_edit_spin_new(hbox2, _("Length:"), 1, SKIN_SIZE_MAX, NULL);
	pd->stretch_button = ui_edit_toggle_new(hbox2, _("Stretch"));
	hbox2 = ui_edit_frame_new(vbox2, FALSE, NULL);;
	pd->border_1_spin = ui_edit_spin_new(hbox2, _("Border 1:"), 0, SKIN_SIZE_MAX, NULL);
	pd->border_2_spin = ui_edit_spin_new(hbox2, _("Border 2:"), 0, SKIN_SIZE_MAX, NULL);

	pd->sizeable_button = ui_edit_toggle_new(vbox, _("Dynamic size"));

	pd->reverse_button = ui_edit_toggle_new(vbox, _("Reverse"));
	pd->length_spin = ui_edit_spin_new(vbox, _("Trough length:"), 1, SKIN_SIZE_MAX, NULL);

	pd->image_entry = ui_edit_path_entry_new(vbox, _("Image:"), "SLIK_slider_image");
	pd->image = ui_edit_image_new(vbox);
	ui_edit_path_entry_connect_image(pd->image_entry, pd->image);

	pd->item_key_entry = ui_edit_entry_new(vbox, _("Item link key:"));

	button = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(slider_edit_page_add_cb), pd);
	gtk_widget_show(button);

	store = gtk_list_store_new(6, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING,
				      G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	pd->list = ui_edit_list_new(hbox, store,
				    G_CALLBACK(slider_edit_page_list_cb), pd);
	g_object_unref(store);

	ui_edit_list_add_column(pd->list, _("Image"), SLIDER_COLUMN_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(pd->list, _("Key"), SLIDER_COLUMN_KEY, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Vertical"), SLIDER_COLUMN_VERTICAL, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Press"), SLIDER_COLUMN_PRESS, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Prelight"), SLIDER_COLUMN_PRELIGHT, FALSE, FALSE);

	gtk_widget_show(hbox);

	return hbox;
}

static void slider_edit_page_add(GtkWidget *widget, gpointer data)
{
	SliderListData *sd = data;
	SliderPage *pd;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	pd = g_object_get_data(G_OBJECT(widget), "page");
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->list)));

	pixbuf = ui_edit_list_pixbuf_from_file(sd->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   SLIDER_COLUMN_POINTER, sd,
			   SLIDER_COLUMN_IMAGE, pixbuf,
			   SLIDER_COLUMN_KEY, sd->key,
			   SLIDER_COLUMN_VERTICAL, sd->vertical,
			   SLIDER_COLUMN_PRESS, sd->has_press,
			   SLIDER_COLUMN_PRELIGHT, sd->has_prelight, -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

void slider_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = slider_get_pixbuf;

	od->func_edit_write = slider_edit_write;

	od->func_edit_read = slider_edit_read;
	od->func_edit_free = slider_edit_free;

	od->func_edit_props = slider_edit_props;

	od->func_edit_page_new = slider_edit_page_new;
	od->func_edit_page_add = slider_edit_page_add;
}


