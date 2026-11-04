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


#ifndef SONGINFO_H
#define SONGINFO_H


void view_song_info(const gchar *path);

/* misc utils */

GtkWidget *songinfo_add_label(GtkWidget *vbox, const gchar *text, const gchar *data);
GtkWidget *songinfo_add_label_bool(GtkWidget *vbox, const gchar *text, gint t);
GtkWidget *songinfo_add_label_number(GtkWidget *vbox, const gchar *text, gint n, const gchar *extra);

GtkWidget *songinfo_add_entry(GtkWidget *vbox, const gchar *label, const gchar *text,
			      gint length, GtkSignalFunc change_cb, gpointer data);

GtkWidget *songinfo_add_spin(GtkWidget *vbox, const gchar *label, gint value,
			     gint lower, gint upper, GtkSignalFunc change_cb, gpointer data);

#endif

