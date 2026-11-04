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

#include "ui2_button.h"
#include "ui2_button_edit.h"

#include "ui2_editor.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "ui_tabcomp.h"
#include "ui_utildlg.h"


enum {
	BUTTON_COLUMN_POINTER = 0,
	BUTTON_COLUMN_IMAGE,
	BUTTON_COLUMN_KEY,
	BUTTON_COLUMN_INDICATOR,
	BUTTON_COLUMN_PRELIGHT,
	BUTTON_COLUMN_COUNT
};

typedef struct _ButtonDetail ButtonDetail;
struct _ButtonDetail
{
	ButtonData *button;
	GtkWidget *prelight_button;
	GtkWidget *indicator_button;

	WidgetData *wd;
	UIData *ui;
};

typedef struct _ButtonListData ButtonListData;
struct _ButtonListData
{
	gchar *key;
	gchar *text_id;
	gchar *data;

	gchar *image;
	gint has_light;
	gint has_prelight;
	gchar *clip_mask;
};

typedef struct _ButtonPage ButtonPage;
struct _ButtonPage
{
	GtkWidget *key_entry;
	GtkWidget *text_id_entry;
	GtkWidget *data_entry;

	GtkWidget *image_entry;
	GtkWidget *has_light_button;
	GtkWidget *has_prelight_button;
	GtkWidget *clip_mask_entry;

	GtkWidget *list;
	GtkWidget *image;

	EditData *ed;
};

static GdkPixbuf *button_get_pixbuf(gpointer widget)
{
	ButtonData *button = widget;

	return button->overlay;
}

static void button_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	ButtonData *button = wd->widget;
	gchar *image;
	gchar *clip_mask;

	image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "image"),
					 dir, button->overlay, "button_", wd->key);
	if (button->clip_mask)
		{
		clip_mask = ui_edit_copy_unique_file(ui_widget_get_data(wd, "clip_mask"),
						     dir, button->pixbuf, "button_mask_", wd->key);
		}
	else
		{
		clip_mask = NULL;
		}

	if (image) ui_edit_widget_set_path_key(wd, "image", dir, image);
	if (clip_mask) ui_edit_widget_set_path_key(wd, "clip_mask", dir, clip_mask);

	ui_edit_write_section(f, "button", wd->key);

	ui_edit_write_key_char(f, "image", image);
	ui_edit_write_key_int(f, "x", button->x);
	ui_edit_write_key_int(f, "y", button->y);

	ui_edit_write_key_bool(f, "prelight", button->has_prelight);
	ui_edit_write_key_bool(f, "indicator", button->has_light);
	if (clip_mask) ui_edit_write_key_char(f, "clip_mask", clip_mask);

	g_free(image);
	g_free(clip_mask);
}

static ButtonListData *button_edit_list_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while(work)
		{
		ButtonListData *bd = work->data;
		if (strcmp(image, bd->image) == 0) return bd;
		work = work->next;
		}
	return NULL;
}

static gpointer button_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	ButtonData *button = wd->widget;
	ButtonListData *bd;
	const gchar *image;

	image = ui_widget_get_data(wd, "image");

	if (!image || button_edit_list_find(list, image)) return NULL;

	bd = g_new0(ButtonListData, 1);
	bd->image = g_strdup(image);
	bd->has_light = button->has_light;
	bd->has_prelight = button->has_prelight;
	bd->clip_mask = g_strdup(ui_widget_get_data(wd, "clip_mask"));

	bd->key = g_strdup(wd->key);
	bd->data = g_strdup(ui_widget_get_data(wd, "data"));
	bd->text_id = g_strdup(wd->text_id);

	return bd;
}

static void button_edit_free(gpointer data)
{
	ButtonListData *bd = data;

	g_free(bd->key);
	g_free(bd->text_id);
	g_free(bd->data);
	g_free(bd->image);
	g_free(bd->clip_mask);
	g_free(bd);
}

static gpointer button_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail)
{
	ButtonData *button = wd->widget;
	ButtonDetail *bd = detail;

	if (!bd)
		{
		bd = g_new0(ButtonDetail, 1);

		bd->indicator_button = ui_edit_toggle_new(vbox, _("Indicator"));
		gtk_widget_set_sensitive(bd->indicator_button, FALSE);

		bd->prelight_button = ui_edit_toggle_new(vbox, _("Prelight"));
		gtk_widget_set_sensitive(bd->prelight_button, FALSE);
		}

	bd->button = button;
	bd->wd = wd;
	bd->ui = ui;

	ui_edit_toggle_set(bd->indicator_button, button->has_light);
	ui_edit_toggle_set(bd->prelight_button, button->has_prelight);

	return bd;
}

static void button_edit_add_cb(GtkWidget *widget, gpointer data)
{
	ButtonPage *pd = data;
	ButtonData *button;
	const gchar *key;
	const gchar *text_id;
	const gchar *image;
	const gchar *clip_mask;

	key = ui_edit_entry_get(pd->key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	image = ui_edit_entry_get(pd->image_entry);
	clip_mask = ui_edit_entry_get(pd->clip_mask_entry);

	if (!key || !image || !isfile(image))
		{
		warning_dialog(_("Button error"), _("Button must contain a key and valid image."));
		return;
		}

	button = button_new_from_file(image, 0, 0,
				      ui_edit_toggle_get(pd->has_prelight_button),
				      ui_edit_toggle_get(pd->has_light_button), clip_mask);
	if (button)
		{
		WidgetData *wd;
		wd = button_register(pd->ed->ui->skin, button, key, text_id);
		ui_widget_set_data(wd, "clip_mask", clip_mask);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	tab_completion_append_to_history(pd->image_entry, image);
	tab_completion_append_to_history(pd->clip_mask_entry, clip_mask);
}

static void button_edit_page_sync(ButtonPage *pd, ButtonListData *bd)
{
	if (!bd) return;

	ui_edit_entry_set(pd->key_entry, bd->key);
	ui_edit_entry_set(pd->text_id_entry, bd->text_id);
	ui_edit_entry_set(pd->data_entry, bd->data);

	ui_edit_entry_set(pd->image_entry, bd->image);

	ui_edit_toggle_set(pd->has_light_button, bd->has_light);
	ui_edit_toggle_set(pd->has_prelight_button, bd->has_prelight);

	ui_edit_entry_set(pd->clip_mask_entry, bd->clip_mask);
}

static void button_edit_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	ButtonPage *pd = data;
	ButtonListData *bd;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, BUTTON_COLUMN_POINTER, &bd, -1);
	button_edit_page_sync(pd, bd);
}

static void button_edit_page_destroy_cb(GtkWidget *widget, gpointer data)
{
	ButtonPage *pd = data;

	g_free(pd);
}

static GtkWidget *button_edit_page_new(EditData *ed)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *button;
	ButtonPage *pd;
	GtkListStore *store;

	pd = g_new0(ButtonPage, 1);
	pd->ed = ed;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	g_object_set_data(G_OBJECT(hbox), "page", pd);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(button_edit_page_destroy_cb), pd);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	pd->key_entry = ui_edit_key_entry_new(vbox, ed->ui, button_type_id());
	pd->data_entry = ui_edit_entry_new(vbox, _("Data:"));
	pd->text_id_entry = ui_edit_entry_new(vbox, _("Text id:"));

	pd->has_light_button = ui_edit_toggle_new(vbox, _("Indicator"));
	pd->has_prelight_button = ui_edit_toggle_new(vbox, _("Prelight"));

	pd->image_entry = ui_edit_path_entry_new(vbox, _("Image:"), "SLIK_button_image");
	pd->image = ui_edit_image_new(vbox);
	ui_edit_path_entry_connect_image(pd->image_entry, pd->image);
	pd->clip_mask_entry = ui_edit_path_entry_new(vbox, _("Clip mask:"), "SLIK_button_clip_mask");

	button = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(button_edit_add_cb), pd);
	gtk_widget_show(button);

	store = gtk_list_store_new(5, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING,
				      G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	pd->list = ui_edit_list_new(hbox, store,
				    G_CALLBACK(button_edit_page_list_cb), pd);
	g_object_unref(store);

	ui_edit_list_add_column(pd->list, _("Image"), BUTTON_COLUMN_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(pd->list, _("Key"), BUTTON_COLUMN_KEY, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Indicator"), BUTTON_COLUMN_INDICATOR, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Prelight"), BUTTON_COLUMN_PRELIGHT, FALSE, FALSE);
	
	gtk_widget_show(hbox);

	return hbox;
}

static void button_edit_page_add(GtkWidget *widget, gpointer data)
{
	ButtonListData *bd = data;
	ButtonPage *pd;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	pd = g_object_get_data(G_OBJECT(widget), "page");
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->list)));

	pixbuf = ui_edit_list_pixbuf_from_file(bd->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   BUTTON_COLUMN_POINTER, bd,
			   BUTTON_COLUMN_IMAGE, pixbuf,
			   BUTTON_COLUMN_KEY, bd->key,
			   BUTTON_COLUMN_INDICATOR, bd->has_light,
			   BUTTON_COLUMN_PRELIGHT, bd->has_prelight, -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

void button_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = button_get_pixbuf;

	od->func_edit_write = button_edit_write;

	od->func_edit_read = button_edit_read;
	od->func_edit_free = button_edit_free;

	od->func_edit_props = button_edit_props;

	od->func_edit_page_new = button_edit_page_new;
	od->func_edit_page_add = button_edit_page_add;
}




