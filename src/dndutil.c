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
#include "dndutil.h"

#include "playlist.h"
#include "ui_bookmark.h"
#include "ui_fileops.h"


/*
 *-----------------------------------------------------------------------------
 * drag and drop for main window
 *-----------------------------------------------------------------------------
 */
enum {
	TARGET_URI_LIST,
	TARGET_X_URL,
	TARGET_NETSCAPE_URL
};

static GtkTargetEntry main_drop_types[] = {
	{ "text/uri-list", 0, TARGET_URI_LIST },
	{ "x-url/http",    0, TARGET_X_URL },
	{ "x-url/ftp",     0, TARGET_X_URL },
	{ "_NETSCAPE_URL", 0, TARGET_NETSCAPE_URL }
};
static gint main_drop_count = 4;

static void dnd_main_get_data(GtkWidget *widget,
			GdkDragContext *context, gint x, gint y,
			GtkSelectionData *selection_data, guint info,
			guint time, gpointer data)
{
	GList *list = NULL;

	switch (info)
		{
		case TARGET_URI_LIST:
		case TARGET_X_URL:
		case TARGET_NETSCAPE_URL:
			list = uri_list_from_text(selection_data->data, FALSE);
			break;
		default:
			list = NULL;
			break;
		}

	if (!list) return;

	if (!list->next)
		{
		gchar *name = list->data;
		if (isdir(name))
			{
			playlist_append_from_dir(name, FALSE);
			}
		else if (is_playlist(name))
			{
			playlist_load_from_file(name, FALSE, TRUE, TRUE);
			if (status == STATUS_STOP)
				{
				current_song_set_and_play(playlist_get_first(), NULL);
				}
			}
		else if (drop_single_to_playlist)
			{
			playlist_add(name, TRUE);
			if (status == STATUS_STOP)
				{
				current_song_set_and_play(playlist_get_count() - 1, NULL);
				}
			}
		else
			{
			current_song_set_and_play(-1, name);
			}
		}
	else
		{
		GList *work = list;
		while(work)
			{
			gchar *name = work->data;
			if (isdir(name))
				{
				playlist_append_from_dir(name, FALSE);
				}
			if (!is_playlist(name))
				{
				playlist_add(name, TRUE);
				}
			work = work->next;
			}
		}

	g_list_foreach(list, (GFunc)g_free, NULL);
	g_list_free(list);
}

void dnd_init(void)
{
	/* main window */
	gtk_drag_dest_set(main_window->window,
			GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
			main_drop_types, main_drop_count, 
			GDK_ACTION_COPY);
	g_signal_connect(G_OBJECT(main_window->window), "drag_data_received",
			 G_CALLBACK(dnd_main_get_data), NULL);
}



