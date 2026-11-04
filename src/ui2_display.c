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
#include "ui2_display.h"

#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"

#include <gdk/gdkkeysyms.h> /* for key values */

#include <math.h>


/*
 *-----------------------------------------------------------------------------
 * Screen refreshing
 *-----------------------------------------------------------------------------
 */

/* ok, so the freeze is ugly, but does help when first loading a skin */

static void ui_display_render_freeze(UIData *ui)
{
	ui->frozen = TRUE;
}

static void ui_display_render_thaw(UIData *ui)
{
	ui->frozen = FALSE;
}

void ui_display_render_area(UIData *ui, gint x, gint y, gint w, gint h, WidgetData *wd)
{
	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap;

	if (ui->frozen) return;
	if (w < 1 || h < 1) return;

	pixbuf = skin_get_pixbuf(ui->skin);
	pixmap = skin_get_buffer(ui->skin);

	if (!pixbuf || !pixmap || !ui->display->window) return;

	if (wd && wd == ui->focus_widget && (slik_focus_enable || ui->focus_enable) &&
	    GTK_WIDGET_HAS_FOCUS(ui->display))
		{
		gint wx, wy, ww, wh;
		if (!ui_widget_focus_draw(ui, wd, x, y, w, h) &&
		    ui_widget_get_geometry(wd, &wx, &wy, &ww, &wh))
			{
			ui_display_draw_focus(ui, pixbuf, wx, wy, ww, wh, x, y, w, h, NULL);
			}
		}

	if (debug_mode) printf("render: %d, %d (%d x %d)\n", x, y, w, h);

	gdk_pixbuf_render_to_drawable(pixbuf, pixmap,
				      ui->display->style->fg_gc[GTK_WIDGET_STATE(ui->display)],
				      x, y, x, y, w, h,
				      GDK_RGB_DITHER_NONE, 1, 1); /* FIXME: no dithering */

	gdk_draw_drawable(ui->display->window, ui->display->style->fg_gc[GTK_WIDGET_STATE(ui->display)],
			  pixmap, x, y, x, y, w, h);
}

void ui_display_redraw_area(UIData *ui, gint x, gint y, gint w, gint h, gint update)
{
	GList *work;

	pixbuf_copy_area(ui->skin->overlay, x, y,
			 ui->skin->pixbuf, x, y, w, h, FALSE);
	ui_display_render_freeze(ui);

	work = ui->skin->widget_list;
	while(work)
		{
		WidgetData *wd = work->data;
		if (ui_widget_contacts_area(wd, x, y, w, h))
			{
			ui_widget_draw(ui, wd, update, TRUE);
			}
		work = work->next;
		}

	ui_display_render_thaw(ui);
	ui_display_render_area(ui, x, y, w, h, ui->focus_widget);
}

void ui_display_draw_focus(UIData *ui, GdkPixbuf *pixbuf, gint x, gint y, gint w, gint h,
			   gint clip_x, gint clip_y, gint clip_w, gint clip_h, GdkPixbuf *clip_pb)
{
	gint rx, ry, rw, rh;
	SkinData *skin;

	if (!util_clip_region(x, y, w, h,
			      clip_x, clip_y, clip_w, clip_h,
			      &rx, &ry, &rw, &rh)) return;

	skin = ui->skin;

	if (skin->focus_overlay)
		{
		if (skin->focus_has_border)
			{
			util_pixbuf_copy_border_clipped(skin->focus_overlay,
							skin->focus_border_left, skin->focus_border_right,
							skin->focus_border_top, skin->focus_border_bottom,
							pixbuf, x, y, w, h,
							clip_x, clip_y, clip_w, clip_h,
							skin->focus_box_alpha);
			}
		else
			{
			gint fx, fy, fw, fh;
			gint bx, by, bw, bh;

			fw = gdk_pixbuf_get_width(skin->focus_overlay);
			fh = gdk_pixbuf_get_height(skin->focus_overlay);

			fx = (skin->focus_anchor_right) ? MAX((x + w - fw), 0) : x;
			fy = (skin->focus_anchor_bottom) ? MAX((y + h - fh), 0) : y;

			if (util_clip_region(rx, ry, rw, rh, fx, fy, fw, fh,
					     &bx, &by, &bw, &bh))
				{
				pixbuf_copy_area_alpha(skin->focus_overlay, 0, 0,
						       pixbuf, bx, by, bw, bh, skin->focus_box_alpha);
				}
			}
		}
	else
		{
		guint8 r, g, b, a;

		r = skin->focus_box_r;
		g = skin->focus_box_g;
		b = skin->focus_box_b;
		a = skin->focus_box_alpha;

		if (skin->focus_box_filled)
			{
			/* filled */
			pixbuf_draw_rect_fill(pixbuf, rx, ry, rw, rh, r, g, b, a);
			}
		else
			{
			gint bx, by, bw, bh;
			if (util_clip_region(rx, ry, rw, rh, x, y, w, 2,
					     &bx, &by, &bw, &bh))
				{
				pixbuf_draw_rect_fill(pixbuf, bx, by, bw, bh, r, g, b, a);
				}
			if (util_clip_region(rx, ry, rw, rh, x, y + h - 2, w, 2,
					     &bx, &by, &bw, &bh))
				{
				pixbuf_draw_rect_fill(pixbuf, bx, by, bw, bh, r, g, b, a);
				}
			if (util_clip_region(rx, ry, rw, rh, x, y + 2, 2, h - 4,
					     &bx, &by, &bw, &bh))
				{
				pixbuf_draw_rect_fill(pixbuf, bx, by, bw, bh, r, g, b, a);
				}
			if (util_clip_region(rx, ry, rw, rh, x + w - 2, y + 2, 2, h - 4,
					     &bx, &by, &bw, &bh))
				{
				pixbuf_draw_rect_fill(pixbuf, bx, by, bw, bh, r, g, b, a);
				}
			}
		}
}

/*
 *-----------------------------------------------------------------------------
 * misc
 *-----------------------------------------------------------------------------
 */

void ui_display_sync_shape(UIData *ui)
{
	if (!ui->skin) return;

	if (ui->window)
		{
		if (slik_allow_shapes)
			{
			gtk_widget_shape_combine_mask(ui->window, ui->skin->mask_buffer, 0, 0);
			}
		gtk_widget_set_size_request(ui->display, ui->skin->width, ui->skin->height);
		gtk_window_resize(GTK_WINDOW(ui->window), ui->skin->width, ui->skin->height);

		/* wonder why this is here? Try removing it and changing skin to a new size */
		gdk_window_resize(ui->display->window, ui->skin->width, ui->skin->height);
		gdk_window_resize(ui->window->window, ui->skin->width, ui->skin->height);
		}
	else
		{
		if (slik_allow_shapes)
			{
			gtk_widget_shape_combine_mask(ui->display, ui->skin->mask_buffer, 0, 0);
			}
		gtk_widget_set_size_request(ui->display, ui->skin->width, ui->skin->height);
		}
}

static void ui_display_draw_all(UIData *ui, gint update, gint force)
{
	GList *work;

	work = ui->skin->widget_list;
	while(work)
		{
		ui_widget_draw(ui, work->data, update, force);
		work = work->next;
		}
}

void ui_display_sync(UIData *ui, gint update)
{
	if (!ui->skin) return;

	ui_display_render_freeze(ui);
	ui_display_draw_all(ui, update, TRUE);
	ui_display_render_thaw(ui);

	/* do it all at once */
	ui_display_render_area(ui, 0, 0, ui->skin->width, ui->skin->height, ui->focus_widget);
}

void ui_display_sync_all(UIData *ui)
{
	if (!ui->skin) return;

	skin_check_bounds(ui->skin);

	skin_sync_buffer(ui->skin, ui);
	ui_display_sync_shape(ui);

	ui_display_sync(ui, TRUE);
}

static void ui_display_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	UIData *ui = data;
	GdkPixmap *pixmap;

	pixmap = skin_get_buffer(ui->skin);
	if (!pixmap) return;

	gdk_draw_drawable(ui->display->window, ui->display->style->fg_gc[GTK_WIDGET_STATE(ui->display)],
			  pixmap,
			  (gint)event->area.x, (gint)event->area.y,
			  (gint)event->area.x, (gint)event->area.y,
			  (gint)event->area.width, (gint)event->area.height);
}

static gint ui_display_move_idle_cb(gpointer data)
{
	UIData *ui = data;

	skin_sync_back(ui->skin, ui);
	ui_display_sync(ui, FALSE);

	ui->root_win_idle = -1;

	return FALSE;
}

static void ui_display_move(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	UIData *ui = data;

	if (debug_mode) printf("move event\n");

	if (ui->root_win_idle == -1 &&
	    (ui->skin->transparent || slik_transparency_force))
		{
		ui->root_win_idle = gtk_idle_add(ui_display_move_idle_cb, ui);
		}
}

/*
 *-----------------------------------------------------------------------------
 * Focus (keyboard stuff)
 *-----------------------------------------------------------------------------
 */

typedef enum {
	UI_FOCUS_NONE = 0,
	UI_FOCUS_LEFT,
	UI_FOCUS_UP,
	UI_FOCUS_RIGHT,
	UI_FOCUS_DOWN,
	UI_FOCUS_TAB_FORWARD,
	UI_FOCUS_TAB_BACKWARD
} UIMoveFocusType;

#define DIST(x, y) (sqrt(x * x + y * y))

static WidgetData *ui_display_focus_nearest(UIData *ui, gint x, gint y)
{
	WidgetData *wd = NULL;
	gint dx, dy;
	GList *work;

	if (!slik_focus_enable && !ui->focus_enable) return NULL;

	dx = ui->skin->width;
	dy = ui->skin->height;

	work = ui->skin->widget_list;
	while (work)
		{
		WidgetData *tmp;
		gint tx, ty, tw, th;

		tmp = work->data;
		work = work->next;
		if (ui_widget_can_focus(tmp) && 
		    ui_widget_get_geometry(tmp, &tx, &ty, &tw, &th))
			{
			tx = tx + (tw / 2);
			ty = ty + (th / 2);
			if (DIST((x - tx), (y - ty)) < DIST(dx, dy))
				{
				dx = abs(x - tx);
				dy = abs(y - ty);
				wd = tmp;
				}
			}
		}

	return wd;
}

static WidgetData *ui_display_focus_nearest_clipped(UIData *ui, WidgetData *current,
						    gint clip_x, gint clip_y, gint clip_w, gint clip_h)
{
	WidgetData *wd = NULL;
	gint cx, cy, cw, ch;

	if (current && ui_widget_get_geometry(current, &cx, &cy, &cw, &ch))
		{
		gint dx, dy;
		gint px, py;
		GList *work;

		px = cx + (cw / 2);
		py = cy + (ch / 2);

		dx = ui->skin->width;
		dy = ui->skin->height;

		work = ui->skin->widget_list;
		while (work)
			{
			WidgetData *tmp;
			gint x, y, w, h;

			tmp = work->data;
			work = work->next;
			if (current != tmp && ui_widget_can_focus(tmp) &&
			    ui_widget_get_geometry(tmp, &x, &y, &w, &h))
				{
				gint dmy;
				if (util_clip_region(clip_x, clip_y, clip_w, clip_h,
						     x, y, w, h, &dmy, &dmy, &dmy, &dmy))
					{
					gint tx, ty;
					gint ax, ay;

					tx = x + (w / 2);
					ty = y + (h / 2);

					ax = abs(tx - px);
					ay = abs(ty - py);

					if (DIST(ax, ay) < DIST(dx, dy))
						{
						dy = ay;
						dx = ax;
						wd = tmp;
						}
					}
				}
			}
		}

	return wd;
}

static WidgetData *ui_display_focus_find(UIData *ui, WidgetData *current, gint x_dir, gint y_dir)
{
	WidgetData *wd = NULL;
	gint cx, cy, cw, ch;

	if (!slik_focus_enable && !ui->focus_enable) return NULL;

	if (x_dir == 0 && y_dir == 0) return current;

	if (current && ui_widget_get_geometry(current, &cx, &cy, &cw, &ch))
		{
		/* create a region that we are (trying to) move into */
		if (x_dir < 0)
			{
			cw = cx - 1;
			cx = 0;
			}
		else if (x_dir > 0)
			{
			cx += cw + 1;
			cw = ui->skin->width - cx;
			}
		if (y_dir < 0)
			{
			ch = cy - 1;
			cy = 0;
			}
		else if (y_dir > 0)
			{
			cy += ch + 1;
			ch = ui->skin->height - cy;
			}

		wd = ui_display_focus_nearest_clipped(ui, current, cx, cy, cw, ch);

		if (!wd)
			{
			/* try it all again, this time with a broader range */
			if (x_dir != 0)
				{
				cy = 0;
				ch = ui->skin->height;
				}
			if (y_dir != 0)
				{
				cx = 0;
				cw = ui->skin->width;
				}

			wd = ui_display_focus_nearest_clipped(ui, current, cx, cy, cw, ch);
			}

		}
	else
		{
		wd = ui_display_focus_nearest(ui, 0, 0);
		}

	if (!wd) wd = current;

	return wd;
}

static WidgetData *ui_display_focus_next(UIData *ui, WidgetData *current, gint forward)
{
	GList *start = NULL;
	GList *work;

	if (current)
		{
		start = g_list_find(ui->skin->widget_list, current);
		}
	if (!start)
		{
		if (forward)
			{
			start = ui->skin->widget_list;
			}
		else
			{
			start = g_list_last(ui->skin->widget_list);
			}
		}
	if (!start) return current;

	if (forward)
		{
		work = start->next;
		}
	else
		{
		work = start->prev;
		}

	while (work)
		{
		WidgetData *wd;

		wd = work->data;
		if (current != wd && ui_widget_can_focus(wd)) return wd;

		if (forward)
			{
			work = work->next;
			}
		else
			{
			work = work->prev;
			}
		}

	return current;
}

static void ui_display_focus_clear(UIData *ui)
{
	if (ui->focus_widget)
		{
		gint x, y, w, h;

		if (ui_widget_get_geometry(ui->focus_widget, &x, &y, &w, &h))
			{
			ui->focus_widget = NULL;
			ui_display_redraw_area(ui, x, y, w, h, FALSE);
			}
		else
			{
			ui->focus_widget = NULL;
			}
		}
}


static gint ui_display_focus_move(UIData *ui, UIMoveFocusType focus)
{
	WidgetData *new_widget;
	gint x, y, w, h;

	if (!slik_focus_enable && !ui->focus_enable) return FALSE;

	new_widget = NULL;
	switch (focus)
		{
		case UI_FOCUS_LEFT:
			new_widget = ui_display_focus_find(ui, ui->focus_widget, -1, 0);
			break;
		case UI_FOCUS_UP:
			new_widget = ui_display_focus_find(ui, ui->focus_widget, 0, -1);
			break;
		case UI_FOCUS_RIGHT:
			new_widget = ui_display_focus_find(ui, ui->focus_widget, 1, 0);
			break;
		case UI_FOCUS_DOWN:
			new_widget = ui_display_focus_find(ui, ui->focus_widget, 0, 1);
			break;
		case UI_FOCUS_TAB_FORWARD:
			new_widget = ui_display_focus_next(ui, ui->focus_widget, TRUE);
			break;
		case UI_FOCUS_TAB_BACKWARD:
			new_widget = ui_display_focus_next(ui, ui->focus_widget, FALSE);
			break;
		case UI_FOCUS_NONE:
		default:
			break;
		}

	if (new_widget == ui->focus_widget) return FALSE;

	ui_display_focus_clear(ui);

	ui->focus_widget = new_widget;
	if (new_widget &&
	    ui_widget_get_geometry(new_widget, &x, &y, &w, &h))
		{
		ui_display_render_area(ui, x, y, w, h, new_widget);
		}

	return TRUE;
}

static gint ui_display_key_press(GtkWidget *w, GdkEventKey *event, gpointer data)
{
	UIData *ui = data;
	gint ret = FALSE;

	if (!ui->skin) return FALSE;

	/* pop up menus should work _always_, so test before poss. bail out */
	if (event->keyval == GDK_F10 ||
	    event->keyval == GDK_Menu)
		{
		if (ui->click_func)
			{
			/* hmm, perhaps the function table needs a menu callback
			 * instead of synthesizing a button 3 press ? 
			 */
			ui->click_func(ui, 3, event->time, ui->click_data);
			}
		return TRUE;
		}

	if (!slik_focus_enable && !ui->focus_enable) return FALSE;

	if (ui->active_widget) return FALSE;

	if (ui->focus_widget &&
	    ui_widget_focus_key_event(ui, ui->focus_widget, event))
		{
		ret = TRUE;
		}
	else
		{
		switch (event->keyval)
			{
			case GDK_Left: case GDK_KP_Left:
				ret = ui_display_focus_move(ui, UI_FOCUS_LEFT);
				break;
			case GDK_Right: case GDK_KP_Right:
				ret = ui_display_focus_move(ui, UI_FOCUS_RIGHT);
				break;
			case GDK_Up: case GDK_KP_Up:
				ret = ui_display_focus_move(ui, UI_FOCUS_UP);
				break;
			case GDK_Down: case GDK_KP_Down:
				ret = ui_display_focus_move(ui, UI_FOCUS_DOWN);
				break;
			case GDK_ISO_Left_Tab:
				ret = ui_display_focus_move(ui, UI_FOCUS_TAB_BACKWARD);
				break;
			case GDK_Tab:
				ret = ui_display_focus_move(ui, UI_FOCUS_TAB_FORWARD);
				break;
			default:
				break;
			}
		}

#if 0
	if (ret) gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "key_press_event");
#endif

	return ret;
}

static gint ui_display_scrolled(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	UIData *ui = data;
	gint button = -1;

	if (event->direction == GDK_SCROLL_UP)
		{
		button = 4;
		}
	else if (event->direction == GDK_SCROLL_DOWN)
		{
		button = 5;
		}

	if (ui->click_func && button > 0)
		{
		ui->click_func(ui, button, event->time, ui->click_data);
		}

	return TRUE;
}

static gint ui_display_focus_in(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	UIData *ui = data;

	GTK_WIDGET_SET_FLAGS(widget, GTK_HAS_FOCUS);

	if (!ui->skin || (!slik_focus_enable && !ui->focus_enable)) return FALSE;

	if (!ui->focus_widget) ui->focus_widget = ui_display_focus_next(ui, NULL, TRUE);
	if (ui->focus_widget)
		{
		gint x, y, w, h;

		if (ui_widget_get_geometry(ui->focus_widget, &x, &y, &w, &h))
			{
			ui_display_render_area(ui, x, y, w, h, ui->focus_widget);
			}
		}

	return FALSE;
}

static gint ui_display_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	UIData *ui = data;

	GTK_WIDGET_UNSET_FLAGS(widget, GTK_HAS_FOCUS);

	if (!ui->skin || (!slik_focus_enable && !ui->focus_enable)) return FALSE;

	if (ui->focus_widget)
		{
		gint x, y, w, h;

		if (ui_widget_get_geometry(ui->focus_widget, &x, &y, &w, &h))
			{
			ui_display_redraw_area(ui, x, y, w, h, FALSE);
			}
		}

	return FALSE;
}

/*
 *-----------------------------------------------------------------------------
 * Button motion / press handling
 *-----------------------------------------------------------------------------
 */

static gint ui_display_motion(GtkWidget *w, GdkEventMotion *event, gpointer data)
{
	UIData *ui = data;
	gint x = (gint)event->x;
	gint y = (gint)event->y;
	GList *work;

	if (debug_skin) printf("motion:%d x %d\n", x, y);

	if (!ui->skin) return FALSE;

	if (ui->active_widget)
		{
		ui_widget_motion(ui, ui->active_widget, x, y);
		return FALSE;
		}

	if (ui->in_move)
		{
		GdkModifierType modmask;
		gint px, py;

		if (!ui->window) return FALSE;

		gdk_window_get_pointer (NULL, &px, &py, &modmask);
		px -= ui->press_x;
		py -= ui->press_y;
		if (slik_smart_placement &&
		    !(event->state & GDK_SHIFT_MASK))
			{
			gint w = gdk_screen_width();
			gint h = gdk_screen_height();
			if (px < 8 && px > -8) px = 0;
			if (py < 8 && py > -8) py = 0;
			if (px + ui->skin->width > w - 8 && px + ui->skin->width < w + 8) px = w - ui->skin->width;
			if (py + ui->skin->height > h - 8 && py + ui->skin->height < h +8) py = h - ui->skin->height;
			}
		gdk_window_move(ui->window->window, px, py);
		return TRUE;
		}

	work = ui->skin->widget_list;
	while (work)
		{
		ui_widget_motion(ui, work->data, x, y);
		work = work->next;
		}

	return FALSE;
}

static gint ui_display_pressed(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	UIData *ui = data;
	GdkModifierType modmask;
	gint x = (gint)event->x;
	gint y = (gint)event->y;
	GList *work;

	if (debug_mode) printf("pressed:%d x %d, %d\n", x, y, event->button);

	if (!ui->skin) return FALSE;

	/* edit uis have no window, and we only want button 1 to work on them */
	if (event->button > 1 && !ui->window) return FALSE;

	if (event->button >= 2)
		{
		if (ui->click_func)
			{
			ui->click_func(ui, event->button, event->time, ui->click_data);
			return TRUE;
			}
		return FALSE;
		}

	if (event->type != GDK_BUTTON_PRESS) return FALSE;

	gtk_grab_add(ui->display);

	ui->press_x = x;
	ui->press_y = y;

	work = ui->skin->widget_list;
	while (work)
		{
		if (ui_widget_press(ui, work->data, x, y))
			{
			ui->active_widget = work->data;
			if (debug_mode) printf("pressed widget: \"%s\" (%d)\n", ui->active_widget->key, ui->active_widget->type);

			if ((slik_focus_enable || ui->focus_enable) &&
			    ui_widget_can_focus(ui->active_widget))
				{
				if (ui->focus_widget != ui->active_widget)
					{
					gint x, y, w, h;
					ui_display_focus_clear(ui);
					ui->focus_widget = ui->active_widget;
					if (ui_widget_get_geometry(ui->focus_widget, &x, &y, &w, &h))
						{
						ui_display_render_area(ui, x, y, w, h, ui->focus_widget);
						}
					}
				if (!GTK_WIDGET_HAS_FOCUS(ui->display))
					{
					gtk_widget_grab_focus(ui->display);
					}
				}

			return TRUE;
			}
		work = work->next;
		}

	if (ui->allow_move)
		{
		ui->in_move = TRUE;
		gdk_window_get_pointer (NULL, &ui->press_root_x, &ui->press_root_y, &modmask);
		}
	ui->in_press = (event->x >= 0 && event->x <= ui->skin->width &&
			event->y >= 0 && event->y <= ui->skin->height);

	return ui->in_press;
}

static gint ui_display_released(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	UIData *ui = data;
	gint x = (gint)event->x;
	gint y = (gint)event->y;

	if (debug_mode) printf("released:%d x %d, %d\n", x, y, event->button);

	if (!ui->skin) return FALSE;
	if (!ui->in_press && !ui->active_widget) return FALSE;

	gtk_grab_remove(ui->display);

	if (ui->active_widget)
		{
		if (debug_mode) printf("releasing widget: \"%s\" (%d)\n", ui->active_widget->key, ui->active_widget->type);
		ui_widget_release(ui, ui->active_widget, x, y);
		ui->active_widget = NULL;
		}
	else
		{
		GdkModifierType modmask;
		gint px;
		gint py;
		gdk_window_get_pointer (NULL, &px, &py, &modmask);
		if (px == ui->press_root_x && py == ui->press_root_y && ui->window)
			{
			gdk_window_raise (ui->window->window);
			}
		}

	ui->in_press = FALSE;
	ui->in_move = FALSE;

	return FALSE;
}

static void ui_display_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	UIData *ui = data;

	if (!ui->skin) return;

	if (!ui->active_widget)
		{
		GList *work;

		work = ui->skin->widget_list;
		while(work)
			{
			ui_widget_reset(ui, work->data);
			work = work->next;
			}
		}
}

void ui_display_events_init(UIData *ui)
{
	GTK_WIDGET_SET_FLAGS(ui->display, GTK_CAN_FOCUS);

	gtk_widget_set_events(ui->display, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
			      GDK_POINTER_MOTION_MASK | 
			      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	g_signal_connect(G_OBJECT(ui->display),"motion_notify_event",
			 G_CALLBACK(ui_display_motion), ui);
	g_signal_connect(G_OBJECT(ui->display),"button_press_event",
			 G_CALLBACK(ui_display_pressed), ui);
	g_signal_connect(G_OBJECT(ui->display),"button_release_event",
			 G_CALLBACK(ui_display_released), ui);
	g_signal_connect(G_OBJECT(ui->display),"leave_notify_event",
			 G_CALLBACK(ui_display_leave), ui);

	g_signal_connect(G_OBJECT(ui->display),"key_press_event",
			 G_CALLBACK(ui_display_key_press), ui);
	g_signal_connect(G_OBJECT(ui->display),"scroll_event",
			 G_CALLBACK(ui_display_scrolled), ui);

	g_signal_connect(G_OBJECT(ui->display),"focus_in_event",
			 G_CALLBACK(ui_display_focus_in), ui);
	g_signal_connect(G_OBJECT(ui->display),"focus_out_event",
			 G_CALLBACK(ui_display_focus_out), ui);

	ui->active_widget = NULL;
	ui->in_press = FALSE;
	ui->in_move = FALSE;

	/* the expose event */
	g_signal_connect(GTK_OBJECT(ui->display), "expose_event",
			   G_CALLBACK(ui_display_expose), ui);
	if (ui->window)
		{
		g_signal_connect(GTK_OBJECT(ui->window), "configure_event",
				   G_CALLBACK(ui_display_move), ui);
		}
}

