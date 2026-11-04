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

#include "ui2_includes.h"
#include "ui2_typedefs.h"
#include "ui2_menu.h"

#include "ui2_display.h"
#include "ui2_list.h"
#include "ui2_main.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"


#include <gdk/gdkkeysyms.h> /* for key values */


typedef struct _MenuItemData MenuItemData;
struct _MenuItemData
{
	gchar *text;

	gchar *path;		/* path to this menu (/item, /submenu/item, etc.) */
	gint id;		/* numerical id assigned when menu was added */

	gint is_toggle;
	gint toggle_active;

	gint sensitive;

	MenuItemData *parent;	/* parent of this menu, top is NULL */
	GList *children;	/* if non NULL, this is a submenu */

	void (*select_func)(UIMenuData *md, const gchar *path, gint id, gpointer select_data);
	gpointer select_data;

	UIData *ui;		/* when submenu showing, this points to the window */
	gint x;
	gint y;
	GdkPixbuf *backing;
	gint go_left;

	UIMenuData *md;
};


static void ui_menu_submenu_activate(UIMenuData *md, MenuItemData *mi,
				     gint x, gint y, gint button, guint32 event_time);

static UIData *ui_menu_submenu_new(MenuItemData *mi, const gchar *class, const gchar *key);

static void ui_menu_submenu_show(UIMenuData *md, MenuItemData *mi, gint x, gint y);
static void ui_menu_submenu_ungrab(MenuItemData *mi, guint32 event_time);
static void ui_menu_submenu_grab(MenuItemData *mi, guint32 event_time);
static void ui_menu_submenu_grab_focus(UIMenuData *md, MenuItemData *mi, gint button, guint32 event_time);


/*
 *---------------------------------------------------------
 * misc
 *---------------------------------------------------------
 */

static gint ui_menu_is_button_press(void)
{
	GdkEvent *event;
	gint ret = 0;

	event = gtk_get_current_event();

	if (event && event->type == GDK_BUTTON_PRESS)
		{
		ret = event->button.button;
		}
	if (event) gdk_event_free(event);

	return ret;
}

static guint32 ui_menu_event_time(void)
{
	GdkEvent *event;
	guint32 ret = 0;

	event = gtk_get_current_event();
	if (!event) return 0;

	if (event->type == GDK_BUTTON_PRESS)
		{
		ret = event->button.time;
		}
	else if (event->type == GDK_KEY_PRESS)
		{
		ret = event->key.time;
		}

	gdk_event_free(event);
	return ret;
}

static void ui_menu_hide_all(UIMenuData *md)
{
	GList *work;

	work = g_list_last(md->submenu_list);
	while (work)
		{
		MenuItemData *mi = work->data;
		work = work->prev;

		if (mi->ui && GTK_WIDGET_VISIBLE(mi->ui->window))
			{
			gtk_widget_hide(mi->ui->window);
			}
		}
}

/*
 *---------------------------------------------------------
 * menu items, etc.
 *---------------------------------------------------------
 */

static void ui_menu_item_free(MenuItemData *mi)
{
	if (!mi) return;

	if (mi->children)
		{
		GList *work;

		work = mi->children;
		while (work)
			{
			MenuItemData *tmp = work->data;
			work = work->next;

			ui_menu_item_free(tmp);
			}
		g_list_free(mi->children);
		}

	if (mi->ui) gtk_widget_destroy(mi->ui->window);
	if (mi->backing) gdk_pixbuf_unref(mi->backing);

	g_free(mi->text);
	g_free(mi->path);
	g_free(mi);
}

static gint simple_base_length(const gchar *path)
{
	gint p;

	if (!path) return 0;

	p = strlen(path) - 1;
	if (p < 0) return 0;

	while (path[p] != '/' && p > 0) p--;
	if (p == 0 && path[p] == '/') p++;
	return p;
}

static MenuItemData *ui_menu_find_parent(GList *list, const gchar *path)
{
	GList *work;

	if (!list || !path || path[0] != '/') return NULL;

	work = list;
	while (work)
		{
		MenuItemData *mi;
		MenuItemData *ret;
		gint l;

		mi = work->data;
		work = work->next;

		l = simple_base_length(path);
		if (l == strlen(mi->path) && strncmp(mi->path, path, l) == 0)
			{
			return mi;
			}
		ret = ui_menu_find_parent(mi->children, path);
		if (ret) return ret;
		}

	return NULL;
}

static MenuItemData *ui_menu_item_add_real(UIMenuData *md, const gchar *text, const gchar *path, gint id,
					   void (*select_func)(UIMenuData *md, const gchar *path, gint id, gpointer select_data),
					   gpointer select_data)
{
	MenuItemData *mi;
	MenuItemData *parent;

	if (!md || !text) return NULL;

	mi = g_new0(MenuItemData, 1);

	mi->text = g_strdup(text);
	mi->path = g_strdup((path) ? path : "/");
	mi->id = id;

	mi->select_func = select_func;
	mi->select_data = select_data;

	mi->ui = NULL;
	mi->backing = NULL;
	mi->go_left = FALSE;

	mi->sensitive = TRUE;

	parent = ui_menu_find_parent(md->list, path);
	mi->parent = parent;
	mi->md = md;

	if (parent)
		{
		if (debug_mode) printf("menu %s is submenu of %s\n", mi->text, parent->text);
		parent->children = g_list_append(parent->children, mi);
		}
	else
		{
		md->list = g_list_append(md->list, mi);
		}

	return mi;
}

void ui_menu_item_add(UIMenuData *md, const gchar *text, const gchar *path, gint id,
		      void (*select_func)(UIMenuData *md, const gchar *path, gint id, gpointer select_data),
		      gpointer select_data)
{
	ui_menu_item_add_real(md, text, path, id, select_func, select_data);
}

void ui_menu_item_add_check(UIMenuData *md, const gchar *text, const gchar *path, gint id, gint active,
			    void (*select_func)(UIMenuData *md, const gchar *path, gint id, gpointer select_data),
			    gpointer select_data)
{
	MenuItemData *mi;

	mi = ui_menu_item_add_real(md, text, path, id, select_func, select_data);
	if (!mi) return;
	mi->is_toggle = TRUE;
	mi->toggle_active = active;
}

void ui_menu_item_add_sensitive(UIMenuData *md, const gchar *text, const gchar *path, gint id, gint sensitive,
				void (*select_func)(UIMenuData *md, const gchar *path, gint id, gpointer select_data),
				gpointer select_data)
{
	MenuItemData *mi;

	mi = ui_menu_item_add_real(md, text, path, id, select_func, select_data);
	if (mi) mi->sensitive = sensitive;
}

/*
 *---------------------------------------------------------
 * menu computation
 *---------------------------------------------------------
 */

static void ui_menu_calc_size(MenuItemData *mi)
{
	UIData *ui;
	SkinData *skin;
	WidgetData *wd;
	gint w, h;
	gint ww, wh;
	gint px, py;

	ui = mi->ui;
	skin = ui->skin;

	w = skin->width_def;
	h = skin->height_def;

	wd = skin_widget_get_by_key(skin, "menu", list_type_id());
	if (wd)
		{
		ListData *ld;
		GList *work;
		gint rows;
		gint string_width;

		ld = wd->widget;

		rows = MAX(0, g_list_length(mi->children) - 1);
		string_width = 32;

		work = mi->children;
		while (work)
			{
			MenuItemData *mi;
			gint cw;

			mi = work->data;
			work = work->next;

			cw = font_string_length(ld->font, mi->text);
			if (cw > string_width) string_width = cw;
			}

		h += ld->row_height * rows;
		w += string_width;

		mi->y -= ld->y + ld->border_top;
		}
	else
		{
		printf("warning: ui menu \"%s\" contains no list widget\n", ui->key);
		}

	ww = gdk_screen_width();
	wh = gdk_screen_height();

	if (mi->parent)
		{
		MenuItemData *parent;
		ListData *ld;

		parent = mi->parent;

		wd = skin_widget_get_by_key(parent->ui->skin, "menu", list_type_id());
		if (wd)
			{
			ld = wd->widget;
			}
		else
			{
			ld = NULL;
			}

		if (parent->go_left ||
		    mi->x >= ww - w ||
		    (ld && mi->x < parent->x + ld->x + (ld->width - ld->border_right)))
			{
			mi->go_left = TRUE;
			}

		if (mi->go_left)
			{
			if (ld)
				{
				px = CLAMP(parent->x - w + ld->x + ld->border_left, 0, ww - w);
				}
			else
				{
				px = CLAMP(parent->x - w, 0, ww - w);
				}
			}
		else
			{
			px = CLAMP(mi->x, 0, ww - w);
			}
		}
	else
		{
		px = CLAMP(mi->x, 0, ww - w);
		}

	py = CLAMP(mi->y, 0, wh - h);

	if (px < mi->x) mi->go_left = TRUE;

	mi->x = px;
	mi->y = py;

	skin_resize(ui, w, h);
}

/*
 *---------------------------------------------------------
 * mouse / keyboard callbacks
 *---------------------------------------------------------
 */

static gint ui_menu_proximity(MenuItemData *mi, gint x, gint y, MenuItemData *second)
{
	if (second)
		{
		if (ui_menu_proximity(mi, x, y, NULL)) return FALSE;
		return ui_menu_proximity(second, mi->x - second->x + x, mi->y - second->y + y, NULL);
		}

	return (x >= 0 && x <= mi->ui->skin->width &&
		y >= 0 && y <= mi->ui->skin->height);
}

#if 0
static gint ui_menu_press_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	MenuItemData *mi = data;

	return FALSE;
}
#endif

static gint ui_menu_release_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	MenuItemData *mi = data;
	GList *work;

	if (!ui_menu_proximity(mi, event->x, event->y, NULL))
		{
		if (mi->parent && ui_menu_proximity(mi, event->x, event->y, mi->parent))
			{
			gtk_widget_hide(mi->ui->window);
			}
		else
			{
			ui_menu_hide_all(mi->md);
			}
		return TRUE;
		}

	if (mi->md->press_button != 0 && mi->md->press_button == event->button)
		{
		mi->md->press_button = 0;
		if (!mi->md->submenu_list->next && !mi->md->selection_done)
			{
			gtk_grab_add(mi->ui->display);
			}
		return FALSE;
		}

	work = g_list_last(mi->md->submenu_list);
	if (work->data != mi) return FALSE;

	ui_menu_hide_all(mi->md);
	return TRUE;
}

static gint ui_menu_key_cb(GtkWidget *w, GdkEventKey *event, gpointer data)
{
	MenuItemData *mi = data;
	gint ret = FALSE;

	switch (event->keyval)
		{
		case GDK_Escape:
			ui_menu_hide_all(mi->md);
			ret = TRUE;
			break;
		default:
			break;
		}

	return ret;
}

/*
 *---------------------------------------------------------
 * the list callbacks
 *---------------------------------------------------------
 */

static gint ui_menu_list_size_cb(ListData *list, const gchar *key, gpointer data)
{
	MenuItemData *mi = data;

	list_set_menu_style(list, TRUE);

	return g_list_length(mi->children);
}

static gint ui_menu_item_info_cb(ListData *list, const gchar *key,
				 gint row, ListRowData *rd, gpointer data)
{
	MenuItemData *mi = data;
	MenuItemData *item;
	gint flag;

	item = g_list_nth_data(mi->children, row);
	if (!item) return FALSE;

	list_row_column_set_text(list, rd, "text", item->text);

	if (item->children)
		{
		flag = 1;
		}
	else if (item->is_toggle)
		{
		flag = (item->toggle_active) ? 3 : 2;
		}
	else
		{
		flag = 0;
		}
	list_row_set_flag(rd, flag);
	list_row_set_sensitive(rd, item->sensitive && mi->sensitive);

	return TRUE;
}

static gint ui_menu_list_activate(MenuItemData *mi, ListData *ld, gint row)
{
	gint x, y;

	if (!mi->children) return FALSE;

	x = ld->x + ld->width - ld->border_right;
	y = ld->y + ld->border_top + (ld->row_height * (row - ld->row_start));

	x += mi->parent->x;
	y += mi->parent->y;

	ui_menu_submenu_activate(mi->md, mi, x, y, 0, ui_menu_event_time());

	return TRUE;
}

static void ui_menu_list_click_cb(ListData *list, const gchar *key, gint row,
				  ListRowData *rd, gint button, gpointer data)
{
	MenuItemData *mi = data;
	MenuItemData *item;

	item = g_list_nth_data(mi->children, row);
	if (!item)
		{
		mi->md->selection_done = TRUE;
		return;
		}

	if (ui_menu_list_activate(item, list, row)) return;

	item->md->selection_done = TRUE;
	if (item->select_func)
		{
		item->select_func(item->md, item->path, item->id, item->select_data);
		}
	
	ui_menu_hide_all(mi->md);
}

static void ui_menu_list_select_cb(ListData *list, const gchar *key, gint row,
				   ListRowData *rd, gpointer data)
{
	ui_menu_list_click_cb(list, key, row, rd, 0, data);
}

static gint ui_menu_list_move_cb(ListData *list, const gchar *key,
				 gint row, gint activated, gint up, gpointer data)
{
	MenuItemData *mi = data;
	MenuItemData *item;

	item = g_list_nth_data(mi->children, row);
	if (!item)
		{
		mi->md->selection_done = TRUE;
		return FALSE;
		}

	if (activated)
		{
		if (up && ui_menu_list_activate(item, list, row)) return TRUE;
		if (!up && mi->parent)
			{
			gtk_widget_hide(mi->ui->window);
			return TRUE;
			}
		}

	return FALSE;
}

static gint ui_menu_back_cb(UIData *ui, GdkPixbuf *pixbuf, gpointer data)
{
	MenuItemData *mi = data;

	if (!GTK_WIDGET_VISIBLE(mi->ui->display) || !GTK_WIDGET_REALIZED(mi->ui->display)) return FALSE;

	util_pixbuf_fill_from_root_window(pixbuf, mi->x, mi->y, TRUE);

	return TRUE;
}

/*
 *---------------------------------------------------------
 * submenu handling
 *---------------------------------------------------------
 */

static void ui_menu_submenu_hide_cb(GtkWidget *widget, gpointer data)
{
	MenuItemData *mi = data;
	UIMenuData *md;

	md = mi->md;

	if (md->preview == mi)
		{
		md->preview = NULL;
		return;
		}

	gtk_grab_remove(mi->ui->display);
	md->submenu_list = g_list_remove(md->submenu_list, mi);

	if (!mi->parent)
		{
		printf("warning: backed off tree at %s\n", mi->path);
		return;
		}

	ui_menu_submenu_grab(mi->parent, ui_menu_event_time());
}

static gint ui_menu_submenu_delete_cb(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	gtk_widget_hide(widget);
	return TRUE;
}

static void ui_menu_submenu_activate(UIMenuData *md, MenuItemData *mi,
				     gint x, gint y, gint button, guint32 event_time)
{
	if (!mi->ui)
		{
		const gchar *key = NULL;

		ui_menu_submenu_new(mi, md->ui->class, mi->path);
		g_signal_connect(G_OBJECT(mi->ui->window), "delete_event",
				 G_CALLBACK(ui_menu_submenu_delete_cb), mi);
		g_signal_connect(G_OBJECT(mi->ui->window), "hide",
				 G_CALLBACK(ui_menu_submenu_hide_cb), mi);

		if (mi->parent)
			{
			MenuItemData *parent;
			WidgetData *wd;

			parent = mi->parent;
			wd = skin_widget_get_by_key(parent->ui->skin, "menu", list_type_id());
			if (wd)
				{
				key = ui_widget_get_data(wd, "data");
				}
			if (!key) key = parent->ui->skin_mode_key;
			}

		if (!key) key = "skindata_menu";

		if (!ui_skin_load(mi->ui, md->ui->skin_path, key))
			{
			/* try default */
			if (!ui_skin_load(mi->ui, md->ui->skin_path, "skindata_menu"))
				{
				/* fall back to something */
				ui_skin_load(mi->ui, NULL, "skindata_menu");
				}
			}
		}

	md->submenu_list = g_list_append(md->submenu_list, mi);

	ui_menu_submenu_ungrab(mi->parent, event_time);

	ui_menu_submenu_show(md, mi, x, y);
	ui_menu_submenu_grab_focus(md, mi, button, event_time);
}

/*
 *---------------------------------------------------------
 * setup, show, etc.
 *---------------------------------------------------------
 */

static void ui_menu_destroy(GtkWidget *widget, gpointer data)
{
	UIMenuData *md = data;
	GList *work;

	work = md->list;
	while (work)
		{
		MenuItemData *mi;

		mi = work->data;
		work = work->next;

		ui_menu_item_free(mi);
		}
	g_list_free(md->list);

	g_free(md);
}

static void ui_menu_hide_cb(GtkWidget *widget, gpointer data)
{
	UIData *ui = data;

	gtk_grab_remove(ui->display);
	ui_close(ui);
}

static gint ui_menu_delete_cb(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	gtk_widget_hide(widget);
	return TRUE;
}

static UIData *ui_menu_submenu_new(MenuItemData *mi, const gchar *class, const gchar *key)
{
	UIData *ui;
	GtkWidget *window;

	window = gtk_window_new(GTK_WINDOW_POPUP);

	ui = ui_new_into_container(class, key, window);
	ui->window = window;
	mi->ui = ui;

	ui_focus_set(ui, TRUE);

#if 0
	g_signal_connect(G_OBJECT(ui->display), "button_press_event",
			 G_CALLBACK(ui_menu_press_cb), mi);
#endif
	g_signal_connect(G_OBJECT(ui->display), "button_release_event",
			 G_CALLBACK(ui_menu_release_cb), mi);
	g_signal_connect(G_OBJECT(ui->display), "key_press_event",
			 G_CALLBACK(ui_menu_key_cb), mi);

	/* set it up */
	list_register_key("menu", ui,
			  ui_menu_list_size_cb, mi,
			  ui_menu_item_info_cb, mi,
			  ui_menu_list_click_cb, mi,
			  ui_menu_list_select_cb, mi,
			  NULL, NULL);
	list_register_menu_funcs("menu", ui, ui_menu_list_move_cb, mi);

	if (mi->md->ui)
		{
		/* sub windows inheret parent functions */
		ui->skin_func = mi->md->ui->skin_func;
		ui->skin_data = mi->md->ui->skin_data;

		/* and the ui group */
		ui_group_set_child(mi->md->ui, ui);
		}
	ui_set_back_callback(ui, ui_menu_back_cb, mi);

	return ui;
}

UIMenuData *ui_menu_new(const gchar *class, const gchar *key)
{
	UIMenuData *md;
	MenuItemData *mi;

	md = g_new0(UIMenuData, 1);

	md->list = NULL;
	md->preview = NULL;

	ui_menu_item_add(md, "/", "/", 0, NULL, NULL);
	mi = md->list->data;
	md->submenu_list = g_list_append(md->submenu_list, mi);
	md->ui = ui_menu_submenu_new(mi, class, key);

	g_signal_connect(G_OBJECT(mi->ui->window), "destroy",
			 G_CALLBACK(ui_menu_destroy), md);
	g_signal_connect(G_OBJECT(mi->ui->window), "delete_event",
			 G_CALLBACK(ui_menu_delete_cb), md);
	g_signal_connect(G_OBJECT(mi->ui->window), "hide",
			 G_CALLBACK(ui_menu_hide_cb), mi->ui);

	return md;
}

static void ui_menu_submenu_show(UIMenuData *md, MenuItemData *mi, gint x, gint y)
{
	if (!mi || !mi->ui->skin) return;

	if (x < 0 || y < 0)
		{
		GdkModifierType modmask;
		gdk_window_get_pointer(NULL, &x, &y, &modmask);
		}

	mi->x = x;
	mi->y = y;
	ui_menu_calc_size(mi);

	if (!GTK_WIDGET_REALIZED(mi->ui->window)) gtk_widget_realize(mi->ui->window);
	gdk_window_move(mi->ui->window->window, mi->x, mi->y);
	gtk_widget_show(mi->ui->window);

	mi->md->selection_done = FALSE;
}

static void ui_menu_submenu_ungrab(MenuItemData *mi, guint32 event_time)
{
	if (GTK_WIDGET_VISIBLE(mi->ui->window))
		{
		gdk_pointer_ungrab(event_time);
		gdk_keyboard_ungrab(event_time);
		}
	gtk_grab_remove(mi->ui->display);
}

static void ui_menu_submenu_grab(MenuItemData *mi, guint32 event_time)
{
	gdk_pointer_grab(mi->ui->display->window, FALSE,
			 GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
			 NULL, NULL, event_time);
	gdk_keyboard_grab(mi->ui->display->window, TRUE, event_time);
	gtk_grab_add(mi->ui->display);
}

static void ui_menu_submenu_grab_focus(UIMenuData *md, MenuItemData *mi, gint button, guint32 event_time)
{
	WidgetData *wd;

	/* we peek at current event if -1 */
	if (button < 0) button = ui_menu_is_button_press();

	/* set focus to menu list widget,
	 * enable it for display,
	 * then redraw first row to show the focus
	 */
	wd = skin_widget_get_by_key(mi->ui->skin, "menu", list_type_id());
	if (wd)
		{
		mi->ui->focus_widget = wd;
		if (button > 0)
			{
			ListData *ld = wd->widget;

			mi->ui->active_widget = wd;

			/* force pressed state */
			ld->pressed = TRUE;
			ld->press_row = 0;
			}
		}

	GTK_WIDGET_SET_FLAGS(mi->ui->display, GTK_HAS_FOCUS);
	list_row_update("menu", mi->ui, 0);

	ui_menu_submenu_grab(mi, event_time);

	mi->md->press_button = button;
}

void ui_menu_show(UIMenuData *md, gint x, gint y, gint button, guint32 event_time)
{
	ui_menu_submenu_show(md, md->list->data, x, y);
	ui_menu_submenu_grab_focus(md, md->list->data, -1, event_time);
}

