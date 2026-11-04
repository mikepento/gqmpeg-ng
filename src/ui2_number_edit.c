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

#include "ui2_number.h"
#include "ui2_number_edit.h"

#include "ui2_display.h"
#include "ui2_editor.h"
#include "ui2_main.h"
#include "ui2_skin.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "ui_pixbuf_ops.h"
#include "ui_tabcomp.h"
#include "ui_utildlg.h"


static void number_edit_page_add(GtkWidget *widget, gpointer data);

/*
 *-------
 * digits
 *-------
 */


typedef struct _DigitListData DigitListData;
struct _DigitListData
{
	gchar *key;
	gchar *text_id;
	gchar *data;

	gchar *image;
};

static GdkPixbuf *digit_get_pixbuf(gpointer widget)
{
	DigitData *digit = widget;

	return digit->overlay;
}

static void digit_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	DigitData *digit = wd->widget;
	gchar *image;

	image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "image"),
					 dir, digit->overlay, "digit_", wd->key);

	if (image) ui_edit_widget_set_path_key(wd, "image", dir, image);

	ui_edit_write_section(f, "digit", wd->key);

	ui_edit_write_key_char(f, "image", image);

	g_free(image);
}

static DigitListData *digit_edit_list_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while(work)
		{
		DigitListData *dd = work->data;
		if (strcmp(image, dd->image) == 0) return dd;
		work = work->next;
		}
	return NULL;
}

static gpointer digit_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	DigitListData *dd;
	const gchar *image;

	image = ui_widget_get_data(wd, "image");

	if (!image || digit_edit_list_find(list, image)) return NULL;

	dd = g_new0(DigitListData, 1);
	dd->image = g_strdup(image);

	dd->key = g_strdup(wd->key);
	dd->data = g_strdup(ui_widget_get_data(wd, "data"));
	dd->text_id = g_strdup(wd->text_id);

	return dd;
}

static void digit_edit_free(gpointer data)
{
	DigitListData *dd = data;

	g_free(dd->key);
	g_free(dd->text_id);
	g_free(dd->data);
	g_free(dd->image);
	g_free(dd);
}

static gpointer digit_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail_data)
{
	return detail_data;
}

static gint digit_edit_removable(gpointer widget)
{
	DigitData *digit = widget;

	return (digit->ref < 2);
}

void digit_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = digit_get_pixbuf;

	od->func_edit_write = digit_edit_write;

	od->func_edit_read = digit_edit_read;
	od->func_edit_free = digit_edit_free;

	od->func_edit_props = digit_edit_props;

	od->func_edit_page_add = number_edit_page_add;

	od->func_edit_removable = digit_edit_removable;
}


/*
 *-------
 * numbers
 *-------
 */


enum {
	NUMBER_COLUMN_POINTER = 0,
	NUMBER_COLUMN_IMAGE,
	NUMBER_COLUMN_KEY,
	NUMBER_COLUMN_COUNT
};


typedef struct _NumberDetail NumberDetail;
struct _NumberDetail
{
	NumberData *number;
	GtkWidget *zero_button;
	GtkWidget *center_button;
	GtkWidget *spin_length;
	GtkWidget *spin_rotation;
	GtkWidget *digit_key_entry;

	WidgetData *wd;
	UIData *ui;
};

typedef struct _NumberPage NumberPage;
struct _NumberPage
{
	GtkWidget *key_entry;
	GtkWidget *text_id_entry;
	GtkWidget *data_entry;

	GtkWidget *image_entry;
	GtkWidget *digit_id_entry;

	GtkWidget *zeros_button;
	GtkWidget *center_button;
	GtkWidget *length_spin;
	GtkWidget *rotation_spin;

	GtkWidget *list;
	GtkWidget *image;

	GtkWidget *digit_key_entry;

	EditData *ed;
};


static GdkPixbuf *number_get_pixbuf(gpointer widget)
{
	NumberData *number = widget;

	return number->digits->overlay;
}

static void number_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	NumberData *number = wd->widget;
	gchar *image = NULL;
	const gchar *file;
	const gchar *digit_id;

	file = ui_widget_get_data(wd, "image");
	digit_id = ui_widget_get_data(wd, "digit_id");
	if (file || (!file && !digit_id))
		{
		image = ui_edit_copy_unique_file(file, dir, number->digits->overlay, "digit_", wd->key);
		}

	if (image) ui_edit_widget_set_path_key(wd, "image", dir, image);

	ui_edit_write_section(f, "number", wd->key);

	if (image)
		{
		ui_edit_write_key_char(f, "image", image);
		}
	else
		{
		ui_edit_write_key_char(f, "digit", digit_id);
		}
	ui_edit_write_key_int(f, "x", number->x);
	ui_edit_write_key_int(f, "y", number->y);
	ui_edit_write_key_int(f, "length", number->length);

	ui_edit_write_key_bool(f, "zeros", number->zeros);
	ui_edit_write_key_bool(f, "center", number->centered);
	ui_edit_write_key_int(f, "rotation", number->rotation);

	g_free(image);
}

static gpointer number_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	NumberData *number;
	DigitListData *dd;
	const gchar *image;

	number = wd->widget;

	image = ui_widget_get_data(wd, "image");

	if (!image || digit_edit_list_find(list, image)) return NULL;

	dd = g_new0(DigitListData, 1);
	dd->image = g_strdup(image);

	dd->key = g_strdup(wd->key);
	dd->data = g_strdup(ui_widget_get_data(wd, "data"));
	dd->text_id = g_strdup(wd->text_id);

	return dd;
}

static void number_edit_props_alter_cb(GtkWidget *button, gpointer data)
{
	NumberDetail *nd = data;

	nd->number->zeros = ui_edit_toggle_get(nd->zero_button);
	nd->number->centered = ui_edit_toggle_get(nd->center_button);
	gtk_widget_set_sensitive(nd->zero_button, !nd->number->centered);

	ui_widget_draw(nd->ui, nd->wd, TRUE, TRUE);
}

static void number_edit_props_length_cb(GtkAdjustment *adjustment, gpointer data)
{
	NumberDetail *nd = data;
	gint new_length;
	gint old_w, new_w;
	gint w, h;
	gint max_w;
	gint cw, ch;

	new_length = ui_edit_spin_get(nd->spin_length);

	if (digit_rotation_vertical(nd->number->rotation))
		{
		max_w = nd->ui->skin->height;
		}
	else
		{
		max_w = nd->ui->skin->width;
		}

	if (new_length * nd->number->digits->width > max_w)
		{
		new_length = max_w / nd->number->digits->width;
		ui_edit_spin_set_blocking(nd->spin_length, new_length, nd);
		}

	if (new_length == nd->number->length) return;

	old_w = nd->number->digits->width * nd->number->length;
	new_w = nd->number->digits->width * new_length;
	if (digit_rotation_vertical(nd->number->rotation))
		{
		w = nd->number->digits->height;
		h = new_w;
		cw = 0;
		ch = MAX(0, (old_w - new_w));
		}
	else
		{
		w = new_w;
		h = nd->number->digits->height;
		cw = MAX(0, (old_w - new_w));
		ch = 0;
		}

	if (new_length > nd->number->length)
		{
		if (nd->number->x + w > nd->ui->skin->width)
			{
			ui_widget_set_coord(nd->ui, nd->wd, nd->ui->skin->width - w, nd->number->y, TRUE);
			}
		if (nd->number->y + h > nd->ui->skin->height)
			{
			ui_widget_set_coord(nd->ui, nd->wd, nd->number->x, nd->ui->skin->height - h, TRUE);
			}
		}

	if (old_w > w) ui_edit_widget_draw_highlight(nd->ui, nd->wd, FALSE);

	nd->number->length = new_length;
	gdk_pixbuf_unref(nd->number->pixbuf);
	nd->number->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);

	ui_edit_widget_resync(nd->ui, nd->wd, TRUE, cw, ch);
}

static void number_edit_props_rotation_cb(GtkAdjustment *adjustment, gpointer data)
{
	NumberDetail *nd = data;
	NumberData *number;
	gint new_r;
	gint cw, ch;

	new_r = ui_edit_spin_get(nd->spin_rotation);
	number = nd->number;

	if (digit_rotation_vertical(new_r) != digit_rotation_vertical(number->rotation))
		{
		gint new_w, new_h;
		gint old_w, old_h;

		ui_edit_widget_draw_highlight(nd->ui, nd->wd, FALSE);

		if (digit_rotation_vertical(new_r))
			{
			old_w = number->length * number->digits->width;
			old_h = number->digits->height;
			if (number->y + number->length * number->digits->width > nd->ui->skin->height)
				{
				number->length = MAX(1, ((nd->ui->skin->height - number->y) / number->digits->width));
				ui_edit_spin_set_blocking(nd->spin_length, number->length, nd);
				}
			new_w = old_h;
			new_h = number->length * number->digits->width;
			}
		else
			{
			old_w = number->digits->height;
			old_h = number->length * number->digits->width;
			if (number->x + number->length * number->digits->width > nd->ui->skin->width)
				{
				number->length = MAX(1, ((nd->ui->skin->width - number->x) / number->digits->width));
				ui_edit_spin_set_blocking(nd->spin_length, number->length, nd);
				}
			new_w = number->length * number->digits->width;
			new_h = old_w;
			}

		gdk_pixbuf_unref(number->pixbuf);
		number->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, new_w, new_h);

		cw = MAX(0, old_w - new_w);
		ch = MAX(0, old_h - new_h);
		}
	else
		{
		cw = ch = 0;
		}

	number->rotation = new_r;
	ui_edit_widget_resync(nd->ui, nd->wd, TRUE, cw, ch);
}

static gpointer number_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail)
{
	NumberData *number = wd->widget;
	NumberDetail *nd = detail;

	GtkObject *adj;

	if (!nd)
		{
		GtkWidget *hbox;

		nd = g_new0(NumberDetail, 1);

		hbox = ui_edit_frame_new(vbox, FALSE, NULL);

		nd->spin_length = ui_edit_spin_new(hbox, _("Length:"), 1, 8,  &adj);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(number_edit_props_length_cb), nd);

		nd->spin_rotation = ui_edit_spin_new(hbox, _("Rotation:"), 0, 270, &adj);
		GTK_ADJUSTMENT(adj)->step_increment = 90.0;
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(nd->spin_rotation), TRUE);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(number_edit_props_rotation_cb), nd);

		nd->zero_button = ui_edit_toggle_new(vbox, _("Pad with zeros"));
		g_signal_connect(G_OBJECT(nd->zero_button), "clicked",
				 G_CALLBACK(number_edit_props_alter_cb), nd);

		nd->center_button = ui_edit_toggle_new(vbox, _("Center"));
		g_signal_connect(G_OBJECT(nd->center_button), "clicked",
				 G_CALLBACK(number_edit_props_alter_cb), nd);

		hbox = ui_edit_frame_new(vbox, FALSE, NULL);

		nd->digit_key_entry = ui_edit_entry_new(hbox, _("Digit key:"));
		ui_edit_frame_sensitive(nd->digit_key_entry, FALSE, TRUE);
		}

	nd->number = number;
	nd->wd = wd;
	nd->ui = ui;

	ui_edit_toggle_set_blocking(nd->zero_button, number->zeros, nd);
	gtk_widget_set_sensitive(nd->zero_button, !nd->number->centered);

	ui_edit_toggle_set_blocking(nd->center_button, number->centered, nd);
	ui_edit_spin_set_blocking(nd->spin_length, number->length, nd);
	ui_edit_spin_set_blocking(nd->spin_rotation, number->rotation, nd);
	ui_edit_entry_set(nd->digit_key_entry, ui_widget_get_data(wd, "digit_id"));

	return nd;
}

static void number_edit_page_digit_add_cb(GtkWidget *widget, gpointer data)
{
	NumberPage *pd = data;
	DigitData *digit;
	const gchar *key;
	const gchar *text_id;
	const gchar *image;

	key = ui_edit_entry_get(pd->digit_key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	image = ui_edit_entry_get(pd->image_entry);

	if (!key || !image || !isfile(image))
		{
		warning_dialog(_("Digit error"), _("Digit must contain a key and valid image."));
		return;
		}

	digit = digit_new_from_file(image);
	
	if (digit)
		{
		WidgetData *wd;
		wd = digit_register(pd->ed->ui->skin, digit, key, text_id);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	tab_completion_append_to_history(pd->image_entry, image);
}

static void number_edit_page_add_cb(GtkWidget *widget, gpointer data)
{
	NumberPage *pd = data;
	NumberData *number;
	DigitData *digit;
	const gchar *key;
	const gchar *text_id;
	const gchar *digit_id;
	const gchar *image;
	gint length;
	gint zeros;
	gint center;
	gint rotation;

	key = ui_edit_entry_get(pd->key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	digit_id = ui_edit_entry_get(pd->digit_id_entry);
	image = ui_edit_entry_get(pd->image_entry);

	if (!key ||
	    ( (!digit_id || !ui_widget_exists(pd->ed->ui, digit_id, digit_type_id())) && (digit_id || !image || !isfile(image)) ) )
		{
		warning_dialog(_("Number error"), _("Number must contain a key, and valid image or digit key."));
		return;
		}

	length = ui_edit_spin_get(pd->length_spin);
	zeros = ui_edit_toggle_get(pd->zeros_button);
	center = ui_edit_toggle_get(pd->center_button);
	rotation = ui_edit_spin_get(pd->rotation_spin);

	if (digit_id)
		{
		WidgetData *wd;

		wd = skin_widget_get_by_key(pd->ed->ui->skin, digit_id, digit_type_id());
		digit = wd->widget;
		image = NULL;
		}
	else
		{
		digit = digit_new_from_file(image);
		}

	if (!digit) return;
	number = number_new(digit, 0, 0, length, zeros, center, rotation);
	
	if (number)
		{
		WidgetData *wd;
		wd = number_register(pd->ed->ui->skin, number, key, text_id);
		ui_widget_set_data(wd, "digit_id", digit_id);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	if (!digit_id) tab_completion_append_to_history(pd->image_entry, image);
}

static void number_edit_page_sync(NumberPage *pd, DigitListData *dd)
{
	if (!dd) return;

	ui_edit_entry_set(pd->image_entry, dd->image);
}

static void number_edit_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	NumberPage *pd = data;
	DigitListData *dd;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, NUMBER_COLUMN_POINTER, &dd, -1);
	number_edit_page_sync(pd, dd);
}

static void number_edit_page_digit_cb(GtkWidget *entry, gpointer data)
{
	NumberPage *pd = data;
	gint state;

	state = (strlen(gtk_entry_get_text(GTK_ENTRY(pd->digit_id_entry))) == 0);

	ui_edit_frame_sensitive(pd->image_entry, state, TRUE);
}

static void number_edit_page_center_cb(GtkWidget *button, gpointer data)
{
	NumberPage *pd = data;

	gtk_widget_set_sensitive(pd->zeros_button, !ui_edit_toggle_get(pd->center_button));
}

static void number_edit_page_destroy_cb(GtkWidget *widget, gpointer data)
{
	NumberPage *pd = data;

	g_free(pd);
}

static GtkWidget *number_edit_page_new(EditData *ed)
{
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *sep;
	GtkObject *adj;
	NumberPage *pd;
	GtkListStore *store;

	pd = g_new0(NumberPage, 1);
	pd->ed = ed;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	g_object_set_data(G_OBJECT(hbox), "page", pd);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(number_edit_page_destroy_cb), pd);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	pd->key_entry = ui_edit_key_entry_new(vbox, ed->ui, number_type_id());
	pd->data_entry = ui_edit_entry_new(vbox, _("Data:"));
	pd->text_id_entry = ui_edit_entry_new(vbox, _("Text id:"));

	hbox2 = ui_edit_frame_new(vbox, FALSE, NULL);
	pd->length_spin = ui_edit_spin_new(hbox2, _("Length:"), 1, 8, NULL);
	pd->rotation_spin = ui_edit_spin_new(hbox2, _("Rotation:"), 0, 270, &adj);
	GTK_ADJUSTMENT(adj)->step_increment = 90.0;
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(pd->rotation_spin), TRUE);

	pd->zeros_button = ui_edit_toggle_new(vbox, _("Pad with zeros"));
	pd->center_button = ui_edit_toggle_new(vbox, _("Center"));
	g_signal_connect(G_OBJECT(pd->center_button), "clicked",
			 G_CALLBACK(number_edit_page_center_cb), pd);

	pd->digit_id_entry = ui_edit_entry_new(vbox, _("Digit key:"));
	g_signal_connect(G_OBJECT(pd->digit_id_entry), "changed",
			 G_CALLBACK(number_edit_page_digit_cb), pd);
	pd->image_entry = ui_edit_path_entry_new(vbox, _("Image:"), "SLIK_number_image");
	pd->image = ui_edit_image_new(vbox);
	ui_edit_path_entry_connect_image(pd->image_entry, pd->image);

	button = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(number_edit_page_add_cb), pd);
	gtk_widget_show(button);

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
	gtk_widget_show(sep);

	frame = ui_edit_frame_new(vbox, TRUE, _("Add digit only"));
	pd->digit_key_entry = ui_edit_entry_new(frame, _("Key:"));

	button = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_end(GTK_BOX(frame), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(number_edit_page_digit_add_cb), pd);
	gtk_widget_show(button);

	store = gtk_list_store_new(3, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	pd->list = ui_edit_list_new(hbox, store,
				    G_CALLBACK(number_edit_page_list_cb), pd);
	g_object_unref(store);

	ui_edit_list_add_column(pd->list, _("Image"), NUMBER_COLUMN_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(pd->list, _("Key"), NUMBER_COLUMN_KEY, FALSE, FALSE);

	/* hacky, but it works */
	ui_edit_widget_id_list_set_page(ed->widget_id_list, digit_type_id(), hbox);

	gtk_widget_show(hbox);

	return hbox;
}

static void number_edit_page_add(GtkWidget *widget, gpointer data)
{
	DigitListData *dd = data;
	NumberPage *pd;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	pd = g_object_get_data(G_OBJECT(widget), "page");
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->list)));

	pixbuf = ui_edit_list_pixbuf_from_file(dd->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   NUMBER_COLUMN_POINTER, dd,
			   NUMBER_COLUMN_IMAGE, pixbuf,
			   NUMBER_COLUMN_KEY, dd->key, -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

void number_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = number_get_pixbuf;

	od->func_edit_write = number_edit_write;

	od->func_edit_read = number_edit_read;
	od->func_edit_free = digit_edit_free;

	od->func_edit_props = number_edit_props;

	od->func_edit_page_new = number_edit_page_new;
	od->func_edit_page_add = number_edit_page_add;
}




