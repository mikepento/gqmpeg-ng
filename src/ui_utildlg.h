/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#ifndef UI_UTILDLG_H
#define UI_UTILDLG_H


#define GENERIC_DIALOG(gd) ((GenericDialog *)gd)

typedef struct _GenericDialog GenericDialog;
struct _GenericDialog
{
	GtkWidget *dialog;	/* window */
	GtkWidget *vbox;	/* place to add widgets */

	GtkWidget *hbox;	/* button hbox */

	gint auto_close;

	void (*default_cb)(GenericDialog *, gpointer);
	void (*cancel_cb)(GenericDialog *, gpointer);
	gpointer data;

	/* private */
	GtkWidget *cancel_button;
};

typedef struct _FileDialog FileDialog;
struct _FileDialog
{
	GenericDialog gd;

	GtkWidget *entry;

	gint type;
	gint multiple_files;

	gchar *source_path;
	GList *source_list;

	gchar *dest_path;
};


GenericDialog *generic_dialog_new(const gchar *title, const gchar *message,
				  const gchar *wmclass, const gchar *wmsubclass, gint auto_close,
				  void (*cancel_cb)(GenericDialog *, gpointer), gpointer data);
void generic_dialog_close(GenericDialog *gd);
GtkWidget *generic_dialog_add(GenericDialog *gd, const gchar *text,
			      void (*func_cb)(GenericDialog *, gpointer), gint is_default);
GtkWidget *generic_dialog_add_stock(GenericDialog *gd, const gchar *text, const gchar *stock_id,
				    void (*func_cb)(GenericDialog *, gpointer), gint is_default);
void generic_dialog_attach_default(GenericDialog *gd, GtkWidget *widget);

GenericDialog *warning_dialog(const gchar *title, const gchar *message);

FileDialog *file_dialog_new(const gchar *title, const gchar *message,
			    const gchar *wmclass, const gchar *wmsubclass,
			    void (*cancel_cb)(FileDialog *, gpointer), gpointer data);
void file_dialog_close(FileDialog *fd);

GtkWidget *file_dialog_add_button(FileDialog *fd, const gchar *text,
				  void (*func_cb)(FileDialog *, gpointer), gint is_default);
GtkWidget *file_dialog_add_button_stock(FileDialog *fd, const gchar *text, const gchar *stock_id,
					void (*func_cb)(FileDialog *, gpointer), gint is_default);

/* default_path is default base directory, and is only used if no history
 *  exists for history_key (HOME is used if default_path is NULL).
 * path can be a full path or only a file name. If name only, appended to
 *  the default_path or the last history (see default_path)
 */
void file_dialog_add_path_widgets(FileDialog *fd, const gchar *default_path, const gchar *path,
				  const gchar *history_key, const gchar *filter, const gchar *filter_desc);

void file_dialog_add_filter(FileDialog *fd, const gchar *filter, const gchar *filter_desc, gint set);
void file_dialog_clear_filter(FileDialog *fd);
void file_dialog_sync_history(FileDialog *fd, gint dir_only);


#endif

