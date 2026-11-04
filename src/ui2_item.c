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
#include "ui2_item.h"
#include "ui2_item_edit.h"

#include "ui2_display.h"
#include "ui2_main.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"

#include <math.h>

typedef struct _ItemCallbackData ItemCallbackData;
struct _ItemCallbackData
{
	gint (*status_get_func)(ItemData *item, const gchar *key, gpointer data);
	gpointer status_get_data;
};

static WidgetType type_id = -1;


/*
 *-----------------------------
 * new / free
 *-----------------------------
 */

ItemData *item_new(GdkPixbuf *pb, gint x, gint y, gint sections)
{
	ItemData *item;

	item_type_init();

	util_size(&x);
	util_size(&y);

	if (!pb) return NULL;
	if (sections < 1) sections = 1;

	item = g_new0(ItemData, 1);

	item->overlay = util_size_pixbuf(pb, TRUE);
	item->width = gdk_pixbuf_get_width(item->overlay);
	item->height = gdk_pixbuf_get_height(item->overlay) / sections;
	item->sections = sections;
	item->x = x;
	item->y = y;
	item->current = 0;

	item->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, item->width, gdk_pixbuf_get_height(item->overlay));

	return item;
}

ItemData *item_new_from_data(gchar **data, gint x, gint y, gint sections)
{
	GdkPixbuf *pb;

	pb = gdk_pixbuf_new_from_xpm_data((const char **)data);

	return item_new(pb, x, y, sections);
}

ItemData *item_new_from_file(const gchar *file, gint x, gint y, gint sections)
{
	GdkPixbuf *pb;

	pb = util_pixbuf_new_from_file(file);

	return item_new(pb, x, y, sections);
}

void item_free(ItemData *item)
{
	if (!item) return;
	if (item->pixbuf) gdk_pixbuf_unref(item->pixbuf);
	if (item->overlay) gdk_pixbuf_unref(item->overlay);
	g_free(item);
}

static void item_free_cb(gpointer data)
{
	item_free((ItemData *)data);
}

/*
 *-----------------------------
 * draw
 *-----------------------------
 */

static void item_draw_real(ItemData *item, gint section, GdkPixbuf *pb, UIData *ui)
{
	if (!item || section >= item->sections) return;

	item->current = section;

	pixbuf_copy_area(item->pixbuf, 0, item->height * section,
			 pb, item->x, item->y,
			 item->width, item->height, FALSE);
	ui_display_render_area(ui, item->x, item->y, item->width, item->height, NULL);
}

static void item_draw(gpointer data, const gchar *key, gint update, gint force, GdkPixbuf *pb, UIData *ui)
{
	ItemData *item = data;
	gint section;

	if (!item) return;

	section = item->current;
	if (update)
		{
		ItemCallbackData *cd;

		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->status_get_func)
			{
			section = cd->status_get_func(item, key, cd->status_get_data);
			}
		}

	if (force || section != item->current)
		{
		item_draw_real(item, section, pb, ui);
		}
}

static void item_section_animate(ItemData *item, gint enabled, GdkPixbuf *pb, UIData *ui)
{
	if (!item) return;

	if (enabled && item->sections > 1)
		{
		item->current++;
		if (item->current >= item->sections) item->current = 1;
		item_draw_real(item, item->current, pb, ui);
		}
	else
		{
		item_draw_real(item, 0, pb, ui);
		}
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static void item_back_set(gpointer data, GdkPixbuf *pb)
{
	ItemData *item = data;
	gint i;

	if (!item) return;

	for (i = 0; i < item->sections; i++)
		{
		pixbuf_copy_area(pb, item->x, item->y, item->pixbuf,
				 0, i * item->height, item->width, item->height, FALSE);
		}

	pixbuf_copy_area_alpha(item->overlay, 0, 0,
			       item->pixbuf, 0, 0, item->width, gdk_pixbuf_get_height(item->overlay), 255);
}

static gint item_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	ItemData *item = widget;

	*x = item->x;
	*y = item->y;
	*w = item->width;
	*h = item->height;

	return TRUE;
}

static void item_set_coord(gpointer widget, gint x, gint y)
{
	ItemData *item = widget;

	item->x = x;
	item->y = y;
}

static WidgetData *item_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	ItemData *item;
	gchar *filename;
	gint x, y;
	gint sections;

	/* req */
	if (!key_list_read_int(list, "x", &x)) return NULL;
	if (!key_list_read_int(list, "y", &y)) return NULL;
	if (!key_list_read_int(list, "sections", &sections)) return NULL;
	filename = key_list_read_path(list, "image", skin_dir);
	if (!filename) return NULL;

	/* opt */
#if 0
	animated = key_list_read_bool(list, "animated");
#endif

	item = item_new_from_file(filename, x, y, sections);

	if (item)
		{
		wd = item_register(skin, item, key, NULL);

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

WidgetData *item_register(SkinData *skin, ItemData *item, const gchar *key, const gchar *text_id)
{
	return skin_register_widget(skin, key, text_id, type_id, item);
}

WidgetData *item_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				  gint sections,
				  SkinData *skin, const gchar *text_id)
{
	ItemData *item;

	item = item_new_from_data(data, x, y, sections);
	return item_register(skin, item, key, text_id);
}

RegisterData *item_register_key(const gchar *key, UIData *ui,
				gint (*status_get_func)(ItemData *item, const gchar *key, gpointer data), gpointer status_get_data)
{
	ItemCallbackData *cd;

	item_type_init();

	cd = g_new0(ItemCallbackData, 1);

	cd->status_get_func = status_get_func;
	cd->status_get_data = status_get_data;

	return ui_register_key(ui, key, type_id, cd, sizeof(ItemCallbackData));
}

/*
 *-----------------------------
 * app funcs
 *-----------------------------
 */

static void item_section_animate_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ItemData *item;
	gint enabled;

	item = wd->widget;
	enabled = !(GPOINTER_TO_INT(data));

	item_section_animate(item, enabled, pb, ui);
}

gint item_animation_step(const gchar *key, UIData *ui, gint report_only, gint reset)
{
	if (report_only)
		{
		WidgetData *wd;
		ItemData *item;

		wd = skin_widget_get_by_key(ui->skin, key, type_id);
		if (!wd) return FALSE;

		item = wd->widget;
		return (item->current != 0);
		}

	return skin_widget_for_each_key(ui, key, type_id, item_section_animate_cb, GINT_TO_POINTER(reset));
}

gint item_section_get(const gchar *key, UIData *ui)
{
	WidgetData *wd;
	ItemData *item;

	wd = skin_widget_get_by_key(ui->skin, key, type_id);
	if (!wd) return 0;

	item = wd->widget;
	return item->current;
}

static void item_section_set_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ItemData *item;
	gint section;

	item = wd->widget;
	section = GPOINTER_TO_INT(data);

	if (section != item->current) item_draw_real(item, section, pb, ui);
}

gint item_section_set(const gchar *key, UIData *ui, gint section)
{
	return skin_widget_for_each_key(ui, key, type_id, item_section_set_cb, GINT_TO_POINTER(section));
}

static void item_section_percent_set_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ItemData *item;
	gint section;
	gfloat *val = data;

	item = wd->widget;
	section = rint(*val * (float)(item->sections - 1));

	if (section != item->current) item_draw_real(item, section, pb, ui);
}

gint item_section_set_by_percent(const gchar *key, UIData *ui, gfloat value)
{
	return skin_widget_for_each_key(ui, key, type_id, item_section_percent_set_cb, &value);
}

gint item_section_by_percent(ItemData *item, gfloat value)
{
	if (!item) return 0;

	return (gint)rint(value * ((gfloat)item->sections - 1));
}

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType item_type_id(void)
{
	return type_id;
}

void item_type_init(void)
{
	WidgetObjectData *od;

	if (type_id != -1) return;

	od = ui_widget_type_new("item");
	type_id = od->type;

	od->func_draw = item_draw;
	od->func_back = item_back_set;
	od->func_free = item_free_cb;

	od->func_get_geometry = item_get_geometry;
	od->func_set_coord = item_set_coord;

	od->func_parse = item_parse;

	item_type_init_edit(od);
}


