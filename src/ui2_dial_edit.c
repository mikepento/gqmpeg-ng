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

#include "ui2_dial.h"
#include "ui2_dial_edit.h"

#include "ui2_display.h"
#include "ui2_editor.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "ui_tabcomp.h"
#include "ui_utildlg.h"


enum {
	DIAL_COLUMN_POINTER = 0,
	DIAL_COLUMN_IMAGE,
	DIAL_COLUMN_KEY,
	DIAL_COLUMN_PRESS,
	DIAL_COLUMN_PRELIGHT,
	DIAL_COLUMN_COUNT
};


typedef struct _DialDetail DialDetail;
struct _DialDetail
{
	DialData *dial;
	GtkWidget *reverse_button;
	GtkWidget *item_key_entry;
	GtkWidget *spin_center_x;
	GtkWidget *spin_center_y;
	GtkWidget *spin_handle_x;
	GtkWidget *spin_handle_y;
	GtkWidget *spin_begin;
	GtkWidget *spin_end;
	GtkWidget *spin_clip_w;
	GtkWidget *spin_clip_h;
	GtkWidget *has_press_button;
	GtkWidget *prelight_button;
	GtkWidget *fixed_button;
	GtkWidget *radius_spin;

	WidgetData *wd;
	UIData *ui;
};

typedef struct _DialListData DialListData;
struct _DialListData
{
	gchar *key;
	gchar *text_id;
	gchar *data;

	gchar *image;
	gint angle_start;
	gint angle_end;
	gint center_x;
	gint center_y;
	gint clip_x;
	gint clip_y;
	gint clip_w;
	gint clip_h;
	gint offset_x;
	gint offset_y;
	gint has_press;
	gint has_prelight;
	gint reversed;
	gchar *clip_mask;

	gint fixed_handle;
	gint fixed_radius;

	gchar *item_key;
};

typedef struct _DialPage DialPage;
struct _DialPage
{
	GtkWidget *key_entry;
	GtkWidget *text_id_entry;
	GtkWidget *data_entry;

	GtkWidget *image_entry;
	GtkWidget *angle_start_spin;
	GtkWidget *angle_end_spin;
	GtkWidget *center_x_spin;
	GtkWidget *center_y_spin;
	GtkWidget *clip_w_spin;
	GtkWidget *clip_h_spin;
	GtkWidget *offset_x_spin;
	GtkWidget *offset_y_spin;

	GtkWidget *reverse_button;
	GtkWidget *has_press_button;
	GtkWidget *has_prelight_button;

	GtkWidget *fixed_button;
	GtkWidget *radius_spin;

	GtkWidget *clip_mask_entry;
	GtkWidget *item_key_entry;

	GtkWidget *list;
	GtkWidget *image;

	EditData *ed;
};


static GdkPixbuf *dial_get_pixbuf(gpointer data)
{
	DialData *dial = data;
	return dial->overlay;
}

static void dial_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	DialData *dial = wd->widget;
	gchar *image;
	gchar *clip_mask;

	image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "image"),
					 dir, dial->overlay, "dial", wd->key);
	if (dial->clip_mask)
		{
		clip_mask = ui_edit_copy_unique_file(ui_widget_get_data(wd, "clip_mask"),
						     dir, dial->pixbuf, "dial_mask_", wd->key);
		}
	else
		{
		clip_mask = NULL;
		}

	if (image) ui_edit_widget_set_path_key(wd, "image", dir, image);
	if (clip_mask) ui_edit_widget_set_path_key(wd, "clip_mask", dir, clip_mask);

	ui_edit_write_section(f, "dial", wd->key);

	ui_edit_write_key_char(f, "image", image);
	ui_edit_write_key_int(f, "axis_x", dial->center_x);
	ui_edit_write_key_int(f, "axis_y", dial->center_y);
	ui_edit_write_key_int(f, "angle_begin", dial->angle_start);
	ui_edit_write_key_int(f, "angle_end", dial->angle_end);
	ui_edit_write_key_int(f, "handle_offset_x", dial->offset_x);
	ui_edit_write_key_int(f, "handle_offset_y", dial->offset_y);
	ui_edit_write_key_int(f, "clip_x", dial->x);
	ui_edit_write_key_int(f, "clip_y", dial->y);
	ui_edit_write_key_int(f, "clip_width", dial->width);
	ui_edit_write_key_int(f, "clip_height", dial->height);

	ui_edit_write_key_bool(f, "pressable", dial->has_press);
	ui_edit_write_key_bool(f, "prelight", dial->has_prelight);
	ui_edit_write_key_bool(f, "reversed", dial->reversed);
	if (clip_mask) ui_edit_write_key_char(f, "clip_mask", clip_mask);
	if (dial->item_key) ui_edit_write_key_char(f, "item_link", dial->item_key);

	if (dial->fixed_handle)
		{
		ui_edit_write_key_bool(f, "fixed_handle", dial->fixed_handle);
		ui_edit_write_key_int(f, "fixed_radius", dial->fixed_radius);
		}

	g_free(image);
	g_free(clip_mask);
}

static DialListData *dial_edit_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while(work)
		{
		DialListData *dd = work->data;
		if (strcmp(image, dd->image) == 0) return dd;
		work = work->next;
		}
	return NULL;
}

static gpointer dial_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	DialData *dial = wd->widget;
	DialListData *dd;
	const gchar *image;

	image = ui_widget_get_data(wd, "image");

	if (!image || dial_edit_find(list, image)) return NULL;

	dd = g_new0(DialListData, 1);
	dd->image = g_strdup(image);

	dd->angle_start = dial->angle_start;
	dd->angle_end = dial->angle_end;
	dd->center_x = dial->center_x;
	dd->center_y = dial->center_y;
	dd->clip_x = dial->x;
	dd->clip_y = dial->y;
	dd->clip_w = dial->width;
	dd->clip_h = dial->height;
	dd->offset_x = dial->offset_x;
	dd->offset_y = dial->offset_y;

	dd->reversed = dial->reversed;

	dd->has_press = dial->has_press;
	dd->has_prelight = dial->has_prelight;
	dd->clip_mask = g_strdup(ui_widget_get_data(wd, "clip_mask"));
	dd->item_key = g_strdup(dial->item_key);

	dd->fixed_handle = dial->fixed_handle;
	dd->fixed_radius = dial->fixed_radius;

	dd->key = g_strdup(wd->key);
	dd->data = g_strdup(ui_widget_get_data(wd, "data"));
	dd->text_id = g_strdup(wd->text_id);

	return dd;
}

static void dial_edit_free(gpointer data)
{
	DialListData *dd = data;

	g_free(dd->key);
	g_free(dd->text_id);
	g_free(dd->data);
	g_free(dd->image);
	g_free(dd->clip_mask);
	g_free(dd->item_key);
	g_free(dd);
}

static void dial_edit_props_reversed_cb(GtkWidget *button, gpointer data)
{
	DialDetail *dd = data;

	dd->dial->reversed = GTK_TOGGLE_BUTTON(button)->active;

	ui_widget_draw(dd->ui, dd->wd, TRUE, TRUE);
}

static void dial_edit_props_item_key_cb(GtkWidget *entry, gpointer data)
{
	DialDetail *dd = data;
	const gchar *buf;

	g_free(dd->dial->item_key);
	buf = gtk_entry_get_text(GTK_ENTRY(entry));
	if (buf && strlen(buf) > 0)
		{
		dd->dial->item_key = g_strdup(buf);
		}
	else
		{
		dd->dial->item_key = NULL;
		}

	ui_widget_draw(dd->ui, dd->wd, TRUE, TRUE);
}

static void dial_edit_props_spin_cb(GtkAdjustment *adjustment, gpointer data)
{
	DialDetail *dd = data;
	gint angle_start;
	gint angle_end;
	gint update = TRUE;

	dd->dial->center_x = ui_edit_spin_get(dd->spin_center_x);
	dd->dial->center_y = ui_edit_spin_get(dd->spin_center_y);

	dd->dial->offset_x = ui_edit_spin_get(dd->spin_handle_x);
	dd->dial->offset_y = ui_edit_spin_get(dd->spin_handle_y);

	angle_start = ui_edit_spin_get(dd->spin_begin);
	angle_end = ui_edit_spin_get(dd->spin_end);

	if (angle_start != dd->dial->angle_start || angle_end != dd->dial->angle_end)
		{
		gint start;

		update = FALSE;

		start = (angle_start != dd->dial->angle_start);

		/* copied from dial_new */
		dd->dial->angle_start = angle_start;
		dd->dial->angle_end = angle_end;

		dd->dial->angle_length = angle_end - angle_start;
		if (angle_end < angle_start) dd->dial->angle_length += 360;

		dd->dial->value = start ? 0.0 : 1.0;
		dd->dial->position = (float)dd->dial->angle_length * dd->dial->value;
		}

	if (!dd->dial->clip_mask)
		{
		/* resizes are tedious */
		gint x, y, w, h;

		w = ui_edit_spin_get(dd->spin_clip_w);
		h = ui_edit_spin_get(dd->spin_clip_h);
		if (w != dd->dial->width || h != dd->dial->height)
			{
			x = dd->dial->x;
			y = dd->dial->y;
			if (w > dd->ui->skin->width - x)
				{
				x = dd->ui->skin->width - w;
				if (x < 0)
					{
					x = 0;
					w = dd->ui->skin->width;
					}
				}
			if (h > dd->ui->skin->height - y)
				{
				y = dd->ui->skin->height - h;
				if (y < 0)
					{
					y = 0;
					h = dd->ui->skin->height;
					}
				}
			if (w != dd->dial->width || h != dd->dial->height || x != dd->dial->x || y != dd->dial->y)
				{
				gint old_w, old_h;

				ui_edit_widget_draw_highlight(dd->ui, dd->wd, FALSE);

				if (x != dd->dial->x || y != dd->dial->y) ui_widget_set_coord(dd->ui, dd->wd, x, y, TRUE);

				old_w = dd->dial->width;
				old_h = dd->dial->height;

				dd->dial->width = w;
				dd->dial->height = h;

				gdk_pixbuf_unref(dd->dial->pixbuf);
				dd->dial->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);

				ui_edit_widget_resync(dd->ui, dd->wd, TRUE,
						      MAX(0, old_w - w), MAX(0, old_h - h));

				return;
				}
			}
		}

	ui_widget_draw(dd->ui, dd->wd, update, TRUE);
}

static void dial_edit_props_radius_cb(GtkAdjustment *adjustment, gpointer data)
{
	DialDetail *dd = data;

	dd->dial->fixed_radius = ui_edit_spin_get(dd->radius_spin);

	ui_widget_draw(dd->ui, dd->wd, TRUE, TRUE);
}

static void dial_edit_props_fixed_cb(GtkWidget *widget, gpointer data)
{
	DialDetail *dd = data;

	dd->dial->fixed_handle = ui_edit_toggle_get(dd->fixed_button);
	ui_edit_frame_sensitive(dd->radius_spin, dd->dial->fixed_handle, TRUE);
	if (dd->dial->fixed_handle && dd->dial->fixed_radius < 1) dd->dial->fixed_radius = 1;

	ui_widget_draw(dd->ui, dd->wd, TRUE, TRUE);
}

static gpointer dial_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail)
{
	DialData *dial = wd->widget;
	DialDetail *dd = detail;

	if (!dd)
		{
		GtkWidget *hbox;
		GtkWidget *hbox2;
		GtkObject *adj;

		dd = g_new0(DialDetail, 1);

		hbox = ui_edit_frame_new(vbox, FALSE, NULL);

		dd->has_press_button = ui_edit_toggle_new(hbox, _("Press"));
		dd->prelight_button = ui_edit_toggle_new(hbox, _("Prelight"));

		hbox = ui_edit_frame_new(vbox, FALSE, NULL);

		hbox2 = ui_edit_frame_new(hbox, TRUE, _("Rotation axis"));

		dd->spin_center_x = ui_edit_spin_new(hbox2, _("x:"), -1000, 1000, &adj);
		g_signal_connect(adj, "value_changed",
				 G_CALLBACK(dial_edit_props_spin_cb), dd);

		dd->spin_center_y = ui_edit_spin_new(hbox2, _("y:"), -1000, 1000, &adj);
		g_signal_connect(adj, "value_changed",
				 G_CALLBACK(dial_edit_props_spin_cb), dd);

		hbox2 = ui_edit_frame_new(hbox, TRUE, _("Handle rotation axis"));

		dd->spin_handle_x = ui_edit_spin_new(hbox2, _("x:"), -1000, 1000, &adj);
		g_signal_connect(adj, "value_changed",
				 G_CALLBACK(dial_edit_props_spin_cb), dd);

		dd->spin_handle_y = ui_edit_spin_new(hbox2, _("y:"), -1000, 1000, &adj);
		g_signal_connect(adj, "value_changed",
				 G_CALLBACK(dial_edit_props_spin_cb), dd);

		hbox = ui_edit_frame_new(vbox, TRUE, _("Endpoints"));

		dd->spin_begin = ui_edit_spin_new(hbox, _("begin:"), 0, 359, &adj);
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(dd->spin_begin), TRUE);
		g_signal_connect(adj, "value_changed",
				 G_CALLBACK(dial_edit_props_spin_cb), dd);

		dd->spin_end = ui_edit_spin_new(hbox, _("end:"), 0, 359, &adj);
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(dd->spin_end), TRUE);
		g_signal_connect(adj, "value_changed",
				 G_CALLBACK(dial_edit_props_spin_cb), dd);

		dd->reverse_button = ui_edit_toggle_new(hbox, _("Reverse"));
		g_signal_connect(GTK_OBJECT(dd->reverse_button), "clicked",
				 G_CALLBACK(dial_edit_props_reversed_cb), dd);

		hbox = ui_edit_frame_new(vbox, TRUE, _("Clipping area"));

		dd->spin_clip_w = ui_edit_spin_new(hbox, _("width:"), 1, 2000, &adj);
		g_signal_connect(adj, "value_changed",
				 G_CALLBACK(dial_edit_props_spin_cb), dd);

		dd->spin_clip_h = ui_edit_spin_new(hbox, _("height:"), 1, 2000, &adj);
		g_signal_connect(adj, "value_changed",
				 G_CALLBACK(dial_edit_props_spin_cb), dd);

		hbox = ui_edit_frame_new(vbox, FALSE, NULL);
		
		dd->fixed_button = ui_edit_toggle_new(hbox, _("Fixed handle"));
		g_signal_connect(GTK_OBJECT(dd->fixed_button), "clicked",
				 G_CALLBACK(dial_edit_props_fixed_cb), dd);
		dd->radius_spin = ui_edit_spin_new(hbox, _("Radius:"), 1, 2000, &adj);
		g_signal_connect(adj, "value_changed",
				 G_CALLBACK(dial_edit_props_radius_cb), dd);

		hbox = ui_edit_frame_new(vbox, FALSE, NULL);

		dd->item_key_entry = ui_edit_entry_new(hbox, _("Item link key:"));
		g_signal_connect(G_OBJECT(dd->item_key_entry), "changed",
				 G_CALLBACK(dial_edit_props_item_key_cb), dd);
		}

	dd->dial = dial;
	dd->wd = wd;
	dd->ui = ui;

	ui_edit_toggle_set_blocking(dd->reverse_button, dial->reversed, dd);

	ui_edit_toggle_set(dd->has_press_button, dial->has_press);
	ui_edit_toggle_set(dd->prelight_button, dial->has_prelight);

	ui_edit_entry_set_blocking(dd->item_key_entry, dial->item_key, dd);

	ui_edit_spin_set_blocking(dd->spin_center_x, dial->center_x, dd);
	ui_edit_spin_set_blocking(dd->spin_center_y, dial->center_y, dd);

	ui_edit_spin_set_blocking(dd->spin_handle_x, dial->offset_x, dd);
	ui_edit_spin_set_blocking(dd->spin_handle_y, dial->offset_y, dd);

	ui_edit_spin_set_blocking(dd->spin_begin, dial->angle_start, dd);
	ui_edit_spin_set_blocking(dd->spin_end, dial->angle_end, dd);

	ui_edit_frame_sensitive(dd->spin_clip_w,!dial->clip_mask , FALSE);
	ui_edit_spin_set_blocking(dd->spin_clip_w, dial->width, dd);
	ui_edit_spin_set_blocking(dd->spin_clip_h, dial->height, dd);

	ui_edit_toggle_set_blocking(dd->fixed_button, dial->fixed_handle, dd);
	ui_edit_frame_sensitive(dd->radius_spin, dial->fixed_handle, TRUE);
	ui_edit_spin_set_blocking(dd->radius_spin, dial->fixed_radius, dd);

	return dd;
}

static void dial_edit_page_add_cb(GtkWidget *widget, gpointer data)
{
	DialPage *pd = data;
	DialData *dial;
	const gchar *key;
	const gchar *text_id;
	const gchar *image;
	const gchar *clip_mask;
	const gchar *item_key;

	key = ui_edit_entry_get(pd->key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	image = ui_edit_entry_get(pd->image_entry);
	clip_mask = ui_edit_entry_get(pd->clip_mask_entry);
	item_key = ui_edit_entry_get(pd->item_key_entry);

	if (!key || !image || !isfile(image))
		{
		warning_dialog(_("Dial error"), _("Dial must contain a key and valid image."));
		return;
		}

	dial = dial_new_from_file(image, 0, 0,
				  ui_edit_spin_get(pd->center_x_spin),
				  ui_edit_spin_get(pd->center_y_spin),
				  ui_edit_spin_get(pd->angle_start_spin),
				  ui_edit_spin_get(pd->angle_end_spin),
				  ui_edit_spin_get(pd->offset_x_spin),
				  ui_edit_spin_get(pd->offset_y_spin),
				  ui_edit_toggle_get(pd->has_press_button),
				  ui_edit_toggle_get(pd->has_prelight_button),
				  ui_edit_toggle_get(pd->reverse_button),
				  ui_edit_spin_get(pd->clip_w_spin),
				  ui_edit_spin_get(pd->clip_h_spin),
				  clip_mask, item_key);
	dial_set_fixed_handle(dial,
		              ui_edit_toggle_get(pd->fixed_button),
			      ui_edit_spin_get(pd->radius_spin));
	if (dial)
		{
		WidgetData *wd;
		wd = dial_register(pd->ed->ui->skin, dial, key, text_id);
		ui_widget_set_data(wd, "clip_mask", clip_mask);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	tab_completion_append_to_history(pd->image_entry, image);
	tab_completion_append_to_history(pd->clip_mask_entry, clip_mask);
}

static void dial_edit_page_sync(DialPage *pd, DialListData *dd)
{
	if (!dd) return;

	ui_edit_entry_set(pd->key_entry, dd->key);
	ui_edit_entry_set(pd->text_id_entry, dd->text_id);
	ui_edit_entry_set(pd->data_entry, dd->data);

	ui_edit_entry_set(pd->image_entry, dd->image);

	ui_edit_toggle_set(pd->reverse_button, dd->reversed);
	ui_edit_toggle_set(pd->has_press_button, dd->has_press);
	ui_edit_toggle_set(pd->has_prelight_button, dd->has_prelight);

	ui_edit_spin_set(pd->angle_start_spin, dd->angle_start);
	ui_edit_spin_set(pd->angle_end_spin, dd->angle_end);

	ui_edit_spin_set(pd->center_x_spin, dd->center_x - dd->clip_x);
	ui_edit_spin_set(pd->center_y_spin, dd->center_y - dd->clip_y);
	ui_edit_spin_set(pd->clip_w_spin, dd->clip_w);
	ui_edit_spin_set(pd->clip_h_spin, dd->clip_h);
	ui_edit_spin_set(pd->offset_x_spin, dd->offset_x);
	ui_edit_spin_set(pd->offset_y_spin, dd->offset_y);

	ui_edit_toggle_set(pd->fixed_button, dd->fixed_handle);
	ui_edit_spin_set(pd->radius_spin, dd->fixed_radius);

	ui_edit_entry_set(pd->clip_mask_entry, dd->clip_mask);
	ui_edit_entry_set(pd->item_key_entry, dd->item_key);
}

static void dial_edit_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	DialPage *pd = data;
	DialListData *dd;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, DIAL_COLUMN_POINTER, &dd, -1);
	dial_edit_page_sync(pd, dd);
}

static void dial_edit_page_clip_cb(GtkWidget *entry, gpointer data)
{
	DialPage *pd = data;
	gint state;

	state = (strlen(gtk_entry_get_text(GTK_ENTRY(pd->clip_mask_entry))) == 0);

	ui_edit_frame_sensitive(pd->clip_w_spin, state, FALSE);
}

static void dial_edit_page_fixed_cb(GtkWidget *widget, gpointer data)
{
	DialPage *pd = data;

	ui_edit_frame_sensitive(pd->radius_spin, ui_edit_toggle_get(pd->fixed_button), TRUE);
}

static void dial_edit_page_destroy_cb(GtkWidget *widget, gpointer data)
{
	DialPage *pd = data;

	g_free(pd);
}

static GtkWidget *dial_edit_page_new(EditData *ed)
{
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *button;
	DialPage *pd;
	GtkListStore *store;

	pd = g_new0(DialPage, 1);
	pd->ed = ed;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	g_object_set_data(G_OBJECT(hbox), "page", pd);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(dial_edit_page_destroy_cb), pd);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	pd->key_entry = ui_edit_key_entry_new(vbox, ed->ui, dial_type_id());
	pd->data_entry = ui_edit_entry_new(vbox, _("Data:"));
	pd->text_id_entry = ui_edit_entry_new(vbox, _("Text id:"));

	frame = ui_edit_frame_new(vbox, FALSE, NULL);
	pd->has_press_button = ui_edit_toggle_new(frame, _("Press"));
	pd->has_prelight_button = ui_edit_toggle_new(frame, _("Prelight"));

	frame = ui_edit_frame_new(vbox, TRUE, _("Endpoints"));
	pd->angle_start_spin = ui_edit_spin_new(frame, _("Begin:"), 0, 359, NULL);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(pd->angle_start_spin), TRUE);
	pd->angle_end_spin = ui_edit_spin_new(frame, _("End:"), 0, 359, NULL);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(pd->angle_end_spin), TRUE);
	pd->reverse_button = ui_edit_toggle_new(frame, _("Reverse"));

	frame = ui_edit_frame_new(vbox, TRUE, _("Rotation axis"));
	pd->center_x_spin = ui_edit_spin_new(frame, _("x:"), 0, 2000, NULL);
	pd->center_y_spin = ui_edit_spin_new(frame, _("y:"), 0, 2000, NULL);

	frame = ui_edit_frame_new(vbox, TRUE, _("Clipping area"));
	pd->clip_w_spin = ui_edit_spin_new(frame, _("width:"), 1, 2000, NULL);
	pd->clip_h_spin = ui_edit_spin_new(frame, _("height:"), 1, 2000, NULL);

	pd->clip_mask_entry = ui_edit_path_entry_new(vbox, _("Clip mask:"), "SLIK_dial_clip_mask");
	g_signal_connect(G_OBJECT(pd->clip_mask_entry), "changed",
			 G_CALLBACK(dial_edit_page_clip_cb), pd);

	frame = ui_edit_frame_new(vbox, TRUE, _("Handle rotation axis"));
	pd->offset_x_spin = ui_edit_spin_new(frame, _("x:"), -2000, 2000, NULL);
	pd->offset_y_spin = ui_edit_spin_new(frame, _("y:"), -2000, 2000, NULL);

	frame = ui_edit_frame_new(vbox, FALSE, NULL);
	pd->fixed_button = ui_edit_toggle_new(frame, _("Fixed handle"));
	g_signal_connect(G_OBJECT(pd->fixed_button), "clicked",
			 G_CALLBACK(dial_edit_page_fixed_cb), pd);
	pd->radius_spin = ui_edit_spin_new(frame, _("Radius:"), 1, 2000, NULL);
	ui_edit_frame_sensitive(pd->radius_spin, FALSE, TRUE);

	pd->image_entry = ui_edit_path_entry_new(vbox, _("Image:"), "SLIK_dial_image");
	pd->image = ui_edit_image_new(vbox);
	ui_edit_path_entry_connect_image(pd->image_entry, pd->image);

	pd->item_key_entry = ui_edit_entry_new(vbox, _("Item link key:"));

	button = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(dial_edit_page_add_cb), pd);
	gtk_widget_show(button);

	store = gtk_list_store_new(5, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING,
				      G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	pd->list = ui_edit_list_new(hbox, store,
				    G_CALLBACK(dial_edit_page_list_cb), pd);
	g_object_unref(store);

	ui_edit_list_add_column(pd->list, _("Image"), DIAL_COLUMN_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(pd->list, _("Key"), DIAL_COLUMN_KEY, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Press"), DIAL_COLUMN_PRESS, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Prelight"), DIAL_COLUMN_PRELIGHT, FALSE, FALSE);

	gtk_widget_show(hbox);

	return hbox;
}

static void dial_edit_page_add(GtkWidget *widget, gpointer data)
{
	DialListData *dd = data;
	DialPage* pd;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	pd = g_object_get_data(G_OBJECT(widget), "page");
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->list)));

	pixbuf = ui_edit_list_pixbuf_from_file(dd->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   DIAL_COLUMN_POINTER, dd,
			   DIAL_COLUMN_IMAGE, pixbuf,
			   DIAL_COLUMN_KEY, dd->key,
			   DIAL_COLUMN_PRESS, dd->has_press,
			   DIAL_COLUMN_PRELIGHT, dd->has_prelight, -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

void dial_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = dial_get_pixbuf;

	od->func_edit_write = dial_edit_write;

	od->func_edit_read = dial_edit_read;
	od->func_edit_free = dial_edit_free;

	od->func_edit_props = dial_edit_props;

	od->func_edit_page_new = dial_edit_page_new;
	od->func_edit_page_add = dial_edit_page_add;
}





