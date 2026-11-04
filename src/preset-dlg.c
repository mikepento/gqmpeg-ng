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
#include "preset-dlg.h"

#include "display.h"
#include "players.h"
#include "types.h"
#include "ui_fileops.h"
#include "ui_utildlg.h"

/*
 *-----------------------------------------------------------------------------
 * load a file
 *-----------------------------------------------------------------------------
 */

static void preset_dialog_close_cb(FileDialog *fd, gpointer data)
{
	file_dialog_close(fd);
}

static void preset_dialog_set_cb(GtkWidget *button, gpointer data)
{
	FileDialog *fd = data;
	gint n;

	if (!fd->dest_path || isdir(fd->dest_path)) return;

	n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "preset_num"));

	if (n > -1 && n < 10)
		{
		g_free(preset_name[n]);
		g_free(preset_file[n]);

		preset_name[n] = g_strdup(filename_from_path(fd->dest_path));
		preset_file[n] = g_strdup(fd->dest_path);

		display_presets_refresh();
		}

	file_dialog_sync_history(fd, TRUE);
	file_dialog_close(fd);
}

void preset_dialog(const gchar *path, gint playlist)
{
	FileDialog *fd;
	GtkWidget *hbox;
	const gchar *text;
	const gchar *hist_key;
	gchar *base;
	gchar *filter;
	gint i;

	if (playlist)
		{
		text = _("Set preset to playlist:");
		hist_key = "preset_dlg_playlist";
		}
	else
		{
		text = _("Set preset to song:");
		hist_key = "preset_dlg_file";
		}

	fd = file_dialog_new(_("GQmpeg - set preset"), text, "GQmpeg", "preset_set",
			     preset_dialog_close_cb, NULL);

	if (playlist)
		{
		base = g_strconcat(homedir(), "/", GQMPEG_RC_DIR_PLAYLIST, NULL);;
		}
	else
		{
		base = NULL;
		}
	file_dialog_add_path_widgets(fd, base, path, hist_key, ".gqmpeg;.gqoob", _("GQmpeg Playlist File"));
	g_free(base);

	file_dialog_add_filter(fd, ".m3u", _("m3u Playlist"), FALSE);

	filter = filter_get_text_list();
	file_dialog_add_filter(fd, filter, _("Song Files"), !playlist);
	g_free(filter);

	i = 0;
	while (player_module_id_exists(i))
		{
		filter = filter_get_text_list_by_id(i);
		if (filter)
			{
			file_dialog_add_filter(fd, filter, filter_get_description_by_id(i), FALSE);
			g_free(filter);
			}
		i++;
		}

	hbox = gtk_hbox_new(TRUE, 10);
	gtk_box_pack_end(GTK_BOX(GENERIC_DIALOG(fd)->vbox), hbox, FALSE, FALSE, 5);
	gtk_box_reorder_child(GTK_BOX(GENERIC_DIALOG(fd)->vbox), hbox, 0);
	gtk_widget_show(hbox);

	for (i = 0; i < 10; i++)
		{
		GtkWidget *button;
		gchar *buf;

		buf = g_strdup_printf("%d", i + 1);
		button = gtk_button_new_with_label(buf);
		g_free(buf);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		g_signal_connect(G_OBJECT(button), "clicked",
				 G_CALLBACK(preset_dialog_set_cb), fd);
		gtk_container_add(GTK_CONTAINER(hbox), button);

		g_object_set_data(G_OBJECT(button), "preset_num", GINT_TO_POINTER(i));
		gtk_widget_show(button);
		}

	gtk_widget_show(GENERIC_DIALOG(fd)->dialog);
}

