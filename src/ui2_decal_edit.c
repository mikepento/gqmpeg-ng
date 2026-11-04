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
#include "ui2_typedefs.h"

#include "ui2_decal.h"
#include "ui2_decal_edit.h"

#include "ui2_display.h"
#include "ui2_editor.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "ui_tabcomp.h"
#include "ui_utildlg.h"


enum {
	DECAL_COLUMN_POINTER = 0,
	DECAL_COLUMN_IMAGE,
	DECAL_COLUMN_KEY,
	DECAL_COLUMN_COUNT
};


typedef struct _DecalDetail DecalDetail;
struct _DecalDetail
{
	DecalData *decal;

	GtkWidget *image_entry;

	GtkWidget *rect_r_spin;
	GtkWidget *rect_g_spin;
	GtkWidget *rect_b_spin;
	GtkWidget *rect_fill_button;

	GtkWidget *border_left_spin;
	GtkWidget *border_right_spin;
	GtkWidget *border_top_spin;
	GtkWidget *border_bottom_spin;
	GtkWidget *border_stretch_button;
	GtkWidget *stretch_button;

	GtkWidget *sizeable_button;
	GtkWidget *alpha_spin;

	GtkWidget *width_spin;
	GtkWidget *height_spin;
	GtkWidget *reset_button;

	WidgetData *wd;
	UIData *ui;
};

typedef struct _DecalListData DecalListData;
struct _DecalListData
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

	gint alpha;
	gint sizeable;

	gint width;
	gint height;
};

typedef struct _DecalPage DecalPage;
struct _DecalPage
{
	DecalDetail dd;

	GtkWidget *key_entry;
	GtkWidget *text_id_entry;
	GtkWidget *data_entry;

	GtkWidget *list;

	EditData *ed;
};


static GdkPixbuf *decal_get_pixbuf(gpointer data)
{
	DecalData *decal = data;
	return decal->overlay;
}

static void decal_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	DecalData *decal = wd->widget;
	gchar *image;

	image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "image"),
					 dir, decal->overlay, "decal_", wd->key);

	if (image) ui_edit_widget_set_path_key(wd, "image", dir, image);

	ui_edit_write_section(f, "decal", wd->key);

	ui_edit_write_key_int(f, "x", decal->x);
	ui_edit_write_key_int(f, "y", decal->y);
	ui_edit_write_key_int(f, "width", decal->width);
	ui_edit_write_key_int(f, "height", decal->height);

	ui_edit_write_key_bool(f, "sizeable", decal->sizeable);
	ui_edit_write_key_int(f, "alpha", decal->alpha);

	ui_edit_write_key_int(f, "border_left", decal->border_left);
	ui_edit_write_key_int(f, "border_right", decal->border_right);
	ui_edit_write_key_int(f, "border_top", decal->border_top);
	ui_edit_write_key_int(f, "border_bottom", decal->border_bottom);

	if (image)
		{
		ui_edit_write_key_char(f, "image", image);
		ui_edit_write_key_bool(f, "stretch", decal->stretch);
		ui_edit_write_key_bool(f, "border_stretch", decal->border_stretch);
		}
	else
		{
		ui_edit_write_key_int(f, "rect_red", decal->rect_r);
		ui_edit_write_key_int(f, "rect_green", decal->rect_g);
		ui_edit_write_key_int(f, "rect_blue", decal->rect_b);
		ui_edit_write_key_int(f, "rect_fill", decal->rect_fill);
		}

	g_free(image);
}

static DecalListData *decal_edit_list_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while(work)
		{
		DecalListData *id = work->data;
		if (strcmp(image, id->image) == 0) return id;
		work = work->next;
		}
	return NULL;
}

static gpointer decal_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	DecalData *decal = wd->widget;
	DecalListData *id;
	const gchar *image;

	image = ui_widget_get_data(wd, "image");

	if (!image || decal_edit_list_find(list, image)) return NULL;

	id = g_new0(DecalListData, 1);
	id->image = g_strdup(image);
	id->width = decal->width;
	id->height = decal->height;
	id->sizeable = decal->sizeable;

	id->border_left = decal->border_left;
	id->border_right = decal->border_right;
	id->border_top = decal->border_top;
	id->border_bottom = decal->border_bottom;
	id->border_stretch = decal->border_stretch;
	id->stretch = decal->stretch;
	id->alpha = decal->alpha;

	id->key = g_strdup(wd->key);
	id->data = g_strdup(ui_widget_get_data(wd, "data"));
	id->text_id = g_strdup(wd->text_id);

	return id;
}

static void decal_edit_free(gpointer data)
{
	DecalListData *id = data;
	g_free(id->key);
	g_free(id->text_id);
	g_free(id->data);
	g_free(id->image);
	g_free(id);
}

static void decal_details_pack(DecalDetail *dd, GtkWidget *box, gint add_page)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *vbox2;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), vbox, TRUE, TRUE, 0);
	gtk_widget_show(vbox);

	vbox2 = ui_edit_vframe_new(vbox, TRUE, _("Size"));

	hbox = ui_edit_frame_new(vbox2, FALSE, NULL);
	dd->width_spin = ui_edit_spin_new(hbox, _("Width:"), 1, 2000, NULL);
	dd->height_spin = ui_edit_spin_new(hbox, _("Height:"), 1, 2000, NULL);
	hbox = ui_edit_frame_new(vbox2, FALSE, NULL);
	dd->sizeable_button = ui_edit_toggle_new(hbox, _("Dynamic"));
	if (!add_page)
		{
		dd->reset_button = gtk_button_new_with_label("Reset to image");
		gtk_box_pack_start(GTK_BOX(hbox), dd->reset_button, FALSE, FALSE, 0);
		gtk_widget_show(dd->reset_button);
		}
	else
		{
		dd->reset_button = NULL;
		}

	vbox2 = ui_edit_vframe_new(vbox, TRUE, _("Border"));

	hbox = ui_edit_frame_new(vbox2, FALSE, NULL);
	dd->border_left_spin = ui_edit_spin_new(hbox, _("Left:"), 0, 1000, NULL);
	dd->border_right_spin = ui_edit_spin_new(hbox, _("Right:"), 0, 1000, NULL);
	hbox = ui_edit_frame_new(vbox2, FALSE, NULL);
	dd->border_top_spin = ui_edit_spin_new(hbox, _("Top:"), 0, 1000, NULL);
	dd->border_bottom_spin = ui_edit_spin_new(hbox, _("Bottom:"), 0, 1000, NULL);
	dd->border_stretch_button = ui_edit_toggle_new(vbox2, _("Stretch"));

	vbox2 = ui_edit_vframe_new(vbox, TRUE, _("Color"));

	hbox = ui_edit_frame_new(vbox2, FALSE, NULL);
	dd->rect_r_spin = ui_edit_spin_new(hbox, _("Red:"), 0, 255, NULL);
	dd->rect_g_spin = ui_edit_spin_new(hbox, _("Green:"), 0, 255, NULL);
	dd->rect_b_spin = ui_edit_spin_new(hbox, _("Blue:"), 0, 255, NULL);

	dd->rect_fill_button = ui_edit_toggle_new(vbox2, _("Fill"));

	if (add_page)
		{
		GtkWidget *image;

		dd->image_entry = ui_edit_path_entry_new(vbox, _("Image:"), "SLIK_decal_image");
		image = ui_edit_image_new(vbox);
		ui_edit_path_entry_connect_image(dd->image_entry, image);
		}

	hbox = ui_edit_frame_new(vbox, FALSE, NULL);
	dd->alpha_spin = ui_edit_spin_new(hbox, _("Alpha:"), 0, 255, NULL);
	dd->stretch_button = ui_edit_toggle_new(hbox, _("Stretch center"));
}

static void decal_edit_details_image_sensitive(DecalDetail *dd, gint active)
{
	gtk_widget_set_sensitive(dd->border_stretch_button, active);
	ui_edit_frame_sensitive(dd->rect_r_spin, !active, FALSE);
	gtk_widget_set_sensitive(dd->stretch_button, active);

	if (dd->reset_button) gtk_widget_set_sensitive(dd->reset_button, active);
}

static void decal_props_border_cb(GtkObject *object, gpointer data)
{
	DecalDetail *dd = data;
	DecalData *decal;

	decal = dd->decal;

	decal->border_left = MIN(ui_edit_spin_get(dd->border_left_spin), decal->width);
	decal->border_right = MIN(ui_edit_spin_get(dd->border_right_spin), decal->width);
	decal->border_top = MIN(ui_edit_spin_get(dd->border_top_spin), decal->height);
	decal->border_bottom = MIN(ui_edit_spin_get(dd->border_bottom_spin), decal->height);

	ui_display_sync_all(dd->ui);
	ui_edit_widget_draw_highlight(dd->ui, dd->wd, TRUE);
}

static void decal_props_geometry_cb(GtkObject *object, gpointer data)
{
	DecalDetail *dd = data;
	gint old_w, old_h;
	gint w, h;

	old_w = dd->decal->width;
	old_h = dd->decal->height;

	w = ui_edit_spin_get(dd->width_spin);
	h = ui_edit_spin_get(dd->height_spin);

	if (w > dd->ui->skin->width)
		{
		w = dd->ui->skin->width;
		ui_edit_spin_set_blocking(dd->width_spin, w, dd);
		}
	if (dd->decal->x + w > dd->ui->skin->width)
		{
		ui_widget_set_coord(dd->ui, dd->wd, dd->ui->skin->width - w, dd->decal->y, TRUE);
		}
	if (h > dd->ui->skin->height)
		{
		h = dd->ui->skin->height;
		ui_edit_spin_set_blocking(dd->height_spin, h, dd);
		}
	if (dd->decal->y + h > dd->ui->skin->height)
		{
		ui_widget_set_coord(dd->ui, dd->wd, dd->decal->x, dd->ui->skin->height - h, TRUE);
		}

	if (old_w == w && old_h == h) return;

	dd->decal->width = w;
	dd->decal->height = h;

	decal_props_border_cb(object, data);
}

static void decal_props_reset_cb(GtkWidget *widget, gpointer data)
{
	DecalDetail *dd = data;

	if (!dd->decal->overlay) return;

	dd->decal->width = gdk_pixbuf_get_width(dd->decal->overlay);
	dd->decal->height = gdk_pixbuf_get_height(dd->decal->overlay);

	ui_edit_spin_set_blocking(dd->width_spin, dd->decal->width, dd);
	ui_edit_spin_set_blocking(dd->height_spin, dd->decal->height, dd);

	decal_props_border_cb(GTK_OBJECT(widget), data);
}

static void decal_props_generic_cb(GtkObject *object, gpointer data)
{
	DecalDetail *dd = data;
	DecalData *decal;

	decal = dd->decal;

	decal->sizeable = ui_edit_toggle_get(dd->sizeable_button);
	decal->border_stretch = ui_edit_toggle_get(dd->border_stretch_button);
	decal->stretch = ui_edit_toggle_get(dd->stretch_button);

	decal->rect_r = ui_edit_spin_get(dd->rect_r_spin);
	decal->rect_g = ui_edit_spin_get(dd->rect_g_spin);
	decal->rect_b = ui_edit_spin_get(dd->rect_b_spin);
	decal->rect_fill = ui_edit_toggle_get(dd->rect_fill_button);

	decal->alpha = ui_edit_spin_get(dd->alpha_spin);

	ui_display_sync_all(dd->ui);
	ui_edit_widget_draw_highlight(dd->ui, dd->wd, TRUE);
}

static void decal_props_connect_spin(GtkWidget *spin, DecalDetail *dd, GtkSignalFunc func)
{
	GtkAdjustment *adj;

	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	g_signal_connect(G_OBJECT(adj), "value_changed", func, dd);
}

static void decal_props_signals(DecalDetail *dd)
{
	decal_props_connect_spin(dd->width_spin, dd, G_CALLBACK(decal_props_geometry_cb));
	decal_props_connect_spin(dd->height_spin, dd, G_CALLBACK(decal_props_geometry_cb));

	decal_props_connect_spin(dd->border_left_spin, dd, G_CALLBACK(decal_props_border_cb));
	decal_props_connect_spin(dd->border_right_spin, dd, G_CALLBACK(decal_props_border_cb));
	decal_props_connect_spin(dd->border_top_spin, dd, G_CALLBACK(decal_props_border_cb));
	decal_props_connect_spin(dd->border_bottom_spin, dd, G_CALLBACK(decal_props_border_cb));

	decal_props_connect_spin(dd->rect_r_spin, dd, G_CALLBACK(decal_props_generic_cb));
	decal_props_connect_spin(dd->rect_g_spin, dd, G_CALLBACK(decal_props_generic_cb));
	decal_props_connect_spin(dd->rect_b_spin, dd, G_CALLBACK(decal_props_generic_cb));
	decal_props_connect_spin(dd->alpha_spin, dd, G_CALLBACK(decal_props_generic_cb));

	g_signal_connect(G_OBJECT(dd->sizeable_button), "toggled",
			 G_CALLBACK(decal_props_generic_cb), dd);
	g_signal_connect(G_OBJECT(dd->border_stretch_button), "toggled",
			 G_CALLBACK(decal_props_generic_cb), dd);
	g_signal_connect(G_OBJECT(dd->stretch_button), "toggled",
			 G_CALLBACK(decal_props_generic_cb), dd);
	g_signal_connect(G_OBJECT(dd->rect_fill_button), "toggled",
			 G_CALLBACK(decal_props_generic_cb), dd);

	g_signal_connect(G_OBJECT(dd->reset_button), "clicked",
			 G_CALLBACK(decal_props_reset_cb), dd);
}

static gpointer decal_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail)
{
	DecalData *decal = wd->widget;
	DecalDetail *dd = detail;

	if (!dd)
		{
		dd = g_new0(DecalDetail, 1);

		decal_details_pack(dd, vbox, FALSE);
		decal_props_signals(dd);
		}

	dd->decal = decal;
	dd->wd = wd;
	dd->ui = ui;

	ui_edit_spin_set_blocking(dd->width_spin, decal->width, dd);
	ui_edit_spin_set_blocking(dd->height_spin, decal->height, dd);
	ui_edit_toggle_set_blocking(dd->sizeable_button, decal->sizeable, dd);
	ui_edit_spin_set_blocking(dd->alpha_spin, decal->alpha, dd);

	ui_edit_toggle_set_blocking(dd->stretch_button, decal->stretch, dd);
	ui_edit_toggle_set_blocking(dd->border_stretch_button, decal->border_stretch, dd);

	ui_edit_spin_set_blocking(dd->border_left_spin, decal->border_left, dd);
	ui_edit_spin_set_blocking(dd->border_right_spin, decal->border_right, dd);
	ui_edit_spin_set_blocking(dd->border_top_spin, decal->border_top, dd);
	ui_edit_spin_set_blocking(dd->border_bottom_spin, decal->border_bottom, dd);

	ui_edit_spin_set_blocking(dd->rect_r_spin, decal->rect_r, dd);
	ui_edit_spin_set_blocking(dd->rect_g_spin, decal->rect_g, dd);
	ui_edit_spin_set_blocking(dd->rect_b_spin, decal->rect_b, dd);
	ui_edit_toggle_set_blocking(dd->rect_fill_button, decal->rect_fill, dd);

	decal_edit_details_image_sensitive(dd, (decal->overlay != NULL));

	return dd;
}

static void decal_edit_page_add_cb(GtkWidget *widget, gpointer data)
{
	DecalPage *pd = data;
	DecalDetail *dd;
	DecalData *decal;
	const gchar *key;
	const gchar *text_id;
	const gchar *image;

	dd = (DecalDetail *)pd;

	key = ui_edit_entry_get(pd->key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	image = ui_edit_entry_get(dd->image_entry);

	if (image && !isfile(image))
		{
		warning_dialog(_("Decal warning"), _("Image not found, new decal is color only."));
		}

	if (!key) key = "unnamed";

	decal = decal_new(0, 0,
			  ui_edit_spin_get(dd->width_spin),
			  ui_edit_spin_get(dd->height_spin),
			  ui_edit_toggle_get(dd->sizeable_button),
			  ui_edit_spin_get(dd->alpha_spin));
	decal_set_image(decal, NULL, NULL, image,
			ui_edit_toggle_get(dd->stretch_button),
			ui_edit_toggle_get(dd->border_stretch_button));
	decal_set_color(decal,
			ui_edit_spin_get(dd->rect_r_spin),
			ui_edit_spin_get(dd->rect_g_spin),
			ui_edit_spin_get(dd->rect_b_spin),
			ui_edit_toggle_get(dd->rect_fill_button));
	decal_set_border(decal,
			 ui_edit_spin_get(dd->border_left_spin),
			 ui_edit_spin_get(dd->border_right_spin),
			 ui_edit_spin_get(dd->border_top_spin),
			 ui_edit_spin_get(dd->border_bottom_spin));
	if (!decal->overlay &&
	    decal->border_left < 1 && decal->border_right < 1 &&
	    decal->border_top < 1 && decal->border_bottom < 1)
		{
		decal_set_border(decal, 1, 1, 1, 1);
		}

	if (decal)
		{
		WidgetData *wd;
		wd = decal_register(pd->ed->ui->skin, decal, key, text_id);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	if (image) tab_completion_append_to_history(dd->image_entry, image);
}

static void decal_edit_page_sync(DecalPage *pd, DecalListData *id)
{
	DecalDetail *dd;

	if (!id) return;
	dd = (DecalDetail *)pd;

	ui_edit_entry_set(pd->key_entry, id->key);
	ui_edit_entry_set(pd->text_id_entry, id->text_id);
	ui_edit_entry_set(pd->data_entry, id->data);

	ui_edit_entry_set(dd->image_entry, id->image);

	ui_edit_spin_set(dd->width_spin, id->width);
	ui_edit_spin_set(dd->height_spin, id->height);
	ui_edit_toggle_set(dd->sizeable_button, id->sizeable);
	ui_edit_spin_set(dd->alpha_spin, id->alpha);

	ui_edit_toggle_set(dd->stretch_button, id->stretch);
	ui_edit_toggle_set(dd->border_stretch_button, id->border_stretch);

	ui_edit_spin_set(dd->border_left_spin, id->border_left);
	ui_edit_spin_set(dd->border_right_spin, id->border_right);
	ui_edit_spin_set(dd->border_top_spin, id->border_top);
	ui_edit_spin_set(dd->border_bottom_spin, id->border_bottom);
}

static void decal_edit_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	DecalPage *pd = data;
	DecalListData *id;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, DECAL_COLUMN_POINTER, &id, -1);
	decal_edit_page_sync(pd, id);
}

static void decal_edit_page_destroy_cb(GtkWidget *widget, gpointer data)
{
	DecalPage *pd = data;

	g_free(pd);
}

static void decal_edit_image_change_cb(GtkWidget *widget, gpointer data)
{
	DecalDetail *dd = data;

	decal_edit_details_image_sensitive(dd, (strlen(gtk_entry_get_text(GTK_ENTRY(widget))) > 0));
}

static GtkWidget *decal_edit_page_new(EditData *ed)
{
	DecalDetail *dd;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *button;
	DecalPage *pd;
	GtkListStore *store;

	pd = g_new0(DecalPage, 1);
	pd->ed = ed;

	dd = (DecalDetail *)pd;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	g_object_set_data(G_OBJECT(hbox), "page", pd);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(decal_edit_page_destroy_cb), pd);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	pd->key_entry = ui_edit_key_entry_new(vbox, ed->ui, decal_type_id());
	pd->data_entry = ui_edit_entry_new(vbox, _("Data:"));
	pd->text_id_entry = ui_edit_entry_new(vbox, _("Text id:"));

	decal_details_pack(dd, vbox, TRUE);

	g_signal_connect(G_OBJECT(dd->image_entry), "changed",
			 G_CALLBACK(decal_edit_image_change_cb), dd);


	ui_edit_spin_set(dd->alpha_spin, 255);
	ui_edit_spin_set(dd->width_spin, 10);
	ui_edit_spin_set(dd->height_spin, 10);

	button = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(decal_edit_page_add_cb), pd);
	gtk_widget_show(button);

	store = gtk_list_store_new(3, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	pd->list = ui_edit_list_new(hbox, store,
				    G_CALLBACK(decal_edit_page_list_cb), pd);
	g_object_unref(store);

	ui_edit_list_add_column(pd->list, _("Image"), DECAL_COLUMN_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(pd->list, _("Key"), DECAL_COLUMN_KEY, FALSE, FALSE);

	gtk_widget_show(hbox);

	return hbox;
}

static void decal_edit_page_add(GtkWidget *widget, gpointer data)
{
	DecalListData *id = data;
	DecalPage *pd;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	pd = g_object_get_data(G_OBJECT(widget), "page");
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->list)));

	pixbuf = ui_edit_list_pixbuf_from_file(id->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   DECAL_COLUMN_POINTER, id,
			   DECAL_COLUMN_IMAGE, pixbuf,
			   DECAL_COLUMN_KEY, id->key, -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

void decal_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = decal_get_pixbuf;

	od->func_edit_write = decal_edit_write;

	od->func_edit_read = decal_edit_read;
	od->func_edit_free = decal_edit_free;

	od->func_edit_props = decal_edit_props;

	od->func_edit_page_new = decal_edit_page_new;
	od->func_edit_page_add = decal_edit_page_add;
}
