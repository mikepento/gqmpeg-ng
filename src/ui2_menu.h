/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#ifndef UI2_MENU_H
#define UI2_MENU_H


typedef struct _UIMenuData UIMenuData;
struct _UIMenuData
{
	GList *list;
	GList *submenu_list;	/* list of submenu uis */

	gint press_button;
	gint selection_done;

	UIData *ui;		/* top level menu */
	gpointer preview;
};


void ui_menu_item_add(UIMenuData *md, const gchar *text, const gchar *path, gint id,
		      void (*select_func)(UIMenuData *md, const gchar *path, gint id, gpointer select_data),
		      gpointer select_data);
void ui_menu_item_add_check(UIMenuData *md, const gchar *text, const gchar *path, gint id, gint active,
			    void (*select_func)(UIMenuData *md, const gchar *path, gint id, gpointer select_data),
			    gpointer select_data);
void ui_menu_item_add_sensitive(UIMenuData *md, const gchar *text, const gchar *path, gint id, gint sensitive,
				void (*select_func)(UIMenuData *md, const gchar *path, gint id, gpointer select_data),
				gpointer select_data);

UIMenuData *ui_menu_new(const gchar *class, const gchar *key);
void ui_menu_show(UIMenuData *md, gint x, gint y, gint button, guint32 event_time);


#endif


