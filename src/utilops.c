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
#include "utilops.h"

#include "filelist.h"
#include "playlist.h"
#include "ui_bookmark.h"
#include "ui_fileops.h"
#include "ui_tabcomp.h"


/*
 *--------------------------------------------------------------------------
 * call these when names change, files move, deleted, etc.
 * so that any open windows are also updated
 *--------------------------------------------------------------------------
 */

void file_maint_renamed(const gchar *source, const gchar *dest)
{
	playlist_replace(source, dest);
	filelist_refresh();
}

/* under most cases ignore_list should be NULL */
void file_maint_removed(const gchar *path, GList *ignore_list)
{
	playlist_remove(path, 0, TRUE);
	filelist_refresh();
}

/* special case for correct main window behavior */
void file_maint_moved(const gchar *source, const gchar *dest, GList *ignore_list)
{
	playlist_replace(source, dest);
	filelist_refresh();
}

void file_maint_copied(const gchar *source, const gchar *dest)
{
}

/*
 *--------------------------------------------------------------------------
 * The file manipulation dialogs
 *--------------------------------------------------------------------------
 */


enum {
	DIALOG_NEW_DIR,
	DIALOG_COPY,
	DIALOG_MOVE,
	DIALOG_DELETE,
	DIALOG_RENAME
};

typedef struct _FileDataMult FileDataMult;
struct _FileDataMult
{
	gint confirm_all;
	gint confirmed;
	gint skip;
	GList *source_list;
	GList *source_next;
	gchar *dest_base;
	gchar *source;
	gchar *dest;
	gint copy;

	gint rename;
	gint rename_auto;
	gint rename_all;

	GtkWidget *rename_box;
	GtkWidget *rename_entry;
	GtkWidget *rename_auto_box;

	GtkWidget *yes_all_button;
};

typedef struct _FileDataSingle FileDataSingle;
struct _FileDataSingle
{
	gint confirmed;
	gchar *source;
	gchar *dest;
	gint copy;

	gint rename;
	gint rename_auto;

	GtkWidget *rename_box;
	GtkWidget *rename_entry;
	GtkWidget *rename_auto_box;
};


/*
 *--------------------------------------------------------------------------
 * Move and Copy routines
 *--------------------------------------------------------------------------
 */

/*
 * Multi file move
 */

static FileDataMult *file_data_multiple_new(GList *source_list, const gchar *dest, gint copy)
{
	FileDataMult *fdm = g_new0(FileDataMult, 1);
	fdm->confirm_all = FALSE;
	fdm->confirmed = FALSE;
	fdm->skip = FALSE;
	fdm->source_list = source_list;
	fdm->source_next = fdm->source_list;
	fdm->dest_base = g_strdup(dest);
	fdm->source = NULL;
	fdm->dest = NULL;
	fdm->copy = copy;
	return fdm;
}

static void file_data_multiple_free(FileDataMult *fdm)
{
	path_list_free(fdm->source_list);
	g_free(fdm->dest_base);
	g_free(fdm->dest);
	g_free(fdm);
}

static void file_util_move_multiple(FileDataMult *fdm);

static void file_util_move_multiple_ok_cb(GenericDialog *gd, gpointer data)
{
	FileDataMult *fdm = data;

	fdm->confirmed = TRUE;

	if (fdm->rename_auto)
		{
		gchar *buf;

		buf = unique_filename_simple(fdm->dest);
		if (buf)
			{
			g_free(fdm->dest);
			fdm->dest = buf;
			}
		else
			{
			/* unique failed? well, return to the overwrite prompt :( */
			fdm->confirmed = FALSE;
			}
		}
	else if (fdm->rename)
		{
		const gchar *name;

		name = gtk_entry_get_text(GTK_ENTRY(fdm->rename_entry));
		if (strlen(name) == 0 ||
		    strcmp(name, filename_from_path(fdm->source)) == 0)
			{
			fdm->confirmed = FALSE;
			}
		else
			{
			g_free(fdm->dest);
			fdm->dest = concat_dir_and_file(fdm->dest_base, name);
			fdm->confirmed = !isname(fdm->dest);
			}
		}

	file_util_move_multiple(fdm);
}

static void file_util_move_multiple_all_cb(GenericDialog *gd, gpointer data)
{
	FileDataMult *fdm = data;

	fdm->confirm_all = TRUE;

	if (fdm->rename_auto) fdm->rename_all = TRUE;

	file_util_move_multiple(fdm);
}

static void file_util_move_multiple_skip_cb(GenericDialog *gd, gpointer data)
{
	FileDataMult *fdm = data;

	fdm->skip = TRUE;
	fdm->confirmed = TRUE;

	file_util_move_multiple(fdm);
}

static void file_util_move_multiple_skip_all_cb(GenericDialog *gd, gpointer data)
{
	FileDataMult *fdm = data;

	fdm->skip = TRUE;
	fdm->confirm_all = TRUE;
	file_util_move_multiple(fdm);
}

static void file_util_move_multiple_continue_cb(GenericDialog *gd, gpointer data)
{
	FileDataMult *fdm = data;

	fdm->confirmed = TRUE;
	file_util_move_multiple(fdm);
}

static void file_util_move_multiple_cancel_cb(GenericDialog *gd, gpointer data)
{
	FileDataMult *fdm = data;

	file_data_multiple_free(fdm);
}

/* rename option */

static void file_util_move_multiple_rename_auto_cb(GtkWidget *widget, gpointer data)
{
	GenericDialog *gd = data;
	FileDataMult *fdm;

	fdm = gd->data;

	fdm->rename_auto = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	gtk_widget_set_sensitive(fdm->rename_box, !fdm->rename_auto);
	gtk_widget_set_sensitive(fdm->rename_entry, (!fdm->rename_auto && fdm->rename));

	if (fdm->rename_auto)
		{
		gchar *preview;

		preview = unique_filename_simple(fdm->dest);
		if (preview) gtk_entry_set_text(GTK_ENTRY(fdm->rename_entry), filename_from_path(preview));
		g_free(preview);
		}

	gtk_widget_set_sensitive(fdm->yes_all_button, (fdm->rename_auto || !fdm->rename));
}

static void file_util_move_multiple_rename_cb(GtkWidget *widget, gpointer data)
{
	GenericDialog *gd = data;
	FileDataMult *fdm;

	fdm = gd->data;

	fdm->rename = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	gtk_widget_set_sensitive(fdm->rename_entry, fdm->rename);
	gtk_widget_set_sensitive(fdm->yes_all_button, !fdm->rename);

	if (fdm->rename) gtk_widget_grab_focus(fdm->rename_entry);
}

static GenericDialog *file_util_move_multiple_confirm_dialog(FileDataMult *fdm)
{
	GenericDialog *gd;
	gchar *text;
	GtkWidget *hbox;

	text = g_strdup_printf(_("Overwrite file:\n %s\n with:\n %s"), fdm->dest, fdm->source);
	gd = generic_dialog_new(_("Overwrite file - GQmpeg"), text, "GQmpeg", "dlg_confirm", TRUE,
				file_util_move_multiple_cancel_cb, fdm);
	g_free(text);

	generic_dialog_add_stock(gd, _("Yes"), GTK_STOCK_YES, file_util_move_multiple_ok_cb, TRUE);
	fdm->yes_all_button = generic_dialog_add(gd, _("Yes to all"),
						 file_util_move_multiple_all_cb, FALSE);
	generic_dialog_add_stock(gd, _("Skip"), GTK_STOCK_GO_FORWARD, file_util_move_multiple_skip_cb, FALSE);
	generic_dialog_add_stock(gd, _("Skip all"), GTK_STOCK_GOTO_LAST, file_util_move_multiple_skip_all_cb, FALSE);

	/* rename option */

	fdm->rename = FALSE;
	fdm->rename_all = FALSE;
	fdm->rename_auto = FALSE;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(gd->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	fdm->rename_auto_box = gtk_check_button_new_with_label(_("Auto rename"));
	g_signal_connect(G_OBJECT(fdm->rename_auto_box), "clicked",
			 G_CALLBACK(file_util_move_multiple_rename_auto_cb), gd);
	gtk_box_pack_start(GTK_BOX(hbox), fdm->rename_auto_box, FALSE, FALSE, 0);
	gtk_widget_show(fdm->rename_auto_box);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(gd->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	fdm->rename_box = gtk_check_button_new_with_label(_("Rename"));
	g_signal_connect(G_OBJECT(fdm->rename_box), "clicked",
			 G_CALLBACK(file_util_move_multiple_rename_cb), gd);
	gtk_box_pack_start(GTK_BOX(hbox), fdm->rename_box, FALSE, FALSE, 0);
	gtk_widget_show(fdm->rename_box);

	fdm->rename_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(fdm->rename_entry), filename_from_path(fdm->dest));
	gtk_widget_set_sensitive(fdm->rename_entry, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), fdm->rename_entry, TRUE, TRUE, 0);
	gtk_widget_show(fdm->rename_entry);

	return gd;
}

static void file_util_move_multiple(FileDataMult *fdm)
{
	while (fdm->dest || fdm->source_next)
		{
		gint success = FALSE;
		gint skip_file = FALSE;

		if (!fdm->dest)
			{
			GList *work = fdm->source_next;
			fdm->source = work->data;
			fdm->dest = concat_dir_and_file(fdm->dest_base, filename_from_path(fdm->source));
			fdm->source_next = work->next;
			fdm->confirmed = FALSE;
			}

		if (fdm->dest && fdm->source && strcmp(fdm->dest, fdm->source) == 0)
			{
			if (!fdm->confirmed)
				{
				GenericDialog *gd;
				const gchar *title;
				gchar *text;

				if (fdm->copy)
					{
					title = _("Source to copy matches destination");
					text = g_strdup_printf(_("Unable to copy file:\n%s\nto itself."), fdm->dest);
					}
				else
					{
					title = _("Source to move matches destination");
					text = g_strdup_printf(_("Unable to move file:\n%s\nto itself."), fdm->dest);
					}

				gd = generic_dialog_new(title, text, "GQmpeg", "dlg_confirm", TRUE,
							file_util_move_multiple_cancel_cb, fdm);
				g_free(text);
				generic_dialog_add_stock(gd, _("Continue"), GTK_STOCK_GO_FORWARD,
							 file_util_move_multiple_continue_cb, TRUE);

				gtk_widget_show(gd->dialog);
				return;
				}
			skip_file = TRUE;
			}
		else if (isfile(fdm->dest))
			{
			if (!fdm->confirmed && !fdm->confirm_all)
				{
				GenericDialog *gd;

				gd = file_util_move_multiple_confirm_dialog(fdm);
				gtk_widget_show(gd->dialog);
				return;
				}
			if (fdm->skip) skip_file = TRUE;
			}

		if (skip_file)
			{
			success = TRUE;
			if (!fdm->confirm_all) fdm->skip = FALSE;
			}
		else
			{
			gint try = TRUE;

			if (fdm->confirm_all && fdm->rename_all && isfile(fdm->dest))
				{
				gchar *buf;
				buf = unique_filename_simple(fdm->dest);
				if (buf)
					{
					g_free(fdm->dest);
					fdm->dest = buf;
					}
				else
					{
					try = FALSE;
					}
				}
			if (try)
				{
				if (fdm->copy)
					{
					if (copy_file(fdm->source, fdm->dest));
						{
						success = TRUE;
						file_maint_copied(fdm->source, fdm->dest);
						}
					}
				else
					{
					if (move_file(fdm->source, fdm->dest))
						{
						success = TRUE;
						file_maint_moved(fdm->source, fdm->dest, fdm->source_list);
						}
					}
				}
			}

		g_free(fdm->dest);
		fdm->dest = NULL;

		if (!success)
			{
			GenericDialog *gd;
			const gchar *title;
			gchar *text;

			if (fdm->copy)
				{
				title = _("Error copying file");
				text = g_strdup_printf(_("Unable to copy file:\n%s\nto:\n%s\nduring multiple file copy."), fdm->source, fdm->dest);
				}
			else
				{
				title = _("Error moving file");
				text = g_strdup_printf(_("Unable to move file:\n%s\nto:\n%s\nduring multiple file move."), fdm->source, fdm->dest);
				}
			gd = generic_dialog_new(title, text, "GQmpeg", "dlg_confirm", TRUE,
						file_util_move_multiple_cancel_cb, fdm);
			g_free(text);

			generic_dialog_add_stock(gd, _("Continue"), GTK_STOCK_GO_FORWARD, file_util_move_multiple_continue_cb, TRUE);
			gtk_widget_show(gd->dialog);
			return;
			}
		}

	file_data_multiple_free(fdm);
}

/*
 * Single file move
 */

static FileDataSingle *file_data_single_new(const gchar *source, const gchar *dest, gint copy)
{
	FileDataSingle *fds = g_new0(FileDataSingle, 1);
	fds->confirmed = FALSE;
	fds->source = g_strdup(source);
	fds->dest = g_strdup(dest);
	fds->copy = copy;
	return fds;
}

static void file_data_single_free(FileDataSingle *fds)
{
	g_free(fds->source);
	g_free(fds->dest);
	g_free(fds);
}

static void file_util_move_single(FileDataSingle *fds);

static void file_util_move_single_ok_cb(GenericDialog *gd, gpointer data)
{
	FileDataSingle *fds = data;

	fds->confirmed = TRUE;

	if (fds->rename_auto)
		{
		gchar *buf;

		buf = unique_filename_simple(fds->dest);
		if (buf)
			{
			g_free(fds->dest);
			fds->dest = buf;
			}
		else
			{
			/* unique failed? well, return to the overwrite prompt :( */
			fds->confirmed = FALSE;
			}
		}
	else if (fds->rename)
		{
		const gchar *name;

		name = gtk_entry_get_text(GTK_ENTRY(fds->rename_entry));
		if (strlen(name) == 0 ||
		    strcmp(name, filename_from_path(fds->source)) == 0)
			{
			fds->confirmed = FALSE;
			}
		else
			{
			gchar *base;

			base = remove_level_from_path(fds->dest);
			g_free(fds->dest);
			fds->dest = concat_dir_and_file(base, name);
			fds->confirmed = !isname(fds->dest);

			g_free(base);
			}
		}

	file_util_move_single(fds);
}

static void file_util_move_single_cancel_cb(GenericDialog *gd, gpointer data)
{
	FileDataSingle *fds = data;

	file_data_single_free(fds);
}

static void file_util_move_single_rename_auto_cb(GtkWidget *widget, gpointer data)
{
	GenericDialog *gd = data;
	FileDataSingle *fds;

	fds = gd->data;

	fds->rename_auto = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	gtk_widget_set_sensitive(fds->rename_box, !fds->rename_auto);
	gtk_widget_set_sensitive(fds->rename_entry, (!fds->rename_auto && fds->rename));

	if (fds->rename_auto)
		{
		gchar *preview;

		preview = unique_filename_simple(fds->dest);
		if (preview) gtk_entry_set_text(GTK_ENTRY(fds->rename_entry), filename_from_path(preview));
		g_free(preview);
		}
}

static void file_util_move_single_rename_cb(GtkWidget *widget, gpointer data)
{
	GenericDialog *gd = data;
	FileDataSingle *fds;

	fds = gd->data;

	fds->rename = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	gtk_widget_set_sensitive(fds->rename_entry, fds->rename);

	if (fds->rename) gtk_widget_grab_focus(fds->rename_entry);
}

static void file_util_move_single(FileDataSingle *fds)
{
	if (fds->dest && fds->source && strcmp(fds->dest, fds->source) == 0)
		{
		warning_dialog(_("Source matches destination"),
			       _("Source and destination are the same, operation cancelled."));
		}
	else if (isfile(fds->dest) && !fds->confirmed)
		{
		GenericDialog *gd;
		gchar *text;

		GtkWidget *hbox;

		text = g_strdup_printf(_("Overwrite file:\n%s\n with:\n%s"), fds->dest, fds->source);
		gd = generic_dialog_new(_("Overwrite file - GQmpeg"), text, "GQmpeg", "dlg_confirm", TRUE,
					file_util_move_single_cancel_cb, fds);
		g_free(text);

		generic_dialog_add_stock(gd, _("Overwrite"), GTK_STOCK_OK, file_util_move_single_ok_cb, TRUE);

		/* rename option */

		fds->rename = FALSE;
		fds->rename_auto = FALSE;

		hbox = gtk_hbox_new(FALSE, 5);
		gtk_box_pack_start(GTK_BOX(gd->vbox), hbox, FALSE, FALSE, 0);
		gtk_widget_show(hbox);

		fds->rename_auto_box = gtk_check_button_new_with_label(_("Auto rename"));
		g_signal_connect(G_OBJECT(fds->rename_auto_box), "clicked",
				 G_CALLBACK(file_util_move_single_rename_auto_cb), gd);
		gtk_box_pack_start(GTK_BOX(hbox), fds->rename_auto_box, FALSE, FALSE, 0);
		gtk_widget_show(fds->rename_auto_box);

		hbox = gtk_hbox_new(FALSE, 5);
		gtk_box_pack_start(GTK_BOX(gd->vbox), hbox, FALSE, FALSE, 0);
		gtk_widget_show(hbox);

		fds->rename_box = gtk_check_button_new_with_label(_("Rename"));
		g_signal_connect(G_OBJECT(fds->rename_box), "clicked",
				 G_CALLBACK(file_util_move_single_rename_cb), gd);
		gtk_box_pack_start(GTK_BOX(hbox), fds->rename_box, FALSE, FALSE, 0);
		gtk_widget_show(fds->rename_box);

		fds->rename_entry = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(fds->rename_entry), filename_from_path(fds->dest));
		gtk_widget_set_sensitive(fds->rename_entry, FALSE);
		gtk_box_pack_start(GTK_BOX(hbox), fds->rename_entry, TRUE, TRUE, 0);
		gtk_widget_show(fds->rename_entry);

		gtk_widget_show(gd->dialog);
		return;
		}
	else
		{
		gint success = FALSE;
		if (fds->copy)
			{
			if(copy_file(fds->source, fds->dest));
				{
				success = TRUE;
				file_maint_copied(fds->source, fds->dest);
				}
			}
		else
			{
			if (move_file(fds->source, fds->dest))
				{
				success = TRUE;
				file_maint_moved(fds->source, fds->dest, NULL);
				}
			}
		if (!success)
			{
			gchar *title;
			gchar *text;
			if (fds->copy)
				{
				title = _("Error copying file");
				text = g_strdup_printf(_("Unable to copy file:\n%s\nto:\n%s"), fds->source, fds->dest);
				}
			else
				{
				title = _("Error moving file");
				text = g_strdup_printf(_("Unable to move file:\n%s\nto:\n%s"), fds->source, fds->dest);
				}
			warning_dialog(title, text);
			g_free(text);
			}
		}

	file_data_single_free(fds);
}

/*
 * file move dialog
 */

static void file_util_move_do(FileDialog *fd)
{
	file_dialog_sync_history(fd, TRUE);

	if (fd->multiple_files)
		{
		file_util_move_multiple(file_data_multiple_new(fd->source_list, fd->dest_path, fd->type));
		fd->source_list = NULL;
		}
	else
		{
		if (isdir(fd->dest_path))
			{
			gchar *buf = concat_dir_and_file(fd->dest_path, filename_from_path(fd->source_path));
			gtk_entry_set_text(GTK_ENTRY(fd->entry), buf);
			g_free(buf);
			}
		file_util_move_single(file_data_single_new(fd->source_path, fd->dest_path, fd->type));
		}

	file_dialog_close(fd);
}

static void file_util_move_check(FileDialog *fd)
{
	if (fd->multiple_files && !isdir(fd->dest_path))
		{
		if (isfile(fd->dest_path))
			{
			warning_dialog(_("Invalid destination"),
				       _("When operating with multiple files, please select\na folder, not a file."));
			}
		else
			warning_dialog(_("Invalid folder"),
				       _("Please select an existing folder"));
		return;
		}

	file_util_move_do(fd);
}

static void file_util_move_cb(FileDialog *fd, gpointer data)
{
	file_util_move_check(fd);
}

static void file_util_move_cancel_cb(FileDialog *fd, gpointer data)
{
	file_dialog_close(fd);
}

static void real_file_util_move(const gchar *source_path, GList *source_list, const gchar *dest_path, gint copy)
{
	FileDialog *fd;
	gchar *path = NULL;
	gint multiple;
	gchar *text;
	const gchar *title;
	const gchar *op_text;
	const gchar *stock_id;

	if (!source_path && !source_list) return;

	if (source_path)
		{
		path = g_strdup(source_path);
		multiple = FALSE;
		}
	else if (source_list->next)
		{
		multiple = TRUE;
		}
	else
		{
		path = g_strdup(source_list->data);
		path_list_free(source_list);
		source_list = NULL;
		multiple = FALSE;
		}

	if (copy)
		{
		title = _("Copy - GQmpeg");
		op_text = _("Copy");
		if (path)
			text = g_strdup_printf(_("Copy file:\n%s\nto:"), path);
		else
			text = g_strdup(_("Copy multiple files to:"));
		stock_id = GTK_STOCK_COPY;
		}
	else
		{
		title = _("Move - GQmpeg");
		op_text = _("Move");
		if (path)
			text = g_strdup_printf(_("Move file:\n%s\nto:"), path);
		else
			text = g_strdup(_("Move multiple files to:"));
		stock_id = GTK_STOCK_OK;
		}

	fd = file_dialog_new(title, text, "GQmpeg", "dlg_copymove", file_util_move_cancel_cb, NULL);
	g_free(text);

	file_dialog_add_button_stock(fd, op_text, stock_id, file_util_move_cb, TRUE);

	file_dialog_add_path_widgets(fd, NULL, dest_path, "move_copy", NULL, NULL);

	fd->type = copy;
	fd->source_path = path;
	fd->source_list = source_list;
	fd->multiple_files = multiple;

	gtk_widget_show(GENERIC_DIALOG(fd)->dialog);
}

void file_util_move(const gchar *source_path, GList *source_list, const gchar *dest_path)
{
	real_file_util_move(source_path, source_list, dest_path, FALSE);
}

void file_util_copy(const gchar *source_path, GList *source_list, const gchar *dest_path)
{
	real_file_util_move(source_path, source_list, dest_path, TRUE);
}

void file_util_move_simple(GList *list, const gchar *dest_path)
{
	if (!list) return;
	if (!dest_path)
		{
		path_list_free(list);
		return;
		}

	if (!list->next)
		{
		const gchar *source;
		gchar *dest;

		source = list->data;
		dest = concat_dir_and_file(dest_path, filename_from_path(source));

		file_util_move_single(file_data_single_new(source, dest, FALSE));
		g_free(dest);
		path_list_free(list);
		return;
		}

	file_util_move_multiple(file_data_multiple_new(list, dest_path, FALSE));
}

void file_util_copy_simple(GList *list, const gchar *dest_path)
{
	if (!list) return;
	if (!dest_path)
		{
		path_list_free(list);
		return;
		}

	if (!list->next)
		{
		const gchar *source;
		gchar *dest;

		source = list->data;
		dest = concat_dir_and_file(dest_path, filename_from_path(source));

		file_util_move_single(file_data_single_new(source, dest, TRUE));
		g_free(dest);
		path_list_free(list);
		return;
		}

	file_util_move_multiple(file_data_multiple_new(list, dest_path, TRUE));
}


/*
 *--------------------------------------------------------------------------
 * Delete routines
 *--------------------------------------------------------------------------
 */

/*
 * delete multiple files
 */

static void file_util_delete_multiple_ok_cb(GenericDialog *gd, gpointer data);
static void file_util_delete_multiple_cancel_cb(GenericDialog *gd, gpointer data);

static void file_util_delete_multiple_ok_cb(GenericDialog *gd, gpointer data)
{
	GList *source_list = data;

	while(source_list)
		{
		gchar *path = source_list->data;

		source_list = g_list_remove(source_list, path);

		if (!unlink_file(path))
			{
			if (source_list)
				{
				GenericDialog *d;
				gchar *text;

				text = g_strdup_printf(_("Unable to delete file:\n %s\n Continue multiple delete operation?"), path);
				d = generic_dialog_new(_("Delete failed"), text, "GQmpeg", "dlg_confirm", TRUE,
						       file_util_delete_multiple_cancel_cb, source_list);
				g_free(text);

				generic_dialog_add_stock(d, _("Continue"), GTK_STOCK_GO_FORWARD, file_util_delete_multiple_ok_cb, TRUE);
				gtk_widget_show(d->dialog);
				}
			else
				{
				gchar *text;
				
				text = g_strdup_printf(_("Unable to delete file:\n%s"), path);
				warning_dialog(_("Delete failed"), text);
				g_free(text);
				}
			g_free(path);
			return;
			}
		else
			{
			file_maint_removed(path, source_list);
			}
		g_free(path);
		}
}

static void file_util_delete_multiple_cancel_cb(GenericDialog *gd, gpointer data)
{
	GList *source_list = data;

	path_list_free(source_list);
}

static void file_util_delete_multiple_review_skip(GenericDialog *gd, gint next)
{
	GtkWidget *label_review;
	GtkWidget *button_back;
	GtkWidget *button_next;
	GtkWidget *button_label;
	GList *list;
	GList *list_point;
	const gchar *path;
	gchar *buf;

	list = gd->data;
	label_review = g_object_get_data(G_OBJECT(gd->dialog), "label_review");
	button_back = g_object_get_data(G_OBJECT(gd->dialog), "button_back");
	button_next = g_object_get_data(G_OBJECT(gd->dialog), "button_next");
	button_label = g_object_get_data(G_OBJECT(gd->dialog), "button_label");
	list_point = g_object_get_data(G_OBJECT(gd->dialog), "list_point");

	if (!list || !button_label) return;

	if (list_point)
		{
		if (next)
			{
			if (list_point->next) list_point = list_point->next;
			}
		else
			{
			if (list_point->prev) list_point = list_point->prev;
			}
		}
	else
		{
		list_point = list;
		}

	if (!list_point) return;

	path = list_point->data;

	gtk_label_set_text(GTK_LABEL(label_review), path);

	buf = g_strdup_printf(_("File %d of %d"),
			      g_list_index(list, (gpointer)path) + 1,
			      g_list_length(list));
	gtk_label_set_text(GTK_LABEL(button_label), buf);
	g_free(buf);

	gtk_widget_set_sensitive(button_back, (list_point->prev != NULL) );
	gtk_widget_set_sensitive(button_next, (list_point->next != NULL) );

	g_object_set_data(G_OBJECT(gd->dialog), "list_point", list_point);
}

static void file_util_delete_multiple_review_back(GtkWidget *button, gpointer data)
{
	GenericDialog *gd = data;

	file_util_delete_multiple_review_skip(gd, FALSE);
}

static void file_util_delete_multiple_review_next(GtkWidget *button, gpointer data)
{
	GenericDialog *gd = data;

	file_util_delete_multiple_review_skip(gd, TRUE);
}

static void file_util_delete_multiple(GList *source_list)
{
	GenericDialog *gd;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *arrow;
	GtkWidget *label;
	gchar *buf;

	gd = generic_dialog_new(_("Delete files - GQmpeg"), _("About to delete multiple files..."),
				"GQmpeg", "dlg_confirm", TRUE,
				file_util_delete_multiple_cancel_cb, source_list);

	label = gtk_label_new("");
	g_object_set_data(G_OBJECT(gd->dialog), "label_review", label);
	gtk_box_pack_start(GTK_BOX(gd->vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(gd->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = gtk_button_new();
	arrow = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(button), arrow);
	gtk_widget_show(arrow);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(file_util_delete_multiple_review_back), gd);
	gtk_widget_set_sensitive(button, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);
	g_object_set_data(G_OBJECT(gd->dialog), "button_back", button);

	button = gtk_button_new();
	arrow = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(button), arrow);
	gtk_widget_show(arrow);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(file_util_delete_multiple_review_next), gd);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);
	g_object_set_data(G_OBJECT(gd->dialog), "button_next", button);

	buf = g_strdup_printf(_("Review %d files"), g_list_length(source_list) );
	label = gtk_label_new(buf);
	g_free(buf);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	g_object_set_data(G_OBJECT(gd->dialog), "button_label", label);

	generic_dialog_add_stock(gd, _("Delete"), GTK_STOCK_DELETE, file_util_delete_multiple_ok_cb, TRUE);

	gtk_widget_show(gd->dialog);
}

/*
 * delete single file
 */

static void file_util_delete_ok_cb(GenericDialog *gd, gpointer data)
{
	gchar *path = data;

	if (!unlink_file(path))
		{
		gchar *text = g_strdup_printf(_("Unable to delete file:\n%s"), path);
		warning_dialog(_("File deletion failed"), text);
		g_free(text);
		}
	else
		{
		file_maint_removed(path, NULL);
		}

	g_free(path);
}

static void file_util_delete_cancel_cb(GenericDialog *gd, gpointer data)
{
	gchar *path = data;

	g_free(path);
}

static void file_util_delete_single(const gchar *path)
{
	gchar *buf = g_strdup(path);
	GenericDialog *gd;
	gchar *text;

	text = g_strdup_printf(_("About to delete the file:\n %s"), buf);
	gd = generic_dialog_new(_("Delete file - GQmpeg"), text, "GQmpeg", "dlg_confirm", TRUE,
				file_util_delete_cancel_cb, buf);
	g_free(text);

	generic_dialog_add_stock(gd, _("Delete"), GTK_STOCK_DELETE, file_util_delete_ok_cb, TRUE);

	gtk_widget_show(gd->dialog);
}

void file_util_delete(const gchar *source_path, GList *source_list)
{
	if (!source_path && !source_list) return;

	if (source_path)
		{
		file_util_delete_single(source_path);
		}
	else if (!source_list->next)
		{
		file_util_delete_single(source_list->data);
		path_list_free(source_list);
		}
	else
		{
		file_util_delete_multiple(source_list);
		}
}

/*
 *--------------------------------------------------------------------------
 * Rename routines
 *--------------------------------------------------------------------------
 */

/*
 * rename multiple files
 */

enum {
	RENAME_COLUMN_PATH = 0,
	RENAME_COLUMN_NAME,
	RENAME_COLUMN_PREVIEW,
	RENAME_COLUMN_COUNT
};

typedef struct _RenameDataMult RenameDataMult;
struct _RenameDataMult
{
	FileDialog *fd;

	gint rename_auto;

	GtkWidget *listview;
	GtkWidget *button_auto;

	GtkWidget *rename_box;
	GtkWidget *rename_label;
	GtkWidget *rename_entry;

	GtkWidget *auto_box;
	GtkWidget *auto_entry_front;
	GtkWidget *auto_spin_start;
	GtkWidget *auto_spin_pad;
	GtkWidget *auto_entry_end;

	gint update_idle_id;
};

static void file_util_rename_multiple(RenameDataMult *rd);

static void file_util_rename_multiple_ok_cb(GenericDialog *gd, gpointer data)
{
	RenameDataMult *rd = data;
	GtkWidget *dialog;

	dialog = GENERIC_DIALOG(rd->fd)->dialog;
	if (!GTK_WIDGET_VISIBLE(dialog)) gtk_widget_show(dialog);

	rd->fd->type = TRUE;
	file_util_rename_multiple(rd);
}

static void file_util_rename_multiple_cancel_cb(GenericDialog *gd, gpointer data)
{
	RenameDataMult *rd = data;
	GtkWidget *dialog;

	dialog = GENERIC_DIALOG(rd->fd)->dialog;
	if (!GTK_WIDGET_VISIBLE(dialog)) gtk_widget_show(dialog);
}

static gint file_util_rename_multiple_find_row(RenameDataMult *rd, const gchar *path, GtkTreeIter *iter)
{
	GtkTreeModel *store;
	gint valid;
	gint row = 0;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(rd->listview));
	valid = gtk_tree_model_get_iter_first(store, iter);
	while (valid)
		{
		gchar *path_n;
		gint ret;

		gtk_tree_model_get(GTK_TREE_MODEL(store), iter, RENAME_COLUMN_PATH, &path_n, -1);
		ret = (strcmp(path_n, path) == 0);
		g_free(path_n);
		if (ret) return row;

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), iter);
		row++;
		}

	return -1;
}

static void file_util_rename_multiple(RenameDataMult *rd)
{
	FileDialog *fd;

	fd = rd->fd;

	if (isfile(fd->dest_path) && !fd->type)
		{
		GenericDialog *gd;
		gchar *text;

		text = g_strdup_printf(_("Overwrite file:\n%s\nby renaming:\n%s"), fd->dest_path, fd->source_path);
		gd = generic_dialog_new(_("Overwrite file - GQmpeg"), text, "GQmpeg", "dlg_confirm", TRUE,
					file_util_rename_multiple_cancel_cb, rd);
		g_free(text);

		generic_dialog_add_stock(gd, _("Overwrite"), GTK_STOCK_OK, file_util_rename_multiple_ok_cb, TRUE);

		gtk_widget_hide(GENERIC_DIALOG(fd)->dialog);

		gtk_widget_show(gd->dialog);
		return;
		}
	else
		{
		if (!rename_file(fd->source_path, fd->dest_path))
			{
			gchar *text = g_strdup_printf(_("Unable to rename file:\n%s\n to:\n%s"),
						      filename_from_path(fd->source_path),
						      filename_from_path(fd->dest_path));
			warning_dialog(_("Error renaming file"), text);
			g_free(text);
			}
		else
			{
			GtkTreeModel *store;
			GtkTreeIter iter;
			GtkTreeIter next;
			gint row;

			file_maint_renamed(fd->source_path, fd->dest_path);

			store = gtk_tree_view_get_model(GTK_TREE_VIEW(rd->listview));
			row = file_util_rename_multiple_find_row(rd, rd->fd->source_path, &iter);

			if (row >= 0 &&
			    (gtk_tree_model_iter_nth_child(store, &next, NULL, row + 1) ||
			    (row > 0 && gtk_tree_model_iter_nth_child(store, &next, NULL, row - 1)) ) )
				{
				GtkTreeSelection *selection;

				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(rd->listview));
				gtk_tree_selection_select_iter(selection, &next);
				gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
				}
			else
				{
				file_dialog_close(rd->fd);
				}
			}
		}
}

static void file_util_rename_multiple_auto(RenameDataMult *rd)
{
	const gchar *front;
	const gchar *end;
	gint start_n;
	gint padding;
	gint n;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint valid;
	gint success;

	history_combo_append_history(rd->auto_entry_front, NULL);
	history_combo_append_history(rd->auto_entry_end, NULL);

	front = gtk_entry_get_text(GTK_ENTRY(rd->auto_entry_front));
	end = gtk_entry_get_text(GTK_ENTRY(rd->auto_entry_end));
	start_n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(rd->auto_spin_start));
	padding = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(rd->auto_spin_pad));

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(rd->listview));

	/* first check for name conflicts */
	success = TRUE;
	n = start_n;
	valid = gtk_tree_model_get_iter_first(store, &iter);
	while (valid && success)
		{
		gchar *dest;
		gchar *base;
		gchar *path;

		gtk_tree_model_get(store, &iter, RENAME_COLUMN_PATH, &path, -1);

		base = remove_level_from_path(path);
		dest = g_strdup_printf("%s/%s%0*d%s", base, front, padding, n, end);
		if (isname(dest)) success = FALSE;
		g_free(dest);
		g_free(base);
		g_free(path);

		n++;
		valid = gtk_tree_model_iter_next(store, &iter);
		}

	if (!success)
		{
		warning_dialog(_("Auto rename"), _("Can not auto rename with the selected\nnumber set, one or more files exist that\nmatch the resulting name list.\n"));
		return;
		}

	/* select the first iter, so that on fail the correct info is given to user */
	if (gtk_tree_model_get_iter_first(store, &iter))
		{
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(rd->listview));
		gtk_tree_selection_select_iter(selection, &iter);
		}

	/* now do it for real */
	success = TRUE;
	n = start_n;
	while (success && gtk_tree_model_get_iter_first(store, &iter))
		{
		gchar *dest;
		gchar *base;
		gchar *path;

		gtk_tree_model_get(store, &iter, RENAME_COLUMN_PATH, &path, -1);

		base = remove_level_from_path(path);
		dest = g_strdup_printf("%s/%s%0*d%s", base, front, padding, n, end);
		if (!rename_file(path, dest))
			{
			success = FALSE;
			}
		else
			{
			file_maint_renamed(path, dest);
			}

		g_free(dest);
		g_free(base);
		g_free(path);

		if (success)
			{
			gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
			if (gtk_tree_model_get_iter_first(store, &iter))
				{
				GtkTreeSelection *selection;

				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(rd->listview));
				gtk_tree_selection_select_iter(selection, &iter);
				}
			}

		n++;
		}

	if (!success)
		{
		gchar *buf;

		n--;
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(rd->auto_spin_start), (float)n);

		buf = g_strdup_printf(_("Failed to rename\n%s\nThe number was %d."), filename_from_path(rd->fd->source_path), n);
		warning_dialog(_("Auto rename"), buf);
		g_free(buf);

		return;
		}

	file_dialog_close(rd->fd);
}

static void file_util_rename_multiple_cb(FileDialog *fd, gpointer data)
{
	RenameDataMult *rd = data;
	gchar *base;
	const gchar *name;

	if (rd->rename_auto)
		{
		file_util_rename_multiple_auto(rd);
		return;
		}

	name = gtk_entry_get_text(GTK_ENTRY(rd->rename_entry));
	base = remove_level_from_path(fd->source_path);

	g_free(fd->dest_path);
	fd->dest_path = concat_dir_and_file(base, name);
	g_free(base);

	if (strlen(name) == 0 || strcmp(fd->source_path, fd->dest_path) == 0)
		{
		return;
		}

	fd->type = FALSE;
	file_util_rename_multiple(rd);
}

static void file_util_rename_multiple_close_cb(FileDialog *fd, gpointer data)
{
	RenameDataMult *rd = data;

	file_dialog_close(rd->fd);
}

static gboolean file_util_rename_multiple_select_cb(GtkTreeSelection *selection, GtkTreeModel *store, GtkTreePath *tpath,
						    gboolean path_currently_selected, gpointer data)
{
	RenameDataMult *rd = data;
	GtkTreeIter iter;
	const gchar *name;
	gchar *path = NULL;

	if (path_currently_selected ||
	    !gtk_tree_model_get_iter(store, &iter, tpath)) return TRUE;
	gtk_tree_model_get(store, &iter, RENAME_COLUMN_PATH, &path, -1);

	g_free(rd->fd->source_path);
	rd->fd->source_path = path;

	name = filename_from_path(rd->fd->source_path);
	gtk_label_set_text(GTK_LABEL(rd->rename_label), name);
	gtk_entry_set_text(GTK_ENTRY(rd->rename_entry), name);

	if (GTK_WIDGET_VISIBLE(rd->rename_box))
		{
		gtk_widget_grab_focus(rd->rename_entry);
		}

	return TRUE;
}

static void file_util_rename_multiple_preview_update(RenameDataMult *rd)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	const gchar *front;
	const gchar *end;
	gint valid;
	gint start_n;
	gint padding;
	gint n;

	front = gtk_entry_get_text(GTK_ENTRY(rd->auto_entry_front));
	end = gtk_entry_get_text(GTK_ENTRY(rd->auto_entry_end));
	start_n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(rd->auto_spin_start));
	padding = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(rd->auto_spin_pad));

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(rd->listview));
	n = start_n;
	valid = gtk_tree_model_get_iter_first(store, &iter);
	while (valid)
		{
		gchar *dest;

		dest = g_strdup_printf("%s%0*d%s", front, padding, n, end);
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, RENAME_COLUMN_PREVIEW, dest, -1);
		g_free(dest);

		n++;
		valid = gtk_tree_model_iter_next(store, &iter);
		}

}

static gboolean file_util_rename_multiple_idle_cb(gpointer data)
{
	RenameDataMult *rd = data;

	file_util_rename_multiple_preview_update(rd);

	rd->update_idle_id = -1;
	return FALSE;
}

static void file_util_rename_multiple_preview_order_cb(GtkTreeModel *treemodel, GtkTreePath *tpath,
						       GtkTreeIter *iter, gpointer data)
{
	RenameDataMult *rd = data;

	if (rd->rename_auto && rd->update_idle_id == -1)
		{
		rd->update_idle_id = gtk_idle_add(file_util_rename_multiple_idle_cb, rd);
		}
}

static void file_util_rename_multiple_preview_entry_cb(GtkWidget *entry, gpointer data)
{
	RenameDataMult *rd = data;
	file_util_rename_multiple_preview_update(rd);
}

static void file_util_rename_multiple_preview_adj_cb(GtkAdjustment *adj, gpointer data)
{
	RenameDataMult *rd = data;
	file_util_rename_multiple_preview_update(rd);
}

static void file_util_rename_mulitple_auto_toggle(GtkWidget *widget, gpointer data)
{
	RenameDataMult *rd = data;
	GtkTreeViewColumn *column;

	rd->rename_auto = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rd->button_auto));

	if (rd->rename_auto)
		{
		if (GTK_WIDGET_VISIBLE(rd->rename_box)) gtk_widget_hide(rd->rename_box);
		if (!GTK_WIDGET_VISIBLE(rd->auto_box)) gtk_widget_show(rd->auto_box);
		file_util_rename_multiple_preview_update(rd);
		}
	else
		{
		if (GTK_WIDGET_VISIBLE(rd->auto_box)) gtk_widget_hide(rd->auto_box);
		if (!GTK_WIDGET_VISIBLE(rd->rename_box)) gtk_widget_show(rd->rename_box);
		}

	column = gtk_tree_view_get_column(GTK_TREE_VIEW(rd->listview), RENAME_COLUMN_PREVIEW - 1);
	gtk_tree_view_column_set_visible(column, rd->rename_auto);
}

static GtkWidget *furm_simple_label(GtkWidget *box, const gchar *text)
{
	GtkWidget *hbox;
	GtkWidget *label;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	return label;
}

static GtkWidget *furm_simple_vlabel(GtkWidget *box, const gchar *text, gint expand)
{
	GtkWidget *vbox;
	GtkWidget *label;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), vbox, expand, expand, 0);
	gtk_widget_show(vbox);

	label = gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	return vbox;
}

static GtkTreeViewColumn *file_util_rename_multiple_add_column(RenameDataMult *rd, const gchar *text, gint n)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	column = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(column, text);
        gtk_tree_view_column_set_min_width(column, 4);
        gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(column, renderer, TRUE);
        gtk_tree_view_column_add_attribute(column, renderer, "text", n);
        gtk_tree_view_append_column(GTK_TREE_VIEW(rd->listview), column);

	return column;
}

static void file_util_rename_multiple_destroy_cb(GtkWidget *widget, gpointer data)
{
	RenameDataMult *rd = data;

	if (rd->update_idle_id != -1) gtk_idle_remove(rd->update_idle_id);

	g_free(rd);
}

static void file_util_rename_multiple_do(GList *source_list)
{
	RenameDataMult *rd;
	GtkWidget *pane;
	GtkWidget *scrolled;
	GtkListStore *store;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *box2;
	GtkWidget *label;
	GtkObject *adj;
	GtkWidget *combo;
	GList *work;

	rd = g_new0(RenameDataMult, 1);

	rd->fd = file_dialog_new(_("Rename - GQmpeg"), _("Rename multiple files:"), "GQmpeg", "dlg_rename",
			         file_util_rename_multiple_close_cb, rd);
	file_dialog_add_button_stock(rd->fd, _("Rename"), GTK_STOCK_OK, file_util_rename_multiple_cb, TRUE);

	rd->fd->source_path = g_strdup(source_list->data);
	rd->fd->dest_path = NULL;

	rd->update_idle_id = -1;

	vbox = GENERIC_DIALOG(rd->fd)->vbox;

	pane = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), pane, TRUE, TRUE, 0);
	gtk_widget_show(pane);
	
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_paned_pack1(GTK_PANED(pane), scrolled, TRUE, TRUE);
	gtk_widget_show(scrolled);

	store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	rd->listview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(rd->listview));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_SINGLE);
	gtk_tree_selection_set_select_function(selection, file_util_rename_multiple_select_cb, rd, NULL);

	file_util_rename_multiple_add_column(rd, _("Original Name"), RENAME_COLUMN_NAME);
	column = file_util_rename_multiple_add_column(rd, _("Preview"), RENAME_COLUMN_PREVIEW);
	gtk_tree_view_column_set_visible(column, FALSE);
	
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(rd->listview), TRUE);
	g_signal_connect(G_OBJECT(store), "row_changed",
			 G_CALLBACK(file_util_rename_multiple_preview_order_cb), rd);
	gtk_widget_set_size_request(rd->listview, 250, 150);

	gtk_container_add(GTK_CONTAINER(scrolled), rd->listview);
	gtk_widget_show(rd->listview);

	work = source_list;
	while (work)
		{
		gchar *path = work->data;
		GtkTreeIter iter;

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, RENAME_COLUMN_PATH, path, RENAME_COLUMN_NAME, filename_from_path(path), -1);

		work = work->next;
		}

	path_list_free(source_list);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	rd->button_auto = gtk_check_button_new_with_label(_("Auto rename"));
	g_signal_connect(G_OBJECT(rd->button_auto), "clicked",
			 G_CALLBACK(file_util_rename_mulitple_auto_toggle), rd);
	gtk_box_pack_end(GTK_BOX(hbox), rd->button_auto, FALSE, FALSE, 0);
	gtk_widget_show(rd->button_auto);

	rd->rename_box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), rd->rename_box, FALSE, FALSE, 0);
	gtk_widget_show(rd->rename_box);

	furm_simple_label(rd->rename_box, _("Rename:"));
	rd->rename_label = furm_simple_label(rd->rename_box, filename_from_path(rd->fd->source_path));
	furm_simple_label(rd->rename_box, _("to:"));

	rd->rename_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(rd->rename_entry), filename_from_path(rd->fd->source_path));
	gtk_box_pack_start(GTK_BOX(rd->rename_box), rd->rename_entry, FALSE, FALSE, 0);
	generic_dialog_attach_default(GENERIC_DIALOG(rd->fd), rd->rename_entry);
	gtk_widget_grab_focus(rd->rename_entry);
	gtk_widget_show(rd->rename_entry);

	rd->auto_box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), rd->auto_box, FALSE, FALSE, 0);
	/* do not show it here */

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(rd->auto_box), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	box2 = furm_simple_vlabel(hbox, _("Begin text"), TRUE);

	combo = history_combo_new(&rd->auto_entry_front, "", "numerical_rename_prefix", -1);
	g_signal_connect(G_OBJECT(rd->auto_entry_front), "changed",
			 G_CALLBACK(file_util_rename_multiple_preview_entry_cb), rd);
	gtk_box_pack_start(GTK_BOX(box2), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);
	
	box2 = furm_simple_vlabel(hbox, _("Start #"), FALSE);

	adj = gtk_adjustment_new(0.0, 0.0, 1000000.0, 1.0, 1.0, 10.0);
	rd->auto_spin_start = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(file_util_rename_multiple_preview_adj_cb), rd);
	gtk_box_pack_start(GTK_BOX(box2), rd->auto_spin_start, FALSE, FALSE, 0);
	gtk_widget_show(rd->auto_spin_start);

	box2 = furm_simple_vlabel(hbox, _("End text"), TRUE);

	combo = history_combo_new(&rd->auto_entry_end, "", "numerical_rename_suffix", -1);
	g_signal_connect(G_OBJECT(rd->auto_entry_end), "changed",
			 G_CALLBACK(file_util_rename_multiple_preview_entry_cb), rd);
	gtk_box_pack_start(GTK_BOX(box2), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(rd->auto_box), hbox, FALSE, FALSE, 5);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Padding:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	adj = gtk_adjustment_new(1.0, 1.0, 8.0, 1.0, 1.0, 1.0);
	rd->auto_spin_pad = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(file_util_rename_multiple_preview_adj_cb), rd);
	gtk_box_pack_start(GTK_BOX(hbox), rd->auto_spin_pad, FALSE, FALSE, 0);
	gtk_widget_show(rd->auto_spin_pad);

	g_signal_connect(G_OBJECT(GENERIC_DIALOG(rd->fd)->dialog), "destroy",
			 G_CALLBACK(file_util_rename_multiple_destroy_cb), rd);

	gtk_widget_show(GENERIC_DIALOG(rd->fd)->dialog);
}

/*
 * rename single file
 */

static void file_util_rename_single(FileDataSingle *fds);

static void file_util_rename_single_ok_cb(GenericDialog *gd, gpointer data)
{
	FileDataSingle *fds = data;
	fds->confirmed = TRUE;
	file_util_rename_single(fds);
}

static void file_util_rename_single_cancel_cb(GenericDialog *gd, gpointer data)
{
	FileDataSingle *fds = data;
	file_data_single_free(fds);
}

static void file_util_rename_single(FileDataSingle *fds)
{
	if (isfile(fds->dest) && !fds->confirmed)
		{
		GenericDialog *gd;
		gchar *text;

		text = g_strdup_printf(_("Overwrite file:\n%s\nby renaming:\n%s"), fds->dest, fds->source);
		gd = generic_dialog_new(_("Overwrite file - GQmpeg"), text, "GQmpeg", "dlg_confirm", TRUE,
					file_util_rename_single_cancel_cb, fds);
		g_free(text);

		generic_dialog_add_stock(gd, _("Overwrite"), GTK_STOCK_OK, file_util_rename_single_ok_cb, TRUE);

		gtk_widget_show(gd->dialog);

		return;
		}
	else
		{
		if (!rename_file(fds->source, fds->dest))
			{
			gchar *text = g_strdup_printf(_("Unable to rename file:\n%s\nto:\n%s"), filename_from_path(fds->source), filename_from_path(fds->dest));
			warning_dialog(_("Error renaming file"), text);
			g_free(text);
			}
		else
			{
			file_maint_renamed(fds->source, fds->dest);
			}
		}
	file_data_single_free(fds);
}

static void file_util_rename_single_cb(FileDialog *fd, gpointer data)
{
	const gchar *name;
	gchar *path;

	name = gtk_entry_get_text(GTK_ENTRY(fd->entry));
	path = concat_dir_and_file(fd->dest_path, name);

	if (strlen(name) == 0 || strcmp(fd->source_path, path) == 0)
		{
		g_free(path);
		return;
		}

	file_util_rename_single(file_data_single_new(fd->source_path, path, fd->type));

	g_free(path);
	file_dialog_close(fd);
}

static void file_util_rename_single_close_cb(FileDialog *fd, gpointer data)
{
	file_dialog_close(fd);
}

static void file_util_rename_single_do(const gchar *source_path)
{
	FileDialog *fd;

	fd = file_dialog_new(_("Rename - GQmpeg"), _("Rename file:"), "GQmpeg", "dlg_rename",
			     file_util_rename_single_close_cb, NULL);

	file_dialog_add_button_stock(fd, _("Rename"), GTK_STOCK_OK, file_util_rename_single_cb, TRUE);

	fd->source_path = g_strdup(source_path);
	fd->dest_path = remove_level_from_path(source_path);

	fd->entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(GENERIC_DIALOG(fd)->vbox), fd->entry, FALSE, FALSE, 0);
	gtk_entry_set_text(GTK_ENTRY(fd->entry), filename_from_path(fd->source_path));
	gtk_editable_select_region(GTK_EDITABLE(fd->entry), 0, strlen(gtk_entry_get_text(GTK_ENTRY(fd->entry))));
	generic_dialog_attach_default(GENERIC_DIALOG(fd), fd->entry);
	gtk_widget_grab_focus(fd->entry);
	gtk_widget_show(fd->entry);

	gtk_widget_show(GENERIC_DIALOG(fd)->dialog);
}

void file_util_rename(const gchar *source_path, GList *source_list)
{
	if (!source_path && !source_list) return;

	if (source_path)
		{
		file_util_rename_single_do(source_path);
		}
	else if (!source_list->next)
		{
		file_util_rename_single_do(source_list->data);
		path_list_free(source_list);
		}
	else
		{
		file_util_rename_multiple_do(source_list);
		}
}

/*
 *--------------------------------------------------------------------------
 * Create directory routines
 *--------------------------------------------------------------------------
 */

static void file_util_create_dir_do(const gchar *base, const gchar *name)
{
	gchar *path;

	path = concat_dir_and_file(base, name);

	if (isdir(path))
		{
		gchar *text = g_strdup_printf(_("The folder:\n%s\nalready exists."), name);
		warning_dialog(_("Folder exists"), text);
		g_free(text);
		}
	else if (isname(path))
		{
		gchar *text = g_strdup_printf(_("The path:\n%s\nalready exists as a file."), name);
		warning_dialog(_("Could not create folder"), text);
		g_free(text);
		}
	else
		{
		if (!mkdir_utf8(path, 0755))
			{
			gchar *text = g_strdup_printf(_("Unable to create folder:\n%s"), name);
			warning_dialog(_("Error creating folder"), text);
			g_free(text);
			}
		else
			{
#if 0
			if (strcmp(base, current_path) == 0)
				{
				gchar *buf = g_strdup(current_path);
				filelist_change_to(buf);
				g_free(buf);
				}
#endif
			}
		}

	g_free(path);
}

static void file_util_create_dir_cb(FileDialog *fd, gpointer data)
{
	const gchar *name;

	name = gtk_entry_get_text(GTK_ENTRY(fd->entry));

	if (strlen(name) == 0) return;

	if (name[0] == '/')
		{
		gchar *buf;
		buf  = remove_level_from_path(name);
		file_util_create_dir_do(buf, filename_from_path(name));
		g_free(buf);
		}
	else
		{
		file_util_create_dir_do(fd->dest_path, name);
		}

	file_dialog_close(fd);
}

static void file_util_create_dir_close_cb(FileDialog *fd, gpointer data)
{
	file_dialog_close(fd);
}

void file_util_create_dir(const gchar *path)
{
	FileDialog *fd;
	gchar *text;

	if (!isdir(path)) return;

	text = g_strdup_printf(_("Create folder in:\n%s\nnamed:"), path);
	fd = file_dialog_new(_("New folder - GQmpeg"), text, "GQmpeg", "dlg_newdir",
			     file_util_create_dir_close_cb, NULL);
	g_free(text);

	file_dialog_add_button_stock(fd, _("Create"), GTK_STOCK_OK, file_util_create_dir_cb, TRUE);

	fd->dest_path = g_strdup(path);

	fd->entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(GENERIC_DIALOG(fd)->vbox), fd->entry, FALSE, FALSE, 0);
	generic_dialog_attach_default(GENERIC_DIALOG(fd), fd->entry);
	gtk_widget_grab_focus(fd->entry);
	gtk_widget_show(fd->entry);

	gtk_widget_show(GENERIC_DIALOG(fd)->dialog);
}

