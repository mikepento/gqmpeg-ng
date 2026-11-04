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
#include "ui2_slider.h"
#include "ui2_slider_edit.h"

#include "ui2_display.h"
#include "ui2_item.h"
#include "ui2_main.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"

#include <gdk/gdkkeysyms.h> /* for key values */


#define SLIDER_KEY_RELEASE_TIME 333


typedef struct _SliderCallbackData SliderCallbackData;
struct _SliderCallbackData
{
	gfloat (*status_get_func)(SliderData *slider, const gchar *key, gpointer data);
	void (*slider_press_func)(SliderData *slider, const gchar *key, gfloat value, gpointer data);
	void (*slider_release_func)(SliderData *slider, const gchar *key, gfloat value, gpointer data);
	void (*slider_drag_func)(SliderData *slider, const gchar *key, gfloat value, gpointer data);

	gpointer status_get_data;
	gpointer slider_press_data;
	gpointer slider_release_data;
	gpointer slider_drag_data;
};

typedef struct _SliderSetData SliderSetData;
struct _SliderSetData
{
	gfloat step_size;
	gint page_size;
};

static WidgetType type_id = -1;


static void slider_key_movement_reset_timer(SliderData *slider, gint start);


/*
 *-----------------------------
 * new / free
 *-----------------------------
 */

SliderData *slider_new(GdkPixbuf *pb, gint x, gint y,
		       gint length, gint vertical, gint reversed,
		       gint has_press, gint prelight,
		       const gchar *item_key)
{
	SliderData *slider;
	gint width;
	gint height;

	slider_type_init();

	if (!pb) return NULL;

	util_size(&x);
	util_size(&y);
	util_size(&length);

	slider = g_new0(SliderData, 1);

	slider->overlay = util_size_pixbuf(pb, TRUE);

	width = gdk_pixbuf_get_width(slider->overlay);
	height = gdk_pixbuf_get_height(slider->overlay);

	slider->vertical = vertical;
	slider->reversed = reversed;
	slider->length = length;
	if (vertical)
		{
		slider->width = width;
		slider->height = length;
		slider->handle_width = width;
		slider->handle_height = (height - length) / (1 + has_press + prelight);
		slider->handle_size = slider->handle_height;
		}
	else
		{
		slider->width = length;
		slider->height = height;
		slider->handle_width = (width - length) / (1 + has_press + prelight);
		slider->handle_height = height;
		slider->handle_size = slider->handle_width;
		}
	slider->has_press = has_press;
	slider->has_prelight = prelight;
	if (slider->reversed)
		{
		slider->position = slider->length - slider->handle_size;
		slider->value = 1.0;
		}
	else
		{
		slider->position = 0;
		slider->value = 0.0;
		}
	slider->pushed = FALSE;
	slider->x = x;
	slider->y = y;
	slider->item_key = g_strdup(item_key);

	slider->sizeable = FALSE;
	slider->real_length = length;
	slider->border_1 = 0;
	slider->border_2 = 0;
	slider->stretch = FALSE;

	slider->step_size = 0.01;
	slider->page_size = 10;
	slider->key_timer_id = -1;

	return slider;
}

SliderData *slider_new_from_data(gchar **data, gint x, gint y,
				 gint length, gint vertical, gint reversed,
				 gint has_press, gint prelight,
				 const gchar *item_key)
{
	GdkPixbuf *pb;

	pb = gdk_pixbuf_new_from_xpm_data((const char **)data);

	return slider_new(pb, x, y, length, vertical, reversed, has_press, prelight, item_key);
}

SliderData *slider_new_from_file(const gchar *file, gint x, gint y,
				 gint length, gint vertical, gint reversed,
				 gint has_press, gint prelight,
				 const gchar *item_key)
{
	GdkPixbuf *pb;

	pb = util_pixbuf_new_from_file(file);

	return slider_new(pb, x, y, length, vertical, reversed, has_press, prelight, item_key);
}

void slider_set_attributes(SliderData *slider, gint sizeable,
			   gint real_length, gint border_1, gint border_2, gint stretch)
{
	if (!slider) return;

	util_size(&real_length);
	util_size(&border_1);
	util_size(&border_2);

	slider->real_length = real_length;
	slider->border_1 = border_1;
	slider->border_2 = border_2;
	slider->stretch = stretch;

	slider->sizeable = sizeable;

	if (slider->reversed) slider->position = slider->real_length - slider->handle_size;

	if (slider->vertical)
		{
		slider->height = slider->real_length;
		}
	else
		{
		slider->width = slider->real_length;
		}
}

void slider_free(SliderData *slider)
{
	if (!slider) return;

	slider_key_movement_reset_timer(slider, FALSE);

	if (slider->pixbuf) gdk_pixbuf_unref(slider->pixbuf);
	if (slider->overlay) gdk_pixbuf_unref(slider->overlay);
	g_free(slider->item_key);
	g_free(slider);
}

static void slider_free_cb(gpointer data)
{
	slider_free((SliderData *)data);
}

void slider_sync_overlay(SliderData *slider)
{
	if (slider->pixbuf) gdk_pixbuf_unref(slider->pixbuf);
	slider->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, slider->width, slider->height);
}

/*
 *-----------------------------
 * draw
 *-----------------------------
 */

static void slider_force_item_draw_cb(WidgetData *wd, gpointer data, UIData *ui)
{
	SliderData *slider = data;
	ItemData *item = wd->widget;

	item->current = item_section_by_percent(item, slider->value);

	wd->hidden = FALSE;
	ui_widget_draw(ui, wd, FALSE, TRUE);
	wd->hidden = TRUE;
}

static void slider_draw_real(SliderData *slider, gfloat value, gint move,
			     gint prelight, gint pushed, gint force, GdkPixbuf *pb, UIData *ui)
{
	gint new_pos;
	gint old_l;
	gint new_l;
	gint rx, ry, rw, rh;

	if (!slider) return;

	if (move)
		{
		if (value < 0.0) value = 0.0;
		if (value > 1.0) value = 1.0;
		slider->value = value;
		}

	new_pos = (float)(slider->real_length - slider->handle_size) * slider->value;
	if (new_pos < 0) new_pos = 0;
	if (new_pos > slider->real_length - slider->handle_size) new_pos = slider->real_length - slider->handle_size;

	old_l = slider->length + (slider->handle_size * (slider->pushed * slider->has_press + ((1 + slider->has_press) * slider->prelit * slider->has_prelight)));

	if (force)
		{
		new_l = old_l;
		}
	else
		{
		new_l = slider->length + (slider->handle_size * (pushed * slider->has_press + ((1 + slider->has_press) * prelight * slider->has_prelight)));
		slider->pushed = pushed;
		slider->prelit = (!slider->pushed && slider->has_prelight && prelight);
		}

	rx = ry = rw = rh = 0;

	if (force)
		{
		pixbuf_copy_area(slider->pixbuf, 0, 0, pb,
				 slider->x, slider->y, slider->width, slider->height, FALSE);
		rx = slider->x;
		ry = slider->y;
		rw = slider->width;
		rh = slider->height;
		}
	else if (slider->position != new_pos || old_l != new_l)
		{
		/* only redraw where it is */
		if (slider->vertical)
			{
			pixbuf_copy_area(slider->pixbuf, 0, slider->position, pb,
					 slider->x, slider->y + slider->position,
					 slider->handle_width, slider->handle_height, FALSE);
			rx = slider->x;
			ry = slider->y + slider->position;
			}
		else
			{
			pixbuf_copy_area(slider->pixbuf, slider->position, 0, pb,
					 slider->x + slider->position, slider->y,
					 slider->handle_width, slider->handle_height, FALSE);
			rx = slider->x + slider->position;
			ry = slider->y;
			}
		rw = slider->handle_width;
		rh = slider->handle_height;
		}

	if (force || slider->position != new_pos || old_l != new_l)
		{
		slider->position = new_pos;

		if (slider->item_key)
			{
			ui_widget_for_each_key_one(ui, slider->item_key, item_type_id(),
						   slider_force_item_draw_cb, slider);
			}

		if (slider->vertical)
			{
			pixbuf_copy_area_alpha(slider->overlay, 0, new_l, pb,
					       slider->x, slider->y + slider->position,
					       slider->handle_width, slider->handle_height, 255);
			if (slider->y + slider->position < ry)
				{
				gint n;
				n = slider->y + slider->position;
				rh += ry - n;
				ry = n;
				}
			if (slider->y + slider->position + slider->handle_size > ry + rh)
				{
				rh = slider->y + slider->position + slider->handle_size - ry;
				}
			}
		else
			{
			pixbuf_copy_area_alpha(slider->overlay, new_l, 0, pb,
					       slider->x + slider->position, slider->y,
					       slider->handle_width, slider->handle_height, 255);
			if (slider->x + slider->position < rx)
				{
				gint n;
				n = slider->x + slider->position;
				rw += rx - n;
				rx = n;
				}
			if (slider->x + slider->position + slider->handle_size > rx + rw)
				{
				rw = slider->x + slider->position + slider->handle_size - rx;
				}
			}

		ui_display_render_area(ui, rx, ry, rw, rh, slider->wd);
		}
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static void slider_draw(gpointer data, const gchar *key, gint update, gint force, GdkPixbuf *pb, UIData *ui)
{
	SliderData *slider = data;
	gfloat value;

	value = slider->value;

	if (update)
		{
		SliderCallbackData *cd;

		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->status_get_func)
			{
			value = cd->status_get_func(slider, key, cd->status_get_data);
			if (slider->reversed) value = 1.0 - value;
			}
		}

	if (force || value != slider->value)
		{
		if (value != slider->value)
			{
			slider_draw_real(slider, value, TRUE, slider->prelit, slider->pushed, force, pb, ui);
			}
		else
			{
			slider_draw_real(slider, slider->value, FALSE, FALSE, FALSE, TRUE, pb, ui);
			}
		}
}

static void slider_reset(gpointer data, const gchar *key, GdkPixbuf *pb, UIData *ui)
{
	SliderData *slider = data;

	slider_draw_real(slider, slider->value, FALSE, FALSE, FALSE, FALSE, pb, ui);
}

static gint slider_test_proximity(SliderData *slider, gint x, gint y)
{
	return (x >= slider->x && x < slider->x + slider->width &&
		y >= slider->y && y < slider->y + slider->height);
}

static gfloat slider_value_public(SliderData *slider)
{
	return (slider->reversed ? 1.0 - slider->value : slider->value);
}

static gfloat slider_calc_value_by_coord(SliderData *slider, gint x, gint y)
{
	gint pos;
	gfloat val;

	if (slider->vertical)
		{
		pos = y - slider->y - (slider->handle_size / 2);
		}
	else
		{
		pos = x - slider->x - (slider->handle_size / 2);
		}
	val = (float)pos / (slider->real_length - slider->handle_size);
	if (val < 0.0) val = 0.0;
	if (val > 1.0) val = 1.0;

	return val;
}

static void slider_motion(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	SliderData *slider = data;

	if (!slider || slider->key_timer_id != -1) return;

	if (slider->pushed)
		{
		gfloat val;

		val = slider_calc_value_by_coord(slider, x, y);

		if (slider->value != val)
			{
			SliderCallbackData *cd;

			slider_draw_real(slider, val, TRUE, FALSE, TRUE, FALSE, pb, ui);

			cd = ui_get_registered_callbacks(ui, key, type_id);
			if (cd && cd->slider_drag_func)
				{
				cd->slider_drag_func(slider, key, slider_value_public(slider), cd->slider_drag_data);
				}
			}

		return;
		}

	if (slider_test_proximity(slider, x, y))
		{
		slider_draw_real(slider, 0.0, FALSE, TRUE, FALSE, FALSE, pb, ui);
		}
	else
		{
		slider_draw_real(slider, 0.0, FALSE, FALSE, FALSE, FALSE, pb, ui);
		}
}

static gint slider_press(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	SliderData *slider = data;

	if (!slider || slider->key_timer_id != -1) return FALSE;

	if (slider_test_proximity(slider, x, y))
		{
		SliderCallbackData *cd;
		gfloat val;

		val = slider_calc_value_by_coord(slider, x, y);

		slider_draw_real(slider, val, TRUE, FALSE, TRUE, FALSE, pb, ui);

		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->slider_press_func)
			{
			cd->slider_press_func(slider, key, slider_value_public(slider), cd->slider_press_data);
			}
		return TRUE;
		}

	return FALSE;
}

static void slider_release(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	SliderData *slider = data;
	SliderCallbackData *cd;
	gfloat val;

	if (!slider || !slider->pushed || slider->key_timer_id != -1) return;

	val = slider_calc_value_by_coord(slider, x, y);

	if (slider_test_proximity(slider, x, y))
		{
		slider_draw_real(slider, val, TRUE, TRUE, FALSE, FALSE, pb, ui);
		}
	else
		{
		slider_draw_real(slider, val, TRUE, FALSE, FALSE, FALSE, pb, ui);
		}

	cd = ui_get_registered_callbacks(ui, key, type_id);
	if (cd && cd->slider_release_func)
		{
		cd->slider_release_func(slider, key, slider_value_public(slider), cd->slider_release_data);
		}
}

static void slider_back_set(gpointer data, GdkPixbuf *pb)
{
	SliderData *slider = data;

	if (!slider) return;

	pixbuf_copy_area(pb, slider->x, slider->y, slider->pixbuf, 0, 0,
			 slider->width, slider->height, FALSE);

	if (slider->vertical)
		{
		if (slider->border_1 > 0)
			{
			pixbuf_copy_area_alpha(slider->overlay, 0, 0,
					       slider->pixbuf, 0, 0,
					       slider->width, slider->border_1, 255);
			}
		pixbuf_copy_fill_alpha(slider->overlay, 0, slider->border_1,
				       slider->width, slider->length - slider->border_1 - slider->border_2,
				       slider->pixbuf, 0, slider->border_1,
				       slider->width, slider->height - slider->border_1 - slider->border_2,
				       slider->stretch,  255);
		if (slider->border_2 > 0)
			{
			pixbuf_copy_area_alpha(slider->overlay, 0, slider->length - slider->border_2,
					       slider->pixbuf, 0, slider->height - slider->border_2,
					       slider->width, slider->border_2, 255);
			}
		}
	else
		{
		if (slider->border_1 > 0)
			{
			pixbuf_copy_area_alpha(slider->overlay, 0, 0,
					       slider->pixbuf, 0, 0,
					       slider->border_1, slider->height, 255);
			}
		pixbuf_copy_fill_alpha(slider->overlay, slider->border_1, 0,
				       slider->length - slider->border_1 - slider->border_2, slider->height,
				       slider->pixbuf, slider->border_1, 0,
				       slider->width - slider->border_1 - slider->border_2, slider->height,
				       slider->stretch,  255);
		if (slider->border_2 > 0)
			{
			pixbuf_copy_area_alpha(slider->overlay, slider->length - slider->border_2, 0,
					       slider->pixbuf, slider->width - slider->border_2, 0,
					       slider->border_2, slider->height, 255);
			}
		}
}

static gint slider_key_release_cb(gpointer data)
{
	SliderData *slider = data;
	UIData *ui;
	GdkPixbuf *pb;

	ui = skin_get_ui(slider->skin);
	pb = skin_get_pixbuf(slider->skin);

	if (slider->pushed)
		{
		SliderCallbackData *cd;
		slider_draw_real(slider, 0.0, FALSE, FALSE, FALSE, FALSE, pb, ui);

		cd = ui_get_registered_callbacks(ui, slider->wd->key, type_id);

		if (cd && cd->slider_release_func)
			{
			cd->slider_release_func(slider, slider->wd->key, slider_value_public(slider), cd->slider_release_data);
			}
		}

	slider->key_timer_id = -1;
	return FALSE;
}

static void slider_key_movement_reset_timer(SliderData *slider, gint start)
{
	if (slider->key_timer_id != -1)
		{
		gtk_timeout_remove(slider->key_timer_id);
		slider->key_timer_id = -1;
		}

	if (start)
		{
		slider->key_timer_id = gtk_timeout_add(SLIDER_KEY_RELEASE_TIME, slider_key_release_cb, slider);
		}
}

static void slider_key_movement(SliderData *slider, const gchar *key, gfloat value, GdkPixbuf *pb, UIData *ui)
{
	SliderCallbackData *cd;
	gint was_pushed;

	value = CLAMP(slider->value + value, 0.0, 1.0);
	if (value == slider->value) return;

	was_pushed = slider->pushed;
	slider_draw_real(slider, value, TRUE, FALSE, TRUE, FALSE, pb, ui);

	cd = ui_get_registered_callbacks(ui, key, type_id);

	if (!was_pushed)
		{
		if (cd && cd->slider_press_func)
			{
			cd->slider_press_func(slider, key, slider_value_public(slider), cd->slider_press_data);
			}
		}
	else
		{
		if (cd && cd->slider_drag_func)
			{
			cd->slider_drag_func(slider, key, slider_value_public(slider), cd->slider_drag_data);
			}
		}

	slider_key_movement_reset_timer(slider, TRUE);
}

static gint slider_key_event_cb(gpointer widget, const gchar *key, GdkEventKey *event, GdkPixbuf *pb, UIData *ui)
{
	SliderData *slider = widget;
	gint ret = FALSE;

	if (event->state & GDK_CONTROL_MASK) return FALSE;

	switch (event->keyval)
		{
		case GDK_Up:
		case GDK_KP_Up:
			if (slider->vertical)
				{
				slider_key_movement(slider, key, 0 - slider->step_size, pb, ui);
				ret = TRUE;
				}
			break;
		case GDK_Down:
		case GDK_KP_Down:
			if (slider->vertical)
				{
				slider_key_movement(slider, key, slider->step_size, pb, ui);
				ret = TRUE;
				}
			break;
		case GDK_Left:
		case GDK_KP_Left:
			if (!slider->vertical)
				{
				slider_key_movement(slider, key, 0 - slider->step_size, pb, ui);
				ret = TRUE;
				}
			break;
		case GDK_Right:
		case GDK_KP_Right:
			if (!slider->vertical)
				{
				slider_key_movement(slider, key, slider->step_size, pb, ui);
				ret = TRUE;
				}
			break;
		case GDK_Page_Up:
		case GDK_KP_Page_Up:
			slider_key_movement(slider, key,
					    (0 - slider->step_size) * (gfloat)slider->page_size, pb, ui);
			ret = TRUE;
			break;
		case GDK_Page_Down:
		case GDK_KP_Page_Down:
			slider_key_movement(slider, key,
					    slider->step_size * (gfloat)slider->page_size, pb, ui);
			ret = TRUE;
			break;
		case GDK_Home:
		case GDK_KP_Home:
			slider_key_movement(slider, key, -1.0, pb, ui);
			ret = TRUE;
			break;
		case GDK_End:
		case GDK_KP_End:
			slider_key_movement(slider, key, 1.0, pb, ui);
			ret = TRUE;
			break;
		default:
			break;
		}

	return ret;
}

static void slider_handle_geometry(SliderData *slider, gint *x, gint *y, gint *w, gint *h)
{
	if (slider->vertical)
		{
		*x = slider->x;
		*y = slider->y + slider->position;
		}
	else
		{
		*x = slider->x + slider->position;
		*y = slider->y;
		}
	*w = slider->handle_width;
	*h = slider->handle_height;
}

static gint slider_focus_draw_cb(gpointer widget, const gchar *key,
				 gint x, gint y, gint w, gint h, GdkPixbuf *pb, UIData *ui)
{
	SliderData *slider = widget;
	gint hx, hy, hw, hh;

	slider_handle_geometry(slider, &hx, &hy, &hw, &hh);
	ui_display_draw_focus(ui, pb, hx, hy, hw, hh,
			      x, y, w, h, NULL);

	return TRUE;
}

static gint slider_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	SliderData *slider = widget;

	*x = slider->x;
	*y = slider->y;
	*w = slider->width;
	*h = slider->height;

	return TRUE;
}

static void slider_set_coord(gpointer widget, gint x, gint y)
{
	SliderData *slider = widget;

	slider->x = x;
	slider->y = y;
}

static void slider_set_size(gpointer widget, gint dev_w, gint dev_h)
{
	SliderData *slider = widget;

	if (!slider->sizeable) return;

	if (slider->vertical)
		{
		slider->real_length = MAX(slider->real_length + dev_h, slider->handle_size);
		slider->real_length = MAX(slider->real_length, slider->border_1 + slider->border_2 + 1);
		slider->height = slider->real_length;
		}
	else
		{
		slider->real_length = MAX(slider->real_length + dev_w, slider->handle_size);
		slider->real_length = MAX(slider->real_length, slider->border_1 + slider->border_2 + 1);
		slider->width = slider->real_length;
		}

	slider_sync_overlay(slider);
}

static WidgetData *slider_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	SliderData *slider;
	gchar *filename;
	gint x, y;
	gint has_press;
	gint prelight;
	gint vertical;
	gint reverse;
	gint length;
	const gchar *item_key;
	gint sizeable;
	gint trough_length;
	gint border_1, border_2;
	gint stretch;

	/* req */
	if (!key_list_read_int(list, "x", &x)) return NULL;
	if (!key_list_read_int(list, "y", &y)) return NULL;
	if (!key_list_read_int(list, "length", &length)) return NULL;
	filename = key_list_read_path(list, "image", skin_dir);
	if (!filename) return NULL;

	/* opt */
	has_press = key_list_read_bool(list, "pressable");
	prelight = key_list_read_bool(list, "prelight");
	vertical = key_list_read_bool(list, "vertical");
	reverse = key_list_read_bool(list, "reversed");
	item_key = key_list_read_chars(list, "item_link", NULL);

	sizeable = key_list_read_bool(list, "sizeable");
	if (!key_list_read_int(list, "trough_length", &trough_length)) trough_length = length;
	if (!key_list_read_int(list, "border_1", &border_1)) border_1 = 0;
	if (!key_list_read_int(list, "border_2", &border_2)) border_2 = 0;
	stretch = key_list_read_bool(list, "stretch");

	slider = slider_new_from_file(filename, x, y,
				      length, vertical, reverse,
				      has_press, prelight, item_key);
	slider_set_attributes(slider, sizeable, trough_length, border_1, border_2, stretch);

	if (slider)
		{
		wd = slider_register(skin, slider, key, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			}
		}

	g_free(filename);

	return wd;
}

static void slider_init(gpointer widget, const gchar *key, UIData *ui)
{
	SliderData *slider = widget;

	slider_sync_overlay(slider);
}

/*
 *-----------------------------
 * register ui / app side
 *-----------------------------
 */

WidgetData *slider_register(SkinData *skin, SliderData *slider, const gchar *key, const gchar *text_id)
{
	WidgetData *wd;

	wd = skin_register_widget(skin, key, text_id, type_id, slider);
	slider->wd = wd;
	slider->skin = skin;

	return wd;
}

WidgetData *slider_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				    gint length, gint vertical, gint reversed,
				    gint has_press, gint prelight,
				    const gchar *item_key,
				    SkinData *skin, const gchar *text_id)
{
	SliderData *slider;

	slider = slider_new_from_data(data, x, y, length, vertical, reversed,
				      has_press, prelight, item_key);
	return slider_register(skin, slider, key, text_id);
}

RegisterData *slider_register_key(const gchar *key, UIData *ui,
				  gfloat (*status_get_func)(SliderData *slider, const gchar *key, gpointer data), gpointer status_get_data,
				  void (*slider_press_func)(SliderData *slider, const gchar *key, gfloat value, gpointer data), gpointer slider_press_data,
				  void (*slider_release_func)(SliderData *slider, const gchar *key, gfloat value, gpointer data), gpointer slider_release_data,
				  void (*slider_drag_func)(SliderData *slider, const gchar *key, gfloat value, gpointer data), gpointer slider_drag_data)
{
	SliderCallbackData *cd;

	slider_type_init();

	cd = g_new0(SliderCallbackData, 1);

	cd->status_get_func = status_get_func;
	cd->slider_press_func = slider_press_func;
	cd->slider_release_func = slider_release_func;
	cd->slider_drag_func = slider_drag_func;

	cd->status_get_data = status_get_data;
	cd->slider_press_data = slider_press_data;
	cd->slider_release_data = slider_release_data;
	cd->slider_drag_data = slider_drag_data;

	return ui_register_key(ui, key, type_id, cd, sizeof(SliderCallbackData));
}

/*
 *-----------------------------
 * app funcs
 *-----------------------------
 */

static void slider_value_set_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	SliderData *slider;
	gfloat *value = data;

	slider = wd->widget;

	if (!slider->pushed)
		{
		slider_draw_real(slider, slider->reversed ? 1.0 - (*value) : *value,
				 TRUE, slider->prelit, slider->pushed, FALSE, pb, ui);	
		}
}

gint slider_value_set(const gchar *key, UIData *ui, gfloat value)
{
	return skin_widget_for_each_key(ui, key, type_id, slider_value_set_cb, &value);
}

gfloat slider_value_get(const gchar *key, UIData *ui)
{
	WidgetData *wd;
	SliderData *slider;

	wd = skin_widget_get_by_key(ui->skin, key, type_id);
	if (!wd || wd->type != type_id) return -1.0;

	slider = wd->widget;
	return slider_value_public(slider);
}

static void slider_step_size_set_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	SliderData *slider;
	SliderSetData *sd = data;

	slider = wd->widget;

	slider->step_size = sd->step_size;
	slider->page_size = sd->page_size;
}

gint slider_step_size_set(const gchar *key, UIData *ui, gfloat step_size, gint page_size)
{
	SliderSetData sd;

	sd.step_size = step_size;
	sd.page_size = page_size;

	return skin_widget_for_each_key(ui, key, type_id, slider_step_size_set_cb, &sd);
}

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType slider_type_id(void)
{
	return type_id;
}

void slider_type_init(void)
{
	WidgetObjectData *od;

	if (type_id != -1) return;

	od = ui_widget_type_new("slider");
	type_id = od->type;

	od->func_draw = slider_draw;
	od->func_reset = slider_reset;
	od->func_press = slider_press;
	od->func_release = slider_release;
	od->func_motion = slider_motion;
	od->func_back = slider_back_set;
	od->func_free = slider_free_cb;

	od->func_focus_key_event = slider_key_event_cb;
	od->func_focus_draw = slider_focus_draw_cb;

	od->func_get_geometry = slider_get_geometry;
	od->func_set_coord = slider_set_coord;
	od->func_set_size = slider_set_size;

	od->func_parse = slider_parse;
	od->func_init = slider_init;

	slider_type_init_edit(od);
}

