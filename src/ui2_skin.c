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

#include "ui2_includes.h"
#include "ui2_typedefs.h"
#include "ui2_skin.h"

#include <math.h>

#include "ui2_button.h"
#include "ui2_display.h"
#include "ui2_main.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"

#include <gdk/gdkkeysyms.h> /* for key values */


/*
 *-------------
 * background
 *-------------
 */

static void skin_back_setup(SkinData *skin)
{
	GdkPixbuf *s;
	GdkPixbuf *d;
	gint w, h;

	if (!skin->real_overlay) return;

	if (skin->overlay &&
	    (skin->width != gdk_pixbuf_get_width(skin->overlay) ||
	     skin->height != gdk_pixbuf_get_height(skin->overlay)))
		{
		gdk_pixbuf_unref(skin->overlay);
		skin->overlay = NULL;
		}

	if (!skin->overlay)
		{
		skin->overlay = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
					       gdk_pixbuf_get_has_alpha(skin->real_overlay), 8,
					       skin->width, skin->height);
		}

	s = skin->real_overlay;
	w = gdk_pixbuf_get_width(s);
	h = gdk_pixbuf_get_height(s);

	d = skin->overlay;

	if (skin->has_border)
		{
		pixbuf_copy_fill_border(s, d,
					0, 0,
					gdk_pixbuf_get_width(d), gdk_pixbuf_get_height(d),
					skin->border_left, skin->border_left_stretch,
					skin->border_right, skin->border_right_stretch,
					skin->border_top, skin->border_top_stretch,
					skin->border_bottom, skin->border_bottom_stretch,
					skin->stretch, TRUE);
		}
	else
		{
		pixbuf_copy_fill(s, 0, 0, w, h,
				 d, 0, 0, skin->width, skin->height,
				 skin->stretch, TRUE);
		}

	if (skin->mask_buffer) g_object_unref(skin->mask_buffer);
	skin->mask_buffer = NULL;
	if (skin->mask)
		{
		gdk_pixbuf_render_pixmap_and_mask(skin->mask, NULL, &skin->mask_buffer, 128);
		}
	else
		{
		gdk_pixbuf_render_pixmap_and_mask(skin->overlay, NULL, &skin->mask_buffer, 1);
		}
}

static void skin_back_pass2(SkinData *skin, UIData *ui)
{
	guint8 alpha;

	if (!skin->real_overlay || !skin->overlay) return;

	alpha =  (slik_transparency_force) ? slik_transparency_force_a : 255;

	if (skin->has_border)
		{
		pixbuf_copy_fill_border_alpha(skin->real_overlay, skin->overlay,
					      0, 0,
					      gdk_pixbuf_get_width(skin->overlay), gdk_pixbuf_get_height(skin->overlay),
					      skin->border_left, skin->border_left_stretch,
					      skin->border_right, skin->border_right_stretch,
					      skin->border_top, skin->border_top_stretch,
					      skin->border_bottom, skin->border_bottom_stretch,
					      skin->stretch, alpha);
		}
	else
		{
		gint w, h;

		w = gdk_pixbuf_get_width(skin->real_overlay);
		h = gdk_pixbuf_get_height(skin->real_overlay);
		pixbuf_copy_fill_alpha(skin->real_overlay, 0, 0, w, h,
				       skin->overlay, 0, 0, skin->width, skin->height,
				       skin->stretch, alpha);
		}
}

/*
 *-------------
 * skin side keys
 *-------------
 */

WidgetData *skin_register_widget(SkinData *skin, const gchar *key, const gchar *text_id, WidgetType type, gpointer widget)
{
	WidgetData *wd;

	if (key == NULL)
		{
		printf("Attempt to register skin widget with NULL key!\n");
		return NULL;
		}
	if (debug_mode) printf("skin registering widget \"%s\" (%d)\n", key, type);
	wd = ui_widget_new(key, text_id, type, widget);
	skin->widget_list = g_list_append(skin->widget_list, wd);

	return wd;
}

void skin_register_free_all(SkinData *skin)
{
	while(skin->widget_list)
		{
		WidgetData *wd = skin->widget_list->data;
		skin->widget_list = g_list_remove(skin->widget_list, wd);

		ui_widget_free(wd);
		}
}

WidgetData *skin_widget_get_by_key(SkinData *skin, const gchar *key, WidgetType type)
{
	GList *work;

	if (!key) return NULL;

	work = skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;

		if (wd->type == type && strcmp(key, wd->key) == 0) return wd;

		work = work->next;
		}
	return NULL;
}

WidgetData *skin_widget_get_by_text_id(SkinData *skin, const gchar *text_id)
{
	GList *work;

	if (!text_id) return NULL;

	work = skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;

		if (wd->text_id && strcmp(text_id, wd->text_id) == 0) return wd;

		work = work->next;
		}
	return NULL;
}

static gint skin_widget_for_each_do(UIData *ui, const gchar *key, WidgetType type,
				    void (*func)(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui),
				    gpointer data)
{
	gint count = 0;
	GList *work;

	if (!ui->skin) return 0;

	work = ui->skin->widget_list;
	while (work)
		{
		WidgetData *wd = work->data;

		if (wd->type == type && wd->in_bounds && !wd->hidden && strcmp(key, wd->key) == 0)
			{
			func(wd, data, ui->skin->pixbuf, ui);
			count++;
			}

		work = work->next;
		}

	return count;
}

gint skin_widget_for_each_key(UIData *ui, const gchar *key, WidgetType type,
			      void (*func)(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui),
			      gpointer data)
{
	gint count;
	GList *work;
	UIData *parent;

	if (!func || !key) return 0;

	if (ui->parent)
		{
		parent = ui->parent;
		}
	else
		{
		parent = ui;
		}

	count = skin_widget_for_each_do(ui, key, type, func, data);

	/* obey private keys */
	if (ui_registered_key_is_private(ui, key, type))
		{
		return count;
		}

	/*
	 * Children keys override the parent's,
	 * so if at least one hit, do not propogate up to parent.
	 */
	if (parent != ui && count > 0) return count;

	if (parent != ui)
		{
		count += skin_widget_for_each_do(parent, key, type, func, data);
		}

	work = parent->children;
	while (work)
		{
		UIData *child;

		child = work->data;

		if (child != ui)
			{
			count += skin_widget_for_each_do(child, key, type, func, data);
			}

		work = work->next;
		}

	/* editor updates (only parent can have an editor) */
	if (parent->edit)
		{
		skin_widget_for_each_key(parent->edit, key, type, func, data);
		}

	return count;
}

static gint skin_widget_for_each_key_simple(SkinData *skin, const gchar *key, WidgetType type,
					    void (*func)(WidgetData *wd, gpointer data),
					    gpointer data)
{
	gint count = 0;
	GList *work;

	if (!func || !key) return 0;

	work = skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;

		if (!key || (wd->type == type && strcmp(key, wd->key) == 0))
			{
			func(wd, data);
			count++;
			}

		work = work->next;
		}

	return count;
}

/*
 *-------------
 * skin stuff
 *-------------
 */

SkinData *skin_new(void)
{
	SkinData *skin;

	skin = g_new0(SkinData, 1);
	skin->widget_list = NULL;
	skin->transparent = FALSE;
	skin->ui = NULL;

	skin->background_filename = NULL;
	skin->background_mask_filename = NULL;
	skin->focus_filename = NULL;

	skin->focus_overlay = NULL;
	skin->focus_stretch = FALSE;
	skin->focus_has_border = FALSE;
	skin->focus_border_left = 0;
	skin->focus_border_right = 0;
	skin->focus_border_top = 0;
	skin->focus_border_bottom = 0;
	skin->focus_anchor_right = FALSE;
	skin->focus_anchor_bottom = FALSE;

	skin->focus_box_filled = FALSE;
	skin->focus_box_alpha = 128;
	skin->focus_box_r = 255;
	skin->focus_box_g = 0;
	skin->focus_box_b = 0;

	skin->scratch_pixbuf = NULL;
	skin->scratch_pixmap = NULL;

	skin->resize_idle_id = -1;

	return skin;
}

void skin_free(SkinData *skin)
{
	if (!skin) return;

	if (skin->resize_idle_id != -1) gtk_idle_remove(skin->resize_idle_id);

	while (skin->widget_list)
		{
		WidgetData *wd = skin->widget_list->data;
		skin->widget_list = g_list_remove(skin->widget_list, wd);
		ui_widget_free(wd);
		}

	if (skin->overlay) gdk_pixbuf_unref(skin->overlay);
	if (skin->pixbuf) gdk_pixbuf_unref(skin->pixbuf);
	if (skin->buffer) g_object_unref(skin->buffer);
	if (skin->mask) gdk_pixbuf_unref(skin->mask);
	if (skin->mask_buffer) g_object_unref(skin->mask_buffer);

	if (skin->real_overlay) gdk_pixbuf_unref(skin->real_overlay);

	if (skin->focus_overlay) gdk_pixbuf_unref(skin->focus_overlay);

	if (skin->scratch_pixbuf) gdk_pixbuf_unref(skin->scratch_pixbuf);
	if (skin->scratch_pixmap) g_object_unref(skin->scratch_pixmap);

	g_free(skin->background_filename);
	g_free(skin->background_mask_filename);
	g_free(skin->focus_filename);

	g_free(skin);
}

void skin_get_scratch(SkinData *skin, gint w, gint h, GdkPixbuf **pixbuf, GdkPixmap **pixmap)
{
	gint old_w, old_h;

	if (skin->scratch_pixbuf)
		{
		old_w = gdk_pixbuf_get_width(skin->scratch_pixbuf);
		old_h = gdk_pixbuf_get_height(skin->scratch_pixbuf);
		}
	else
		{
		old_w = 0;
		old_h = 0;
		}

	if (w > old_w || h > old_h)
		{
		/* scratch buffer dimensions can only grow,
		 * to reduce needing to recreate for each widget size
		 */
		if (old_w > w) w = old_w;
		if (old_h > h) h = old_h;

		if (skin->scratch_pixbuf) gdk_pixbuf_unref(skin->scratch_pixbuf);
		skin->scratch_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);

		if (skin->scratch_pixmap) g_object_unref(skin->scratch_pixmap);
		skin->scratch_pixmap = gdk_pixmap_new(skin->ui->display->window, w, h, -1);
		}

	if (pixbuf) *pixbuf = skin->scratch_pixbuf;
	if (pixmap) *pixmap = skin->scratch_pixmap;
}

GdkPixmap *skin_get_buffer(SkinData *skin)
{
	return skin->buffer;
}

GdkPixbuf *skin_get_pixbuf(SkinData *skin)
{
	return skin->pixbuf;
}

UIData *skin_get_ui(SkinData *skin)
{
	if (!skin) return NULL;
	return skin->ui;
}

static void skin_sync_back_on_widgets(SkinData *skin, UIData *ui)
{
	gint priority;

	for (priority = UI_WIDGET_PRIORITY_HIGH; priority >= 0; priority--)
		{
		GList *work;

		work = skin->widget_list;
		while (work)
			{
			WidgetData *wd = work->data;
			work = work->next;

			if (wd->od->priority == priority) ui_widget_sync_back(ui, wd);
			}
		}
}

void skin_sync_back(SkinData *skin, UIData *ui)
{
	gint pass2 = FALSE;

	if (!skin || !skin->overlay || !skin->pixbuf) return;

	if (ui->back_func &&
	    (gdk_pixbuf_get_has_alpha(skin->overlay) || slik_transparency_force) &&
	    ui->back_func(ui, skin->overlay, ui->back_data))
		{
		pass2 = TRUE;
#if 0
		if (slik_transparency_force)
			{
			pixbuf_copy_area_alpha(skin->overlay, 0, 0, pb, 0, 0, skin->width, skin->height,
					       slik_transparency_force_a);
			}
		else
			{
			pixbuf_copy_area_alpha(skin->overlay, 0, 0, pb, 0, 0, skin->width, skin->height, 255);
			}
#endif
		}
	else if (skin->transparent || slik_transparency_force)
		{
		gint x, y;

		if (ui->window)
			{
			gdk_window_get_position(ui->window->window, &x, &y);
			}
		else
			{
			gdk_window_get_position(ui->display->window, &x, &y);
			}

		util_pixbuf_fill_from_root_window(skin->overlay, x, y, FALSE);

		pass2 = TRUE;
#if 0
		if (slik_transparency_force)
			{
			pixbuf_copy_area_alpha(skin->overlay, 0, 0, pb, 0, 0, skin->width, skin->height,
					       slik_transparency_force_a);
			}
		else
			{
			pixbuf_copy_area_alpha(skin->overlay, 0, 0, pb, 0, 0, skin->width, skin->height, 255);
			}
#endif
		}
#if 0
	else
		{
		pixbuf_copy_area(skin->overlay, 0, 0, pb, 0, 0, skin->width, skin->height, FALSE);
		}
#endif

	if (pass2) skin_back_pass2(skin, ui);

	skin_sync_back_on_widgets(skin, ui);
	pixbuf_copy_area(skin->overlay, 0, 0, skin->pixbuf, 0, 0, skin->width, skin->height, FALSE);
}

void skin_set_underlay(SkinData *skin, UIData *ui, GdkPixbuf *pb)
{
	if (!skin || !skin->pixbuf || !pb) return;

	skin_back_setup(skin);
	pixbuf_copy_area(pb, 0, 0, skin->overlay, 0, 0, skin->width, skin->height, FALSE);
#if 0
	pixbuf_copy_area_alpha(skin->overlay, 0, 0, skin->pixbuf, 0, 0, skin->width, skin->height, 255);
#endif
	skin_back_pass2(skin, ui);
	skin_sync_back_on_widgets(skin, ui);
	pixbuf_copy_area(skin->overlay, 0, 0, skin->pixbuf, 0, 0, skin->width, skin->height, FALSE);
}

void skin_sync_buffer(SkinData *skin, UIData *ui)
{
	if (!skin) return;

	skin_back_setup(skin);

	if (!GTK_WIDGET_REALIZED(ui->display)) gtk_widget_realize(ui->display);

	if (skin->buffer) g_object_unref(skin->buffer);
	skin->buffer = gdk_pixmap_new(ui->display->window, skin->width, skin->height, -1);

	if (skin->pixbuf) gdk_pixbuf_unref(skin->pixbuf);
	skin->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, skin->width, skin->height);

	skin_sync_back(skin, ui);
}

void skin_debug_print_registered_keys(SkinData *skin)
{
	GList *work;

	if (!skin)
		{
		printf("no skin\n");
		return;
		}

	printf("SKIN widget keys (%3d):\n", g_list_length(skin->widget_list));
	printf("-[key]----------------[in bounds]-[type]-----[text id]--[data]-\n");

	work = skin->widget_list;
	while(work)
		{
		WidgetData *wd;
		const gchar *data;

		wd = work->data;
		work = work->next;

		data = ui_widget_get_data(wd, "data");

		printf("%-30s %1d   %-10s %-10s %s\n", wd->key, wd->in_bounds, ui_widget_type_to_text(wd->type), (wd->text_id) ? wd->text_id : "", (data) ? data : "");
		}
}

void skin_widget_check_bounds(SkinData *skin, WidgetData *wd)
{
	gint x, y, w, h;

	if (wd->od && wd->od->is_visible &&
	    ui_widget_get_geometry(wd, &x, &y, &w, &h))
		{
		if (x < 0 || y < 0 ||
		    x + w > skin->width || y + h > skin->height)
			{
			wd->in_bounds = FALSE;
			}
		else
			{
			wd->in_bounds = TRUE;
			}
		}
}

void skin_check_bounds(SkinData *skin)
{
	GList *work;

	if (!skin) return;

	work = skin->widget_list;
	while (work)
		{
		WidgetData *wd = work->data;
		skin_widget_check_bounds(skin, wd);
		work = work->next;
		}
}

/* this also takes on the work of calling set_size */
static void skin_move_anchored(SkinData *skin, gint move_w, gint move_h)
{
	GList *work;

	work = skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;
		gint x, y;

		if ((wd->anchor_right || wd->anchor_bottom) &&
		    ui_widget_get_geometry(wd, &x, &y, NULL, NULL))
			{
			if (wd->anchor_right) x += move_w;
			if (wd->anchor_bottom) y += move_h;
			ui_widget_set_coord(skin->ui, wd, x, y, FALSE);
			}
		ui_widget_set_size(skin->ui, wd, move_w, move_h, FALSE);

		work = work->next;
		}
}

void skin_resize(UIData *ui, gint w, gint h)
{
	SkinData *skin;
	gint x, y;
	gint dw, dh;

	skin = ui->skin;

	if (skin->resize_idle_id != -1)
		{
		gtk_idle_remove(skin->resize_idle_id);
		skin->resize_idle_id = -1;
		}

	if (w > SKIN_SIZE_MAX) w = SKIN_SIZE_MAX;
	if (h > SKIN_SIZE_MAX) h = SKIN_SIZE_MAX;
	if (w < SKIN_SIZE_MIN) w = SKIN_SIZE_MIN;
	if (h < SKIN_SIZE_MIN) h = SKIN_SIZE_MIN;

	/* keyboard resize expects this to be in sync */
	skin->resize_idle_width = w;
	skin->resize_idle_height = h;

	if (w == skin->width && h == skin->height) return;
	if (w < 4 || h < 4) return;

	x = y = 0;

	if (ui->window && ui->window->window) gdk_window_get_position(ui->window->window, &x, &y);

	dw = w - skin->width;
	dh = h - skin->height;

	skin->width = w;
	skin->height = h;

	skin_move_anchored(skin, dw, dh);

	ui_display_sync_all(ui);

	if (ui->window && ui->window->window) gdk_window_move(ui->window->window, x, y);
}

/*
 *-------------
 * skin widget init (after setting to a ui)
 *-------------
 */

void skin_widgets_init(SkinData *skin, UIData *ui)
{
	GList *work;

	if (!skin || !ui) return;

	work = skin->widget_list;
	while (work)
		{
		WidgetData *wd = work->data;

		if (wd->od && wd->od->func_init)
			{
			wd->od->func_init(wd->widget, wd->key, ui);
			}
		
		work = work->next;
		}
}

/*
 *-------------
 * skin resizing button handling
 *-------------
 */

static void (*skin_size_func_motion)(gpointer, const gchar *, gint, gint, GdkPixbuf *, UIData *) = NULL;
static gint (*skin_size_func_key)(gpointer widget, const gchar *key, GdkEventKey *event, GdkPixbuf *pb, UIData *ui) = NULL;

static gint skin_resize_idle_cb(gpointer data)
{
	SkinData *skin = data;

	if (skin->resize_idle_id == -1) return FALSE;

	skin->resize_idle_id = -1;

	skin->ui->press_x += skin->resize_idle_width - skin->width;
	skin->ui->press_y += skin->resize_idle_height - skin->height;

	skin_resize(skin->ui, skin->resize_idle_width, skin->resize_idle_height);

	return FALSE;
}

static void skin_resize_real_motion(gpointer widget, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	ButtonData *button = widget;

	if (button->active && ui->skin->sizeable && ui->skin->real_overlay)
		{
		SkinData *skin = ui->skin;
		gint mx, my;
		gint nw, nh;
		gint w, h;

		mx = x - ui->press_x;
		my = y - ui->press_y;

		/* sanity check */
		if (skin->width_inc < 1) skin->width_inc = 1;
		if (skin->height_inc < 1) skin->height_inc = 1;

		/* lock to increments */
		mx = (gint)floor((float)mx / skin->width_inc + 0.5) * skin->width_inc;
		my = (gint)floor((float)my / skin->height_inc + 0.5) * skin->height_inc;

		/* compute new size */
		nw = skin->width + mx;
		nh = skin->height + my;

		/* clamp to valid range */
		if (nw < skin->width_min) nw = skin->width_min;
		if (nw > skin->width_max) nw = skin->width_max;
		if (nh < skin->height_min) nh = skin->height_min;
		if (nh > skin->height_max) nh = skin->height_max;

		w = nw - skin->width;
		h = nh - skin->height;

		if ((w != 0 || h != 0) &&
		    button->x + w >= 0 &&
		    button->y + h >= 0)
			{
			/* resize it */
			skin->resize_idle_width = nw;
			skin->resize_idle_height = nh;
			if (skin->resize_idle_id == -1) skin->resize_idle_id = gtk_idle_add(skin_resize_idle_cb, skin);
			}
		}

	/* pass it on to the button (at this time not necessary,
	 * but the button motion API may change in the future */
	if (skin_size_func_motion) skin_size_func_motion(widget, key, x, y, ui->skin->pixbuf, ui);
}

static gint skin_resize_real_key(gpointer widget, const gchar *key, GdkEventKey *event, GdkPixbuf *pb, UIData *ui)
{
	SkinData *skin;
	gint nw, nh;
	gint ret = TRUE;

	if (!ui->skin->sizeable || !ui->skin->real_overlay ||
	    !(event->state & GDK_SHIFT_MASK))
		{
		if (skin_size_func_key)
			{
			/* pass key press on to button */
			return skin_size_func_key(widget, key, event, ui->skin->pixbuf, ui);
			}
		return FALSE;
		}

	skin = ui->skin;

	if (skin->resize_idle_width < 4 || skin->resize_idle_height < 4)
		{
		/* probably default skin size, hence no skin_resize has been called yet */
		nw = skin->width;
		nh = skin->height;
		}
	else
		{
		nw = skin->resize_idle_width;
		nh = skin->resize_idle_height;
		}

	switch (event->keyval)
		{
		case GDK_Up:
		case GDK_KP_Up:
			nh -= skin->height_inc;
			break;
		case GDK_Down:
		case GDK_KP_Down:
			nh += skin->height_inc;
			break;
		case GDK_Left:
		case GDK_KP_Left:
			nw -= skin->width_inc;
			break;
		case GDK_Right:
		case GDK_KP_Right:
			nw += skin->width_inc;
			break;
		case GDK_Page_Up:
		case GDK_KP_Page_Up:
			nw = skin->width_min;
			nh = skin->height_min;
			break;
		case GDK_Page_Down:
		case GDK_KP_Page_Down:
			nw = skin->width_max;
			nh = skin->height_max;
			break;
		case GDK_Home:
		case GDK_KP_Home:
			nw = skin->width_def;
			nh = skin->height_def;
			break;
		default:
			ret = FALSE;
			break;
		}

	if (ret)
		{
		if (nw < skin->width_min) nw = skin->width_min;
		if (nw > skin->width_max) nw = skin->width_max;
		if (nh < skin->height_min) nh = skin->height_min;
		if (nh > skin->height_max) nh = skin->height_max;

		if (nw != skin->resize_idle_width || nh != skin->resize_idle_height)
			{
			skin->resize_idle_width = nw;
			skin->resize_idle_height = nh;
			if (skin->resize_idle_id == -1) skin->resize_idle_id = gtk_idle_add(skin_resize_idle_cb, skin);
			}
		}

	/* if not handled, pass it on to the button */
	if (!ret && skin_size_func_key)
		{
		return skin_size_func_key(widget, key, event, ui->skin->pixbuf, ui);
		}
	
	return ret;
}

static void skin_resize_setup_cb(WidgetData *wd, gpointer data)
{
	static WidgetObjectData *od = NULL;

	/* since this intercepts func_motion, we need to copy the original button od,
	 * set our own func_motion, and set the WidgetData to use the copy
	 */
	if (!od)
		{
		od = ui_widget_object_copy(ui_widget_object_by_type(button_type_id()));

		skin_size_func_motion = od->func_motion;
		od->func_motion = skin_resize_real_motion;

		skin_size_func_key = od->func_focus_key_event;
		od->func_focus_key_event = skin_resize_real_key;
		}

	if (od && wd->od)
		{
		wd->od = od;

		/* this button always anchors bottom, right */
		wd->anchor_right = TRUE;
		wd->anchor_bottom = TRUE;
		}
}

/* this overrides things like the skin resize button */
void skin_finalize(SkinData *skin)
{
	skin_widget_for_each_key_simple(skin, "skin_size", button_type_id(),
					skin_resize_setup_cb, skin);
}
