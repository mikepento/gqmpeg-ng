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
#include "filelist.h"

#include "types.h"
#include "playlist.h"
#include "playlist-window.h"
#include "ui_fileops.h"

static gint sort_list_cb(void *a, void *b);
static gint filelist_read(const gchar *path);

static gint real_list_count(GList *list);
static GList *real_list_add_item(GList *list, const gchar *path);
static GList *real_list_insert_item(GList *list, const gchar *path, gint n);
static GList *real_list_remove_item(GList *list, gint n);
static GList *real_list_swap_items(GList *list, gint n1, gint n2);
static const gchar *real_list_get_item(GList *list, gint n);
static gchar *real_list_get_path(GList *list, gint n);

/*
 *-----------------------------------------------------------------------------
 * reload file list (private)
 *-----------------------------------------------------------------------------
 */

static gint sort_list_cb(void *a, void *b)
{
	return strcmp((gchar *)a, (gchar *)b);
}

static gint filelist_read(const gchar *path)
{
	DIR *dp;
	struct dirent *dir;
	struct stat ent_sbuf;
	gchar *pathl;

	pathl = path_from_utf8(path);
	if((dp = opendir(pathl))==NULL)
		{
		/* dir not found */
		g_free(pathl);
		return FALSE;
		}
	
	g_list_foreach(dirlist,(GFunc)g_free,NULL);
	g_list_free(dirlist);
	dirlist = NULL;

	g_list_foreach(filelist,(GFunc)g_free,NULL);
	g_list_free(filelist);
	filelist = NULL;

	while ((dir = readdir(dp)) != NULL)
		{
		/* skips removed files */
		if (dir->d_ino > 0)
			{
			gchar *name = dir->d_name;
			if (!file_is_hidden(name))
				{
				gchar *filepath = g_strconcat(pathl, "/", name, NULL);
				if (stat(filepath,&ent_sbuf) >= 0 && S_ISDIR(ent_sbuf.st_mode))
					{
					dirlist = g_list_prepend(dirlist, path_to_utf8(name));
					}
				else
					{
					if (file_is_in_filter(name) || is_playlist(name))
						filelist = g_list_prepend(filelist, path_to_utf8(name));
					}
				g_free(filepath);
				}
			}
		}

	closedir(dp);

	g_free(pathl);

	dirlist = g_list_sort(dirlist, (GCompareFunc) sort_list_cb);
	filelist = g_list_sort(filelist, (GCompareFunc) sort_list_cb);

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * file list routines (private)
 *-----------------------------------------------------------------------------
 */

static gint real_list_count(GList *list)
{
	return g_list_length(list);
}

static GList *real_list_add_item(GList *list, const gchar *path)
{
	return g_list_append(list, g_strdup(path));
}

static GList *real_list_insert_item(GList *list, const gchar *path, gint n)
{
	if (n >= real_list_count(list))
		{
		return g_list_append(list, g_strdup(path));
		}
	else
		{
		return g_list_insert(list, g_strdup(path), n);
		}
}

static GList *real_list_remove_item(GList *list, gint n)
{
	GList *work;
	if (n >= real_list_count(list)) return list;
	work = g_list_nth(filelist, n);
	list = g_list_remove(list, work->data);
	g_free(work->data);
	return list;
}

static GList *real_list_swap_items(GList *list, gint n1, gint n2)
{
	GList *list1;
	GList *list2;
	gint c = real_list_count(list);
	if (n1 >= c || n2 >= c) return list;
	list1 = g_list_nth(list, n2);
	list2 = g_list_nth(list, n1);
	list = g_list_remove(list,list1->data);
	list = g_list_insert(list,list1->data, n1);
	list = g_list_remove(list,list2->data);
	list = g_list_insert(list,list2->data, n2);
	return list;
}

static const gchar *real_list_get_item(GList *list, gint n)
{
	GList *work;
	if (n >= real_list_count(list)) return NULL;
	work = g_list_nth(list, n);
	if (!work) return NULL;
	return work->data;
}

static gchar *real_list_get_path(GList *list, gint n)
{
	GList *work;
	gchar *name;
	if (n >= real_list_count(list)) return NULL;
	work = g_list_nth(list, n);
	if (!work) return NULL;
	if (strcmp(current_path, "/") == 0)
		{
		return g_strconcat(current_path, work->data, NULL);
		}
	name = work->data;
	if (name[0] == '.')
		{
		if (name[1] == '\0')
			{
			return g_strdup(current_path);
			}
		else if (name[1] == '.' && name[2] == '\0')
			{
			return remove_level_from_path(current_path);
			}
		}
	return g_strconcat(current_path, "/", name, NULL);
}

/*
 *-----------------------------------------------------------------------------
 * file list routines (public)
 *-----------------------------------------------------------------------------
 */

gint filelist_count(void)
{
	return real_list_count(filelist);
}

void filelist_add_item(const gchar *path)
{
	filelist = real_list_add_item(filelist, path);
}

void filelist_insert_item(const gchar *path, gint n)
{
	filelist = real_list_insert_item(filelist, path, n);
}

void filelist_remove_item(gint n)
{
	filelist = real_list_remove_item(filelist, n);
}

void filelist_swap_items(gint n1, gint n2)
{
	filelist = real_list_swap_items(filelist, n1, n2);
}

const gchar *filelist_get_item(gint n)
{
	return real_list_get_item(filelist, n);
}

gchar *filelist_get_path(gint n)
{
	return real_list_get_path(filelist, n);
}

/*
 *-----------------------------------------------------------------------------
 * dir list routines (public)
 *-----------------------------------------------------------------------------
 */

gint dirlist_count(void)
{
	return real_list_count(dirlist);
}

void dirlist_add_item(const gchar *path)
{
	dirlist = real_list_add_item(dirlist, path);
}

void dirlist_insert_item(const gchar *path, gint n)
{
	dirlist = real_list_insert_item(dirlist, path, n);
}

void dirlist_remove_item(gint n)
{
	dirlist = real_list_remove_item(dirlist, n);
}

void dirlist_swap_items(gint n1, gint n2)
{
	dirlist = real_list_swap_items(dirlist, n1, n2);
}

const gchar *dirlist_get_item(gint n)
{
	return real_list_get_item(dirlist, n);
}

gchar *dirlist_get_path(gint n)
{
	return real_list_get_path(dirlist, n);
}

/*
 *-----------------------------------------------------------------------------
 * path change / update (public)
 *-----------------------------------------------------------------------------
 */

void filelist_refresh(void)
{
	if (debug_mode) printf("reading dir: %s\n", current_path);
	filelist_read(current_path);
	playlist_window_filelist_update();
}

void change_to_path(const gchar *path)
{
	if (debug_mode) printf("attempt change dir to: %s\n", path);
	if (!isdir(path)) return;

	g_free(current_path);
	current_path = g_strdup(path);
	parse_out_relatives(current_path);

	filelist_refresh();
}


