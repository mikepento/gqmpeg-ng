/*
 * GQmpeg
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "gqmpeg.h"
#include "songinfo.h"

#include "players.h"
#include "ui_fileops.h"

static void close_song_info_window(GtkWidget *widget, gpointer data)
{
	GtkWidget *window = data;
	gtk_widget_destroy(window);
}

static void destroy_song_info_window(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_widget_destroy(widget);
}

void view_song_info(const gchar *path)
{
	GtkWidget *info_widget;
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *button;
	gchar *buf;

	if (!path) return;

	info_widget = player_module_songdata_detail_info(path);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(destroy_song_info_window), NULL);
	buf = g_strconcat(_("Song info - "), filename_from_path(path), NULL);
	gtk_window_set_title (GTK_WINDOW (window), buf);
	g_free(buf);
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);

	vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(window), vbox);
        gtk_widget_show(vbox);

	if (info_widget)
		{
		gtk_box_pack_start(GTK_BOX(vbox), info_widget, FALSE, FALSE, 0);
		gtk_widget_show(info_widget);
		}
	else
		{
		label = gtk_label_new(_("Unknown audio type"));
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
		gtk_widget_show(label);
		}

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = gtk_button_new_with_label(_("Close"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(close_song_info_window), window);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	gtk_widget_show(window);
}

/* misc utils */

GtkWidget *songinfo_add_label(GtkWidget *vbox, const gchar *text, const gchar *data)
{
	gchar *buf;
	GtkWidget *label;

	if (data)
		{
		buf = g_strconcat(text, ": ", data, NULL);
		label = gtk_label_new(buf);
		g_free(buf);
		}
	else
		{
		label = gtk_label_new(text);
		}
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	return label;
}

GtkWidget *songinfo_add_label_bool(GtkWidget *vbox, const gchar *text, gint t)
{
	gchar *data;
	if (t)
		data = _("Yes");
	else
		data = _("No");

	return songinfo_add_label(vbox, text, data);
}

GtkWidget *songinfo_add_label_number(GtkWidget *vbox, const gchar *text, gint n, const gchar *extra)
{
	GtkWidget *label;
	gchar *buf;

	buf = g_strdup_printf("%d%s%s", n, extra ? " " : "", extra ? extra : "");
	label = songinfo_add_label(vbox, text, buf);
	g_free(buf);

	return label;
}

GtkWidget *songinfo_add_entry(GtkWidget *vbox, const gchar *label, const gchar *text,
			      gint length, GtkSignalFunc change_cb, gpointer data)
{
	GtkWidget *entry;
	GtkWidget *wlabel;
	GtkWidget *hbox;
	gchar *buf;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);	

	buf = g_strconcat(label, ":", NULL);
	wlabel = gtk_label_new(buf);
	g_free(buf);
	gtk_box_pack_start(GTK_BOX(hbox), wlabel, FALSE, FALSE, 0);
	gtk_widget_show(wlabel);

	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), length);
	if (text) gtk_entry_set_text(GTK_ENTRY(entry), text);
	g_signal_connect(G_OBJECT(entry), "changed",
			 G_CALLBACK(change_cb), data);
	gtk_box_pack_end(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	gtk_widget_show(entry);

	return entry;
}

GtkWidget *songinfo_add_spin(GtkWidget *vbox, const gchar *label, gint value,
			     gint lower, gint upper, GtkSignalFunc change_cb, gpointer data)
{
	GtkObject *adj;
	GtkWidget *spin;
	GtkWidget *wlabel;
	GtkWidget *hbox;
	gchar *buf;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	buf = g_strconcat(label, ":", NULL);
	wlabel = gtk_label_new(buf);
	g_free(buf);
	gtk_box_pack_start(GTK_BOX(hbox), wlabel, FALSE, FALSE, 0);
	gtk_widget_show(wlabel);

	adj = gtk_adjustment_new((gfloat)value, (gfloat)lower, (gfloat)upper, 1.0, 1.0, 1.0);
	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_ALWAYS);
	gtk_box_pack_end(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
	gtk_widget_show(spin);

	g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(change_cb), data);

	return spin;
}

