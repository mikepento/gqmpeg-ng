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
#include "playlist-dlg.h"

#include "playlist.h"
#include "players.h"
#include "types.h"
#include "ui_fileops.h"
#include "ui_menu.h"
#include "ui_tabcomp.h"
#include "ui_utildlg.h"

enum {
	DIALOG_SAVE,
	DIALOG_LOAD,
	DIALOG_APPEND,
	DIALOG_IMPORT
};


/*
 *-----------------------------------------------------------------------------
 * playlist save and load dialogs (private)
 *-----------------------------------------------------------------------------
 */

static GenericDialog *playlist_save_confirmed(FileDialog *fd, gint overwrite);

static void playlist_dialog_close_cb(FileDialog *fd, gpointer data)
{
	GenericDialog *gd = data;

	if (gd) generic_dialog_close(gd);
	file_dialog_close(fd);
}

static void playlist_confirm_cancel(GenericDialog *gd, gpointer data)
{
	FileDialog *fd = data;
	GENERIC_DIALOG(fd)->data = NULL;
}

static void playlist_confirm_ok(GenericDialog *gd, gpointer data)
{
	FileDialog *fd = data;
	GENERIC_DIALOG(fd)->data = NULL;

	playlist_save_confirmed(fd, TRUE);
	playlist_dialog_close_cb(fd, NULL);
}

static GenericDialog *playlist_save_confirmed(FileDialog *fd, gint overwrite)
{
	if (!overwrite && isfile(fd->dest_path))
		{
		GenericDialog *gd;
		gchar *buf;

		buf = g_strdup_printf(_("Overwrite playlist file:\n%s"), fd->dest_path);
		gd = generic_dialog_new(_("Overwrite File"), buf, "GQmpeg", "confirm", TRUE,
					playlist_confirm_cancel, fd);
		g_free(buf);
		generic_dialog_add_stock(gd, _("Overwrite"), GTK_STOCK_OK, playlist_confirm_ok, TRUE);

		gtk_widget_show(gd->dialog);

		return gd;
		}

	if (!playlist_save(fd->dest_path))
		{
		gchar *buf = g_strdup_printf(_("Failed to save the playlist:\n%s"), fd->dest_path);
		warning_dialog(_("Save Failed"), buf);
		g_free(buf);
		}

	return NULL;
}

static void playlist_dialog_save_cb(FileDialog *fd, gpointer data)
{
	GenericDialog *gd = data;

	if (!fd->dest_path || isdir(fd->dest_path)) return;

	if (gd)	generic_dialog_close(gd);
	gd = NULL;

	if (!strlen(fd->dest_path) > 7 || strncasecmp(fd->dest_path + (strlen(fd->dest_path) - 7), ".gqmpeg", 7) != 0)
		{
		gchar *buf;
		buf = g_strconcat(fd->dest_path, ".gqmpeg", NULL);
		gtk_entry_set_text(GTK_ENTRY(fd->entry), buf);
		g_free(buf);
		}

	gd = playlist_save_confirmed(fd, FALSE);

	if (!gd)
		{
		file_dialog_sync_history(fd, TRUE);
		playlist_dialog_close_cb(fd, NULL);
		}
	else
		{
		GENERIC_DIALOG(fd)->data = gd;
		}
}

static void real_playlist_dialog_load_do(FileDialog *fd, gint append, gint import)
{
	if (!fd->dest_path || isdir(fd->dest_path)) return;

	file_dialog_sync_history(fd, TRUE);

	playlist_load_from_file(fd->dest_path, append, import, TRUE);

	playlist_dialog_close_cb(fd, NULL);
}

static void playlist_dialog_load_cb(FileDialog *fd, gpointer data)
{
	real_playlist_dialog_load_do(fd, FALSE, FALSE);
}

static void playlist_dialog_append_cb(FileDialog *fd, gpointer data)
{
	real_playlist_dialog_load_do(fd, TRUE, FALSE);
}

static void playlist_dialog_import_cb(FileDialog *fd, gpointer data)
{
	real_playlist_dialog_load_do(fd, TRUE, FALSE);
}

static void playlist_save_or_load_dialog(gchar *path, gint type)
{
	FileDialog *fd;
	const gchar *title;
	const gchar *text;
	const gchar *btntext;
	const gchar *btnstock;
	const gchar *hist_key;
	gchar *base;
	void *btnfunc;

	if (type == DIALOG_SAVE)
		{
		title = _("Save Playlist");
		text = _("Save playlist as:");
		btntext = NULL;
		btnstock = GTK_STOCK_SAVE;
		hist_key = "playlist_load_save";
		btnfunc = playlist_dialog_save_cb;
		}
	else if (type == DIALOG_LOAD)
		{
		title = _("Open Playlist");
		text = _("Open playlist from:");
		btntext = NULL;
		btnstock = GTK_STOCK_OPEN;
		hist_key = "playlist_load_save";
		btnfunc = playlist_dialog_load_cb;
		}
	else if (type == DIALOG_APPEND)
		{
		title = _("Append Playlist");
		text = _("Append playlist from:");
		btntext = _("Append");
		btnstock = GTK_STOCK_OPEN;
		hist_key = "playlist_load_save";
		btnfunc = playlist_dialog_append_cb;
		}
	else
		{
		title = _("Import Playlist");
		text = _("Import playlist from:");
		btntext = _("Import");
		btnstock = GTK_STOCK_OPEN;
		hist_key = "playlist_import";
		btnfunc = playlist_dialog_import_cb;
		}

	fd = file_dialog_new(title, text, "GQmpeg", "open",
			     playlist_dialog_close_cb, NULL);
	file_dialog_add_button_stock(fd, btntext, btnstock, btnfunc, TRUE);

	base = g_strconcat(homedir(), "/", GQMPEG_RC_DIR_PLAYLIST, NULL);
	file_dialog_add_path_widgets(fd, base, path, hist_key, ".gqmpeg;.gqoob", _("GQmpeg Playlist Files"));
	g_free(base);

	if (type != DIALOG_SAVE) file_dialog_add_filter(fd, ".m3u", _("m3u Playlist"), FALSE);

	fd->type = type;

	if (type == DIALOG_SAVE && fd->dest_path)
		{
		gtk_editable_select_region(GTK_EDITABLE(fd->entry),
					   strlen(fd->dest_path) - strlen(filename_from_path(fd->dest_path)),
					    strlen(fd->dest_path));
		}
	if (type == DIALOG_IMPORT)
		{
		/* set to all files for import */
		file_dialog_add_filter(fd, "*", NULL, TRUE);
		}

	gtk_widget_show(GENERIC_DIALOG(fd)->dialog);
}

/*
 *-----------------------------------------------------------------------------
 * playlist save and load dialogs (public)
 *-----------------------------------------------------------------------------
 */

void playlist_dialog_save_as(gchar *path)
{
	if (playlist_get_count() < 1)
		{
		warning_dialog(_("Playlist empty"), _("The current playlist is empty, save aborted."));
		return;
		}
	playlist_save_or_load_dialog(path, DIALOG_SAVE);
}

void playlist_dialog_load(gchar *path)
{
	playlist_save_or_load_dialog(path, DIALOG_LOAD);
}

void playlist_dialog_append(gchar *path)
{
	playlist_save_or_load_dialog(path, DIALOG_APPEND);
}

void playlist_dialog_import(gchar *path)
{
	playlist_save_or_load_dialog(path, DIALOG_IMPORT);
}

/*
 *-----------------------------------------------------------------------------
 * playlist add custom (non file-types, or urls, etc.)
 *-----------------------------------------------------------------------------
 */

typedef struct _DlgCustomData DlgCustomData;
struct _DlgCustomData
{
	GenericDialog *gd;
	SongData *sd;
	GtkWidget *entry;
	gint type;
	gint add;
};

static void playlist_dialog_custom_close_cb(GenericDialog *gd, gpointer data)
{
	DlgCustomData *dcd = data;
	g_free(dcd);
	generic_dialog_close(gd);
}

static void playlist_dialog_custom_add_cb(GenericDialog *gd, gpointer data)
{
	DlgCustomData *dcd = data;

	if (dcd->type != -1)
		{
		gchar *path;

		path = typelist_get_path_from_widget(dcd->type, dcd->entry);
		if (path)
			{
			if (dcd->add)
				{
				playlist_add(path, TRUE);
				}
			else
				{
				current_song_set_and_play(-1, path);
				}
			g_free(path);
			}
		}

	playlist_dialog_custom_close_cb(gd, data);
}

static void playlist_dialog_custom_menu_cb(GtkWidget *widget, gpointer data)
{
	DlgCustomData *dcd = data;
	gint c;

	c = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "item_number"));
	if (dcd->type == c) return;

	gtk_widget_destroy(dcd->entry);
	dcd->entry = typelist_get_entry_widget_by_id(c, current_path);
	if (!dcd->entry)
		{
		dcd->entry = gtk_label_new(_("no entry defined"));
		dcd->type = -1;
		}
	else
		{
		dcd->type = c;
		}
	gtk_box_pack_start(GTK_BOX(dcd->gd->vbox), dcd->entry, FALSE, FALSE,0);
	gtk_widget_show(dcd->entry);
}

void playlist_dialog_add_custom_type(gint to_playlist)
{
	DlgCustomData *dcd;
	GtkWidget *optionmenu;
	GtkWidget *menu;
	GtkWidget *item;
	GList *list;
	GList *work;
	gint c = 0;
	gchar *title;

	if (to_playlist)
		{
		title = _("GQmpeg - add custom song type");
		}
	else
		{
		title = _("GQmpeg - open custom song type");
		}

	dcd = g_new0(DlgCustomData, 1);
	dcd->add = to_playlist;

	dcd->gd = generic_dialog_new(title, NULL,
				     "GQmpeg", "custom_add", FALSE,
				     playlist_dialog_custom_close_cb, dcd);
	generic_dialog_add_stock(dcd->gd, NULL, GTK_STOCK_OK, playlist_dialog_custom_add_cb, TRUE);

	optionmenu = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(dcd->gd->vbox), optionmenu, FALSE, FALSE,0);
	gtk_widget_show(optionmenu);

	menu = gtk_menu_new();

	list = typelist_get_description_list();
	work = list;
	while (work)
		{
		item = menu_item_add(menu, (gchar *)work->data,
				     G_CALLBACK(playlist_dialog_custom_menu_cb), dcd);
		g_object_set_data(G_OBJECT(item), "item_number", GINT_TO_POINTER(c));
		c++;
		work = work->next;
		}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);

	if (list)
		{
		dcd->entry = typelist_get_entry_widget_by_id(0, current_path);
		if (!dcd->entry)
			{
			dcd->entry = gtk_label_new(_("no entry defined"));
			dcd->type = -1;
			}
		else
			{
			dcd->type = 0;
			}
		gtk_box_pack_start(GTK_BOX(dcd->gd->vbox), dcd->entry, FALSE, FALSE, 0);
		gtk_widget_show(dcd->entry);
		}
	else
		{
		GtkWidget *label = gtk_label_new(_("No types enabled"));
		gtk_box_pack_start(GTK_BOX(dcd->gd->vbox), label, FALSE, FALSE, 0);
		gtk_widget_show(label);
		dcd->entry = NULL;
		dcd->type = -1;
		}

	g_list_free(list);

	gtk_widget_show(dcd->gd->dialog);
}

/*
 *-----------------------------------------------------------------------------
 * playlist edit custom (non file-types, or urls, etc.)
 *-----------------------------------------------------------------------------
 */

static void playlist_dialog_custom_edit_close_cb(GenericDialog *gd, gpointer data)
{
	DlgCustomData *dcd = data;

	g_free(dcd);
	generic_dialog_close(gd);
}

static void playlist_dialog_custom_edit_change_cb(GenericDialog *gd, gpointer data)
{
	DlgCustomData *dcd = data;

	if (dcd->type != -1)
		{
		gchar *path;
		gint row;

		path = typelist_get_path_from_widget(dcd->type, dcd->entry);
		if (path)
			{
			row = playlist_get_index_by_data(dcd->sd);
			if (row >= 0) playlist_update(row, path);
			g_free(path);
			}
		}

	playlist_dialog_custom_edit_close_cb(gd, data);
}

void playlist_dialog_edit_custom_type(SongData *sd)
{
	GenericDialog *gd;
	DlgCustomData *dcd;

	if (!sd->custom) return;

	dcd = g_new0(DlgCustomData, 1);
	dcd->sd = sd;
	dcd->type = sd->custom_type;

	gd = generic_dialog_new(_("GQmpeg - edit custom song type"), NULL,
				"GQmpeg", "custom_edit", FALSE,
				playlist_dialog_custom_edit_close_cb, dcd);
	generic_dialog_add_stock(gd, NULL, GTK_STOCK_OK, playlist_dialog_custom_edit_change_cb, TRUE);

	dcd->entry = typelist_get_edit_widget_by_id(sd->custom_type, sd);
	if (!dcd->entry)
		{
		dcd->entry = gtk_label_new(_("no entry defined"));
		dcd->type = -1;
		}
	gtk_box_pack_start(GTK_BOX(gd->vbox), dcd->entry, FALSE, FALSE,0);
	gtk_widget_show(dcd->entry);

	gtk_widget_show(gd->dialog);
}

/*
 *-----------------------------------------------------------------------------
 * load a file
 *-----------------------------------------------------------------------------
 */

static void playlist_dialog_file_load_cb(FileDialog *fd, gpointer data)
{
	if (!fd->dest_path || isdir(fd->dest_path)) return;
	if (!isfile(fd->dest_path))
		{
		gchar *buf = g_strdup_printf(_("File %s not found."), fd->dest_path);
		warning_dialog(_("File not found"), buf);
		g_free(buf);
		return;
		}

	if (fd->type)
		{
		playlist_add(fd->dest_path, TRUE);
		}
	else
		{
		current_song_set_and_play(-1, fd->dest_path);
		}

	file_dialog_sync_history(fd, TRUE);

	file_dialog_close(fd);
}

static void playlist_dialog_file_close_cb(FileDialog *fd, gpointer data)
{
	file_dialog_close(fd);
}

void playlist_dialog_load_file(const gchar *path, gint to_playlist)
{
	FileDialog *fd;
	gchar *text;
	gchar *filter;
	gint i;

	if (to_playlist)
		{
		text = _("Add to playlist:");
		}
	else
		{
		text = _("Open file:");
		}

	fd = file_dialog_new(_("GQmpeg - open"), text, "GQmpeg", "open",
			     playlist_dialog_file_close_cb, NULL);
	file_dialog_add_button_stock(fd, NULL, GTK_STOCK_OPEN, playlist_dialog_file_load_cb, TRUE);

	filter = filter_get_text_list();
	file_dialog_add_path_widgets(fd, NULL, path, "file_open", filter, _("Song Files"));
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

	fd->type = to_playlist;

	gtk_widget_show(GENERIC_DIALOG(fd)->dialog);
}

