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

#include "ui2_button.h"
#include "ui2_button_edit.h"

#include "ui2_display.h"
#include "ui2_main.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"

#include <gdk/gdkkeysyms.h> /* for key values */


typedef struct _ButtonCallbackData ButtonCallbackData;
struct _ButtonCallbackData
{
	gint (*status_get_func)(ButtonData *button, const gchar *key, gpointer data);
	void (*button_click_func)(ButtonData *button, const gchar *key, gpointer data);
	void (*button_press_func)(ButtonData *button, const gchar *key, gpointer data);
	void (*button_release_func)(ButtonData *button, const gchar *key, gpointer data);

	gpointer status_get_data;
	gpointer button_click_data;
	gpointer button_press_data;
	gpointer button_release_data;
};

static WidgetType type_id = -1;

/*
 *-----------------------------
 * new / free
 *-----------------------------
 */

ButtonData *button_new(GdkPixbuf *pb, gint x, gint y,
		       gint prelight, gint light, GdkPixbuf *clip_pb)
{
	ButtonData *button;

	button_type_init();

	util_size(&x);
	util_size(&y);

	if (!pb)
		{
		if (clip_pb) gdk_pixbuf_unref(clip_pb);
		return NULL;
		}

	button = g_new0(ButtonData, 1);

	button->overlay = util_size_pixbuf(pb, TRUE);
	button->has_light = light;
	button->has_prelight = prelight;
	button->pushed = FALSE;
	button->lit = FALSE;
	button->prelit = FALSE;
	button->active = FALSE;
	button->x = x;
	button->y = y;

	button->width = gdk_pixbuf_get_width(button->overlay) / (2 + (light * prelight) + prelight + (2 * light));
	button->height = gdk_pixbuf_get_height(button->overlay);

	/* the pixbuf buffer contains the alpha of any clip mask */
	if (clip_pb && gdk_pixbuf_get_has_alpha(clip_pb))
		{
		clip_pb = util_size_pixbuf(clip_pb, FALSE);

		if (button->width == gdk_pixbuf_get_width(clip_pb) && 
		    button->height == gdk_pixbuf_get_height(clip_pb) )
			{
			button->clip_mask = TRUE;
			pixbuf_alpha_force_to_bw(clip_pb, 128);
			}
		else
			{
			printf("skin warning: button clip mask does not match button size\n");
			button->clip_mask = FALSE;
			}
		}
	else
		{
		button->clip_mask = FALSE;
		}

	button->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, button->clip_mask, 8, gdk_pixbuf_get_width(button->overlay), button->height);

	if (button->clip_mask && clip_pb)
		{
		gint i;

		for (i = 0; i < 2 + (2 * button->has_light) + button->has_prelight + (button->has_prelight * button->has_light); i++)
			{
			pixbuf_copy_channel(clip_pb, 0, 0,
					   button->pixbuf, i * button->width, 0,
					   button->width, button->height, 3);
			}

		gdk_pixbuf_unref(clip_pb);
		clip_pb = NULL;
		}

	if (clip_pb)
		{
		printf("skin warning: button clip mask does not have alpha channel\n");
		gdk_pixbuf_unref(clip_pb);
		}

	return button;
}

ButtonData *button_new_from_data(gchar **data, gint x, gint y,
				 gint prelight, gint light, gchar **clip_data)
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
	
	return button_new(pb, x, y, prelight, light, clip_pb);
}


ButtonData *button_new_from_file(const gchar *file, gint x, gint y,
				 gint prelight, gint light, const gchar *clip_file)
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

	return button_new(pb, x, y, prelight, light, clip_pb);
}

void button_free(ButtonData *button)
{
	if (!button) return;
	if (button->pixbuf) gdk_pixbuf_unref(button->pixbuf);
	if (button->overlay) gdk_pixbuf_unref(button->overlay);
	g_free(button);
}

static void button_free_cb(gpointer data)
{
	button_free((ButtonData *)data);
}

/*
 *-----------------------------
 * draw
 *-----------------------------
 */

static gint button_calc_offset(ButtonData *button, gint prelight, gint pressed)
{
	gint r = 0;

	if (pressed) r++;
	if (button->has_prelight && prelight && !pressed)
		{
		if (button->has_light)
			{
			r += 2;
			if (button->lit) r++;
			}
		r+=2;
		}
	else
		{
		if (button->has_light && button->lit) r += 2;
		}

	return r;
}

static void button_draw_real(ButtonData *button, gint prelight, gint pressed, gint force, GdkPixbuf *pb, UIData *ui)
{
	gint l = 0;
	gint old_l;

	if (!button) return;

	old_l = button_calc_offset(button, button->prelit, button->pushed);

	if (force)
		{
		l = old_l;
		old_l = -1;
		}
	else
		{
		l = button_calc_offset(button, prelight, pressed);
		}

	if (l != old_l)
		{
		if (button->clip_mask)
			{
			pixbuf_copy_area_alpha(button->pixbuf, l * button->width, 0,
					       pb, button->x, button->y,
					       button->width, button->height, 255);
			}
		else
			{
			pixbuf_copy_area(button->pixbuf, l * button->width, 0,
					 pb, button->x, button->y,
					 button->width, button->height, FALSE);
			}

		ui_display_render_area(ui, button->x, button->y, button->width, button->height, button->wd);
		}

	if (!force)
		{
		button->pushed = pressed;
		button->prelit = prelight;
		}
}

static void button_light_set(ButtonData *button, gint lit, GdkPixbuf *pb, UIData *ui)
{
	if (!button || !button->has_light) return;

	if (button->lit == lit) return;

	button->lit = lit;

	button_draw_real(button, FALSE, FALSE, TRUE, pb, ui);
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static void button_draw(gpointer data, const gchar *key, gint update, gint force, GdkPixbuf *pb, UIData *ui)
{
	ButtonData *button = data;
	gint state;

	state = button->lit;

	if (update)
		{
		ButtonCallbackData *cd;

		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->status_get_func)
			{
			state = cd->status_get_func(button, key, cd->status_get_data);
			}
		}

	if (force || state != button->lit)
		{
		if (button->has_light && state != button->lit)
			{
			button_light_set(button, state, pb, ui);
			}
		else
			{
			button_draw_real(button, button->prelit, button->pushed, force, pb, ui);
			}
		}
}

static void button_reset(gpointer data, const gchar *key, GdkPixbuf *pb, UIData *ui)
{
	ButtonData *button = data;

	button_draw_real(button, FALSE, FALSE, FALSE, pb, ui);
}

static gint button_test_proximity(ButtonData *button, gint x, gint y)
{
	if (!button) return FALSE;

	if (x < button->x || x >= button->x + button->width ||
	    y < button->y || y >= button->y + button->height) return FALSE;

	if (button->clip_mask && !pixbuf_pixel_is_visible(button->pixbuf, x - button->x, y - button->y)) return FALSE;

	return TRUE;
}

static void button_motion(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	ButtonData *button = data;
	if (!button) return;

	if (button_test_proximity(button, x, y))
		{
		if (button->active)
			{
			button_draw_real(button, FALSE, TRUE, FALSE, pb, ui);
			}
		else
			{
			button_draw_real(button, TRUE, FALSE, FALSE, pb, ui);
			}
		}
	else
		{
		button_draw_real(button, FALSE, FALSE, FALSE, pb, ui);
		}
}

static gint button_press(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	ButtonData *button = data;
	ButtonCallbackData *cd;

	if (!button) return FALSE;

	if (button_test_proximity(button, x, y))
		{
		button->active = TRUE;

		button_draw_real(button, FALSE, TRUE, FALSE, pb, ui);

		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->button_press_func)
			{
			cd->button_press_func(button, key, cd->button_press_data);
			}

		return TRUE;
		}

	return FALSE;
}

static void button_release(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	ButtonData *button = data;
	ButtonCallbackData *cd;

	if (!button || !button->active) return;

	button->active = FALSE;

	button_motion(data, key, x, y, pb, ui);

	cd = ui_get_registered_callbacks(ui, key, type_id);
	if (cd && cd->button_release_func)
		{
		cd->button_release_func(button, key, cd->button_release_data);
		}

	if (cd && cd->button_click_func && button_test_proximity(button, x, y))
		{
		cd->button_click_func(button, key, cd->button_click_data);
		}
}

static gint button_key_event_cb(gpointer data, const gchar *key,
				GdkEventKey *event, GdkPixbuf *pb, UIData *ui)
{
	if (event->keyval == GDK_space)
		{
		ButtonData *button = data;
		ButtonCallbackData *cd;

		cd = ui_get_registered_callbacks(ui, key, type_id);

		/* run the signal gamut, for compatibility */
		if (cd && cd->button_press_func)
			{
			cd->button_press_func(button, key, cd->button_press_data);
			}
		if (cd && cd->button_release_func)
			{
			cd->button_release_func(button, key, cd->button_release_data);
			}
		if (cd && cd->button_click_func)
			{
			cd->button_click_func(button, key, cd->button_click_data);
			}
		
		return TRUE;
		}

	return FALSE;
}

static void button_back_set(gpointer data, GdkPixbuf *pb)
{
	ButtonData *button = data;
	gint i;

	if (!button) return;

	for (i = 0; i < 2 + (2 * button->has_light) + button->has_prelight + (button->has_prelight * button->has_light); i++)
		{
		pixbuf_copy_area(pb, button->x, button->y, button->pixbuf,
				 i * button->width, 0, button->width, button->height, FALSE);
		}

	pixbuf_copy_area_alpha(button->overlay, 0, 0,
			       button->pixbuf, 0, 0, gdk_pixbuf_get_width(button->overlay), button->height, 255);
}

static gint button_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	ButtonData *button = widget;

	*x = button->x;
	*y = button->y;
	*w = button->width;
	*h = button->height;

	return TRUE;
}

static void button_set_coord(gpointer widget, gint x, gint y)
{
	ButtonData *button = widget;

	button->x = x;
	button->y = y;
}

static WidgetData *button_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	ButtonData *button;
	gchar *filename;
	gint x, y;
	gint prelight;
	gint light;
	gchar *clipname;

	/* req */
	if (!key_list_read_int(list, "x", &x)) return NULL;
	if (!key_list_read_int(list, "y", &y)) return NULL;
	filename = key_list_read_path(list, "image", skin_dir);
	if (!filename) return NULL;

	/* opt */
	prelight = key_list_read_bool(list, "prelight");
	light = key_list_read_bool(list, "indicator");
	clipname = key_list_read_path(list, "clip_mask", skin_dir);

	button = button_new_from_file(filename, x, y, prelight, light, clipname);

	if (button)
		{
		wd = button_register(skin, button, key, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			ui_widget_set_data(wd, "clip_mask", clipname);
			}
		}

	g_free(filename);
	g_free(clipname);

	return wd;
}

/*
 *-----------------------------
 * register ui / app side
 *-----------------------------
 */

WidgetData *button_register(SkinData *skin, ButtonData *button, const gchar *key, const gchar *text_id)
{
	WidgetData *wd;

	wd = skin_register_widget(skin, key, text_id, type_id, button);
	button->wd = wd;

	return wd;
}

WidgetData *button_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				    gint prelight, gint light, gchar **clip_data,
				    SkinData *skin, const gchar *text_id)
{
	ButtonData *button;

	button = button_new_from_data(data, x, y, prelight, light, clip_data);
	return button_register(skin, button, key, text_id);
}

RegisterData *button_register_key(const gchar *key, UIData *ui,
				  gint (*status_get_func)(ButtonData *button, const gchar *key, gpointer data), gpointer status_get_data,
				  void (*button_click_func)(ButtonData *button, const gchar *key, gpointer data), gpointer button_click_data,
				  void (*button_press_func)(ButtonData *button, const gchar *key, gpointer data), gpointer button_press_data,
				  void (*button_release_func)(ButtonData *button, const gchar *key, gpointer data), gpointer button_release_data)
{
	ButtonCallbackData *cd;

	button_type_init();

	cd = g_new0(ButtonCallbackData, 1);

	cd->status_get_func = status_get_func;
	cd->button_click_func = button_click_func;
	cd->button_press_func = button_press_func;
	cd->button_release_func = button_release_func;


	cd->status_get_data = status_get_data;
	cd->button_click_data = button_click_data;
	cd->button_press_data = button_press_data;
	cd->button_release_data = button_release_data;

	return ui_register_key(ui, key, type_id, cd, sizeof(ButtonCallbackData));
}

/*
 *-----------------------------
 * app funcs
 *-----------------------------
 */

static void button_state_set_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ButtonData *button;
	gint state;

	button = wd->widget;
	state = GPOINTER_TO_INT(data);

	button_light_set(button, state, pb, ui);
}

gint button_state_set(const gchar *key, UIData *ui, gint state)
{
	return skin_widget_for_each_key(ui, key, type_id, button_state_set_cb, GINT_TO_POINTER(state));
}

gint button_state_get(const gchar *key, UIData *ui)
{
	WidgetData *wd;
	ButtonData *button;

	wd = skin_widget_get_by_key(ui->skin, key, type_id);
	if (!wd || wd->type != type_id) return FALSE;

	button = wd->widget;
	return button->lit;
}

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType button_type_id(void)
{
	return type_id;
}

void button_type_init(void)
{
	WidgetObjectData *od;

	if (type_id != -1) return;

	od = ui_widget_type_new("button");
	type_id = od->type;

	od->func_draw = button_draw;
	od->func_reset = button_reset;
	od->func_press = button_press;
	od->func_release = button_release;
	od->func_motion = button_motion;
	od->func_back = button_back_set;
	od->func_free = button_free_cb;

	od->func_focus_key_event = button_key_event_cb;

	od->func_get_geometry = button_get_geometry;
	od->func_set_coord = button_set_coord;

	od->func_parse = button_parse;

	button_type_init_edit(od);
}




