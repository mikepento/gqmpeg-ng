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
#include "types.h"

/*
 *-----------------------------------------------------------------------------
 * standard file filtering (public)
 *-----------------------------------------------------------------------------
 */

typedef struct _FilterData FilterData;
struct _FilterData
{
	gchar *text;
	const gchar *description;
	gint module_id;
};

static GList *filename_filter = NULL;

gint file_is_hidden(const gchar *name)
{
	if (show_dot_files) return FALSE;
	if (name[0] != '.') return FALSE;
	if (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')) return FALSE;
	return TRUE;
}

gint file_is_in_filter(const gchar *name)
{
	GList *work;
	if (!filename_filter || disable_filtering) return TRUE;

	work = filename_filter;
	while (work)
		{
		FilterData *fd = work->data;
		gint lf = strlen(fd->text);
		gint ln = strlen(name);
		if (ln >= lf)
			{
			if (strncasecmp(name + ln - lf, fd->text, lf) == 0) return TRUE;
			}
		work = work->next;
		}

	return FALSE;
}

void add_to_filter(const gchar *text, const gchar *description, gint module_id)
{
	FilterData *fd;
	fd = g_new0(FilterData, 1);
	fd->text = g_strdup(text);
	fd->description = description;
	fd->module_id = module_id;
	filename_filter = g_list_append(filename_filter, fd);
}

void remove_from_filter(const gchar *text)
{
	GList *work = filename_filter;
	while(work)
		{
		FilterData *fd = work->data;
		if (strcmp(fd->text, text) == 0)
			{
			filename_filter = g_list_remove(filename_filter, fd);
			g_free(fd->text);
			g_free(fd);
			}
		work = work->next;
		}
}

gint filter_determine_module_id(const gchar *path)
{
	GList *work;

	work = filename_filter;
	while (work)
		{
		FilterData *fd = work->data;
		gint lf = strlen(fd->text);
		gint ln = strlen(path);
		if (ln >= lf)
			{
			if (strncasecmp(path + ln - lf, fd->text, lf) == 0) return fd->module_id;
			}
		work = work->next;
		}

	return -1;
}

static gchar *real_filter_get_text_list(gint module_id)
{
	GList *work;
	gchar *buf;
	gchar *ret = NULL;

	work = filename_filter;
	while (work)
		{
		FilterData *fd = work->data;
		if (module_id < 0 || fd->module_id == module_id)
			{
			if (!ret)
				{
				ret = g_strdup(fd->text);
				}
			else
				{
				buf = ret;
				ret = g_strconcat(buf, ";", fd->text, NULL);
				g_free(buf);
				}
			}
		work = work->next;
		}

	return ret;
}

gchar *filter_get_text_list(void)
{
	return real_filter_get_text_list(-1);
}

gchar *filter_get_text_list_by_id(gint module_id)
{
	return real_filter_get_text_list(module_id);
}

const gchar *filter_get_description_by_id(gint module_id)
{
	GList *work;

	work = filename_filter;
	while (work)
		{
		FilterData *fd = work->data;
		/* simply return the first match
		 * (most modules use same desc for all extensions)
		 */
		if (fd->module_id == module_id && fd->description) return fd->description;
		work = work->next;
		}

	return "";
}

/*
 *-----------------------------------------------------------------------------
 * type filtering (public)
 *-----------------------------------------------------------------------------
 */

typedef struct _TypeData TypeData;
struct _TypeData
{
	const gchar *format;
	const gchar *description;
	gint module_id;
	gint live;
	gint (* is_type_func)(const gchar *path);
	GtkWidget *(* entry_setup_func)(const gchar *path);
	GtkWidget *(* edit_func)(SongData *sd);
	gchar *(*get_path_func)(GtkWidget *widget);
};

static GList *typelist = NULL;

void add_to_typelist(const gchar *format, const gchar *description, gint module_id, gint live,
		     gint (*is_type_func)(const gchar *),
		     GtkWidget *(*entry_setup_func)(const gchar *),
		     GtkWidget *(*edit_func)(SongData *),
		     gchar *(*get_path_func)(GtkWidget *))
{
	TypeData *td = g_new0(TypeData, 1);

	td->format = format;
	td->description = description;
	td->module_id = module_id;
	td->live = live;
	td->is_type_func = is_type_func;
	td->entry_setup_func = entry_setup_func;
	td->edit_func = edit_func;
	td->get_path_func = get_path_func;

	typelist = g_list_append(typelist, td);
}

static TypeData * typelist_type_by_id(gint id)
{
	GList *work = g_list_nth(typelist, id);

	if (!work) return NULL;

	return work->data;
}

gint typelist_determine_module_id(const gchar *path)
{
	GList *work;

	if (!path) return -1;

	work = typelist;

	while(work)
		{
		TypeData *td = work->data;
		if (td->is_type_func && td->is_type_func(path))
			{
			return td->module_id;
			}
		work = work->next;
		}

	return -1;
}

gint typelist_determine_type_id(const gchar *path)
{
	GList *work = typelist;
	gint ret = 0;

	while(work)
		{
		TypeData *td = work->data;
		if (td->is_type_func && td->is_type_func(path))
			{
			return ret;
			}
		ret++;
		work = work->next;
		}

	return -1;
}

void typelist_determine_ids(const gchar *path, gint *module_id, gint *custom, gint *custom_type, gint *live)
{
	GList *work = typelist;
	gint ct = 0;

	while(work)
		{
		TypeData *td = work->data;
		if (td->is_type_func && td->is_type_func(path))
			{
			*module_id = td->module_id;
			*custom = TRUE;
			*custom_type = ct;
			*live = td->live;
			return;
			}
		ct++;
		work = work->next;
		}

	*module_id = -1;
	*custom = FALSE;
	*live = FALSE;
}

GtkWidget *typelist_get_entry_widget_by_id(gint id, const gchar *path)
{
	TypeData *td = typelist_type_by_id(id);

	if (td && td->entry_setup_func) return td->entry_setup_func(path);

	return NULL;
}

GtkWidget *typelist_get_edit_widget_by_id(gint id, SongData *sd)
{
	TypeData *td = typelist_type_by_id(id);

	if (td && td->edit_func) return td->edit_func(sd);

	return NULL;
}

gchar *typelist_get_path_from_widget(gint id, GtkWidget *widget)
{
	TypeData *td = typelist_type_by_id(id);

	if (td && td->get_path_func) return td->get_path_func(widget);

	return NULL;
}

GList *typelist_get_description_list(void)
{
	GList *list = NULL;
	GList *work = typelist;

	while(work)
		{
		TypeData *td = work->data;
		list = g_list_append(list, (gchar *)td->description);
		work = work->next;
		}

	return list;
}

