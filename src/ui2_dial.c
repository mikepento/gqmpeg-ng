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
#include "ui2_dial.h"
#include "ui2_dial_edit.h"

#include "ui2_display.h"
#include "ui2_item.h"
#include "ui2_main.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"

#include <math.h>

#ifndef PI
#define PI 3.14159
#endif

typedef struct _DialCallbackData DialCallbackData;
struct _DialCallbackData
{
	gfloat (*status_get_func)(DialData *dial, const gchar *key, gpointer data);
	void (*dial_press_func)(DialData *dial, const gchar *key, gfloat value, gpointer data);
	void (*dial_release_func)(DialData *dial, const gchar *key, gfloat value, gpointer data);
	void (*dial_drag_func)(DialData *dial, const gchar *key, gfloat value, gpointer data);

	gpointer status_get_data;
	gpointer dial_press_data;
	gpointer dial_release_data;
	gpointer dial_drag_data;
};


static WidgetType type_id = -1;


static void real_dial_idle_cancel(DialData *dial);


/*
 *-----------------------------
 * new / free
 *-----------------------------
 */

DialData *dial_new(GdkPixbuf *pb, gint x, gint y,
		   gint center_x, gint center_y,
		   gint angle_start, gint angle_end, gint offset_x, gint offset_y,
		   gint has_press, gint has_prelight, gint reversed,
		   gint clip_w, gint clip_h, GdkPixbuf *clip_pb,
		   const gchar *item_key)
{
	DialData *dial;
	gint width;
	gint height;
	gint max_radius;
	gint t;

	dial_type_init();

	if (!pb)
		{
		if (clip_pb) gdk_pixbuf_unref(clip_pb);
		return NULL;
		}

	util_size(&center_x);
	util_size(&center_y);
	util_size(&x);
	util_size(&y);
	util_size(&clip_w);
	util_size(&clip_h);
	util_size(&offset_x);
	util_size(&offset_y);

	dial = g_new0(DialData, 1);

	dial->overlay = util_size_pixbuf(pb, TRUE);

	width = gdk_pixbuf_get_width(dial->overlay);
	height = gdk_pixbuf_get_height(dial->overlay);

	dial->reversed = reversed;
	dial->angle_start = angle_start;
	dial->angle_end = angle_end;

	dial->angle_length = angle_end - angle_start;
	if (angle_end < angle_start) dial->angle_length += 360;

	dial->x = x;
	dial->y = y;

	dial->item_key = g_strdup(item_key);

	/* the pixbuf buffer contains the alpha of any clip mask */
	if (clip_pb && gdk_pixbuf_get_has_alpha(clip_pb))
		{
		dial->pixbuf = util_size_pixbuf(clip_pb, FALSE);
		dial->width = gdk_pixbuf_get_width(dial->pixbuf);
		dial->height = gdk_pixbuf_get_height(dial->pixbuf);
		clip_pb = NULL;
		dial->clip_mask = TRUE;
		pixbuf_alpha_force_to_bw(dial->pixbuf, 128);
		}
	else
		{
		dial->width = clip_w;
		dial->height = clip_h;
		dial->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, dial->width, dial->height);
		dial->clip_mask = FALSE;
		}
	if (clip_pb)
		{
		printf("skin warning: dial clip mask does not have alpha channel!\n");
		gdk_pixbuf_unref(clip_pb);
		}

	dial->handle_width = width;
	dial->handle_height = height / (1 + has_press + has_prelight);
	dial->offset_x = offset_x;
	dial->offset_y = offset_y;

	dial->has_press = has_press;
	dial->has_prelight = has_prelight;
	dial->pushed = FALSE;

	dial->center_x = center_x;
	dial->center_y = center_y;

	dial->fixed_handle = FALSE;
	dial->fixed_radius = 0;

	if (dial->reversed)
		{
		dial->position = dial->angle_length;
		dial->value = 1.0;
		}
	else
		{
		dial->position = 0.0;
		dial->value = 0.0;
		}

	max_radius = sqrt(abs(center_x - x) * abs(center_x - x) +
			  abs(center_y - y) * abs(center_y - y));
	t = sqrt(abs(center_x - (x + clip_w)) * abs(center_x - (x + clip_w))
		 + abs(center_y - (y + clip_w)) * abs(center_y - (y + clip_w)));
	if (t > max_radius) max_radius = t;

	dial->increment = (float)max_radius * PI / 360.0;
	dial->increment = floor(dial->increment * 1000.0) / 1000.0;

	if (debug_mode) printf("dial increment: %f (r = %d)\n", dial->increment, max_radius);

	dial->pb_normal = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, dial->handle_width, dial->handle_height);
	pixbuf_copy_area(dial->overlay, 0, 0,
			 dial->pb_normal, 0, 0, dial->handle_width, dial->handle_height, TRUE);

	if (has_press)
		{
		dial->pb_pressed = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, dial->handle_width, dial->handle_height);
		pixbuf_copy_area(dial->overlay, 0, dial->handle_height,
				 dial->pb_pressed, 0, 0, dial->handle_width, dial->handle_height, TRUE);
		}
	else
		{
		dial->pb_pressed = NULL;
		}

	if (has_prelight)
		{
		dial->pb_prelit = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, dial->handle_width, dial->handle_height);
		pixbuf_copy_area(dial->overlay, 0, dial->handle_height * (1 + has_press),
				 dial->pb_prelit, 0, 0, dial->handle_width, dial->handle_height, TRUE);
		}
	else
		{
		dial->pb_prelit = NULL;
		}

	dial->idle_id = -1;

	return dial;
}

DialData *dial_new_from_data(gchar **data, gint x, gint y,
			     gint center_x, gint center_y,
			     gint angle_start, gint angle_end, gint offset_x, gint offset_y,
			     gint has_press, gint has_prelight, gint reversed,
			     gint clip_w, gint clip_h, gchar **clip_data,
			     const gchar *item_key)
{
	GdkPixbuf *pb;
	GdkPixbuf *clip_pb;

	pb = gdk_pixbuf_new_from_xpm_data((const char **)data);

	if (clip_data)
		{
		clip_pb = gdk_pixbuf_new_from_xpm_data((const char **)clip_data);
		}
	else
		{
		clip_pb = NULL;
		}

	return dial_new(pb, x, y,
			center_x, center_y,
			angle_start, angle_end, offset_x, offset_y,
			has_press, has_prelight, reversed,
			clip_w, clip_h, clip_pb, item_key);
}

DialData *dial_new_from_file(const gchar *file, gint x, gint y,
			     gint center_x, gint center_y,
			     gint angle_start, gint angle_end, gint offset_x, gint offset_y,
			     gint has_press, gint has_prelight, gint reversed,
			     gint clip_w, gint clip_h, const gchar *clip_file,
			     const gchar *item_key)
{
	GdkPixbuf *pb;
	GdkPixbuf *clip_pb;

	pb = util_pixbuf_new_from_file(file);

	if (clip_file)
		{
		clip_pb = util_pixbuf_new_from_file(clip_file);
		}
	else
		{
		clip_pb = NULL;
		}

	return dial_new(pb, x, y,
			center_x, center_y,
			angle_start, angle_end, offset_x, offset_y,
			has_press, has_prelight, reversed,
			clip_w, clip_h, clip_pb, item_key);
}

void dial_set_fixed_handle(DialData *dial, gint fixed, gint radius)
{
	if (!dial) return;

	dial->fixed_handle = fixed;
	dial->fixed_radius = radius;
}

void dial_free(DialData *dial)
{
	if (!dial) return;

	real_dial_idle_cancel(dial);
	if (dial->pixbuf) gdk_pixbuf_unref(dial->pixbuf);
	if (dial->overlay) gdk_pixbuf_unref(dial->overlay);
	if (dial->pb_normal) gdk_pixbuf_unref(dial->pb_normal);
	if (dial->pb_pressed) gdk_pixbuf_unref(dial->pb_pressed);
	if (dial->pb_prelit) gdk_pixbuf_unref(dial->pb_prelit);
	g_free(dial);
}

static void dial_free_cb(gpointer data)
{
	dial_free((DialData *)data);
}

/*
 *-----------------------------
 * draw
 *-----------------------------
 */

static GdkPixbuf *dial_get_handle_pixbuf(DialData *dial, gint pressed, gint prelight)
{
	if (pressed && dial->has_press && dial->pb_pressed) return dial->pb_pressed;
	if (pressed && dial->has_prelight && dial->pb_prelit) return dial->pb_prelit;

	return dial->pb_normal;
}

static void dial_force_item_draw_cb(WidgetData *wd, gpointer data, UIData *ui)
{
	DialData *dial = data;
	ItemData *item = wd->widget;

	item->current = item_section_by_percent(item, dial->value);

	wd->hidden = FALSE;
	ui_widget_draw(ui, wd, FALSE, TRUE);
	wd->hidden = TRUE;
}

static void real_dial_render(DialData *dial, GdkPixbuf *pb, UIData *ui)
{
	GdkPixbuf *handle;

	if (dial->clip_mask)
		{
		pixbuf_copy_area_alpha(dial->pixbuf, 0, 0, pb,
				       dial->x, dial->y, dial->width, dial->height, 255);
		}
	else
		{
		pixbuf_copy_area(dial->pixbuf, 0, 0, pb,
				 dial->x, dial->y, dial->width, dial->height, FALSE);
		}

	if (dial->item_key)
		{
		ui_widget_for_each_key_one(ui, dial->item_key, item_type_id(),
					   dial_force_item_draw_cb, dial);
		}

	handle = dial_get_handle_pixbuf(dial, dial->pushed, dial->prelit);

	if (dial->fixed_handle && dial->fixed_radius > 0)
		{
		gint x, y;
		double theta;

		theta = ((double)dial->angle_start + dial->position + 90.0) / 180.0 * PI;
		x = dial->center_x + (sin(theta) * (double)dial->fixed_radius) - dial->offset_x;
		y = dial->center_y - (cos(theta) * (double)dial->fixed_radius) - dial->offset_y;

		pixbuf_copy_area_alpha_with_clipping(handle, 0, 0,
						     pb, x, y,
						     dial->handle_width, dial->handle_height, 255,
						     dial->x, dial->y, dial->width, dial->height,
						     dial->clip_mask ? dial->pixbuf : NULL);
		}
	else
		{
		pixbuf_copy_rotate_alpha_with_clipping(handle, dial->offset_x, dial->offset_y,
						       pb, dial->center_x, dial->center_y,
						       (double)((float)dial->angle_start + dial->position + 90.0),
						       dial->x, dial->y, dial->width, dial->height,
						       dial->clip_mask ? dial->pixbuf : NULL);
		}

	ui_display_render_area(ui, dial->x, dial->y, dial->width, dial->height, NULL);
}

static gint real_dial_idle_cb(gpointer data)
{
	DialData *dial = data;
	GdkPixbuf *pb;
	UIData *ui;

	ui = skin_get_ui(dial->skin);
	pb = skin_get_pixbuf(dial->skin);

	if (dial->idle_id != -1) real_dial_render(dial, pb, ui);
	dial->idle_id = -1;

	return FALSE;
}

static void real_dial_idle_set(DialData *dial, GdkPixbuf *pb, UIData *ui)
{
	if (dial->idle_id == -1)
		{
		dial->idle_id = gtk_idle_add(real_dial_idle_cb, dial);
		}
}

static void real_dial_idle_cancel(DialData *dial)
{
	if (dial->idle_id != -1)
		{
		gtk_idle_remove(dial->idle_id);
		dial->idle_id = -1;
		}
}

static void dial_draw_real(DialData *dial, gfloat value, gint move,
			   gint prelight, gint pushed, gint force, GdkPixbuf *pb, UIData *ui, gint idle)
{
	gfloat pos;
	GdkPixbuf *old_pb;
	GdkPixbuf *new_pb;

	if (!dial) return;

	if (move)
		{
		if (value < 0.0) value = 0.0;
		if (value > 1.0) value = 1.0;
		dial->value = value;

		pos = (float)dial->angle_length * dial->value;

		/* lock position to increments, helps cut down on expensive draws */
		pos = floor(pos / dial->increment) * dial->increment;

		if (pos < 0.0) pos = 0.0;
		if (pos > (float)dial->angle_length) pos = (float)dial->angle_length;
		}
	else
		{
		pos = dial->position;
		}

	old_pb = dial_get_handle_pixbuf(dial, dial->pushed, dial->prelit);

	if (force)
		{
		new_pb = old_pb;
		}
	else
		{
		new_pb = dial_get_handle_pixbuf(dial, pushed, prelight);
		dial->pushed = pushed;
		dial->prelit = (!dial->pushed && dial->has_prelight && prelight);
		}

	if (force || dial->position != pos || old_pb != new_pb)
		{
		dial->position = pos;
		if (idle)
			{
			real_dial_idle_set(dial, pb, ui);
			}
		else
			{
			real_dial_idle_cancel(dial);
			real_dial_render(dial, pb, ui);
			}
		}
}

static gfloat dial_value_public(DialData *dial)
{
	return (dial->reversed ? 1.0 - dial->value : dial->value);
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static void dial_draw(gpointer data, const gchar *key, gint update, gint force, GdkPixbuf *pb, UIData *ui)
{
	DialData *dial = data;
	gfloat value;

	value = dial->value;

	if (update)
		{
		DialCallbackData *cd;

		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->status_get_func)
			{
			value = cd->status_get_func(dial, key, cd->status_get_data);
			if (dial->reversed) value = 1.0 - value;

			}
		}

	if (force || value != dial->value)
		{
		if (value != dial->value)
			{
			dial_draw_real(dial, value, TRUE, dial->prelit, dial->pushed, FALSE, pb, ui, FALSE);
			}
		else
			{
			dial_draw_real(dial, 0.0, FALSE, FALSE, FALSE, TRUE, pb, ui, FALSE);
			}
		}
}

static void dial_reset(gpointer data, const gchar *key, GdkPixbuf *pb, UIData *ui)
{
	DialData *dial = data;

	dial_draw_real(dial, dial->value, FALSE, FALSE, FALSE, FALSE, pb, ui, FALSE);
}

static gfloat dial_angle_calc(DialData *dial, gint nx, gint ny)
{
	double a;
	gint x, y;

	x = nx - dial->center_x;
	y = ny - dial->center_y;

	a = atan2((double)y, (double)x);

	a = a / PI * 180.0;

	if (a < 0) a += 360.0;

	if (debug_mode) printf("dial calc angle: %f\n", a);

	return ((gfloat)a);
}

static gfloat dial_real_angle_calc(DialData *dial, gfloat angle)
{
	if (dial->angle_end < dial->angle_start)
		{
		if (angle < (float)dial->angle_end)
			{
			angle += (360.0 - (float)dial->angle_start);
			}
		else
			{
			angle -= (float)dial->angle_start;
			}
		}
	else
		{
		angle -= (float)dial->angle_start;
		}

	return (angle);
}

static gfloat dial_position_calc(DialData *dial, gfloat angle, gfloat press_angle)
{
	gfloat pos;

	angle -= press_angle;
	if (angle >= 360.0) angle -= 360.0;
	if (angle < 0.0) angle += 360.0;

	angle = dial_real_angle_calc(dial, angle);

	if (angle > (float)dial->angle_length) angle = (float)dial->angle_length;
	if (angle < 0.0) angle = 0.0;

	pos = angle / (float)dial->angle_length;

	/* don't allow movement beyond 33 % (to disallow jumping) */
	if (fabs(dial->position - angle) / (float)dial->angle_length > 0.33) pos = dial->position;

	if (pos < 0.0) pos = 0.0;
	if (pos > 1.0) pos = 1.0;

	if (debug_mode) printf("dial calc pos: %f (%f)\n", pos, angle);

	return (pos);
}

static gint dial_hit_test(DialData *dial, gint x, gint y)
{
	GdkPixbuf *pb;
	gint v;
	gint hx, hy;
	double theta;

	if (x < dial->x || x >= dial->x + dial->width ||
	    y < dial->y || y >= dial->y + dial->height) return FALSE;

	if (dial->clip_mask && !pixbuf_pixel_is_visible(dial->pixbuf, x - dial->x, y - dial->y)) return FALSE;

	theta = (dial->angle_start + dial->position + 90.0) / 180.0 * PI;

	if (dial->fixed_handle)
		{
		hx = dial->offset_x - ((dial->center_x - x) + (sin(theta) * (double)dial->fixed_radius));
		hy = dial->offset_y - ((dial->center_y - y) - (cos(theta) * (double)dial->fixed_radius));
		}
	else
		{
		hx = (float)dial->offset_x - 0.25 - ((dial->center_x - x) * cos(theta) - (y - dial->center_y) * sin(theta));
		hy = (float)dial->offset_y - 0.25 + ((dial->center_x - x) * sin(theta) + (y - dial->center_y) * cos(theta));
		}

	pb = dial_get_handle_pixbuf(dial, dial->pushed, dial->prelit);

	v = pixbuf_pixel_is_visible(pb, hx, hy);

	if (debug_mode) printf("dial calcs to %d , %d (%d)\n", hx, hy, v);

	return v;
}

static void dial_motion(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	DialData *dial = data;

	if (!dial) return;

	if (dial->pushed)
		{
		gfloat val;
		gfloat angle;

		angle = dial_angle_calc(dial, x, y);
		val = dial_position_calc(dial, angle, dial->press_angle);

		if (dial->value != val)
			{
			DialCallbackData *cd;

			dial_draw_real(dial, val, TRUE, FALSE, TRUE, FALSE, pb, ui, TRUE);

			cd = ui_get_registered_callbacks(ui, key, type_id);
			if (cd && cd->dial_drag_func)
				{
				cd->dial_drag_func(dial, key, dial_value_public(dial), cd->dial_drag_data);
				}
			}

		return;
		}

	if (dial_hit_test(dial, x, y))
		{
		dial_draw_real(dial, 0.0, FALSE, TRUE, FALSE, FALSE, pb, ui, FALSE);
		}
	else
		{
		dial_draw_real(dial, 0.0, FALSE, FALSE, FALSE, FALSE, pb, ui, FALSE);
		}
}

static gint dial_press(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	DialData *dial = data;

	if (!dial) return FALSE;

	if (dial_hit_test(dial, x, y))
		{
		DialCallbackData *cd;
		gfloat val;
		gfloat angle;

		angle = dial_angle_calc(dial, x, y);
		dial->press_angle = dial_real_angle_calc(dial, angle) - dial->position;
		val = dial_position_calc(dial, angle, dial->press_angle);

		dial_draw_real(dial, val, TRUE, FALSE, TRUE, FALSE, pb, ui, FALSE);
		
		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->dial_press_func)
			{
			cd->dial_press_func(dial, key, dial_value_public(dial), cd->dial_press_data);
			}
		return TRUE;
		}

	return FALSE;
}

static void dial_release(gpointer data, const gchar *key, gint x , gint y, GdkPixbuf *pb, UIData *ui)
{
	DialData *dial = data;
	DialCallbackData *cd;
	gfloat val;
	gfloat angle;

	if (!dial || !dial->pushed) return;

	angle = dial_angle_calc(dial, x, y);
	val = dial_position_calc(dial, angle, dial->press_angle);

	if (dial_hit_test(dial, x, y))
		{
		dial_draw_real(dial, val, TRUE, TRUE, FALSE, FALSE, pb, ui, FALSE);
		}
	else
		{
		dial_draw_real(dial, val, TRUE, FALSE, FALSE, FALSE, pb, ui, FALSE);
		}

	cd = ui_get_registered_callbacks(ui, key, type_id);
	if (cd && cd->dial_release_func)
		{
		cd->dial_release_func(dial, key, dial_value_public(dial), cd->dial_release_data);
		}
}

static void dial_back_set(gpointer data, GdkPixbuf *pb)
{
	DialData *dial = data;

	if (!dial) return;

	real_dial_idle_cancel(dial);
	pixbuf_copy_area(pb, dial->x, dial->y, dial->pixbuf, 0, 0,
			 dial->width, dial->height, FALSE);
}

static gint dial_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	DialData *dial = widget;

	*x = dial->x;
	*y = dial->y;
	*w = dial->width;
	*h = dial->height;

	return TRUE;
}

static void dial_set_coord(gpointer widget, gint x, gint y)
{
	DialData *dial = widget;

	dial->center_x += x - dial->x;
	dial->center_y += y - dial->y;
	dial->x = x;
	dial->y = y;
}

static WidgetData *dial_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	DialData *dial;
	gchar *filename;
	gint center_x, center_y;
	gint angle_begin, angle_end;
	gint offset_x, offset_y;
	gint has_press;
	gint prelight;
	gint reverse;
	gint x, y;
	gint clip_w, clip_h;
	gchar *clip_file;
	gint clip_success;
	gint fixed_handle;
	gint fixed_radius;
	const gchar *item_key;

	/* req */
	if (!key_list_read_int(list, "axis_x", &center_x)) return NULL;
	if (!key_list_read_int(list, "axis_y", &center_y)) return NULL;

	if (!key_list_read_int(list, "angle_begin", &angle_begin)) return NULL;
	if (!key_list_read_int(list, "angle_end", &angle_end)) return NULL;

	if (!key_list_read_int(list, "handle_offset_x", &offset_x)) return NULL;
	if (!key_list_read_int(list, "handle_offset_y", &offset_y)) return NULL;

	if (!key_list_read_int(list, "clip_x", &x)) return NULL;
	if (!key_list_read_int(list, "clip_y", &y)) return NULL;

	filename = key_list_read_path(list, "image", skin_dir);
	if (!filename) return NULL;

	clip_w = clip_h = 1;
	clip_success = (key_list_read_int(list, "clip_width", &clip_w) && key_list_read_int(list, "clip_height", &clip_h));
	clip_file = key_list_read_path(list, "clip_mask", skin_dir);
	if (!clip_file && !clip_success)
		{
		g_free(filename);
		return NULL;
		}
	key_list_read_int(list, "clip_width", &clip_w);
	key_list_read_int(list, "clip_height", &clip_h);

	/* opt */
	has_press = key_list_read_bool(list, "pressable");
	prelight = key_list_read_bool(list, "prelight");
	reverse = key_list_read_bool(list, "reversed");
	item_key = key_list_read_chars(list, "item_link", NULL);
	fixed_handle = key_list_read_bool(list, "fixed_handle");
	if (!key_list_read_int(list, "fixed_radius", &fixed_radius)) fixed_radius = 0;

	dial = dial_new_from_file(filename, x, y,
				  center_x, center_y,
				  angle_begin, angle_end, offset_x, offset_y,
				  has_press, prelight, reverse,
				  clip_w, clip_h, clip_file, item_key);
	if (dial)
		{
		if (fixed_handle && fixed_radius > 0)
			{
			dial_set_fixed_handle(dial, fixed_handle, fixed_radius);
			}
		wd = dial_register(skin, dial, key, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			ui_widget_set_data(wd, "clip_mask", clip_file);
			}
		}

	g_free(filename);
	g_free(clip_file);

	return wd;
}


/*
 *-----------------------------
 * register ui / app side
 *-----------------------------
 */

WidgetData *dial_register(SkinData *skin, DialData *dial, const gchar *key, const gchar *text_id)
{
	dial->skin = skin;
        return skin_register_widget(skin, key, text_id, type_id, dial);
}

WidgetData *dial_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				  gint center_x, gint center_y,
				  gint angle_start, gint angle_end, gint offset_x, gint offset_y,
				  gint has_press, gint has_prelight, gint reversed,
				  gint clip_w, gint clip_h, gchar **clip_data,
				  const gchar *item_key,
				  SkinData *skin, const gchar *text_id)
{
	DialData *dial;

	dial = dial_new_from_data(data, x, y, center_x, center_y,
				  angle_start, angle_end, offset_x, offset_y,
				  has_press, has_prelight, reversed,
				  clip_w, clip_h, clip_data, item_key);
	return dial_register(skin, dial, key, text_id);
}

RegisterData *dial_register_key(const gchar *key, UIData *ui,
				gfloat (*status_get_func)(DialData *dial, const gchar *key, gpointer data), gpointer status_get_data,
				void (*dial_press_func)(DialData *dial, const gchar *key, gfloat value, gpointer data), gpointer dial_press_data,
				void (*dial_release_func)(DialData *dial, const gchar *key, gfloat value, gpointer data), gpointer dial_release_data,
				void (*dial_drag_func)(DialData *dial, const gchar *key, gfloat value, gpointer data), gpointer dial_drag_data)
{
	DialCallbackData *cd;

	dial_type_init();

	cd = g_new0(DialCallbackData, 1);

	cd->status_get_func = status_get_func;
	cd->dial_press_func = dial_press_func;
	cd->dial_release_func = dial_release_func;
	cd->dial_drag_func = dial_drag_func;

	cd->status_get_data = status_get_data;
	cd->dial_press_data = dial_press_data;
	cd->dial_release_data = dial_release_data;
	cd->dial_drag_data = dial_drag_data;

	return ui_register_key(ui, key, type_id, cd, sizeof(DialCallbackData));
}

/*
 *-----------------------------
 * app funcs
 *-----------------------------
 */

static void dial_value_set_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	DialData *dial;
	gfloat *value = data;

	dial = wd->widget;

	if (!dial->pushed)
		{
		dial_draw_real(dial, dial->reversed ? 1.0 - (*value) : *value, TRUE,
			       dial->prelit, dial->pushed, FALSE, pb, ui, FALSE);
		}
}

gint dial_value_set(const gchar *key, UIData *ui, gfloat value)
{
	gint ret;

	ret = skin_widget_for_each_key(ui, key, type_id, dial_value_set_cb, &value);

	return ret;
}

gfloat dial_value_get(const gchar *key, UIData *ui)
{
	WidgetData *wd;
	DialData *dial;

	wd = skin_widget_get_by_key(ui->skin, key, type_id);
	if (!wd) return -1.0;

	dial = wd->widget;

	return dial_value_public(dial);
}

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType dial_type_id(void)
{
	return type_id;
}

void dial_type_init(void)
{
	WidgetObjectData *od;

	if (type_id != -1) return;

	od = ui_widget_type_new("dial");
	type_id = od->type;

	od->func_draw = dial_draw;
	od->func_reset = dial_reset;
	od->func_press = dial_press;
	od->func_release = dial_release;
	od->func_motion = dial_motion;
	od->func_back = dial_back_set;
	od->func_free = dial_free_cb;

	od->func_get_geometry = dial_get_geometry;
	od->func_set_coord = dial_set_coord;

	od->func_parse = dial_parse;

	dial_type_init_edit(od);
}

