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
#include "ui2_decal.h"
#include "ui2_decal_edit.h"

#include "ui2_display.h"
#include "ui2_main.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"

#include <math.h>

typedef struct _DecalCallbackData DecalCallbackData;
struct _DecalCallbackData
{
	gint (*status_get_func)(DecalData *decal, const gchar *key, gpointer data);
	gpointer status_get_data;
};

static WidgetType type_id = -1;


/*
 *-----------------------------
 * new / free
 *-----------------------------
 */

DecalData *decal_new(gint x, gint y, gint width, gint height, gint sizeable, guint8 alpha)
{
	DecalData *decal;

	decal_type_init();

	util_size(&x);
	util_size(&y);
	util_size(&width);
	util_size(&height);

	util_color(NULL, NULL, NULL, &alpha);

	decal = g_new0(DecalData, 1);

	decal->overlay = NULL;
	decal->width = width;
	decal->height = height;
	decal->x = x;
	decal->y = y;

	decal->sizeable = sizeable;
	decal->alpha = alpha;

	decal->border_left = 0;
	decal->border_right = 0;
	decal->border_top = 0;
	decal->border_bottom = 0;

	decal->border_stretch = FALSE;
	decal->stretch = FALSE;

	decal->rect_r = 0;
	decal->rect_g = 0;
	decal->rect_b = 0;
	decal->rect_fill = FALSE;

	return decal;
}

void decal_set_image(DecalData *decal, GdkPixbuf *pb, gchar **data, const gchar *file,
		     gint stretch, gint border_stretch)
{
	if (!decal) return;

	if (!pb && data) pb = gdk_pixbuf_new_from_xpm_data((const char **)data);
	if (!pb && file) pb = util_pixbuf_new_from_file(file);

	if (decal->overlay) gdk_pixbuf_unref(decal->overlay);
	decal->overlay = util_size_pixbuf(pb, TRUE);

	decal->stretch = stretch;
	decal->border_stretch = border_stretch;
}

void decal_set_color(DecalData *decal, guint8 r, guint8 g, guint8 b, gint fill)
{
	if (!decal) return;

	util_color(&r, &g, &b, NULL);

	decal->rect_r = r;
	decal->rect_g = g;
	decal->rect_b = b;
	decal->rect_fill = fill;
}

void decal_set_border(DecalData *decal, gint border_left, gint border_right,
		      gint border_top, gint border_bottom)
{
	if (!decal) return;

	util_size(&border_left);
	util_size(&border_right);
	util_size(&border_top);
	util_size(&border_bottom);

	decal->border_left = border_left;
	decal->border_right = border_right;
	decal->border_top = border_top;
	decal->border_bottom = border_bottom;
}

void decal_free(DecalData *decal)
{
	if (!decal) return;
	if (decal->overlay) gdk_pixbuf_unref(decal->overlay);
	g_free(decal);
}

static void decal_free_cb(gpointer data)
{
	decal_free((DecalData *)data);
}

/*
 *-----------------------------
 * draw
 *-----------------------------
 */

static void decal_render(DecalData *decal, GdkPixbuf *pb)
{
	if (decal->overlay)
		{
		pixbuf_copy_fill_border_alpha(decal->overlay, pb,
					      decal->x, decal->y, decal->width, decal->height,
					      decal->border_left, decal->border_stretch,
					      decal->border_right, decal->border_stretch,
					      decal->border_top, decal->border_stretch,
					      decal->border_bottom, decal->border_stretch,
					      decal->stretch, decal->alpha);
		}
	else
		{
		if (decal->rect_fill)
			{
			pixbuf_draw_rect_fill(pb, decal->x, decal->y, decal->width, decal->height,
					      decal->rect_r, decal->rect_g, decal->rect_b, decal->alpha);
			}
		else
			{
			pixbuf_draw_rect(pb, decal->x, decal->y, decal->width, decal->height,
					 decal->rect_r, decal->rect_g, decal->rect_b, decal->alpha,
					 decal->border_left, decal->border_right,
					 decal->border_top, decal->border_bottom);
			}
		}
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static void decal_back_set(gpointer data, GdkPixbuf *pb)
{
	DecalData *decal = data;

	if (!decal) return;

	decal_render(decal, pb);
}

static gint decal_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	DecalData *decal = widget;

	*x = decal->x;
	*y = decal->y;
	*w = decal->width;
	*h = decal->height;

	return TRUE;
}

static void decal_set_coord(gpointer widget, gint x, gint y)
{
	DecalData *decal = widget;

	decal->x = x;
	decal->y = y;
}

static void decal_set_size(gpointer widget, gint dev_w, gint dev_h)
{
	DecalData *decal = widget;
	UIData *ui;
	WidgetData *wd;

	if (!decal->sizeable) return;

	ui = skin_get_ui(decal->skin);
	if (ui)
		{
		wd = ui_widget_get_by_widget(ui, decal);
		}
	else
		{
		wd = NULL;
		}

	if (!wd || !wd->anchor_right)
		{
		decal->width = MAX(decal->width + dev_w,
				   decal->border_left + decal->border_right);
		}
	if (!wd || !wd->anchor_bottom)
		{
		decal->height = MAX(decal->height + dev_h,
				    decal->border_top + decal->border_bottom);
		}
}

static WidgetData *decal_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	DecalData *decal;
	gchar *filename;
	gint x, y;
	gint width, height;
	gint border_left;
	gint border_right;
	gint border_top;
	gint border_bottom;
	gint border_stretch;
	gint stretch;
	gint rect_r;
	gint rect_g;
	gint rect_b;
	gint rect_fill;
	gint alpha;
	gint sizeable;

	/* req */
	if (!key_list_read_int(list, "x", &x)) return NULL;
	if (!key_list_read_int(list, "y", &y)) return NULL;
	if (!key_list_read_int(list, "width", &width)) return NULL;
	if (!key_list_read_int(list, "height", &height)) return NULL;

	/* opt */
	filename = key_list_read_path(list, "image", skin_dir);

	sizeable = key_list_read_bool(list, "sizeable");
	if (!key_list_read_int(list, "alpha", &alpha)) alpha = 255;

	if (!key_list_read_int(list, "border_left", &border_left)) border_left = 0;
	if (!key_list_read_int(list, "border_right", &border_right)) border_right = 0;
	if (!key_list_read_int(list, "border_top", &border_top)) border_top = 0;
	if (!key_list_read_int(list, "border_bottom", &border_bottom)) border_bottom = 0;

	border_stretch = key_list_read_bool(list, "border_stretch");
	stretch = key_list_read_bool(list, "stretch");

	if (!key_list_read_int(list, "rect_red", &rect_r)) rect_r = 0;
	if (!key_list_read_int(list, "rect_green", &rect_g)) rect_g = 0;
	if (!key_list_read_int(list, "rect_blue", &rect_b)) rect_b = 0;
	rect_fill = key_list_read_bool(list, "rect_fill");

	decal = decal_new(x, y, width, height, sizeable, alpha);

	if (decal)
		{
		decal_set_image(decal, NULL, NULL, filename, stretch, border_stretch);
		decal_set_color(decal, rect_r, rect_g, rect_b, rect_fill);
		decal_set_border(decal, border_left, border_right, border_top, border_bottom);

		wd = decal_register(skin, decal, key, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			}
		}

	g_free(filename);

	return wd;
}


/*
 *-----------------------------
 * register ui / app side
 *-----------------------------
 */

WidgetData *decal_register(SkinData *skin, DecalData *decal, const gchar *key, const gchar *text_id)
{
	if (decal) decal->skin = skin;

	return skin_register_widget(skin, key, text_id, type_id, decal);
}

/*
 *-----------------------------
 * app funcs
 *-----------------------------
 */

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType decal_type_id(void)
{
	return type_id;
}

void decal_type_init(void)
{
	WidgetObjectData *od;

	if (type_id != -1) return;

	od = ui_widget_type_new("decal");
	type_id = od->type;

	od->priority = UI_WIDGET_PRIORITY_HIGH;

	od->func_back = decal_back_set;
	od->func_free = decal_free_cb;

	od->func_get_geometry = decal_get_geometry;
	od->func_set_coord = decal_set_coord;
	od->func_set_size = decal_set_size;

	od->func_parse = decal_parse;

	decal_type_init_edit(od);
}


