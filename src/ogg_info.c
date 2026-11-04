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
#include "io_ogg123.h"

#include "playlist.h"
#include "songinfo.h"
#include "ui_fileops.h"
#include "ui_tree_edit.h"


enum {
	COMMENT_COLUMN_LISTP = 0,
	COMMENT_COLUMN_KEY,
	COMMENT_COLUMN_VALUE,
	COLUMNT_COLUMN_COUNT
};


typedef struct _SongInfoDialog SongInfoDialog;
struct _SongInfoDialog
{
	GtkWidget *vbox;
	GtkWidget *list;
	GtkWidget *save_button;
	GtkWidget *remove_button;
	gchar *path;
	GList *comments;
};

static gchar *comments_reserved[] = { "TITLE", "ARTIST", "ALBUM", "DATE", "GENRE", "TRACKNUMBER", "DESCRIPTION",
				      "COPYRIGHT", "LICENSE", "ORGANIZATION", "CONTACT", "ISRC",
				      "PERFORMER", "LOCATION", "VERSION", NULL };
static gint comments_reserved_count = 15;

static void close_song_info_window(GtkWidget *widget, gpointer data)
{
	SongInfoDialog *s = data;
	g_free(s->path);
	path_list_free(s->comments);
	g_free(s);
}

static void comments_save_cb(GtkWidget *w, gpointer data)
{
	SongInfoDialog *s = data;

	ogg123_comment_set(s->path, s->comments);
	
	playlist_update_by_path(s->path);

	gtk_widget_set_sensitive(s->save_button, FALSE);
}

static void comment_remove_cb(GtkWidget *w, gpointer data)
{
	SongInfoDialog *s = data;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreePath *tpath;
	gint row;
	GList *work;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(s->list));
	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	tpath = gtk_tree_model_get_path(store, &iter);
	row = tree_path_to_row(tpath);
	gtk_tree_path_free(tpath);

	if (row < comments_reserved_count) return;

	gtk_tree_model_get(store, &iter, COMMENT_COLUMN_LISTP, &work, -1);
	if (work && work->next)
		{
		s->comments = g_list_remove(s->comments, work->next->data);
		s->comments = g_list_remove(s->comments, work->data);
		}
	gtk_list_store_remove(GTK_LIST_STORE(store), &iter);

	gtk_widget_set_sensitive(s->save_button, TRUE);
}

static void comment_add_cb(GtkWidget *w, gpointer data)
{
	SongInfoDialog *s = data;
	GtkListStore *store;
	GtkTreeIter iter;
	GList *work;

	work = g_list_append(NULL, g_strdup(_("Newkey")));
	work = g_list_append(work, g_strdup(""));
	s->comments = g_list_concat(s->comments, work);

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(s->list)));

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   COMMENT_COLUMN_LISTP, work,
			   COMMENT_COLUMN_KEY, _("Newkey"),
			   COMMENT_COLUMN_VALUE, NULL, -1);

	gtk_widget_set_sensitive(s->save_button, TRUE);
}

static gint list_row_rename_cb(TreeEditData *ted, const gchar *oldname, const gchar *newname, gpointer data)
{
	SongInfoDialog *s = data;
	GtkTreeModel *store;
	GtkTreeIter iter;
	GList *work;
	gint iskey;

	store = gtk_tree_view_get_model(ted->tree);

	iskey = (ted->column == gtk_tree_view_get_column(ted->tree, COMMENT_COLUMN_KEY - 1));

	gtk_tree_model_get_iter(store, &iter, ted->path);
	gtk_tree_model_get(store, &iter, COMMENT_COLUMN_LISTP, &work, -1);
	if (!work)
		{
		if (iskey)
			{
			work = g_list_append(work, g_strdup(newname));
			work = g_list_append(work, g_strdup(""));
			}
		else
			{
			gint row = tree_path_to_row(ted->path);
			if (row < comments_reserved_count)
				{
				work = g_list_append(work, g_strdup(comments_reserved[row]));
				}
			else
				{
				work = g_list_append(work, g_strdup(""));
				}
			work = g_list_append(work, g_strdup(newname));
			}
		s->comments = g_list_concat(s->comments, work);
		gtk_list_store_set(GTK_LIST_STORE(store), &iter,
				   COMMENT_COLUMN_LISTP, work, -1);
		}

	if (!iskey) work = work->next;
	g_free(work->data);
	work->data = g_strdup(newname);

	gtk_list_store_set(GTK_LIST_STORE(store), &iter,
			   (iskey) ? COMMENT_COLUMN_KEY : COMMENT_COLUMN_VALUE, newname, -1);

	gtk_widget_set_sensitive(s->save_button, TRUE);

	return TRUE;
}

static gint list_press_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	SongInfoDialog *s = data;
	GtkTreeModel *store;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *tcolumn;
	GtkTreePath *tpath;
	GtkTreeIter iter;
	gint column;
	gint ret = FALSE;

	if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bevent->x, bevent->y,
					  &tpath, &tcolumn, NULL, NULL))
		{
		return FALSE;
		}

	if (tcolumn == gtk_tree_view_get_column(GTK_TREE_VIEW(widget), COMMENT_COLUMN_KEY -1))
		{
		column = COMMENT_COLUMN_KEY - 1;
		}
	else
		{
		column = COMMENT_COLUMN_VALUE -1;
		}

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	gtk_tree_model_get_iter(store, &iter, tpath);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	if (gtk_tree_selection_iter_is_selected(selection, &iter) &&
	    !((column == COMMENT_COLUMN_KEY - 1) && tree_path_to_row(tpath) < comments_reserved_count))
		{
		const gchar *key;
		const gchar *value;

		gtk_tree_model_get(store, &iter,
				   COMMENT_COLUMN_KEY, &key,
				   COMMENT_COLUMN_VALUE, &value, -1);
		tree_edit_by_path(GTK_TREE_VIEW(widget), tpath, column,
				  (column == COMMENT_COLUMN_KEY - 1) ? key : value,
				  list_row_rename_cb, s);
		ret = TRUE;
		}

	gtk_tree_path_free(tpath);

	return ret;
}

static void list_select_cb(GtkTreeSelection *selection, gpointer data)
{
	SongInfoDialog *s = data;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(selection, &store, &iter))
		{
		GtkTreePath *tpath;

		tpath = gtk_tree_model_get_path(store, &iter);
		gtk_widget_set_sensitive(s->remove_button, tree_path_to_row(tpath) >= comments_reserved_count);
		gtk_tree_path_free(tpath);
		}
}

GtkWidget *ogg123_info_window(const gchar *file)
{
	SongInfoDialog *s;
	OGGInfo *info;
	GtkWidget *hbox;
	GtkWidget *vbox1;
	GtkWidget *frame;
	GtkWidget *scrolled;
	GtkWidget *button;
	GList *temp, *work;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	gint i;

	s = g_new0(SongInfoDialog, 1);
	info = ogg123_info_get(file);
	s->comments = ogg123_comment_get(file);

	s->path = g_strdup(file);

	s->vbox = gtk_vbox_new(FALSE, 0);
	g_signal_connect(G_OBJECT(s->vbox), "destroy",
			 G_CALLBACK(close_song_info_window), s);
	gtk_container_set_border_width(GTK_CONTAINER(s->vbox), 5);

	songinfo_add_label(s->vbox, _("Filename"), filename_from_path(file));

	frame = gtk_frame_new(_("Song info"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(s->vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 5);
        gtk_container_add(GTK_CONTAINER(frame),vbox1);
        gtk_widget_show(vbox1);

	if (info)
		{
		const gchar *text;

		songinfo_add_label(vbox1, _("Type"), _("ogg bitstream file"));
		songinfo_add_label(vbox1, _("Vendor"), (info->vendor) ? info->vendor : "");

		songinfo_add_label_bool(vbox1, _("Integrity check passed"), info->pass_integrity);
		songinfo_add_label_number(vbox1, _("Channels"), info->channels, NULL);

		text = _("Bit Rate (lowest)");
		if (info->bitrate_lower < 0)
			{
			songinfo_add_label(vbox1, text, _("none set"));
			}
		else
			{
			songinfo_add_label_number(vbox1, text, info->bitrate_lower / 1000, _("Kbits"));
			}
		text = _("Bit Rate (highest)");
		if (info->bitrate_upper < 0)
			{
			songinfo_add_label(vbox1, text, _("none set"));
			}
		else
			{
			songinfo_add_label_number(vbox1, text, info->bitrate_upper / 1000, _("Kbits"));
			}
	
		songinfo_add_label_number(vbox1, _("Bit Rate (encoded)"), info->bitrate_nominal / 1000, _("Kbits"));
		songinfo_add_label_number(vbox1, _("Bit Rate (average)"), info->bitrate_average / 1000, _("Kbits"));
		songinfo_add_label_number(vbox1, _("Length"), info->length, _("seconds"));
		songinfo_add_label_number(vbox1, _("File Size"), info->bytes, _("bytes"));
		}
	else
		{
		songinfo_add_label(vbox1, _("unable to read file info"), NULL);
		}

	frame = gtk_frame_new(_("Comments"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(s->vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER (vbox1), 5);
        gtk_container_add(GTK_CONTAINER(frame), vbox1);
        gtk_widget_show(vbox1);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolled, TRUE, TRUE, 5);
	gtk_widget_show(scrolled);

	store = gtk_list_store_new(3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
	s->list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_widget_set_size_request(s->list, 300, 150);
	g_signal_connect(G_OBJECT(s->list), "button_press_event",
			 G_CALLBACK(list_press_cb), s);
	gtk_container_add(GTK_CONTAINER(scrolled), s->list);
	gtk_widget_show(s->list);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Key"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", COMMENT_COLUMN_KEY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(s->list), column);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Description"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", COMMENT_COLUMN_VALUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(s->list), column);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(s->list));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(selection), "changed",
			 G_CALLBACK(list_select_cb), s);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = gtk_button_new_with_label(_("Add"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(comment_add_cb), s);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	gtk_widget_show(button);

	s->remove_button = gtk_button_new_with_label(_("Remove"));
	g_signal_connect(G_OBJECT(s->remove_button), "clicked",
			 G_CALLBACK(comment_remove_cb), s);
	gtk_container_add(GTK_CONTAINER(hbox), s->remove_button);
	gtk_widget_set_sensitive(s->remove_button, FALSE);
	gtk_widget_show(s->remove_button);

	s->save_button = gtk_button_new_with_label(_("Save Comments"));
	g_signal_connect(G_OBJECT(s->save_button), "clicked",
			 G_CALLBACK(comments_save_cb), s);
	gtk_container_add(GTK_CONTAINER(hbox), s->save_button);
	gtk_widget_set_sensitive(s->save_button, FALSE);
	gtk_widget_show(s->save_button);

	temp = g_list_copy(s->comments);
	i = 0;
	while (comments_reserved[i] != NULL)
		{
		GList *found = NULL;
		GtkTreeIter iter;

		work = s->comments;
		while (work && work->next && !found)
			{
			const gchar *key = work->data;
			if (strcasecmp(key, comments_reserved[i]) == 0) found = work;
			work = work->next;
			work = work->next;
			}

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   COMMENT_COLUMN_LISTP, found,
				   COMMENT_COLUMN_KEY, comments_reserved[i],
				   COMMENT_COLUMN_VALUE, (found) ? found->next->data : NULL, -1);

		if (found)
			{
			temp = g_list_remove(temp, found->next->data);
			temp = g_list_remove(temp, found->data);
			}
		i++;
		}

	work = temp;
	while (work && work->next)
		{
		GtkTreeIter iter;

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   COMMENT_COLUMN_LISTP, g_list_find(s->comments, work->data),
				   COMMENT_COLUMN_KEY, work->data,
				   COMMENT_COLUMN_VALUE, work->next->data, -1);

		work = work->next;
		work = work->next;
		}
	g_list_free(temp);

	ogg123_info_free(info);

	return s->vbox;
}


