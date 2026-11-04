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

#include "ui2_item.h"
#include "ui2_item_edit.h"

#include "ui2_editor.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "ui_tabcomp.h"
#include "ui_utildlg.h"


enum {
	ITEM_COLUMN_POINTER = 0,
	ITEM_COLUMN_IMAGE,
	ITEM_COLUMN_KEY,
	ITEM_COLUMN_SECTIONS,
	ITEM_COLUMN_COUNT
};


typedef struct _ItemDetail ItemDetail;
struct _ItemDetail
{
	GtkWidget *sections_spin;
};

typedef struct _ItemListData ItemListData;
struct _ItemListData
{
	gchar *key;
	gchar *text_id;
	gchar *data;

	gchar *image;
	gint sections;
};

typedef struct _ItemPage ItemPage;
struct _ItemPage
{
	GtkWidget *key_entry;
	GtkWidget *text_id_entry;
	GtkWidget *data_entry;

	GtkWidget *image_entry;
	GtkWidget *section_spin;

	GtkWidget *list;
	GtkWidget *image;

	EditData *ed;
};


static GdkPixbuf *item_get_pixbuf(gpointer data)
{
	ItemData *item = data;
	return item->overlay;
}

static void item_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	ItemData *item = wd->widget;
	gchar *image;

	image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "image"),
					 dir, item->overlay, "item_", wd->key);

	if (image) ui_edit_widget_set_path_key(wd, "image", dir, image);

	ui_edit_write_section(f, "item", wd->key);

	ui_edit_write_key_char(f, "image", image);
	ui_edit_write_key_int(f, "x", item->x);
	ui_edit_write_key_int(f, "y", item->y);
	ui_edit_write_key_int(f, "sections", item->sections);

	g_free(image);
}

static ItemListData *item_edit_list_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while(work)
		{
		ItemListData *id = work->data;
		if (strcmp(image, id->image) == 0) return id;
		work = work->next;
		}
	return NULL;
}

static gpointer item_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	ItemData *item = wd->widget;
	ItemListData *id;
	const gchar *image;

	image = ui_widget_get_data(wd, "image");

	if (!image || item_edit_list_find(list, image)) return NULL;

	id = g_new0(ItemListData, 1);
	id->image = g_strdup(image);
	id->sections = item->sections;

	id->key = g_strdup(wd->key);
	id->data = g_strdup(ui_widget_get_data(wd, "data"));
	id->text_id = g_strdup(wd->text_id);

	return id;
}

static gpointer item_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail)
{
	ItemData *item = wd->widget;
	ItemDetail *id = detail;

	if (!id)
		{
		id = g_new0(ItemDetail, 1);

		id->sections_spin = ui_edit_spin_new(vbox, _("Sections:"), 1, 2000, NULL);
		ui_edit_frame_sensitive(id->sections_spin, FALSE, TRUE);
		}

	ui_edit_spin_set(id->sections_spin, item->sections);

	return id;
}

static void item_edit_free(gpointer data)
{
	ItemListData *id = data;
	g_free(id->key);
	g_free(id->text_id);
	g_free(id->data);
	g_free(id->image);
	g_free(id);
}

static void item_edit_page_add_cb(GtkWidget *widget, gpointer data)
{
	ItemPage *pd = data;
	ItemData *item;
	const gchar *key;
	const gchar *text_id;
	const gchar *image;

	key = ui_edit_entry_get(pd->key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	image = ui_edit_entry_get(pd->image_entry);

	if (!key || !image || !isfile(image))
		{
		warning_dialog(_("Item error"), _("Item must contain a key and valid image."));
		return;
		}

	item = item_new_from_file(image, 0, 0, ui_edit_spin_get(pd->section_spin));
	if (item)
		{
		WidgetData *wd;
		wd = item_register(pd->ed->ui->skin, item, key, text_id);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	tab_completion_append_to_history(pd->image_entry, image);
}

static void item_edit_page_sync(ItemPage *pd, ItemListData *id)
{
	if (!id) return;

	ui_edit_entry_set(pd->key_entry, id->key);
	ui_edit_entry_set(pd->text_id_entry, id->text_id);
	ui_edit_entry_set(pd->data_entry, id->data);

	ui_edit_entry_set(pd->image_entry, id->image);

	ui_edit_spin_set(pd->section_spin, id->sections);
}

static void item_edit_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	ItemPage *pd = data;
	ItemListData *id;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, ITEM_COLUMN_POINTER, &id, -1);
	item_edit_page_sync(pd, id);
}

static void item_edit_page_destroy_cb(GtkWidget *widget, gpointer data)
{
	ItemPage *pd = data;

	g_free(pd);
}

static GtkWidget *item_edit_page_new(EditData *ed)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *item;
	ItemPage *pd;
	GtkListStore *store;

	pd = g_new0(ItemPage, 1);
	pd->ed = ed;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	g_object_set_data(G_OBJECT(hbox), "page", pd);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(item_edit_page_destroy_cb), pd);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	pd->key_entry = ui_edit_key_entry_new(vbox, ed->ui, item_type_id());
	pd->data_entry = ui_edit_entry_new(vbox, _("Data:"));
	pd->text_id_entry = ui_edit_entry_new(vbox, _("Text id:"));

	pd->section_spin = ui_edit_spin_new(vbox, _("Sections:"), 1, 1000, NULL);

	pd->image_entry = ui_edit_path_entry_new(vbox, _("Image:"), "SLIK_item_image");
	pd->image = ui_edit_image_new(vbox);
	ui_edit_path_entry_connect_image(pd->image_entry, pd->image);

	item = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_start(GTK_BOX(vbox), item, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(item), "clicked",
			 G_CALLBACK(item_edit_page_add_cb), pd);
	gtk_widget_show(item);

	store = gtk_list_store_new(4, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING,
				      G_TYPE_INT);
	pd->list = ui_edit_list_new(hbox, store,
				    G_CALLBACK(item_edit_page_list_cb), pd);
	g_object_unref(store);

	ui_edit_list_add_column(pd->list, _("Image"), ITEM_COLUMN_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(pd->list, _("Key"), ITEM_COLUMN_KEY, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Sections"), ITEM_COLUMN_SECTIONS, FALSE, FALSE);

	gtk_widget_show(hbox);

	return hbox;
}

static void item_edit_page_add(GtkWidget *widget, gpointer data)
{
	ItemListData *id = data;
	ItemPage *pd;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	pd = g_object_get_data(G_OBJECT(widget), "page");
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->list)));

	pixbuf = ui_edit_list_pixbuf_from_file(id->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   ITEM_COLUMN_POINTER, id,
			   ITEM_COLUMN_IMAGE, pixbuf,
			   ITEM_COLUMN_KEY, id->key,
			   ITEM_COLUMN_SECTIONS, id->sections, -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

void item_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = item_get_pixbuf;

	od->func_edit_write = item_edit_write;

	od->func_edit_read = item_edit_read;
	od->func_edit_free = item_edit_free;

	od->func_edit_props = item_edit_props;

	od->func_edit_page_new = item_edit_page_new;
	od->func_edit_page_add = item_edit_page_add;
}


