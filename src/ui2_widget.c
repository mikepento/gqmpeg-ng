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
#include "ui2_widget.h"

#include "ui2_display.h"
#include "ui2_skin.h"
#include "ui_pixbuf_ops.h"


/*
 *-------------------------------
 * new / free
 *-------------------------------
 */

void ui_widget_free(WidgetData *wd)
{
	GList *work;

	if (wd->od && wd->od->func_free) wd->od->func_free(wd->widget);

	g_free(wd->key);
	g_free(wd->text_id);

	work = wd->data_list;
	while(work)
		{
		g_free(work->data);
		work = work->next;
		}
	g_list_free(wd->data_list);

	g_free(wd);
}

WidgetData *ui_widget_new(const gchar *key, const gchar *text_id, WidgetType type, gpointer widget)
{
	WidgetData *wd;

	wd = g_new0(WidgetData, 1);
	wd->type = type;
	wd->key = g_strdup(key);
	wd->text_id = g_strdup(text_id);
	wd->widget = widget;
	wd->data_list = NULL;

	wd->od = ui_widget_object_by_type(type);
	if (!wd->od)
		{
		printf("warning: unknown ui widget type %d\n", type);
		}

	wd->hidden = FALSE;
	wd->in_bounds = TRUE;

	wd->anchor_right = FALSE;
	wd->anchor_bottom = FALSE;

	return wd;
}

/*
 *-------------------------------
 * widget ui funcs
 *-------------------------------
 */

void ui_widget_motion(UIData *ui, WidgetData *wd, gint x, gint y)
{
	if (wd->hidden || !wd->in_bounds) return;

	if (wd->od && wd->od->func_motion)
		{
		wd->od->func_motion(wd->widget, wd->key, x, y, ui->skin->pixbuf, ui);
		}
}

gint ui_widget_press(UIData *ui, WidgetData *wd, gint x, gint y)
{
	if (wd->hidden || !wd->in_bounds) return FALSE;

	if (wd->od && wd->od->func_press)
		{
		return wd->od->func_press(wd->widget, wd->key, x, y, ui->skin->pixbuf, ui);
		}

	return FALSE;
}

void ui_widget_release(UIData *ui, WidgetData *wd, gint x, gint y)
{
	if (wd->hidden || !wd->in_bounds) return;

	if (wd->od && wd->od->func_release)
		{
		wd->od->func_release(wd->widget, wd->key, x, y, ui->skin->pixbuf, ui);
		}
}

void ui_widget_draw(UIData *ui, WidgetData *wd, gint update, gint force)
{
	if (wd->hidden || !wd->in_bounds) return;

	if (wd->od && wd->od->func_draw)
		{
		wd->od->func_draw(wd->widget, wd->key, update, force, ui->skin->pixbuf, ui);
		}
}

void ui_widget_reset(UIData *ui, WidgetData *wd)
{
	if (wd->hidden || !wd->in_bounds) return;

	if (wd->od && wd->od->func_reset)
		{
		wd->od->func_reset(wd->widget, wd->key, ui->skin->pixbuf, ui);
		}
}

void ui_widget_sync_back(UIData *ui, WidgetData *wd)
{
	/* even hidden items sync their back if in bounds */
	if (!wd->in_bounds) return;

	if (wd->od && wd->od->func_back)
		{
		wd->od->func_back(wd->widget, ui->skin->overlay);
		}
}

void ui_widget_hide(UIData *ui, WidgetData *wd)
{
	gint x, y, w, h;

	if (wd->hidden) return;

	wd->hidden = TRUE;
	if (ui_widget_get_geometry(wd, &x, &y, &w, &h))
		{
		ui_display_redraw_area(ui, x, y, w, h, FALSE);
		}
}

void ui_widget_show(UIData *ui, WidgetData *wd)
{
	if (!wd->hidden) return;

	wd->hidden = FALSE;
	ui_widget_draw(ui, wd, TRUE, TRUE);
}

/*
 *-------------------------------
 * widget focus funcs
 *-------------------------------
 */

gint ui_widget_focus_draw(UIData *ui, WidgetData *wd, gint x, gint y, gint w, gint h)
{
	if (wd->od && wd->od->func_focus_draw)
		{
		return wd->od->func_focus_draw(wd->widget, wd->key, x, y, w, h, ui->skin->pixbuf, ui);
		}

	return FALSE;
}

gint ui_widget_focus_key_event(UIData *ui, WidgetData *wd, GdkEventKey *event)
{
	if (wd->od && wd->od->func_focus_key_event)
		{
		return wd->od->func_focus_key_event(wd->widget, wd->key, event, ui->skin->pixbuf, ui);
		}

	return FALSE;
}

gint ui_widget_can_focus(WidgetData *wd)
{
	return (wd->od && !wd->hidden && wd->in_bounds && wd->od->func_focus_key_event);
}

/*
 *-------------------------------
 * widget misc funcs
 *-------------------------------
 */

gint ui_widget_for_each_key_one(UIData *ui, const gchar *key, WidgetType type,
			        void (*func)(WidgetData *wd, gpointer data, UIData *ui),
			        gpointer data)
{
	gint count = 0;
	GList *work;

	if (!func || !ui->skin) return 0;

	work = ui->skin->widget_list;
	while (work)
		{
		WidgetData *wd = work->data;

		if (wd->type == type && strcmp(key, wd->key) == 0)
			{
			func(wd, data, ui);
			count++;
			}

		work = work->next;
		}

	return count;
}

gint ui_widget_for_each_key(UIData *ui, const gchar *key, WidgetType type,
			    void (*func)(WidgetData *wd, gpointer data, UIData *ui),
			    gpointer data)
{
	gint count;
	UIData *parent;
	GList *work;

	if (ui->parent)
		{
		parent = ui->parent;
		}
	else
		{
		parent = ui;
		}

	count = ui_widget_for_each_key_one(parent, key, type, func, data);

	work = parent->children;
	while (work)
		{
		UIData *child;

		child = work->data;
		work = work->next;

		count += ui_widget_for_each_key_one(child, key, type, func, data);
		}

	/* editor updates */
	if (parent->edit)
		{
		ui_widget_for_each_key_one(parent->edit, key, type, func, data);
		}

	return count;
}

static void ui_widget_hide_by_key_cb(WidgetData *wd, gpointer data, UIData *ui)
{
	ui_widget_hide(ui, wd);
}

void ui_widget_hide_by_key(UIData *ui, const gchar *key, WidgetType type)
{
	if (!ui || !key) return;

	ui_widget_for_each_key(ui, key, type, ui_widget_hide_by_key_cb, NULL);
}

static void ui_widget_show_by_key_cb(WidgetData *wd, gpointer data, UIData *ui)
{
	ui_widget_show(ui, wd);
}

void ui_widget_show_by_key(UIData *ui, const gchar *key, WidgetType type)
{
	if (!ui || !key) return;

	ui_widget_for_each_key(ui, key, type, ui_widget_show_by_key_cb, NULL);
}

/*
 *-------------------------------
 * widget ui func utils
 *-------------------------------
 */

gint ui_widget_get_geometry(WidgetData *wd, gint *x, gint *y, gint *w, gint *h)
{
	gint nx, ny, nw, nh;

	if (!wd->od || !wd->od->func_get_geometry) return FALSE;

	if (!wd->od->func_get_geometry(wd->widget, &nx, &ny, &nw, &nh)) return FALSE;

	if (x) *x = nx;
	if (y) *y = ny;
	if (w) *w = nw;
	if (h) *h = nh;

	return TRUE;
}

void ui_widget_set_coord(UIData *ui, WidgetData *wd, gint x, gint y, gint redraw)
{
	gint ox, oy, ow, oh;
	gint old_bounds;

	if (!wd->od || !wd->od->func_set_coord) return;

	if (!ui_widget_get_geometry(wd, &ox, &oy, &ow, &oh)) return;

	wd->od->func_set_coord(wd->widget, x, y);

	old_bounds = wd->in_bounds;
	skin_widget_check_bounds(ui->skin, wd);

	if (!wd->in_bounds || !redraw) return;

	/* change back */
	pixbuf_copy_area(ui->skin->overlay, x, y,
			 ui->skin->pixbuf, x, y, ow, oh, FALSE);
	ui_widget_sync_back(ui, wd);

	/* redraw old location */
	if (old_bounds) ui_display_redraw_area(ui, ox, oy, ow, oh, FALSE);

	/* draw it */
	ui_widget_draw(ui, wd, FALSE, TRUE);
}

void ui_widget_set_size(UIData *ui, WidgetData *wd, gint dev_w, gint dev_h, gint redraw)
{
	gint ox, oy, ow, oh;
	gint nx, ny, nw, nh;
	gint old_bounds;

	if (!wd->od || !wd->od->func_set_size) return;

	if (!ui_widget_get_geometry(wd, &ox, &oy, &ow, &oh)) return;

	wd->od->func_set_size(wd->widget, dev_w, dev_h);

	old_bounds = wd->in_bounds;
	skin_widget_check_bounds(ui->skin, wd);

	if (!wd->in_bounds || !redraw) return;

	if (!ui_widget_get_geometry(wd, &nx, &ny, &nw, &nh)) return;

	/* change back */
	pixbuf_copy_area(ui->skin->overlay, nx, ny,
			 ui->skin->pixbuf, nx, ny, nw, nh, FALSE);
	ui_widget_sync_back(ui, wd);

	/* redraw old location */
	if (old_bounds) ui_display_redraw_area(ui, ox, oy, ow, oh, FALSE);

	/* draw it */
	ui_widget_draw(ui, wd, FALSE, TRUE);
}

void ui_widget_set_anchor(WidgetData *wd, gint right, gint bottom)
{
	if (!wd) return;

	wd->anchor_right = right;
	wd->anchor_bottom = bottom;
}

WidgetData *ui_widget_find_by_coord(UIData *ui, gint x, gint y)
{
	GList *work;

	work = g_list_last(ui->skin->widget_list);
	while(work)
		{
		WidgetData *wd = work->data;
		gint wx, wy, ww, wh;

		if (wd->od && wd->od->is_visible && ui_widget_get_geometry(wd, &wx, &wy, &ww, &wh) &&
		    x >= wx && x <= wx+ww && y >= wy && y <= wy+wh) return wd;

		work = work->prev;
		}

	return NULL;
}

gint ui_widget_contacts_area(WidgetData *wd, gint x, gint y, gint w, gint h)
{
	gint wx, wy, ww, wh;

	if (!ui_widget_get_geometry(wd, &wx, &wy, &ww, &wh) ||
	    x > wx + ww || x + w < wx || y > wy + wh || y + h < wy) return FALSE;

	return TRUE;
}

/*
 *-------------------------------
 * widget misc
 *-------------------------------
 */

WidgetData *ui_widget_get_by_widget(UIData *ui, gpointer widget)
{
	GList *work;

	if (!widget || !ui->skin) return NULL;

	work = ui->skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;

		if (wd->widget == widget) return wd;
		work = work->next;
		}
	return NULL;
}

/*
 *-------------------------------
 * widget data attachment
 *-------------------------------
 */

static GList *widget_find_key(WidgetData *wd, const gchar *key)
{
	GList *work;

	work = wd->data_list;
	while(work)
		{
		gchar *skey = work->data;

		if (strcmp(skey, key) == 0) return work;
		work = work->next;
		if (work) work = work->next;
		}

	return NULL;
}

void ui_widget_set_data(WidgetData *wd, const gchar *key, const gchar *data)
{
	GList *work;

	if (!wd) return;

	/* if key exists, change it (or if !data remove it) */
	work = widget_find_key(wd, key);
	if (work)
		{
		if (!data)
			{
			gchar *k;
			gchar *d;

			k = work->data;
			work = work->next;
			d = work->data;

			wd->data_list = g_list_remove(wd->data_list, k);
			wd->data_list = g_list_remove(wd->data_list, d);

			g_free(k);
			g_free(d);
			return;
			}
		work = work->next;
		g_free(work->data);
		work->data = g_strdup(data);
		return;
		}

	if (!key || !data) return;

	wd->data_list = g_list_append(wd->data_list, g_strdup(key));
	wd->data_list = g_list_append(wd->data_list, g_strdup(data));
}

const gchar *ui_widget_get_data(WidgetData *wd, const gchar *key)
{
	GList *work;

	if (!wd || !key) return NULL;

	work = widget_find_key(wd, key);
	if (work && work->next)
		{
		work = work->next;
		return work->data;
		}

	return NULL;
}

/* this a wrapper that get data with a key of "data", for use by the app
 */

const gchar *ui_widget_get_data_by_widget(UIData *ui, gpointer widget)
{
	return ui_widget_get_data(ui_widget_get_by_widget(ui, widget), "data");
}

/*
 *-------------------------------
 * widget function/type registry
 *-------------------------------
 */

static GList *widget_type_list = NULL;
static WidgetType widget_type_count = 0;

WidgetObjectData *ui_widget_type_new(const gchar *description)
{
	WidgetObjectData *od;

	od = g_new0(WidgetObjectData, 1);

	od->type = widget_type_count;
	widget_type_count++;

	od->priority = UI_WIDGET_PRIORITY_NORMAL;

	od->description = g_strdup(description);
	od->is_visible = TRUE;

	widget_type_list = g_list_append(widget_type_list, od);

	return od;
}

WidgetObjectData *ui_widget_object_by_type(WidgetType type)
{
	GList *work;

	work = widget_type_list;
	while(work)
		{
		WidgetObjectData *od = work->data;
		if (od->type == type) return od;
		work = work->next;
		}

	return NULL;
}

WidgetObjectData *ui_widget_object_by_text(const gchar *description)
{
	GList *work;

	if (!description) return NULL;

	work = widget_type_list;
	while(work)
		{
		WidgetObjectData *od = work->data;
		if (od->description && strcmp(od->description, description) == 0) return od;
		work = work->next;
		}

	return NULL;
}

const gchar *ui_widget_type_to_text(WidgetType type)
{
	GList *work;

	work = widget_type_list;
	while(work)
		{
		WidgetObjectData *od = work->data;
		if (od->type == type) return od->description;
		work = work->next;
		}

	return "unknown";
}

WidgetObjectData *ui_widget_object_copy(WidgetObjectData *src)
{
	WidgetObjectData *od;

	if (!src) return NULL;

	od = g_memdup(src, sizeof(WidgetObjectData));

	od->description = g_strdup(src->description);

	return od;
}

