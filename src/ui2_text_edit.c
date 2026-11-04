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

#include "ui2_text.h"
#include "ui2_text_edit.h"

#include "ui2_display.h"
#include "ui2_editor.h"
#include "ui2_main.h"
#include "ui2_skin.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "ui_pixbuf_ops.h"
#include "ui_tabcomp.h"
#include "ui_utildlg.h"


static void text_edit_page_add(GtkWidget *widget, gpointer data);


/*
 *-------
 * fonts
 *-------
 */

typedef struct _FontDetail FontDetail;
struct _FontDetail
{
	GtkWidget *extended_button;

	GtkWidget *font_entry;
};

typedef struct _FontListData FontListData;
struct _FontListData
{
	gchar *key;
	gchar *text_id;
	gchar *data;

	gchar *image;
	gint extended;

	gchar *description;
};

static GdkPixbuf *font_get_pixbuf(gpointer widget)
{
	FontData *font = widget;

	return font->overlay;
}

static void font_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	FontData *font = wd->widget;

	ui_edit_write_section(f, "font", wd->key);

	if (font->overlay)
		{
		gchar *image;

		image = ui_edit_copy_unique_file(ui_widget_get_data(wd, "image"),
						 dir, font->overlay, "font_", wd->key);
		if (image) ui_edit_widget_set_path_key(wd, "image", dir, image);

		ui_edit_write_key_char(f, "image", image);
		ui_edit_write_key_bool(f, "extended", font->extended);

		g_free(image);
		}
	else
		{
		ui_edit_write_key_char(f, "description", font->text_description);
		}
}

static FontListData *font_edit_list_find(GList *list, const gchar *image)
{
	GList *work;
	work = list;
	while(work)
		{
		FontListData *fd = work->data;
		if (strcmp(image, fd->image) == 0) return fd;
		work = work->next;
		}
	return NULL;
}

static gpointer font_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	FontListData *fd;
	FontData *font;
	const gchar *image;

	image = ui_widget_get_data(wd, "image");

	if (!image || font_edit_list_find(list, image)) return NULL;

	font = wd->widget;

	fd = g_new0(FontListData, 1);
	fd->image = g_strdup(image);
	fd->extended = font->extended;

	fd->key = g_strdup(wd->key);
	fd->data = g_strdup(ui_widget_get_data(wd, "data"));
	fd->text_id = g_strdup(wd->text_id);

	return fd;
}

static void font_edit_free(gpointer data)
{
	FontListData *fd = data;

	g_free(fd->key);
	g_free(fd->text_id);
	g_free(fd->data);
	g_free(fd->image);
	g_free(fd);
}

static gpointer font_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail_data)
{
	FontDetail *fd = detail_data;
	FontData *font = wd->widget;

	if (!fd)
		{
		fd = g_new0(FontDetail, 1);

		fd->extended_button = ui_edit_toggle_new(vbox, _("Extended"));
		gtk_widget_set_sensitive(fd->extended_button, FALSE);

		fd->font_entry = ui_edit_entry_new(vbox, _("Font:"));
		ui_edit_frame_sensitive(fd->font_entry, FALSE, TRUE);

		}

	ui_edit_toggle_set(fd->extended_button, font->extended);
	ui_edit_entry_set(fd->font_entry, font->text_description);

	return fd;
}

static gint font_edit_removable(gpointer widget)
{
	FontData *font = widget;

	return (font->ref < 2);
}

void font_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = font_get_pixbuf;

	od->func_edit_write = font_edit_write;

	od->func_edit_read = font_edit_read;
	od->func_edit_free = font_edit_free;

	od->func_edit_props = font_edit_props;

	od->func_edit_page_add = text_edit_page_add;

	od->func_edit_removable = font_edit_removable;
}


/*
 *-------
 * text
 *-------
 */


enum {
	TEXT_COLUMN_POINTER = 0,
	TEXT_COLUMN_IMAGE,
	TEXT_COLUMN_KEY,
	TEXT_COLUMN_EXTENDED,
	TEXT_COLUMN_COUNT
};


typedef struct _TextDetail TextDetail;
struct _TextDetail
{
	TextData *text;
	GtkWidget *spin_width;
	GtkWidget *extended_button;
	GtkWidget *sizeable_button;
	GtkWidget *spin_rotation;
	GtkWidget *font_key_entry;

	GtkWidget *font_entry;

	GtkWidget *red_spin;
	GtkWidget *green_spin;
	GtkWidget *blue_spin;
	GtkWidget *alpha_spin;

	WidgetData *wd;
	UIData *ui;
};

typedef struct _TextPage TextPage;
struct _TextPage
{
	GtkWidget *key_entry;
	GtkWidget *text_id_entry;
	GtkWidget *data_entry;

	GtkWidget *image_entry;
	GtkWidget *font_id_entry;

	GtkWidget *extended_button;
	GtkWidget *sizeable_button;
	GtkWidget *spin_rotation;
	GtkWidget *spin_width;

	GtkWidget *font_entry;

	GtkWidget *red_spin;
	GtkWidget *green_spin;
	GtkWidget *blue_spin;
	GtkWidget *alpha_spin;

	GtkWidget *list;
	GtkWidget *image;

	GtkWidget *font_key_entry;

	EditData *ed;
};


static GdkPixbuf *text_get_pixbuf(gpointer data)
{
	TextData *text = data;
	return text->font->overlay;
}

static void text_edit_write(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir)
{
	TextData *text = wd->widget;
	const gchar *file;
	const gchar *font_id;

	file = ui_widget_get_data(wd, "image");
	font_id = ui_widget_get_data(wd, "font_id");

	ui_edit_write_section(f, "text", wd->key);

	if (!font_id && text->font->overlay)
		{
		gchar *image;

		image = ui_edit_copy_unique_file(file, dir, text->font->overlay, "text_", wd->key);
		ui_edit_widget_set_path_key(wd, "image", dir, image);

		ui_edit_write_key_char(f, "image", image);
		ui_edit_write_key_bool(f, "extended", text->font->extended);

		g_free(image);
		}
	else
		{
		if (font_id)
			{
			ui_edit_write_key_char(f, "font", font_id);
			}
		else
			{
			ui_edit_write_key_char(f, "description", text->font->text_description);
			}

		if (!font_id || !text->font->overlay)
			{
			ui_edit_write_key_int(f, "red", (gint)text->r);
			ui_edit_write_key_int(f, "green", (gint)text->g);
			ui_edit_write_key_int(f, "blue", (gint)text->b);
			ui_edit_write_key_int(f, "alpha", (gint)text->a);
			}
		}

	ui_edit_write_key_int(f, "x", text->x);
	ui_edit_write_key_int(f, "y", text->y);
	ui_edit_write_key_int(f, "width", font_rotation_vertical(text->rotation) ? text->height : text->width);

	ui_edit_write_key_bool(f, "sizeable", text->sizeable);

	ui_edit_write_key_int(f, "rotation", text->rotation);
}

static gpointer text_edit_read(UIData *ui, WidgetData *wd, GList *list)
{
	TextData *text = wd->widget;
	FontListData *fd;
	const gchar *image;

	image = ui_widget_get_data(wd, "image");

	if (!image || font_edit_list_find(list, image)) return NULL;

	fd = g_new0(FontListData, 1);
	fd->image = g_strdup(image);
	fd->extended = text->font->extended;

	fd->key = g_strdup(wd->key);
	fd->data = g_strdup(ui_widget_get_data(wd, "data"));
	fd->text_id = g_strdup(wd->text_id);

	return fd;
}

static void text_edit_props_sizeable_cb(GtkWidget *widget, gpointer data)
{
	TextDetail *td = data;

	td->text->sizeable = ui_edit_toggle_get(td->sizeable_button);
}

static void text_edit_props_rotation_cb(GtkAdjustment *adjustment, gpointer data)
{
	TextDetail *td = data;
	TextData *text;
	gint new_r;
	gint cw, ch;

	text = td->text;
	new_r = ui_edit_spin_get(td->spin_rotation);

	if (font_rotation_vertical(new_r) != font_rotation_vertical(text->rotation))
		{
		gint old_w, old_h;
		gint t;

		old_w = text->width;
		old_h = text->height;

		ui_edit_widget_draw_highlight(td->ui, td->wd, FALSE);

		if (font_rotation_vertical(new_r))
			{
			if (text->y + text->width > td->ui->skin->height)
				{
				text->width = td->ui->skin->height - text->y;
				ui_edit_spin_set_blocking(td->spin_width, text->width, td);
				}
			}
		else
			{
			if (text->x + text->height > td->ui->skin->width)
				{
				text->height = td->ui->skin->width - text->x;
				ui_edit_spin_set_blocking(td->spin_width, text->height, td);
				}
			}

		t = text->width;
		text->width = text->height;
		text->height = t;
		gdk_pixbuf_unref(text->pixbuf);
		text->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, text->width, text->height);

		cw = MAX(0, old_w - text->width);
		ch = MAX(0, old_h - text->height);
		}
	else
		{
		cw = ch = 0;
		}
	text->rotation = new_r;

	ui_edit_widget_resync(td->ui, td->wd, TRUE, cw, ch);
}

static void text_edit_props_width_cb(GtkAdjustment *adjustment, gpointer data)
{
	TextDetail *td = data;
	gint new_w;
	gint old_w;

	new_w = ui_edit_spin_get(td->spin_width);

	if (font_rotation_vertical(td->text->rotation))
		{
		if (new_w > td->ui->skin->height)
			{
			new_w = td->ui->skin->height;
			ui_edit_spin_set_blocking(td->spin_width, new_w, td);
			}

		if (new_w == td->text->height) return;

		old_w = td->text->height;

		if (new_w > td->text->height &&
		    td->text->y + new_w > td->ui->skin->height)
			{
			ui_widget_set_coord(td->ui, td->wd, td->text->x, td->ui->skin->height - new_w, TRUE);
			}

		if (old_w > new_w) ui_edit_widget_draw_highlight(td->ui, td->wd, FALSE);
		
		td->text->height = new_w;
		}
	else
		{
		if (new_w > td->ui->skin->width)
			{
			new_w = td->ui->skin->width;
			ui_edit_spin_set_blocking(td->spin_width, new_w, td);
			}

		if (new_w == td->text->width) return;

		old_w = td->text->width;

		if (new_w > td->text->width &&
		    td->text->x + new_w > td->ui->skin->width)
			{
			ui_widget_set_coord(td->ui, td->wd, td->ui->skin->width - new_w, td->text->y, TRUE);
			}

		if (old_w > new_w) ui_edit_widget_draw_highlight(td->ui, td->wd, FALSE);

		td->text->width = new_w;
		}

	gdk_pixbuf_unref(td->text->pixbuf);
	td->text->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, new_w, td->text->height);

	ui_edit_widget_resync(td->ui, td->wd, TRUE, old_w > new_w ? old_w - new_w : 0, 0);
}

static void text_edit_props_color_cb(GtkAdjustment *adjustment, gpointer data)
{
	TextDetail *td = data;
	TextData *text;

	text = td->text;

	text->r = ui_edit_spin_get(td->red_spin);
	text->g = ui_edit_spin_get(td->green_spin);
	text->b = ui_edit_spin_get(td->blue_spin);
	text->a = ui_edit_spin_get(td->alpha_spin);

	ui_widget_draw(td->ui, td->wd, TRUE, TRUE);
}

static gpointer text_edit_props(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail)
{
	TextData *text = wd->widget;
	TextDetail *td = detail;

	if (!td)
		{
		GtkObject *adj;
		GtkWidget *hbox;

		td = g_new0(TextDetail, 1);

		td->extended_button = ui_edit_toggle_new(vbox, _("Extended"));
		gtk_widget_set_sensitive(td->extended_button, FALSE);

		td->sizeable_button = ui_edit_toggle_new(vbox, _("Dynamic size"));
		g_signal_connect(G_OBJECT(td->sizeable_button), "clicked",
				 G_CALLBACK(text_edit_props_sizeable_cb), td);

		hbox = ui_edit_frame_new(vbox, FALSE, NULL);
		td->spin_width = ui_edit_spin_new(hbox, _("Width:"), 1, 2000, &adj);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(text_edit_props_width_cb), td);
		td->spin_rotation = ui_edit_spin_new(hbox, _("Rotation:"), 0, 270, &adj);
		GTK_ADJUSTMENT(adj)->step_increment = 90.0;
		gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(td->spin_rotation), TRUE);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(text_edit_props_rotation_cb), td);

		td->font_entry = ui_edit_entry_new(vbox, _("Font:"));
		ui_edit_frame_sensitive(td->font_entry, FALSE, TRUE);

		hbox = ui_edit_frame_new(vbox, TRUE, _("Color"));

		td->red_spin = ui_edit_spin_new(hbox, _("Red:"), 0, 255, &adj);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(text_edit_props_color_cb), td);
		td->green_spin = ui_edit_spin_new(hbox, _("Green:"), 0, 255, &adj);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(text_edit_props_color_cb), td);
		td->blue_spin = ui_edit_spin_new(hbox, _("Blue:"), 0, 255, &adj);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(text_edit_props_color_cb), td);

		td->alpha_spin = ui_edit_spin_new(vbox, _("Alpha:"), 0, 255, &adj);
		g_signal_connect(adj, "value_changed",
				   G_CALLBACK(text_edit_props_color_cb), td);

		hbox = ui_edit_frame_new(vbox, FALSE, NULL);

		td->font_key_entry = ui_edit_entry_new(hbox, _("Font key:"));
		ui_edit_frame_sensitive(td->font_key_entry, FALSE, TRUE);
		}

	td->text = text;
	td->wd = wd;
	td->ui = ui;

	ui_edit_toggle_set(td->extended_button, text->font->extended);
	ui_edit_toggle_set_blocking(td->sizeable_button, text->sizeable, td);

	ui_edit_spin_set_blocking(td->spin_width, font_rotation_vertical(text->rotation) ? text->height : text->width, td);
	ui_edit_spin_set_blocking(td->spin_rotation, text->rotation, td);

	ui_edit_entry_set(td->font_entry, text->font->text_description);

	ui_edit_spin_set_blocking(td->red_spin, text->r, td);
	ui_edit_spin_set_blocking(td->green_spin, text->g, td);
	ui_edit_spin_set_blocking(td->blue_spin, text->b, td);
	ui_edit_spin_set_blocking(td->alpha_spin, text->a, td);

	ui_edit_frame_sensitive(td->red_spin, (text->font->overlay == NULL), FALSE);
	ui_edit_frame_sensitive(td->alpha_spin, (text->font->overlay == NULL), TRUE);

	ui_edit_entry_set(td->font_key_entry, ui_widget_get_data(wd, "font_id"));

	return td;
}

static void text_edit_page_font_add_cb(GtkWidget *widget, gpointer data)
{
	TextPage *pd = data;
	FontData *font;
	const gchar *key;
	const gchar *text_id;
	const gchar *font_desc;
	const gchar *image;
	gint extended;

	key = ui_edit_entry_get(pd->font_key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	font_desc = ui_edit_entry_get(pd->font_entry);;
	image = ui_edit_entry_get(pd->image_entry);
	extended = ui_edit_toggle_get(pd->extended_button);

	if (!key || (!font_desc && (!image || !isfile(image)) ) )
		{
		warning_dialog(_("Font error"), _("Font must contain a key and valid font description or image."));
		return;
		}

	if (font_desc)
		{
		font = font_new_from_x(font_desc);
		if (!font)
			{
			warning_dialog(_("Invalid font"), _("Specified font not found, text not created"));
			}
		}
	else
		{
		font = font_new_from_file(image, extended);
		}
	
	if (font)
		{
		WidgetData *wd;
		wd = font_register(pd->ed->ui->skin, font, key, text_id);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	font_unref(font);

	tab_completion_append_to_history(pd->image_entry, image);
}

static void text_edit_page_add_cb(GtkWidget *widget, gpointer data)
{
	TextPage *pd = data;
	TextData *text;
	const gchar *key;
	const gchar *text_id;
	const gchar *font_id;
	const gchar *font_desc;
	const gchar *image;

	key = ui_edit_entry_get(pd->key_entry);
	text_id = ui_edit_entry_get(pd->text_id_entry);
	font_id = ui_edit_entry_get(pd->font_id_entry);
	font_desc = ui_edit_entry_get(pd->font_entry);
	image = ui_edit_entry_get(pd->image_entry);

	if (!key ||
	    (!font_id && !image && !font_desc) ||
	    (font_id && !ui_widget_exists(pd->ed->ui, font_id, font_type_id())) ||
	    (!font_id && !font_desc && image && !isfile(image)))
		{
		warning_dialog(_("Text error"), _("Text must contain a key, and valid image, font description, or font key."));
		return;
		}

	if (font_id)
		{
		FontData *font;
		WidgetData *wd;

		wd = skin_widget_get_by_key(pd->ed->ui->skin, font_id, font_type_id());
		if (wd)
			{
			font = wd->widget;
			text = text_new(font, 0, 0,
					ui_edit_spin_get(pd->spin_width),
					ui_edit_toggle_get(pd->sizeable_button),
					ui_edit_spin_get(pd->spin_rotation));
			}
		else
			{
			text = NULL;
			}
		image = NULL;
		}
	else if (font_desc)
		{
		text = text_new_from_x(font_desc, 0, 0,
				       ui_edit_spin_get(pd->spin_width),
				       ui_edit_toggle_get(pd->sizeable_button),
				       ui_edit_spin_get(pd->spin_rotation));
		if (!text)
			{
			warning_dialog(_("Invalid font"), _("Specified font not found, text not created"));
			}
		}
	else
		{
		text = text_new_from_file(image, 0, 0,
					  ui_edit_spin_get(pd->spin_width),
					  ui_edit_toggle_get(pd->sizeable_button),
					  ui_edit_spin_get(pd->spin_rotation),
					  ui_edit_toggle_get(pd->extended_button));
		}
	if (text)
		{
		WidgetData *wd;

		text_set_color(text, ui_edit_spin_get(pd->red_spin),
			       ui_edit_spin_get(pd->green_spin),
			       ui_edit_spin_get(pd->blue_spin),
			       ui_edit_spin_get(pd->alpha_spin));

		wd = text_register(pd->ed->ui->skin, text, key, text_id);
		ui_widget_set_data(wd, "font_id", font_id);
		ui_edit_widget_add_finish(pd->ed, wd, image, ui_edit_entry_get(pd->data_entry));
		}

	if (!font_id) tab_completion_append_to_history(pd->image_entry, image);
}

static void text_edit_page_sync(TextPage *pd, FontListData *fd)
{
	if (!fd) return;

	ui_edit_entry_set(pd->font_entry, fd->description);
	ui_edit_entry_set(pd->image_entry, fd->image);

	ui_edit_toggle_set(pd->extended_button, fd->extended);
}

static void text_edit_page_list_cb(GtkTreeSelection *selection, gpointer data)
{
	TextPage *pd = data;
	FontListData *fd;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, TEXT_COLUMN_POINTER, &fd, -1);
	text_edit_page_sync(pd, fd);
}

static void text_edit_page_font_cb(GtkWidget *entry, gpointer data)
{
	TextPage *pd = data;
	gint a, b;

	a = (strlen(gtk_entry_get_text(GTK_ENTRY(pd->font_id_entry))) == 0);
	b = (strlen(gtk_entry_get_text(GTK_ENTRY(pd->font_entry))) == 0);

	ui_edit_frame_sensitive(pd->font_entry, a, TRUE);
	ui_edit_frame_sensitive(pd->image_entry, (a && b), TRUE);
}

static void text_edit_page_destroy_cb(GtkWidget *widget, gpointer data)
{
	TextPage *pd = data;

	g_free(pd);
}

static GtkWidget *text_edit_page_new(EditData *ed)
{
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *sep;
	GtkObject *adj;
	TextPage *pd;
	GtkListStore *store;

	pd = g_new0(TextPage, 1);
	pd->ed = ed;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	g_object_set_data(G_OBJECT(hbox), "page", pd);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(text_edit_page_destroy_cb), pd);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	pd->key_entry = ui_edit_key_entry_new(vbox, ed->ui, text_type_id());
	pd->data_entry = ui_edit_entry_new(vbox, _("Data:"));
	pd->text_id_entry = ui_edit_entry_new(vbox, _("Text id:"));

	pd->extended_button = ui_edit_toggle_new(vbox, _("Extended"));
	pd->sizeable_button = ui_edit_toggle_new(vbox, _("Dynamic size"));

	hbox2 = ui_edit_frame_new(vbox, FALSE, NULL);
	
	pd->spin_width = ui_edit_spin_new(hbox2, _("Width:"), 1, 2000, NULL);

	pd->spin_rotation = ui_edit_spin_new(hbox2, _("Rotation:"), 0, 270, &adj);
	GTK_ADJUSTMENT(adj)->step_increment = 90.0;
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(pd->spin_rotation), TRUE);

	pd->font_id_entry = ui_edit_entry_new(vbox, _("Font key:"));
	g_signal_connect(G_OBJECT(pd->font_id_entry), "changed",
			 G_CALLBACK(text_edit_page_font_cb), pd);

	pd->font_entry = ui_edit_font_entry_new(vbox, _("Font:"));
	g_signal_connect(G_OBJECT(pd->font_entry), "changed",
			 G_CALLBACK(text_edit_page_font_cb), pd);

	pd->image_entry = ui_edit_path_entry_new(vbox, _("Image:"), "SLIK_text_image");
	pd->image = ui_edit_image_new(vbox);
	ui_edit_path_entry_connect_image(pd->image_entry, pd->image);

	frame = ui_edit_frame_new(vbox, TRUE, _("Color"));
	pd->red_spin = ui_edit_spin_new(frame, _("Red:"), 0, 255, NULL);
	pd->green_spin = ui_edit_spin_new(frame, _("Green:"), 0, 255, NULL);
	pd->blue_spin = ui_edit_spin_new(frame, _("Blue:"), 0, 255, NULL);

	pd->alpha_spin = ui_edit_spin_new(vbox, _("Alpha:"), 0, 255, NULL);

	button = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(text_edit_page_add_cb), pd);
	gtk_widget_show(button);

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
	gtk_widget_show(sep);

	frame = ui_edit_frame_new(vbox, TRUE, _("Add font only"));
	pd->font_key_entry = ui_edit_entry_new(frame, _("Key:"));

	button = gtk_button_new_with_label(_("Add"));
	gtk_box_pack_end(GTK_BOX(frame), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(text_edit_page_font_add_cb), pd);
	gtk_widget_show(button);

	store = gtk_list_store_new(4, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING,
				      G_TYPE_BOOLEAN);
	pd->list = ui_edit_list_new(hbox, store,
				    G_CALLBACK(text_edit_page_list_cb), pd);
	g_object_unref(store);

	ui_edit_list_add_column(pd->list, _("Image"), TEXT_COLUMN_IMAGE, TRUE, FALSE);
	ui_edit_list_add_column(pd->list, _("Key"), TEXT_COLUMN_KEY, FALSE, FALSE);
	ui_edit_list_add_column(pd->list, _("Extended"), TEXT_COLUMN_EXTENDED, FALSE, FALSE);

	gtk_widget_show(hbox);

	/* hacky, but it works */
	ui_edit_widget_id_list_set_page(ed->widget_id_list, font_type_id(), hbox);

	ui_edit_spin_set(pd->red_spin, 0);
	ui_edit_spin_set(pd->green_spin, 0);
	ui_edit_spin_set(pd->blue_spin, 0);
	ui_edit_spin_set(pd->alpha_spin, 255);
	gtk_entry_set_text(GTK_ENTRY(pd->font_entry), SLIK_DEFAULT_FONT);

	return hbox;
}

static void text_edit_page_add(GtkWidget *widget, gpointer data)
{
	FontListData *fd = data;
	TextPage *pd;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	pd = g_object_get_data(G_OBJECT(widget), "page");
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->list)));

	pixbuf = ui_edit_list_pixbuf_from_file(fd->image);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   TEXT_COLUMN_POINTER, fd,
			   TEXT_COLUMN_IMAGE, pixbuf,
			   TEXT_COLUMN_KEY, fd->key,
			   TEXT_COLUMN_EXTENDED, fd->extended, -1);

	if (pixbuf) gdk_pixbuf_unref(pixbuf);
}

void text_type_init_edit(WidgetObjectData *od)
{
	od->func_get_pixbuf = text_get_pixbuf;

	od->func_edit_write = text_edit_write;

	od->func_edit_read = text_edit_read;
	od->func_edit_free = font_edit_free;

	od->func_edit_props = text_edit_props;

	od->func_edit_page_new = text_edit_page_new;
	od->func_edit_page_add = text_edit_page_add;
}
