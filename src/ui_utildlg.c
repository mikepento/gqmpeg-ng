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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <gtk/gtk.h>

#include <gdk/gdkkeysyms.h> /* for keyboard values */

#include "ui_utildlg.h"

#include "ui_fileops.h"
#include "ui_pathsel.h"
#include "ui_tabcomp.h"


#define DIALOG_BUTTON_SPACING 10


/*
 *-----------------------------------------------------------------------------
 * generic dialog
 *-----------------------------------------------------------------------------
 */ 

void generic_dialog_close(GenericDialog *gd)
{
	gtk_widget_destroy(gd->dialog);
	g_free(gd);
}

static void generic_dialog_click_cb(GtkWidget *widget, gpointer data)
{
	GenericDialog *gd = data;
	void (*func)(GenericDialog *, gpointer);
	gint auto_close;

	func = g_object_get_data(G_OBJECT(widget), "dialog_function");
	auto_close = gd->auto_close;

	if (func) func(gd, gd->data);
	if (auto_close) generic_dialog_close(gd);
}

static gint generic_dialog_default_key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GenericDialog *gd = data;

	if (event->keyval == GDK_Return && GTK_WIDGET_HAS_FOCUS(widget))
		{
		if (gd->default_cb) gd->default_cb(gd, gd->data);
		if (gd->auto_close) generic_dialog_click_cb(widget, data);
		return TRUE;
		}
	return FALSE;
}

void generic_dialog_attach_default(GenericDialog *gd, GtkWidget *widget)
{
	if (!gd || !widget) return;
	g_signal_connect(G_OBJECT(widget), "key_press_event",
			 G_CALLBACK(generic_dialog_default_key_press_cb), gd);
}

static gint generic_dialog_key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GenericDialog *gd = data;

	if (event->keyval == GDK_Escape)
		{
		if (gd->cancel_cb) gd->cancel_cb(gd, gd->data);
		if (gd->auto_close) generic_dialog_click_cb(widget, data);
		return TRUE;
		}
	return FALSE;
}

static gint generic_dialog_delete_cb(GtkWidget *w, GdkEventAny *event, gpointer data)
{
	GenericDialog *gd = data;
	gint auto_close;

	auto_close = gd->auto_close;

	if (gd->cancel_cb) gd->cancel_cb(gd, gd->data);
	if (auto_close) generic_dialog_close(gd);

	return TRUE;
}

static void generic_dialog_show_cb(GtkWidget *widget, gpointer data)
{
	GenericDialog *gd = data;
	if (gd->cancel_button)
	       	{
		gtk_box_reorder_child(GTK_BOX(gd->hbox), gd->cancel_button, -1);
		}

	g_signal_handlers_disconnect_by_func(G_OBJECT(widget),
					     G_CALLBACK(generic_dialog_show_cb), gd);
}

GtkWidget *generic_dialog_add_stock(GenericDialog *gd, const gchar *text, const gchar *stock_id,
				    void (*func_cb)(GenericDialog *, gpointer), gint is_default)
{
	GtkWidget *button;
	if (stock_id)
		{
		if (text)
			{
			GtkWidget *align;
			GtkWidget *hbox;
			GtkWidget *image;
			GtkWidget *label;

			button = gtk_button_new();
			hbox = gtk_hbox_new (FALSE, 2);
			image = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_BUTTON);
			label = gtk_label_new_with_mnemonic(text);
			gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
			gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);
			
			gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
			gtk_widget_show(image);
			gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
			gtk_widget_show(label);

			align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(align), hbox);
			gtk_widget_show(hbox);
			gtk_container_add(GTK_CONTAINER(button), align);
			gtk_widget_show(align);
			}
		else
			{
			button = gtk_button_new_from_stock(stock_id);
			}
		}
	else
		{
		button = gtk_button_new_with_label(text);
		}

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	g_object_set_data(G_OBJECT(button), "dialog_function", func_cb);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(generic_dialog_click_cb), gd);

	gtk_container_add(GTK_CONTAINER(gd->hbox), button);

	if (is_default)
		{
		gtk_widget_grab_default(button);
		gtk_widget_grab_focus(button);
		gd->default_cb = func_cb;
		}

	gtk_widget_show(button);

	return button;
}

GtkWidget *generic_dialog_add(GenericDialog *gd, const gchar *text,
			     void (*func_cb)(GenericDialog *, gpointer), gint is_default)
{
	return generic_dialog_add_stock(gd, text, NULL, func_cb, is_default);
}

static void generic_dialog_setup(GenericDialog *gd,
				 const gchar *title, const gchar *message,
				 const gchar *wmclass, const gchar *wmsubclass, gint auto_close,
				 void (*cancel_cb)(GenericDialog *, gpointer), gpointer data)
{
	GtkWidget *vbox;
	GtkWidget *label;

	gd->auto_close = auto_close;
	gd->data = data;
	gd->cancel_cb = cancel_cb;

	gd->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(gd->dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_wmclass(GTK_WINDOW(gd->dialog), wmsubclass, wmclass);
	g_signal_connect(G_OBJECT(gd->dialog), "delete_event",
			 G_CALLBACK(generic_dialog_delete_cb), gd);
	g_signal_connect(G_OBJECT(gd->dialog), "key_press_event",
			 G_CALLBACK(generic_dialog_key_press_cb), gd);
	gtk_window_set_resizable(GTK_WINDOW(gd->dialog), TRUE);
	gtk_window_set_title(GTK_WINDOW (gd->dialog), title);
	gtk_container_set_border_width(GTK_CONTAINER (gd->dialog), 5);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(gd->dialog), vbox);
	gtk_widget_show(vbox);

	gd->vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gd->vbox, TRUE, TRUE, 0);
	gtk_widget_show(gd->vbox);

	if (message)
		{
		label = gtk_label_new(message);
		gtk_box_pack_start(GTK_BOX(gd->vbox), label, FALSE, FALSE, 0);
		gtk_widget_show(label);
		}

	gd->hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(gd->hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(gd->hbox), DIALOG_BUTTON_SPACING);
	gtk_box_pack_start(GTK_BOX(vbox), gd->hbox, FALSE, FALSE, 5);
	gtk_widget_show(gd->hbox);

	if (gd->cancel_cb)
	       	{
		gd->cancel_button = generic_dialog_add_stock(gd, NULL, GTK_STOCK_CANCEL, gd->cancel_cb, TRUE);
		}
	else
		{
		gd->cancel_button = NULL;
		}

	g_signal_connect(G_OBJECT(gd->dialog), "show",
			 G_CALLBACK(generic_dialog_show_cb), gd);

	gd->default_cb = NULL;
}

GenericDialog *generic_dialog_new(const gchar *title, const gchar *message,
				  const gchar *wmclass, const gchar *wmsubclass, gint auto_close,
				  void (*cancel_cb)(GenericDialog *, gpointer), gpointer data)
{
	GenericDialog *gd;

	gd = g_new0(GenericDialog, 1);
	generic_dialog_setup(gd, title, message, wmclass, wmsubclass, auto_close, cancel_cb, data);
	return gd;
}
/*
 *-----------------------------------------------------------------------------
 * simple warning dialog
 *-----------------------------------------------------------------------------
 */ 

static void warning_dialog_ok_cb(GenericDialog *gd, gpointer data)
{
	/* no op */
}

GenericDialog *warning_dialog(const gchar *title, const gchar *message)
{
	GenericDialog *gd;

	gd = generic_dialog_new(title, message, "SLIK", "warning", TRUE, NULL, NULL);
	generic_dialog_add_stock(gd, NULL, GTK_STOCK_OK, warning_dialog_ok_cb, TRUE);

	gtk_widget_show(gd->dialog);

	return gd;
}

/*
 *-----------------------------------------------------------------------------
 * generic file ops dialog routines
 *-----------------------------------------------------------------------------
 */ 

void file_dialog_close(FileDialog *fd)
{
	g_free(fd->source_path);
	g_free(fd->dest_path);
	if (fd->source_list) path_list_free(fd->source_list);

	generic_dialog_close(GENERIC_DIALOG(fd));
}

FileDialog *file_dialog_new(const gchar *title, const gchar *message,
			    const gchar *wmclass, const gchar *wmsubclass,
			    void (*cancel_cb)(FileDialog *, gpointer), gpointer data)
{
	FileDialog *fd = NULL;

	fd = g_new0(FileDialog, 1);

	generic_dialog_setup(GENERIC_DIALOG(fd), title, message,
			     wmclass, wmsubclass, FALSE,
			     (void *)cancel_cb, data);

	return fd;
}

GtkWidget *file_dialog_add_button(FileDialog *fd, const gchar *text,
				  void (*func_cb)(FileDialog *, gpointer), gint is_default)
{
	return generic_dialog_add(GENERIC_DIALOG(fd), text,
				  (void *)func_cb, is_default);
}

GtkWidget *file_dialog_add_button_stock(FileDialog *fd, const gchar *text, const gchar *stock_id,
					void (*func_cb)(FileDialog *, gpointer), gint is_default)
{
	return generic_dialog_add_stock(GENERIC_DIALOG(fd), text, stock_id,
					(void *)func_cb, is_default);
}

static void file_dialog_entry_cb(GtkWidget *widget, gpointer data)
{
	FileDialog *fd = data;
	g_free(fd->dest_path);
	fd->dest_path = remove_trailing_slash(gtk_entry_get_text(GTK_ENTRY(fd->entry)));
}

static void file_dialog_entry_enter_cb(const gchar *path, gpointer data)
{
	GenericDialog *gd = data;

	file_dialog_entry_cb(NULL, data);

	if (gd->default_cb) gd->default_cb(gd, gd->data);
}

void file_dialog_add_path_widgets(FileDialog *fd, const gchar *default_path, const gchar *path,
				  const gchar *history_key, const gchar *filter, const gchar *filter_desc)
{
	GtkWidget *tabcomp;
	GtkWidget *list;

	if (fd->entry) return;

	tabcomp = tab_completion_new_with_history(&fd->entry, NULL,
		  history_key, -1, file_dialog_entry_enter_cb, fd);
	gtk_box_pack_end(GTK_BOX(GENERIC_DIALOG(fd)->vbox), tabcomp, FALSE, FALSE, 0);
	generic_dialog_attach_default(GENERIC_DIALOG(fd), fd->entry);
	gtk_widget_show(tabcomp);

	if (path && path[0] == '/')
		{
		fd->dest_path = g_strdup(path);
		}
	else
		{
		const gchar *base;

		base = tab_completion_set_to_last_history(fd->entry);

		if (!base) base = default_path;
		if (!base) base = homedir();

		if (path)
			{
			fd->dest_path = concat_dir_and_file(base, path);
			}
		else
			{
			fd->dest_path = g_strdup(base);
			}
		}

	list = path_selection_new_with_files(fd->entry, fd->dest_path, filter, filter_desc);
	path_selection_add_select_func(fd->entry, file_dialog_entry_enter_cb, fd);
	gtk_box_pack_end(GTK_BOX(GENERIC_DIALOG(fd)->vbox), list, TRUE, TRUE, 5);
	gtk_widget_show(list);

	gtk_widget_grab_focus(fd->entry);
	if (fd->dest_path)
		{
		gtk_entry_set_text(GTK_ENTRY(fd->entry), fd->dest_path);
		gtk_editable_set_position(GTK_EDITABLE(fd->entry), strlen(fd->dest_path));
		}

	g_signal_connect(G_OBJECT(fd->entry), "changed",
			 G_CALLBACK(file_dialog_entry_cb), fd);
}

void file_dialog_add_filter(FileDialog *fd, const gchar *filter, const gchar *filter_desc, gint set)
{
	if (!fd->entry) return;
	path_selection_add_filter(fd->entry, filter, filter_desc, set);
}

void file_dialog_clear_filter(FileDialog *fd)
{
	if (!fd->entry) return;
	path_selection_clear_filter(fd->entry);
}

void file_dialog_sync_history(FileDialog *fd, gint dir_only)
{
	if (!fd->dest_path) return;

	if (!dir_only ||
	    (dir_only && isdir(fd->dest_path)) )
		{
		tab_completion_append_to_history(fd->entry, fd->dest_path);
		}
	else
		{
		gchar *buf = remove_level_from_path(fd->dest_path);
		tab_completion_append_to_history(fd->entry, buf);
		g_free(buf);
		}
}


