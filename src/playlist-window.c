/*
 * GQmpeg
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "gqmpeg.h"
#include "playlist-window.h"

#include "btn_funcs.h"
#include "display.h"
#include "dndutil.h"
#include "filelist.h"
#include "menus.h"
#include "playlist.h"
#include "playlist-dlg.h"
#include "songinfo.h"
#include "utilops.h"
#include "ui_bookmark.h"
#include "ui_fileops.h"
#include "ui_menu.h"
#include "ui_tabcomp.h"
#include "ui_tree_edit.h"
#include "ui2_main.h"

#include "icon-rarrow.xpm"
#include "icon-alert.xpm"
#include "icon-question.xpm"
#include "icon-custom.xpm"

#define DIR_DEFAULT_HEIGHT 170
#define DIR_DEFAULT_WIDTH 300

#define PW_SCROLL_DELAY 60
#define PW_SCROLL_THRESHHOLD 24

/*
 *-----------------------------------------------------------------------------
 * static vars
 *-----------------------------------------------------------------------------
 */

static GtkWidget *dirlist_view;
static GtkWidget *filelist_view;
static GtkWidget *playlist_view;
static GtkWidget *pathentry;
static GtkWidget *historypulldown;
static GtkWidget *fileinfolabel;

static GtkWidget *s_scrolled;
static GtkWidget *s_hbox;


#define AUTO_SCROLL_REGION_SIZE 48


static void playlist_window_build(void);

/*
 *-----------------------------------------------------------------------------
 * Playlist population routines.
 *-----------------------------------------------------------------------------
 */

enum {
	COLUMN_ICON = 0,
	COLUMN_TITLE,
	COLUMN_ALBUM,
	COLUMN_ARTIST,
	COLUMN_YEAR,
	COLUMN_TIME,
	COLUMN_GENRE,
	COLUMN_FILENAME,
	COLUMN_COMMENT,
	COLUMN_TRACK,
	COLUMN_ELEMENT_COUNT
};

#define FLIST_COLUMN_NAME 0

/* width of each column, corresponds to enum numbers, above */
static gint playlist_view_widths[COLUMN_ELEMENT_COUNT] = {0, 160, 100, 100, 0, 0, 60, 160, 160, 0};

/* the column order, to change the order just rearrange this list */
static gint playlist_view_config[COLUMN_ELEMENT_COUNT] = {
	COLUMN_ICON,
	COLUMN_TITLE,
	COLUMN_ALBUM,
	COLUMN_ARTIST,
	COLUMN_YEAR,
	COLUMN_TIME,
	COLUMN_GENRE,
	COLUMN_TRACK,
	COLUMN_FILENAME,
	COLUMN_COMMENT
};

static const gchar *playlist_view_titles[COLUMN_ELEMENT_COUNT] = {
	"",
	N_("Title"),
	N_("Album"),
	N_("Artist"),
	N_("Year"),
	N_("Time"),
	N_("Genre"),
	N_("Filename"),
	N_("Comment"),
	N_("Track")
};


/* sorting variables */
static gint playlist_view_sort_column = -1;
static gint playlist_view_sort_invert = 0;

	
static gint playlist_column_lookup(gint type)
{
	gint i;

	for (i = 0; i < COLUMN_ELEMENT_COUNT; i++)
		{
		if (playlist_view_config[i] == type) return i;
		}

	return -1;
}

static gchar* playlist_column_data(SongData* sd, gint column)
{
	gint view;

	if (!sd || (column < 0) || (column >= COLUMN_ELEMENT_COUNT) || column == COLUMN_ICON)
		{
		return "";
		}

	view = playlist_view_config[column];
	switch (view)
		{
		case COLUMN_TITLE:
			if (sd->title)
				{
				return sd->title;
				}
			else
				{
				return (gchar *)filename_from_path(sd->path);
				}
		case COLUMN_ALBUM:
			return sd->album;
		case COLUMN_ARTIST:
			return sd->artist;
		case COLUMN_YEAR:
		        return sd->year;
		case COLUMN_GENRE:
			return sd->genre;
		case COLUMN_FILENAME:
			return (gchar *)filename_from_path(sd->path);
		case COLUMN_COMMENT:
			return sd->comment;
		default:
			return "";
		}
}

static void playlist_view_update_data(GtkListStore *store, GtkTreeIter *iter, SongData *sd)
{
	gchar *length_text;
	gchar *track_text = NULL;

	if (!iter || !sd) return;

	length_text = playlist_data_get_time(sd);
	if (sd->track > 0) track_text = g_strdup_printf("%d", sd->track);

	gtk_list_store_set(store, iter,
			   COLUMN_TITLE, (sd->title) ? sd->title : filename_from_path(sd->path),
			   COLUMN_ALBUM, sd->album,
			   COLUMN_ARTIST, sd->artist,
			   COLUMN_YEAR, sd->year,
			   COLUMN_TIME, length_text,
			   COLUMN_GENRE, sd->genre,
			   COLUMN_FILENAME, filename_from_path(sd->path),
			   COLUMN_COMMENT, sd->comment,
			   COLUMN_TRACK, track_text, -1);

	g_free(length_text);
	g_free(track_text);
}

static GdkPixbuf *playlist_view_pick_flag(SongFlags flags)
{
	static GdkPixbuf *pb_arrow = NULL;
	static GdkPixbuf *pb_warning = NULL;
	static GdkPixbuf *pb_question = NULL;
	static GdkPixbuf *pb_custom = NULL;
	GdkPixbuf *pb = NULL;

	if (flags & SONG_FLAG_PLAYING)
		{
		if (!pb_arrow) pb_arrow = gdk_pixbuf_new_from_xpm_data((const gchar **)icon_rarrow_xpm);
		pb = pb_arrow;
		}
	else if ((flags & SONG_FLAG_NOT_FOUND) || (flags & SONG_FLAG_PLAY_FAIL))
		{
		if (!pb_warning) pb_warning = gdk_pixbuf_new_from_xpm_data((const gchar **)icon_alert_xpm);
		pb = pb_warning;
		}
	else if (flags & SONG_FLAG_UNKNOWN_TYPE)
		{
		if (!pb_question) pb_question = gdk_pixbuf_new_from_xpm_data((const gchar **)icon_question_xpm);
		pb = pb_question;
		}
	else if (flags & SONG_FLAG_CUSTOMIZED)
		{
		if (!pb_custom) gdk_pixbuf_new_from_xpm_data((const gchar **)icon_custom_xpm);
		pb = pb_custom;
		}

	return pb;
}

static void playlist_view_update_flags(GtkListStore *store, GtkTreeIter *iter, SongFlags flags)
{
	GdkPixbuf *pixbuf;

	if (!iter) return;

	pixbuf = playlist_view_pick_flag(flags);
	gtk_list_store_set(store, iter, COLUMN_ICON, pixbuf, -1);
}

static void playlist_view_append(GtkTreeView *view, gint pos)
{
	GtkListStore *store;
	GtkTreeIter iter;
	SongData *sd;

	sd = playlist_get_data(pos);

	store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
	gtk_list_store_append(store, &iter);
	playlist_view_update_data(store, &iter, sd);
	playlist_view_update_flags(store, &iter, sd->flags);
}

static void playlist_view_insert(GtkTreeView *view, gint pos)
{
	GtkListStore *store;
	GtkTreeIter iter;
	SongData *sd;

	sd = playlist_get_data(pos);

	store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
	gtk_list_store_insert(store, &iter, pos);
	playlist_view_update_data(store, &iter, sd);
	playlist_view_update_flags(store, &iter, sd->flags);
}

static void playlist_view_update(GtkTreeView *view, gint pos)
{
	GtkListStore *store;
	GtkTreeIter iter;
	SongData *sd;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
	if (pos < 0 || !gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, pos)) return;

	sd = playlist_get_data(pos);
	playlist_view_update_data(store, &iter, sd);
	playlist_view_update_flags(store, &iter, sd->flags);
}

static gint playlist_sort_compare_func(gconstpointer a, gconstpointer b)
{
	gint result;
	gchar *sa, *sb;
	gint column;

	column = playlist_view_config[playlist_view_sort_column];

	if (column == COLUMN_TIME || column == COLUMN_TRACK)
		{
		gint al, bl;

		if (column == COLUMN_TIME)
			{
			al = ((SongData*)a)->length;
			bl = ((SongData*)b)->length;
			}
		else
			{
			al = ((SongData*)a)->track;
			bl = ((SongData*)b)->track;
			}

		if (al < bl)
			result = -1;
		else if (al > bl)
			result = 1;
		else
			result = 0;
		}
	else
		{
		sa = playlist_column_data((SongData*) a, playlist_view_sort_column);
		sb = playlist_column_data((SongData*) b, playlist_view_sort_column);
		if (!sa) sa = "";
		if (!sb) sb = "";

		result = strcasecmp(sa, sb);
		}

	if (playlist_view_sort_invert)
		return -result;
	else
		return result;
}

static void playlist_view_column_clicked(GtkTreeViewColumn *tcolumn, gpointer data)
{
	GtkListStore *store;
	GtkTreeView *view;
	gint column_id = GPOINTER_TO_INT(data);
	gint i;
	gint l;

	if (playlist_get_count() < 2) return;

	view = GTK_TREE_VIEW(playlist_view);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
	gtk_list_store_clear(store);

	if (column_id == playlist_view_sort_column)
		{
		playlist_view_sort_invert = (!playlist_view_sort_invert);
		}
	else
		{
		playlist_view_sort_column = column_id;
		playlist_view_sort_invert = 0;
		}

	playlist_sort_by_func((GCompareFunc) playlist_sort_compare_func);

	l = playlist_get_count();
	for (i = 0; i < l; i++)
		{
		playlist_view_append(view, i);
		}

	display_total_time_changed();
	display_set_song_number(current_song_get_number());
}

/*
 *-----------------------------------------------------------------------------
 * list window construction
 *-----------------------------------------------------------------------------
 */

static GtkWidget *list_new(GtkListStore *store, GtkWidget **tree)
{
	GtkWidget *scrolled;

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	*tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_container_add(GTK_CONTAINER(scrolled), *tree);
	gtk_widget_show(*tree);

	return scrolled;
}

static void list_add_column(GtkWidget *tree, const gchar *title, gint column_id, gint image,
			    gint size, gint right_justify,
			    GCallback click_func, gpointer click_data)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, title);

	if (size >= 0)
		{
		gtk_tree_view_column_set_min_width(column, 4);
		if (size > 0) gtk_tree_view_column_set_fixed_width(column, size);
		gtk_tree_view_column_set_sizing(column,
			(image || size == 0) ? GTK_TREE_VIEW_COLUMN_GROW_ONLY : GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_resizable(column, (!image));
		}
	else
		{
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		}

	if (!image)
		{
		renderer = gtk_cell_renderer_text_new();
		if (right_justify) g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", column_id);
		}
	else
		{
		renderer = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", column_id);
		}

	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	if (click_func && !image)
		{
		gtk_tree_view_column_set_clickable(column, TRUE);
		g_signal_connect(G_OBJECT(column), "clicked",
				 G_CALLBACK(click_func), click_data);
		}
}


/*
 *-----------------------------------------------------------------------------
 * playlist window file status functions (public)
 *-----------------------------------------------------------------------------
 */

void playlist_window_update_song_icon_by_flags(gint n, SongFlags flags)
{
	GtkListStore *store;
	GtkTreeIter iter;

	display_playlist_row_update(n);

	if (!playlist_window) return;
                                                                                                                   
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_view)));
	if (n < 0 || !gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, n)) return;
                                                                                                                   
	playlist_view_update_flags(store, &iter, flags);
}

void playlist_window_update_song_flags(gint n)
{
	if (n < 0 || n > playlist_get_count()) return;
	playlist_window_update_song_icon_by_flags(n, playlist_get_flags(n));
}

/*
 *-----------------------------------------------------------------------------
 * playlist window management functions (public)
 *-----------------------------------------------------------------------------
 */

void playlist_window_append(gint n)
{
	display_playlist_row_insert(n);

	if (!playlist_window) return;

	playlist_view_append(GTK_TREE_VIEW(playlist_view), n);
	playlist_window_length_update();
}

void playlist_window_insert(gint n)
{
	display_playlist_row_insert(n);

	if (!playlist_window) return;

	playlist_view_insert(GTK_TREE_VIEW(playlist_view), n);

	playlist_window_length_update();
}

void playlist_window_move(gint s, gint t)
{
	GtkTreeModel *store;
	GtkTreeIter iter_s;
	GtkTreeIter iter_t;

	display_playlist_refresh();

	if (!playlist_window || s == t || s < 0 || t < 0) return;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_view));
	if (gtk_tree_model_iter_nth_child(store, &iter_s, NULL, s) &&
	    gtk_tree_model_iter_nth_child(store, &iter_t, NULL, t))
		{
		if (s < t)
			{
			gtk_list_store_move_after(GTK_LIST_STORE(store), &iter_s, &iter_t);
			}
		else
			{
			if (t == 0)
				{
				gtk_list_store_move_after(GTK_LIST_STORE(store), &iter_s, NULL);
				}
			else
				{
				gtk_list_store_move_before(GTK_LIST_STORE(store), &iter_s, &iter_t);
				}
			}		
		}
}

void playlist_window_remove(gint n)
{
	GtkTreeModel *store;
	GtkTreeIter iter;

	display_playlist_row_remove(n);

	if (!playlist_window) return;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_view));
	if (n >= 0 && gtk_tree_model_iter_nth_child(store, &iter, NULL, n))
		{
		gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
		}

	playlist_window_length_update();
}

void playlist_window_update_item(gint n)
{
	display_playlist_row_update(n);

	if (!playlist_window) return;

	playlist_view_update(GTK_TREE_VIEW(playlist_view), n);
}

void playlist_window_clear(void)
{
	GtkListStore *store;

	display_playlist_refresh();

	if (!playlist_window) return;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_view)));
	gtk_list_store_clear(store);

	playlist_window_length_update();
}

void playlist_window_populate(void)
{
	GtkTreeView *view;
	gint i;
	gint l;

	display_playlist_refresh();

	if (!playlist_window) return;

	view = GTK_TREE_VIEW(playlist_view);
	l = playlist_get_count();
	for (i=0; i < l; i++)
		{
		playlist_view_append(view, i);
		}

	playlist_window_length_update();
}

/*
 *-----------------------------------------------------------------------------
 * view selection util functions
 *-----------------------------------------------------------------------------
 */

static void view_select_all(GtkTreeView *view, gint all)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(view);
	if (all)
		{
		gtk_tree_selection_select_all(selection);
		}
	else
		{
		gtk_tree_selection_unselect_all(selection);
		}
}

static void view_select_row(GtkTreeView *view, gint row, gint select)
{
	GtkTreeModel *store;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	store = gtk_tree_view_get_model(view);
	if (row < 0 || !gtk_tree_model_iter_nth_child(store, &iter, NULL, row)) return;

	selection = gtk_tree_view_get_selection(view);
	if (select)
		{
		gtk_tree_selection_select_iter(selection, &iter);
		}
	else
		{
		gtk_tree_selection_unselect_iter(selection, &iter);
		}
}

static gint view_selection_count(GtkTreeView *view)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(view);
	return gtk_tree_selection_count_selected_rows(selection);
}

static gint view_row_is_selected(GtkTreeView *view, gint row)
{
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection(view);
	store = gtk_tree_view_get_model(view);
	if (row >= 0 && gtk_tree_model_iter_nth_child(store, &iter, NULL, row))
		{
		return gtk_tree_selection_iter_is_selected(selection, &iter);
		}

	return FALSE;
}

static gint view_selection_get_first_row(GtkTreeView *view)
{
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GList *list;
	gint row = -1;

	selection = gtk_tree_view_get_selection(view);
	list = gtk_tree_selection_get_selected_rows(selection, &store);

	if (list)
		{
		row = tree_path_to_row((GtkTreePath *)list->data);
		}

	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	return row;
}

static GList *fileview_selection_get_path_list(GtkTreeView *view)
{
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GList *slist;
	GList *work;
	GList *list = NULL;

	selection = gtk_tree_view_get_selection(view);
	slist = gtk_tree_selection_get_selected_rows(selection, &store);

	work = slist;
	while (work)
		{
		GtkTreePath *tpath;
		GtkTreeIter iter;
		gchar *name = NULL;

		tpath = work->data;
		work = work->next;

		gtk_tree_model_get_iter(store, &iter, tpath);
		gtk_tree_model_get(store, &iter, FLIST_COLUMN_NAME, &name, -1);

		list = g_list_append(list, g_strconcat(current_path, "/", name, NULL));
		}

	g_list_foreach(slist, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(slist);

	return list;
}

static gint playlist_selection_sort_cb(gconstpointer a, gconstpointer b)
{
	if (GPOINTER_TO_INT(a) < GPOINTER_TO_INT(b)) return -1;

	if (GPOINTER_TO_INT(a) > GPOINTER_TO_INT(b)) return 1;

	return 0;
}

static GList *playlist_selection_get_number_list(gint sorted) /* don't free data pointers */
{
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GList *slist;
	GList *work;
	GList *list = NULL;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
	slist = gtk_tree_selection_get_selected_rows(selection, &store);

	work = slist;
	while (work)
		{
		GtkTreePath *tpath = work->data;

		list = g_list_prepend(list, GINT_TO_POINTER(tree_path_to_row(tpath)));
		work = work->next;
		}

	g_list_foreach(slist, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(slist);

	if (sorted)
		{
		list = g_list_sort(list, (GCompareFunc) playlist_selection_sort_cb);
		}
	else
		{
		list = g_list_reverse(list);
		}

	return list;
}

static GList *playlist_selection_get_path_list_util(gint only_files)
{
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GList *slist;
	GList *work;
	GList *list = NULL;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
	slist = gtk_tree_selection_get_selected_rows(selection, &store);

	work = slist;
	while (work)
		{
		GtkTreePath *tpath;
		const gchar *path;

		tpath = work->data;
		work = work->next;

		path = playlist_data_get_path(playlist_get_data(tree_path_to_row(tpath)));

		if (!only_files || isfile(path))
			{
			list = g_list_prepend(list, g_strdup(path));
			}
		}

	g_list_foreach(slist, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(slist);

	return g_list_reverse(list);
}

static GList *playlist_selection_get_path_list(void)
{
	return playlist_selection_get_path_list_util(FALSE);
}

static GList *playlist_selection_get_only_file_list(void)
{
	return playlist_selection_get_path_list_util(TRUE);
}

/*
 *-----------------------------------------------------------------------------
 * button press callbacks (public)
 *-----------------------------------------------------------------------------
 */

static void add_dir_func(gint recursive)
{
	gchar *path;
	gint row = -1;
	row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dirlist_view), "click_row"));
	path = dirlist_get_path(row);
	if (!path) return;
	playlist_append_from_dir(path, recursive);
	g_free(path);
}

static void save_pressed(void)
{
	if (playlist_pathname && isfile(playlist_pathname) && !playlist_get_count() < 1)
		{
		playlist_save(playlist_pathname);
		}
	else
		{
		playlist_dialog_save_as(playlist_pathname);
		}
}

static void save_as_pressed(void)
{
	playlist_dialog_save_as(playlist_pathname);
}

static void save_mode_changed(GtkWidget *w, gpointer data)
{
	save_mode_in_playlist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
}

static void append_pressed(void)
{
	playlist_dialog_append(NULL);
}

#if 0
static void import_pressed(void)
{
	playlist_dialog_import(NULL);
}
#endif

static void change_dir_to_home(void)
{
	change_to_path(homedir());
}

static void add_all_button_pressed(void)
{
	gchar *path;
	gint i;
	gint count = filelist_count();
	if (count < 1) return;
	
	for (i=0; i < count; i++)
		{
		path = filelist_get_path(i);
		if (!is_playlist(path))
			{
			playlist_add(path, TRUE);
			}
		g_free(path);
		}
}

static void remove_all_button_pressed(void)
{
	playlist_clear();
}

static void add_custom_pressed(void)
{
	playlist_dialog_add_custom_type(TRUE);
}

static void up_button_pressed(void)
{
	GList *list = playlist_selection_get_number_list(TRUE);
	GList *work;
	gint n;

	if (!list) return;

	n = 0;
	work = list;
	while(work && GPOINTER_TO_INT(work->data) == n)
		{
		n++;
		work = work->next;
		}

	while(work)
		{
		n = GPOINTER_TO_INT(work->data);
		playlist_move(n, n - 1);
		work = work->next;
		}

	g_list_free(list);
}

static void down_button_pressed(void)
{
	GList *list = playlist_selection_get_number_list(TRUE);
	GList *work;
	gint n;

	if (!list) return;

	n = playlist_get_count() - 1;
	work = g_list_last(list);
	while (work && GPOINTER_TO_INT(work->data) == n)
		{
		n--;
		work = work->prev;
		}

	while (work)
		{
		n = GPOINTER_TO_INT(work->data);
		playlist_move(n, n + 1);
		work = work->prev;
		}

	g_list_free(list);
}

static void top_button_pressed(void)
{
	GList *list = playlist_selection_get_number_list(TRUE);
	GList *work;
	gint row = 0;

	if (!list) return;

	work = list;
	while(work && GPOINTER_TO_INT(work->data) == row)
		{
		row++;
		work = work->next;
		}
	while(work)
		{
		gint n = GPOINTER_TO_INT(work->data);
		playlist_move(n, row);
		row++;
		work = work->next;
		}

	g_list_free(list);
}

static void end_button_pressed(void)
{
	GList *list = playlist_selection_get_number_list(TRUE);
	GList *work;
	gint row = playlist_get_count() - 1;

	if (!list) return;

	work = g_list_last(list);
	while(work && GPOINTER_TO_INT(work->data) == row)
		{
		row--;
		work = work->prev;
		}
	while(work)
		{
		gint n = GPOINTER_TO_INT(work->data);
		playlist_move(n, row);
		row--;
		work = work->prev;
		}

	g_list_free(list);
}

/*
 *-----------------------------------------------------------------------------
 * menu dirlist press callbacks (public)
 *-----------------------------------------------------------------------------
 */

void plw_add_dir_pressed(void)
{
	add_dir_func(FALSE);
}

void plw_add_dir_recursive_pressed(void)
{
	add_dir_func(TRUE);
}

/*
 *-----------------------------------------------------------------------------
 * menu filelist press callbacks (public)
 *-----------------------------------------------------------------------------
 */

void plw_filelist_songinfo_pressed(void)
{
	gint row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(filelist_view), "click_row"));
	gchar *path = filelist_get_path(row);
	view_song_info(path);
	g_free(path);
}

void plw_filelist_move_pressed(void)
{
	GList *list = fileview_selection_get_path_list(GTK_TREE_VIEW(filelist_view));
	file_util_move(NULL, list, current_path);
}

void plw_filelist_copy_pressed(void)
{
	GList *list = fileview_selection_get_path_list(GTK_TREE_VIEW(filelist_view));
	file_util_copy(NULL, list, current_path);
}

void plw_filelist_rename_pressed(void)
{
	GList *list = fileview_selection_get_path_list(GTK_TREE_VIEW(filelist_view));
	file_util_rename(NULL, list);
}

void plw_filelist_delete_pressed(void)
{
	GList *list = fileview_selection_get_path_list(GTK_TREE_VIEW(filelist_view));
	file_util_delete(NULL, list);
}

void plw_filelist_add_play_pressed(void)
{
	gint row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(filelist_view), "click_row"));
	gchar *path = filelist_get_path(row);
	playlist_add(path, TRUE);
	g_free(path);
	current_song_set_and_play(playlist_get_count() - 1, NULL);
}

void plw_filelist_play_pressed(void)
{
	gint row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(filelist_view), "click_row"));
	gchar *path = filelist_get_path(row);
	current_song_set_and_play(-1, path);
	g_free(path);
}

void plw_filelist_add_pressed(void)
{
	GList *list = fileview_selection_get_path_list(GTK_TREE_VIEW(filelist_view));
	GList *work;

	if (!list) return;

	work = list;
	while(work)
		{
		gchar *path = work->data;
		if (!is_playlist(path))
			{
			playlist_add(path, TRUE);
			}
		work = work->next;
		}

	path_list_free(list);
}

void plw_filelist_insert_pressed(void)
{
	GList *list = fileview_selection_get_path_list(GTK_TREE_VIEW(filelist_view));
	GList *work;
	gint row;

	if (!list) return;

	row = view_selection_get_first_row(GTK_TREE_VIEW(playlist_view));

	if (row == -1) row = 0;

	work = list;
	while(work)
		{
		gchar *path = work->data;
		if (!is_playlist(path))
			{
			playlist_insert(path, row, TRUE);
			row++;
			}
		work = work->next;
		}

	path_list_free(list);
}

static void plw_load_or_append(gint append)
{
	gint row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(filelist_view), "click_row"));
	gchar *path = filelist_get_path(row);
	playlist_load_from_file(path, append, TRUE, TRUE);
	g_free(path);
}

void plw_load_pressed(void)
{
	plw_load_or_append(FALSE);
}

void plw_append_pressed(void)
{
	plw_load_or_append(TRUE);
}

/*
 *-----------------------------------------------------------------------------
 * menu playlist press callbacks (public)
 *-----------------------------------------------------------------------------
 */

void plw_playlist_songinfo_pressed(void)
{
	gint row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(playlist_view), "click_row"));
	view_song_info(playlist_get_item(row));
}

void plw_playlist_edit_pressed(void)
{
	gint row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(playlist_view), "click_row"));
	playlist_dialog_edit_custom_type(playlist_get_data(row));
}

static gint plw_edit_title_cb(TreeEditData *ted, const gchar *oldname, const gchar *newname, gpointer data)
{
	SongData *sd = data;

	/* sanity check, ipc may have changed this behind our back */
	if (playlist_get_index_by_data(sd) >= 0)
		{
		playlist_data_customize_title(sd, newname);
		}

	return FALSE;
}

static gint plw_edit_comment_cb(TreeEditData *ted, const gchar *oldname, const gchar *newname, gpointer data)
{
	SongData *sd = data;

	/* sanity check, ipc may have changed this behind our back */
	if (playlist_get_index_by_data(sd) >= 0)
		{
		playlist_data_customize_comment(sd, newname);
		}

	return FALSE;
}

static void plw_playlist_edit_util(gint title)
{
	GtkTreeModel *store;
	GtkTreePath *tpath;
	GtkTreeIter iter;
	gint row;
	SongData *sd;

	row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(playlist_view), "click_row"));
	sd = playlist_get_data(row);
	if (!sd) return;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_view));
	if (row < 0 || !gtk_tree_model_iter_nth_child(store, &iter, NULL, row)) return;

	tpath = gtk_tree_model_get_path(store, &iter);
	if (title)
		{
		tree_edit_by_path(GTK_TREE_VIEW(playlist_view), tpath, playlist_column_lookup(COLUMN_TITLE),
				  playlist_data_get_title(sd), plw_edit_title_cb, sd);
		}
	else
		{
		tree_edit_by_path(GTK_TREE_VIEW(playlist_view), tpath, playlist_column_lookup(COLUMN_COMMENT),
				  playlist_data_get_comment(sd), plw_edit_comment_cb, sd);
		}
	gtk_tree_path_free(tpath);
}

void plw_playlist_edit_title_pressed(void)
{
	plw_playlist_edit_util(TRUE);
}

void plw_playlist_edit_comment_pressed(void)
{
	plw_playlist_edit_util(FALSE);
}

void plw_playlist_move_pressed(void)
{
	GList *list = playlist_selection_get_only_file_list();
	file_util_move(NULL, list, current_path);
}

void plw_playlist_copy_pressed(void)
{
	GList *list = playlist_selection_get_only_file_list();
	file_util_copy(NULL, list, current_path);
}

void plw_playlist_rename_pressed(void)
{
	GList *list = playlist_selection_get_only_file_list();
	file_util_rename(NULL, list);
}

/* this _deletes files_ and removes items from the playlist */
void plw_playlist_delete_pressed(void)
{
	GList *list = playlist_selection_get_only_file_list();
	file_util_delete(NULL, list);
}

/* this only removes items from the playlist */
void plw_playlist_remove_pressed(void)
{
	GList *list = playlist_selection_get_number_list(TRUE);
	GList *work;
	gint row;

	if (!list) return;

	/* check that the current song is about to be removed,
	 * if so note next unselected song and set current song to NULL
	 */
	row = current_song_get_number();
	if (row >= 0 && view_row_is_selected(GTK_TREE_VIEW(playlist_view), row))
		{
		gint new_row;

		new_row = playlist_get_next(row);
		while (new_row >= 0 && view_row_is_selected(GTK_TREE_VIEW(playlist_view), new_row))
			{
			new_row = playlist_get_next(new_row);
			}
		if (new_row == -1)
			{
			new_row = playlist_get_prev(row);
			while (new_row >= 0 && view_row_is_selected(GTK_TREE_VIEW(playlist_view), new_row))
				{
				new_row = playlist_get_prev(new_row);
				}
			}

		current_song_set(new_row, NULL);
		}

	display_freeze();
	work = g_list_last(list);
	while (work)
		{
		playlist_remove(NULL, GPOINTER_TO_INT(work->data), FALSE);
		work = work->prev;
		}
	display_thaw();
	display_set_song_count(playlist_get_count());
	display_set_song_number(current_song_get_number());
	display_playlist_refresh();

	if (GPOINTER_TO_INT(list->data) < playlist_get_count())
		{
		view_select_row(GTK_TREE_VIEW(playlist_view), GPOINTER_TO_INT(list->data), TRUE);
		}
	else if (playlist_get_count() > 0)
		{
		view_select_row(GTK_TREE_VIEW(playlist_view), playlist_get_count() - 1, TRUE);
		}

	g_list_free(list);
}

void plw_playlist_play_pressed(void)
{
	gint row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(playlist_view), "click_row"));
	current_song_set_and_play(row, NULL);
}

void plw_playlist_randomize_pressed(void)
{
	playlist_randomize();
}

/*
 *-----------------------------------------------------------------------------
 * dirlist callbacks
 *-----------------------------------------------------------------------------
 */

static gint dirlist_press_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	GtkTreePath *tpath;
	gint row = -1;
	gint ret = FALSE;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bevent->x, bevent->y,
					  &tpath, NULL, NULL, NULL))
		{
		row = tree_path_to_row(tpath);
		gtk_tree_path_free(tpath);
		}

	if (row == -1) return FALSE;

	g_object_set_data(G_OBJECT(dirlist_view), "click_row", GINT_TO_POINTER(row));

	if (bevent->button == 1 && bevent->type == GDK_2BUTTON_PRESS)
		{
		gchar *newdir = dirlist_get_path(row);
		change_to_path(newdir);
		g_free(newdir);
		return TRUE;
		}
	if (bevent->button == 2 || bevent->button == 3)
		{
		view_select_row(GTK_TREE_VIEW(dirlist_view), row, TRUE);
		ret = TRUE;
		}
	if (bevent->button == 3)
		{
		GtkWidget *menu;

		menu = menu_filelist_dir();
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
			       bevent->button, bevent->time);
		
		}

	return ret;
}

/*
 *-----------------------------------------------------------------------------
 * filelist callbacks
 *-----------------------------------------------------------------------------
 */

static gint filelist_press_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	GtkTreePath *tpath;
	gint row = -1;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bevent->x, bevent->y,
					  &tpath, NULL, NULL, NULL))
		{
		row = tree_path_to_row(tpath);
		gtk_tree_path_free(tpath);
		}

	if (row == -1) return FALSE;

	g_object_set_data(G_OBJECT(filelist_view), "click_row", GINT_TO_POINTER(row));

	if (bevent->button == 1)
		{
		if (bevent->type == GDK_BUTTON_PRESS &&
		    !(bevent->state & GDK_SHIFT_MASK ) &&
		    !(bevent->state & GDK_CONTROL_MASK ) &&
		    view_row_is_selected(GTK_TREE_VIEW(filelist_view), row))
			{
			/* this selection handled on release_cb */
			return TRUE;
			}
		if (bevent->type == GDK_2BUTTON_PRESS)
			{
			gchar *newfile = filelist_get_path(row);
			playlist_add(newfile, TRUE);
			g_free(newfile);
			return TRUE;
			}
		}
	if (bevent->button == 2)
		{
		if (view_row_is_selected(GTK_TREE_VIEW(filelist_view), row))
			{
			view_select_row(GTK_TREE_VIEW(filelist_view), row, FALSE);
			}
		else
			{
			view_select_row(GTK_TREE_VIEW(filelist_view), row, TRUE);
			}
		}
	if (bevent->button == 3)
		{
		GtkWidget *menu;
		gchar *path;

		if (!view_row_is_selected(GTK_TREE_VIEW(filelist_view), row))
			{
			view_select_all(GTK_TREE_VIEW(filelist_view), FALSE);
			view_select_row(GTK_TREE_VIEW(filelist_view), row, TRUE);
			}

		path = filelist_get_path(row);
		if (is_playlist(path))
			{
			menu = menu_filelist_playlist();
			}
		else
			{
			menu = menu_filelist_file();
			}
		g_free(path);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
			       bevent->button, bevent->time);
		}

	return (bevent->button == 2 || bevent->button == 3);
}

static gint filelist_release_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	GtkTreePath *tpath;
	gint row = -1;

	if ((bevent->x != 0 || bevent->y != 0) &&
	    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bevent->x, bevent->y,
					  &tpath, NULL, NULL, NULL))
		{
		row = tree_path_to_row(tpath);
		gtk_tree_path_free(tpath);
		}

	if (row == -1) return FALSE;

	if (bevent->button == 1 &&
	    !(bevent->state & GDK_SHIFT_MASK) &&
	    !(bevent->state & GDK_CONTROL_MASK) &&
	    view_row_is_selected(GTK_TREE_VIEW(widget), row))
		{
		view_select_all(GTK_TREE_VIEW(widget), FALSE);
		view_select_row(GTK_TREE_VIEW(widget), row, TRUE);
		return TRUE;
		}

	return FALSE;
}

/*
 *-----------------------------------------------------------------------------
 * playlist callbacks
 *-----------------------------------------------------------------------------
 */

static gint playlist_press_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	GtkTreePath *tpath;
	gint row = -1;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bevent->x, bevent->y,
					  &tpath, NULL, NULL, NULL))
		{
		row = tree_path_to_row(tpath);
		gtk_tree_path_free(tpath);
		}

	if (row == -1) return FALSE;

	g_object_set_data(G_OBJECT(playlist_view), "click_row", GINT_TO_POINTER(row));

	if (bevent->button == 1)
		{
		if (bevent->type == GDK_BUTTON_PRESS &&
		    !(bevent->state & GDK_SHIFT_MASK ) &&
		    !(bevent->state & GDK_CONTROL_MASK ) &&
		    view_row_is_selected(GTK_TREE_VIEW(playlist_view), row))
			{
			/* this selection handled on release_cb */
			return TRUE;
			}
		if (bevent->type == GDK_2BUTTON_PRESS)
			{
			current_song_set_and_play(row, NULL);
			return TRUE;
			}
		}
	if (bevent->button == 2)
		{
		if (view_row_is_selected(GTK_TREE_VIEW(playlist_view), row))
			{
			view_select_row(GTK_TREE_VIEW(playlist_view), row, FALSE);
			}
		else
			{
			view_select_row(GTK_TREE_VIEW(playlist_view), row, TRUE);
			}
		}
	if (bevent->button == 3)
		{
		GtkWidget *menu;

		if (!view_row_is_selected(GTK_TREE_VIEW(playlist_view), row))
			{
			view_select_all(GTK_TREE_VIEW(playlist_view), FALSE);
			view_select_row(GTK_TREE_VIEW(playlist_view), row, TRUE);
			}

		if (playlist_item_is_custom(row))
			{
			menu = menu_playlist_custom();
			}
		else
			{
			menu = menu_playlist();
			}
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
			       bevent->button, bevent->time);
		}

	return (bevent->button == 2 || bevent->button == 3);
}

static gint playlist_release_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	GtkTreePath *tpath;
	gint row = -1;

	if ((bevent->x != 0 || bevent->y != 0) &&
	    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bevent->x, bevent->y,
					  &tpath, NULL, NULL, NULL))
		{
		row = tree_path_to_row(tpath);
		gtk_tree_path_free(tpath);
		}

	if (row == -1) return FALSE;

	if (bevent->button == 1 &&
	    !(bevent->state & GDK_SHIFT_MASK) &&
	    !(bevent->state & GDK_CONTROL_MASK) &&
	    view_row_is_selected(GTK_TREE_VIEW(widget), row))
		{
		view_select_all(GTK_TREE_VIEW(widget), FALSE);
		view_select_row(GTK_TREE_VIEW(widget), row, TRUE);
		return TRUE;
		}

	return FALSE;
}

static gboolean playlist_select_row_cb(GtkTreeSelection *selection, GtkTreeModel *store, GtkTreePath *tpath,
				       gboolean path_currently_selected, gpointer data)
{
	SongData *sd;

	sd = playlist_get_data(tree_path_to_row(tpath));
	if (sd) playlist_update_background(sd);

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * history menu functions (private)
 *-----------------------------------------------------------------------------
 */

static void path_entry_select(const gchar *newdir, gpointer data)
{
	if (isdir(newdir))
		{
		change_to_path(newdir);
		return;
		}
	else if (isfile(newdir))
		{
		gchar *buf;
		gchar *ptr;
		gchar *file;
		GList *list;

		buf = g_strdup(newdir);
		ptr = (gchar *)filename_from_path(buf);
		file = g_strdup(ptr);

		if (ptr > buf) ptr--;
		if (ptr == buf)
			{
			g_free(buf);
			buf = g_strdup("/");
			}
		else
			{
			ptr[0] = '\0';
			}
		if (strcmp(buf, current_path) != 0)
			{
			change_to_path(buf);
			}
		else
			{
			gtk_entry_set_text(GTK_ENTRY(pathentry), current_path);
			}

		list = filelist;
		while(list)
			{
			gchar *name = list->data;
			if (strcmp(name, file) == 0)
				{
				g_free(buf);
				buf = g_strconcat(current_path, "/", name, NULL);
				playlist_add(buf, TRUE);
				g_free(buf);
				g_free(file);
				return;
				}
			list = list->next;
			}

                g_free(buf);
                g_free(file);
		}
}

static void history_menu_select(GtkWidget *widget, gchar *newdir)
{
	change_to_path(newdir);
}

static gchar *truncate_hist_text(gchar *t, gint l)
{
	gchar *tp;
	gchar *tbuf;
	if (l >= strlen(t)) return g_strdup(t);
	tp = t + strlen(t) - l;
	while (tp[0] != '/' && tp < t + strlen(t)) tp++;
		/* this checks to see if directory name is longer than l, if so */
		/* reset the length of name to l, it's better to have a partial */
		/* name than no name at all. */
	if (tp >= t + strlen(t)) tp = t + strlen(t) - l;
	tbuf = g_strconcat("/...", tp, NULL);
	return tbuf;
}

static void update_history_menu(gchar *path)
{
	static GList *history_list = NULL;
	GtkWidget *menu;
	GtkWidget *button;
	gchar *hist;
	gchar *hist_ptr;

	if (history_list)
		{
		g_list_foreach(history_list, (GFunc) g_free, NULL);
		g_list_free(history_list);
		history_list = NULL;
		}

	hist = g_strdup(path);
	hist_ptr = hist + strlen(hist) - 1 ;
	
	menu = gtk_menu_new();
	while (hist_ptr > hist)
		{
		gchar *path;
		gchar *hist_trunc;

		path = g_strdup(hist);
		history_list = g_list_append(history_list, path);

		hist_trunc = truncate_hist_text(hist,32);
		button = menu_item_add_simple(menu, hist_trunc, G_CALLBACK(history_menu_select), path);
		g_free(hist_trunc);

		while (hist_ptr[0] != '/') hist_ptr--;
		hist_ptr[0] = '\0';
		}
	g_free(hist);

	/* looks like we have to add the root directory manually */
	button = menu_item_add_simple(menu, "/", G_CALLBACK(history_menu_select), "/");

	gtk_option_menu_set_menu(GTK_OPTION_MENU(historypulldown), menu);
	gtk_entry_set_text(GTK_ENTRY(pathentry), path);
}

/*
 *-----------------------------------------------------------------------------
 * playlist misc functions (private)
 *-----------------------------------------------------------------------------
 */

void playlist_window_length_update(void)
{
	if (!playlist_window)
		{
		return;
		}
	else
		{
		gchar *buf;

		if (read_file_information)
			{
			gint hrs = playlist_get_length() / 3600;
			gint min = (playlist_get_length() - (hrs * 3600)) / 60;
			gint sec = playlist_get_length() - (hrs * 3600) - (min * 60);
			buf = g_strdup_printf(_("Playlist length: %02d:%02d:%02d , %d songs"), hrs, min, sec, playlist_get_count());
			}
		else
			{
			buf = g_strdup_printf(_("(header loading is off), %d songs"), playlist_get_count());
			}
		gtk_label_set_text(GTK_LABEL(fileinfolabel), buf);
		g_free(buf);
		}
}

void playlist_window_update_titles(void)
{
	gchar *buf;

	if (playlist_window)
		{
		buf = g_strconcat(playlist_filename, _(" - GQmpeg playlist"), NULL);
		gtk_window_set_title (GTK_WINDOW (playlist_window), buf);
		g_free(buf);
		}
	display_set_playlist(playlist_filename);
}

/*
 *-----------------------------------------------------------------------------
 * filelist update functions (private)
 *-----------------------------------------------------------------------------
 */

static void playlist_window_filelist_populate(void)
{
	GtkListStore *store;
	GList *work;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(dirlist_view)));
	gtk_list_store_clear(store);

	work = dirlist;
	while (work)
		{
		gchar *name = work->data;
		GtkTreeIter iter;

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, FLIST_COLUMN_NAME, name, -1);

		work = work->next;
		}

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(filelist_view)));
	gtk_list_store_clear(store);

	work = filelist;
	while (work)
		{
		gchar *name = work->data;
		GtkTreeIter iter;

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, FLIST_COLUMN_NAME, name, -1);

		work = work->next;
		}

	update_history_menu(current_path);
}

void playlist_window_filelist_update(void)
{
	if (!playlist_window) return;

	playlist_window_filelist_populate();
}

/*
 *-----------------------------------------------------------------------------
 * playlist window auto-scroll (private)
 *-----------------------------------------------------------------------------
 */

#if 0
static gint playlist_window_drag_scroll_cb(gpointer data)
{
	GdkWindow *window;
	gint x, y;
	gint w, h;
	gint amt = 0;
	GtkAdjustment *adj;

	if (!playlist_window)
		{
		pw_scroll_id = -1;
		return FALSE;
		}

	window = GTK_CLIST(playlist_clist)->clist_window;
	gdk_window_get_pointer(window, &x, &y, NULL);
	gdk_drawable_get_size(window, &w, &h);

	/* check bounds, remove if out */
	if (x < 0 || x >= w || y < 0 || y >= h)
		{
		pw_scroll_id = -1;
		return FALSE;
		}

	if (h < PW_SCROLL_THRESHHOLD * 2)
		{
		if (y < h / 2)
			{
			amt = 0 - ((h / 2) - y);
			}
		else
			{
			amt = y - (h / 2);
			}
		}
	else if (y < PW_SCROLL_THRESHHOLD)
		{
		amt = 0 - (PW_SCROLL_THRESHHOLD - y);
		}
	else if (y >= h - PW_SCROLL_THRESHHOLD)
		{
		amt = y - (h - PW_SCROLL_THRESHHOLD);
		}

	if (amt != 0)
		{
		adj = gtk_clist_get_vadjustment(GTK_CLIST(playlist_clist));
		gtk_adjustment_set_value(adj, MIN(adj->value + amt, adj->upper - adj->page_size));
		}

	return TRUE;
}

static void playlist_window_drag_scroll(gint scroll)
{
	if (!scroll)
		{
		if (pw_scroll_id != -1)
			{
			gtk_timeout_remove(pw_scroll_id);
			}
		pw_scroll_id = -1;
		return;
		}

	if (pw_scroll_id == -1)
                {
		pw_scroll_id = gtk_timeout_add(PW_SCROLL_DELAY, playlist_window_drag_scroll_cb, NULL);
		}
}
#endif

/*
 *-----------------------------------------------------------------------------
 * playlist window drag and drop (private)
 *-----------------------------------------------------------------------------
 */

enum {
	TARGET_APP_PLAYLIST,
	TARGET_URI_LIST,
	TARGET_TEXT_PLAIN,
	TARGET_X_URL,
	TARGET_NETSCAPE_URL
};

static GtkTargetEntry filelist_drag_types[] = {
	{ "text/uri-list", 0, TARGET_URI_LIST },
	{ "text/plain", 0, TARGET_TEXT_PLAIN }
};
static gint filelist_drag_count = 2;

static GtkTargetEntry playlist_drag_types[] = {
	{ "application/x-gqmpeg-playlist", 0, TARGET_APP_PLAYLIST },
	{ "text/plain", 0, TARGET_TEXT_PLAIN }
};
static gint playlist_drag_count = 2;

static GtkTargetEntry playlist_drop_types[] = {
	{ "application/x-gqmpeg-playlist", 0, TARGET_APP_PLAYLIST },
	{ "text/uri-list", 0, TARGET_URI_LIST },
	{ "x-url/http",    0, TARGET_X_URL },
	{ "x-url/ftp",     0, TARGET_X_URL },
	{ "_NETSCAPE_URL", 0, TARGET_NETSCAPE_URL }
};
static gint playlist_drop_count = 5;

static void playlist_window_dirlist_set_dnd_data(GtkWidget *widget,
		GdkDragContext *context, GtkSelectionData *selection_data,
		guint info, guint time, gpointer data)
{
	gchar *uri_text = NULL;
	gint row = -1;
	gchar *name;
	GList *list;

	row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dirlist_view), "click_row"));
	name = dirlist_get_path(row);

	if (!name) return;

	list = g_list_append(NULL, name);
	switch (info)
		{
		case TARGET_URI_LIST:
			uri_text = uri_text_from_list(list, NULL, FALSE);
			break;
		case TARGET_TEXT_PLAIN:
			uri_text = uri_text_from_list(list, NULL, TRUE);
			break;
		default:
			break;
		}
	g_list_free(list);

	if (!uri_text) return;

	gtk_selection_data_set (selection_data, selection_data->target,
					8, uri_text, strlen(uri_text) + 1);
	g_free(uri_text);
	g_free(name);
}

static void playlist_window_filelist_set_dnd_data(GtkWidget *widget,
		GdkDragContext *context, GtkSelectionData *selection_data,
		guint info, guint time, gpointer data)
{
	GList *list;
	gchar *uri_text = NULL;
	gint uri_len = 0;

	list = fileview_selection_get_path_list(GTK_TREE_VIEW(filelist_view));
	if (!list) return;

	switch (info)
		{
		case TARGET_URI_LIST:
			uri_text = uri_text_from_list(list, &uri_len, FALSE);
			break;
		case TARGET_TEXT_PLAIN:
			uri_text = uri_text_from_list(list, &uri_len, TRUE);
			break;
		default:
			break;
		}

	path_list_free(list);

	if (!uri_text) return;

	gtk_selection_data_set (selection_data, selection_data->target,
					8, uri_text, uri_len);
	g_free(uri_text);
}

static void playlist_window_playlist_set_dnd_data(GtkWidget *widget,
		GdkDragContext *context, GtkSelectionData *selection_data,
		guint info, guint time, gpointer data)
{
	GList *list;
	gchar *uri_text = NULL;
	gint uri_len = 0;

	if (view_selection_count(GTK_TREE_VIEW(playlist_view)) == 0) return;

	switch (info)
		{
		case TARGET_APP_PLAYLIST:
			uri_text = g_strdup("");
			uri_len = 1;
			break;
		case TARGET_TEXT_PLAIN:
			list = playlist_selection_get_path_list();
			if (!list) return;
			uri_text = uri_text_from_list(list, &uri_len, TRUE);
			path_list_free(list);
			break;
		default:
			break;
		}

	if (!uri_text) return;

	gtk_selection_data_set (selection_data, selection_data->target,
					8, uri_text, uri_len);
	g_free(uri_text);
}

static void playlist_window_dnd_motion(GtkWidget *widget, GdkDragContext *context,
				       gint x, gint y, guint time, gpointer data)
{
	widget_auto_scroll_start(widget, gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(widget)),
				 -1, AUTO_SCROLL_REGION_SIZE, NULL, NULL);
}

static void playlist_window_dnd_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer data)
{
	widget_auto_scroll_stop(widget);
}

static void recalc_rows(GList *list, gint s, gint t)
{
	GList *work = list;

	while(work)
		{
		gint n = GPOINTER_TO_INT(work->data);
		if (s < n && n < t)
			{
			n--;
			}
		else if (s > n && n >= t)
			{
			n++;
			}
		work->data = GINT_TO_POINTER(n);
		work = work->next;
		}
}

static void playlist_window_playlist_get_dnd_data(GtkWidget *widget,
				GdkDragContext *context, gint x, gint y,
				GtkSelectionData *selection_data, guint info,
				guint time, gpointer data)
{
	GtkTreePath *tpath;
	GList *list;
	GList *work;
	gint row = -1;

	if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y,
					      &tpath, GTK_TREE_VIEW_DROP_BEFORE))
		{
		row = tree_path_to_row(tpath);
		gtk_tree_path_free(tpath);
		}

	if (debug_mode) printf("drop on %d\n", row);

	switch (info)
		{
		case TARGET_URI_LIST:
		case TARGET_X_URL:
		case TARGET_NETSCAPE_URL:
			list = uri_list_from_text(selection_data->data, FALSE);

			if (!list) return;

			work = list;

			if (row < 0)
				{
				while(work)
					{
					gchar *name = work->data;
					if (isdir(name))
						{
						playlist_append_from_dir(name, FALSE);
						}
					else if (!is_playlist(name))
						{
						playlist_add(name, TRUE);
						}
					work = work->next;
					}
				}
			else
				{
				while(work)
					{
					gchar *name = work->data;
					if (isdir(name))
						{
						row = playlist_insert_from_dir(name, row, FALSE);
						}
					else if (!is_playlist(name))
						{
						playlist_insert(name, row, TRUE);
						row++;
						}
					work = work->next;
					}
				}
			g_list_foreach(list, (GFunc)g_free, NULL);
			g_list_free(list);
			break;
		case TARGET_APP_PLAYLIST:
			list = playlist_selection_get_number_list(TRUE);

			if (!list) return;
			if (view_row_is_selected(GTK_TREE_VIEW(playlist_view), row) ||
			    row < 0)
				{
				g_list_free(list);
				return;
				}

			work = list;
			while(work)
				{
				GList *t = work->next;
				gint n = GPOINTER_TO_INT(work->data);
				list = g_list_remove(list, work->data);
				playlist_move(n, row);
				recalc_rows(list, n, row);
				if (n > row) row++;
				work = t;
				}
			g_list_free(list);
			break;
		default:
			break;
		}
}

static void playlist_window_dnd_init(void)
{
	/* dir list */
	gtk_drag_source_set(dirlist_view, GDK_BUTTON1_MASK,
			    filelist_drag_types, filelist_drag_count,
			    GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
	g_signal_connect(G_OBJECT(dirlist_view), "drag_data_get",
			 G_CALLBACK(playlist_window_dirlist_set_dnd_data), NULL);

	/* file list */
	gtk_drag_source_set(filelist_view, GDK_BUTTON1_MASK,
			    filelist_drag_types,
			    filelist_drag_count, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
	g_signal_connect(G_OBJECT(filelist_view), "drag_data_get",
			 G_CALLBACK(playlist_window_filelist_set_dnd_data), NULL);

	/* playlist list */
	gtk_drag_dest_set(playlist_view,
			  GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
			  playlist_drop_types, playlist_drop_count, 
			  GDK_ACTION_COPY);
	g_signal_connect(G_OBJECT(playlist_view), "drag_data_received",
			 G_CALLBACK(playlist_window_playlist_get_dnd_data), NULL);
	g_signal_connect(G_OBJECT(playlist_view), "drag_motion",
			 G_CALLBACK(playlist_window_dnd_motion), NULL);
	g_signal_connect(G_OBJECT(playlist_view), "drag_leave",
			 G_CALLBACK(playlist_window_dnd_leave), NULL);

	gtk_drag_source_set(playlist_view, GDK_BUTTON1_MASK,
			    playlist_drag_types, playlist_drag_count,
			    GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
	g_signal_connect(G_OBJECT(playlist_view), "drag_data_get",
			 G_CALLBACK(playlist_window_playlist_set_dnd_data), NULL);
}

/*
 *-----------------------------------------------------------------------------
 * playlist window show / hide (public)
 *-----------------------------------------------------------------------------
 */

static GtkWidget *button_new(GtkWidget *box, const gchar *text,
			     GCallback func, gpointer data)
{
	GtkWidget *button;

	button = gtk_button_new_with_mnemonic(text);
	if (func) g_signal_connect(G_OBJECT(button), "clicked", func, data);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	return button;
}

void playlist_window_toggle(void)
{
	if (playlist_window) 
		{
		playlist_window_hide();
		}
	else
		{
		playlist_window_show();
		}
}

void playlist_window_show(void)
{
	if (playlist_window)
		{
		gdk_window_raise(playlist_window->window);
		return;
		}

	playlist_window_build();
	playlist_window_filelist_populate();
	playlist_window_populate();

	display_set_playlist_window();
}

void playlist_window_hide(void)
{
	gint i;
	gchar *buf = NULL;

	if (!playlist_window) return;

	widget_auto_scroll_stop(playlist_view);

	gdk_window_get_root_origin(playlist_window->window, &window_list_x, &window_list_y);
	gdk_drawable_get_size(playlist_window->window, &window_list_w, &window_list_h);
	window_list_div_dir = s_scrolled->allocation.height;
	window_list_div_mid = s_hbox->allocation.width;

	for (i = 1; i < COLUMN_ELEMENT_COUNT; i++)
		{
		GtkTreeViewColumn *column;
		gchar *tmp;
		gint w;
		gint c = 0;

		c = playlist_view_config[i];
		column = gtk_tree_view_get_column(GTK_TREE_VIEW(playlist_view), c);

		g_object_get(G_OBJECT(column), "width", &w, NULL);

		tmp = buf;
		if (!tmp)
			{
			buf = g_strdup_printf("%d", w);
			}
		else
			{
			buf = g_strdup_printf("%s, %d", tmp, w);
			g_free(tmp);
			}
		}
	g_free(window_list_columns);
	window_list_columns = buf;

	gtk_widget_destroy(playlist_window);
	playlist_window = NULL;

	display_set_playlist_window();
}

static gboolean playlist_window_delete_cb(GtkWidget *widget, gpointer data)
{
	playlist_window_hide();

	return FALSE;
}

static void playlist_window_build(void)
{
	GtkWidget *frame;
	GtkWidget *paned;
	GtkWidget *panedd;
	GtkWidget *hbox1;
	GtkWidget *vbox;
	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *button;
	GtkWidget *infopackbox;
	GtkWidget *scrolled;
	GtkWidget *tabcomp;
	GtkListStore *store;
	GtkTreeSelection *selection;

	gchar *buf;
	gint i;
	gint playlist_column_widths[COLUMN_ELEMENT_COUNT];
	gint playlist_aligned_widths[COLUMN_ELEMENT_COUNT];
	gint *column_widths;

	/* set up playlist window */
	playlist_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(playlist_window), TRUE);
	g_signal_connect(G_OBJECT (playlist_window), "delete_event",
			 G_CALLBACK(playlist_window_delete_cb), NULL);
	buf = g_strconcat(playlist_filename,_(" - GQmpeg Playlist"), NULL);
	gtk_window_set_title (GTK_WINDOW (playlist_window), buf);
	g_free(buf);
	gtk_window_set_wmclass(GTK_WINDOW (playlist_window), "playlist", "GQmpeg");
	window_set_icon(playlist_window, (const char **)gqmpeg_icon_xpm, NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(playlist_window), 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (playlist_window), vbox);
	gtk_widget_show (vbox);

	paned = gtk_hpaned_new();
	gtk_box_pack_start (GTK_BOX (vbox), paned, TRUE, TRUE, 0);
	gtk_widget_show (paned);

	s_hbox = gtk_hbox_new (FALSE, 0);
	gtk_paned_add1(GTK_PANED(paned), s_hbox);
	gtk_widget_show (s_hbox);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (s_hbox), vbox1, TRUE, TRUE, 0);
	gtk_widget_show (vbox1);

	gtk_widget_realize(playlist_window);

        /* path entry */
	tabcomp = tab_completion_new(&pathentry, NULL, path_entry_select, NULL);
        gtk_box_pack_start (GTK_BOX (vbox1), tabcomp, FALSE, FALSE,0);
        gtk_widget_show (tabcomp);

	/* history button */
	historypulldown = gtk_option_menu_new ();
	gtk_box_pack_start (GTK_BOX (vbox1), historypulldown, FALSE, FALSE,0);
	gtk_widget_show (historypulldown);

	/* pane for dir-file lists */
	panedd = gtk_vpaned_new();
	gtk_box_pack_start (GTK_BOX (vbox1), panedd, TRUE, TRUE, 0);
	gtk_widget_show (panedd);

	/* directory list */
	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	s_scrolled = list_new(store, &dirlist_view);
	g_object_unref(store);

	gtk_paned_add1(GTK_PANED(panedd), s_scrolled);
	gtk_widget_show(s_scrolled);

	list_add_column(dirlist_view, _("Folders:"), FLIST_COLUMN_NAME, FALSE,
			-1, FALSE, NULL, NULL);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dirlist_view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	g_signal_connect(G_OBJECT(dirlist_view), "button_press_event",
			 G_CALLBACK(dirlist_press_cb), NULL);

	/* file list */
	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	scrolled = list_new(store, &filelist_view);
	g_object_unref(store);

	gtk_paned_add2(GTK_PANED(panedd), scrolled);
	gtk_widget_show(scrolled);

	list_add_column(filelist_view, _("Files:"), FLIST_COLUMN_NAME, FALSE,
			-1, FALSE, NULL, NULL);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(filelist_view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	if (slik_remember_position)
		gtk_paned_set_position(GTK_PANED(panedd), window_list_div_dir);
	else
		gtk_paned_set_position(GTK_PANED(panedd), DIR_DEFAULT_HEIGHT);

	/* button vbox */

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
	gtk_box_pack_start (GTK_BOX (s_hbox), vbox1, FALSE, FALSE, 0);
	gtk_widget_show (vbox1);

	hbox1 = gtk_hbox_new (TRUE, 5);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 5);
	gtk_widget_show(hbox1);

	button_new(hbox1, _("_Home"), G_CALLBACK(change_dir_to_home), NULL);
	button_new(hbox1, _("Close"), G_CALLBACK(playlist_window_hide), NULL);

	frame = gtk_frame_new(_("Playlist"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 5);
	gtk_container_add(GTK_CONTAINER(frame),vbox2);
	gtk_widget_show(vbox2);

	button_new(vbox2, _("_Save"), G_CALLBACK(save_pressed), NULL);
	button_new(vbox2, _("Sa_ve as..."), G_CALLBACK(save_as_pressed), NULL);

	button = gtk_check_button_new_with_label(_("save mode"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), save_mode_in_playlist);
	g_signal_connect(G_OBJECT(button), "toggled",
			 G_CALLBACK(save_mode_changed), NULL);
	gtk_box_pack_start(GTK_BOX(vbox2), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button_new(vbox2, _("_Open"), G_CALLBACK(btn_load_playlist_pressed), NULL);
	button_new(vbox2, _("A_ppend"), G_CALLBACK(append_pressed), NULL);

	frame = gtk_frame_new(_("Move Song"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox1), 5);
	gtk_container_add(GTK_CONTAINER(frame),hbox1);
	gtk_widget_show(hbox1);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox1), vbox2, TRUE, TRUE, 0);
	gtk_widget_show(vbox2);

	button_new(vbox2, _("_Top"), G_CALLBACK(top_button_pressed), NULL);
	button_new(vbox2, _("_End"), G_CALLBACK(end_button_pressed), NULL);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox1), vbox2, TRUE, TRUE, 0);
	gtk_widget_show(vbox2);

	button_new(vbox2, _("_Up"), G_CALLBACK(up_button_pressed), NULL);
	button_new(vbox2, _("_Down"), G_CALLBACK(down_button_pressed), NULL);

	frame = gtk_frame_new(_("Songs"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 5);
	gtk_container_add(GTK_CONTAINER(frame),vbox2);
	gtk_widget_show(vbox2);

	button_new(vbox2, _("Add A_ll"), G_CALLBACK(add_all_button_pressed), NULL);
	button_new(vbox2, _("_Add"), G_CALLBACK(plw_filelist_add_pressed), NULL);
	button_new(vbox2, _("_Insert"), G_CALLBACK(plw_filelist_insert_pressed), NULL);
	button_new(vbox2, _("_Remove"), G_CALLBACK(plw_playlist_remove_pressed), NULL);
	button_new(vbox2, _("Re_move All"), G_CALLBACK(remove_all_button_pressed), NULL);

	button = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox2), button, TRUE, TRUE, 5);
	gtk_widget_show(button);

	button_new(vbox2, _("Add _Custom"), G_CALLBACK(add_custom_pressed), NULL);

	/* play list */
	store = gtk_list_store_new(10, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
				       G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				       G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				       G_TYPE_STRING);
	scrolled = list_new(store, &playlist_view);
	g_object_unref(store);

	gtk_paned_add2(GTK_PANED(paned), scrolled);
	gtk_widget_show(scrolled);

	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(playlist_view), TRUE);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_MULTIPLE);
	gtk_tree_selection_set_select_function(selection, playlist_select_row_cb, NULL, NULL);

	gtk_widget_set_size_request(playlist_view, list_width * 2, -1);

	for (i = 0; i < COLUMN_ELEMENT_COUNT; i++)
		{
		playlist_column_widths[i] = playlist_view_widths[i];
		}
	if (slik_remember_position && window_list_columns &&
	    sscanf(window_list_columns, "%d, %d, %d, %d, %d, %d, %d, %d, %d",
	    &playlist_column_widths[1], &playlist_column_widths[2], &playlist_column_widths[3],
	    &playlist_column_widths[4], &playlist_column_widths[5], &playlist_column_widths[6],
	    &playlist_column_widths[7], &playlist_column_widths[8], &playlist_column_widths[9]) >= 8)
		{
		/* reorder them to internal order */
		for (i = 0; i < COLUMN_ELEMENT_COUNT; i++)
			{
			gint c = playlist_view_config[i];
			playlist_aligned_widths[i] = playlist_column_widths[c];
			}
		column_widths = playlist_aligned_widths;
		}
	else
		{
		column_widths = playlist_column_widths;
		}

	for (i = 0; i < COLUMN_ELEMENT_COUNT; i++)
		{
		gint column_id;

		column_id = playlist_view_config[i];
		list_add_column(playlist_view, _(playlist_view_titles[column_id]), column_id,
				(column_id == COLUMN_ICON), column_widths[column_id], (column_id == COLUMN_TIME),
				G_CALLBACK(playlist_view_column_clicked), GINT_TO_POINTER(i));
		}

	if (slik_remember_position)
		gtk_paned_set_position(GTK_PANED(paned), window_list_div_mid);
	else
		gtk_paned_set_position(GTK_PANED(paned), DIR_DEFAULT_WIDTH);

	/* create info pack box */
	infopackbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), infopackbox, FALSE, FALSE, 0);
	gtk_widget_show (infopackbox);

	/* file info window */
	button = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(button),GTK_SHADOW_IN);
	gtk_box_pack_end (GTK_BOX (infopackbox), button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	fileinfolabel = gtk_label_new ("");
	gtk_container_add (GTK_CONTAINER (button), fileinfolabel);
	gtk_widget_show (fileinfolabel);
	playlist_window_length_update();

	playlist_window_dnd_init();

	/* order of connecting these signals after dnd init is important */

	g_signal_connect(G_OBJECT(filelist_view), "button_press_event",
			 G_CALLBACK(filelist_press_cb), NULL);
	g_signal_connect(G_OBJECT(filelist_view), "button_release_event",
			 G_CALLBACK(filelist_release_cb), NULL);

	g_signal_connect(G_OBJECT(playlist_view), "button_press_event",
			 G_CALLBACK(playlist_press_cb), NULL);
	g_signal_connect(G_OBJECT(playlist_view), "button_release_event",
			 G_CALLBACK(playlist_release_cb), NULL);



	if (slik_remember_position)
		{
		gtk_window_set_default_size(GTK_WINDOW(playlist_window), window_list_w, window_list_h);
		gtk_window_move(GTK_WINDOW(playlist_window), window_list_x, window_list_y);
		}

	gtk_widget_show(playlist_window);
}

