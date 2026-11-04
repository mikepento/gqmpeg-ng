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

#include "ui2_list.h"
#include "ui2_list_edit.h"

#include "ui2_display.h"
#include "ui2_main.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"

#include "ui2_button.h"
#include "ui2_slider.h"


#include <gdk/gdkkeysyms.h> /* for key values */


#define UI2_LIST_SCROLL_DELAY 200
#define UI2_LIST_SCROLL_DELAY_FAST 67
#define UI2_LIST_SCROLL_SPEEDUP 4
#define UI2_LIST_SELECT_TIME 333

typedef struct _ListCallbackData ListCallbackData;
struct _ListCallbackData
{
	gint (*length_request_func)(ListData *list, const gchar *key, gpointer data);
	gint (*row_info_func)(ListData *list, const gchar *key, gint row, ListRowData *rd, gpointer data);
	void (*row_click_func)(ListData *list, const gchar *key, gint row, ListRowData *rd, gint button, gpointer data);
	void (*row_select_func)(ListData *list, const gchar *key, gint row, ListRowData *rd, gpointer data);
	gint (*row_move_func)(ListData *list, const gchar *key, gint source_row, gint dest_row, gpointer data);

	gpointer length_request_data;
	gpointer row_info_data;
	gpointer row_click_data;
	gpointer row_select_data;
	gpointer row_move_data;

	/* only use for menu mode */
	gint (*menu_move_func)(ListData *list, const gchar *key, gint row, gint activated, gint up, gpointer data);
	gpointer menu_move_data;
};


static WidgetType type_id = -1;


static void list_rows_free(ListData *ld);
static gint list_length_sync(ListData *ld, gint length, gint force);
static void list_scroll_update_widgets(ListData *ld);


/*
 *-----------------------------
 * new / free
 *-----------------------------
 */

ListData *list_new(GdkPixbuf *pb, gint x, gint y, gint w, gint h, gint sizeable, gint columns,
		   gint border_top, gint border_right, gint border_bottom, gint border_left, gint stretch)
{
	ListData *ld;

	list_type_init();

	if (!pb) return NULL;

	ld = g_new0(ListData, 1);

	util_size(&x);
	util_size(&y);
	util_size(&w);
	util_size(&h);

	util_size(&border_top);
	util_size(&border_right);
	util_size(&border_bottom);
	util_size(&border_left);

	ld->x = x;
	ld->y = y;
	ld->width = w;
	ld->height = h;
	ld->sizeable = sizeable;

	ld->border_top = border_top;
	ld->border_right = border_right;
	ld->border_bottom = border_bottom;
	ld->border_left = border_left;
	ld->stretch = stretch;

	ld->column_count = columns;
	ld->column_widths = g_new0(gint, columns);
	ld->column_flags = g_new0(ListColumnFlags, columns);
	ld->column_real_widths = g_new0(gint, columns);
	ld->column_keys = g_new0(gchar *, columns);

	ld->columns_right_justify = FALSE;

	ld->font = NULL;
	ld->text_r = 0;
	ld->text_g = 0;
	ld->text_b = 0;
	ld->text_a = 255;

	ld->overlay = util_size_pixbuf(pb, TRUE);

	ld->timeout_id = -1;
	ld->row_prelight = -1;
	ld->row_press = -1;
	ld->focus_row = -1;

	ld->scroll_idle_id = -1;
	
	return ld;
}

void list_set_column_attributes(ListData *ld, gint column, gint length, ListColumnFlags flags, const gchar *key)
{
	if (!ld) return;
	if (column >= ld->column_count) return;

	if (flags & UI_LIST_COLUMN_SIZE_FIXED) util_size(&length);

	ld->column_widths[column] = length;
	ld->column_flags[column] = flags;
	g_free(ld->column_keys[column]);
	ld->column_keys[column] = g_strdup(key);

	if (key && strcmp(key, "flags") == 0) ld->flag_column = column;
}

void list_set_column_justify(ListData *ld, gint justify_right)
{
	if (!ld) return;
	ld->columns_right_justify = justify_right;
}

void list_image_row(ListData *ld, GdkPixbuf *pb, gint has_press, gint has_prelight,
		    gint border_left, gint border_right, gint stretch, GdkPixbuf *divider)
{
	if (!ld || !pb) return;

	util_size(&border_left);
	util_size(&border_right);

	ld->row_overlay = util_size_pixbuf(pb, TRUE);
	ld->row_height = gdk_pixbuf_get_height(ld->row_overlay) / (1 + (has_press) + (has_prelight));
	ld->row_width = gdk_pixbuf_get_width(ld->row_overlay);
	ld->row_border_left = border_left;
	ld->row_border_right = border_right;
	ld->row_stretch = stretch;
	ld->row_has_press = has_press;
	ld->row_has_prelight = has_prelight;

	if (divider)
		{
		ld->divider_overlay = util_size_pixbuf(divider, TRUE);
		ld->divider_width = gdk_pixbuf_get_width(ld->divider_overlay);
		ld->divider_height = MIN(gdk_pixbuf_get_height(ld->divider_overlay), ld->row_height);
		}
	else
		{
		ld->divider_width = 3;
		ld->divider_height = 0;
		}
}

void list_image_row_flag(ListData *ld, GdkPixbuf *pb, gint sections, gint column)
{
	if (!ld || !pb) return;

	ld->flag_overlay = util_size_pixbuf(pb, TRUE);
	ld->flag_sections = sections;
	ld->flag_column = column;

	ld->flag_width = gdk_pixbuf_get_width(ld->flag_overlay);
	ld->flag_height = gdk_pixbuf_get_height(ld->flag_overlay) / sections;
}

void list_set_font(ListData *ld, const gchar *description, GdkPixbuf *pb, gint extended,
		   guint8 r, guint8 g, guint8 b, guint8 a)
{
	if (!ld || (!description && !pb)) return;

	util_color(&r, &g, &b, &a);

	ld->text_r = r;
	ld->text_g = g;
	ld->text_b = b;
	ld->text_a = a;
	
	if (ld->font) return;

	if (pb)
		{
		ld->font = font_new(pb, extended);

		/* no need to pad text end with spaces for lists */
		if (ld->font) ld->font->fill = FALSE;
		}
	else
		{
		ld->font = font_new_from_x(description);
		}

	if (!ld->font)
		{
		printf("ui2_list.c: warning failed to create font for list\n");
		}
}

void list_free(ListData *ld)
{
	gint i;

	if (!ld) return;

	if (ld->timeout_id != -1) g_source_remove(ld->timeout_id);
	if (ld->scroll_idle_id != -1) gtk_idle_remove(ld->scroll_idle_id);

	if (ld->overlay) gdk_pixbuf_unref(ld->overlay);
	if (ld->pixbuf) gdk_pixbuf_unref(ld->pixbuf);
	if (ld->clip_mask) gdk_pixbuf_unref(ld->clip_mask);
	if (ld->row_overlay) gdk_pixbuf_unref(ld->row_overlay);
	if (ld->row_overlay_center) gdk_pixbuf_unref(ld->row_overlay_center);
	if (ld->divider_overlay) gdk_pixbuf_unref(ld->divider_overlay);
	if (ld->flag_overlay) gdk_pixbuf_unref(ld->flag_overlay);

	font_unref(ld->font);

	g_free(ld->column_widths);
	g_free(ld->column_flags);
	g_free(ld->column_real_widths);

	for (i = 0; i < ld->column_count; i++) g_free(ld->column_keys[i]);
	g_free(ld->column_keys);

	list_rows_free(ld);

	g_free(ld);
}

static void list_free_cb(gpointer data)
{
	list_free((ListData *)data);
}

/*
 *-----------------------------
 * row data handlers
 *-----------------------------
 */

static ListRowData *list_row_new(guint columns)
{
	ListRowData *rd;

	rd = g_new0(ListRowData, 1);
	rd->cells = columns;
	rd->text = g_new0(gchar*, columns + 1);
	rd->sensitive = TRUE;
	rd->flag_mask = -1;

	return rd;
}

static void list_row_free(ListRowData *rd)
{
	if (!rd) return;

	if (rd->text)
		{
		gint i;

		for(i = 0; i < rd->cells; i++) g_free(rd->text[i]);
		g_free(rd->text);
		}
	g_free(rd);
}

static void list_row_clear_text(ListRowData *rd)
{
	gint i;

	if (!rd) return;

	for (i = 0; i < rd->cells; i++)
		{
		g_free(rd->text[i]);
		rd->text[i] = NULL;
		}

	/* also clear the flag here */
	rd->flag_mask = -1;
}

static void list_rows_free(ListData *ld)
{
	GList *work;

	work = ld->row_list;
	while (work)
		{
		ListRowData *rd = work->data;
		work = work->next;
		list_row_free(rd);
		}

	g_list_free(ld->row_list);
	ld->row_list = NULL;
}

void list_row_text_set(ListRowData *rd, gint column, const gchar *text)
{
	if (!rd || column >= rd->cells) return;

	g_free(rd->text[column]);
	rd->text[column] = g_strdup(text);
}

static const gchar *list_row_text_get(ListRowData *rd, gint column)
{
	if (!rd || column >= rd->cells) return NULL;

	return rd->text[column];
}

void list_row_column_set_text(ListData *ld, ListRowData *rd, const gchar *column_key, const gchar *text)
{
	gint column;

	if (!ld || !rd || !column_key) return;

	for (column = 0; column < ld->column_count; column++)
		{
		if (ld->column_keys[column] && strcmp(ld->column_keys[column], column_key) == 0)
			{
			list_row_text_set(rd, column, text);
			}
		}
}

void list_row_set_flag(ListRowData *rd, gint flag)
{
	if (rd) rd->flag_mask = flag;
}

void list_row_set_sensitive(ListRowData *rd, gint sensitive)
{
	if (rd) rd->sensitive = sensitive;
}

/*
 *-----------------------------
 * syncing utils
 *-----------------------------
 */

static void list_sync(ListData *ld)
{
	gint i;
	gint w;
	gint area;
	gint row_oh;

	if (!ld->force_sync &&
	    ld->pixbuf &&
	    gdk_pixbuf_get_width(ld->pixbuf) == ld->width &&
	    gdk_pixbuf_get_height(ld->pixbuf) == ld->height &&
	    ld->region_width == ld->width - ld->border_left - ld->border_right &&
	    ld->region_height == ld->height - ld->border_top - ld->border_bottom &&
	    ld->row_overlay_center) return;

	ld->force_sync = FALSE;

	if (ld->pixbuf) gdk_pixbuf_unref(ld->pixbuf);
	ld->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, ld->width, ld->height);

	ld->region_width = ld->width - ld->border_left - ld->border_right;
	ld->region_height = ld->height - ld->border_top - ld->border_bottom;

	row_oh = gdk_pixbuf_get_height(ld->row_overlay);
	area = ld->region_width - ld->row_border_left - ld->row_border_right;

	if (ld->row_overlay_center) gdk_pixbuf_unref(ld->row_overlay_center);
	ld->row_overlay_center = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
						gdk_pixbuf_get_has_alpha(ld->row_overlay),
						8, area, row_oh);
	pixbuf_copy_fill(ld->row_overlay, ld->row_border_left, 0,
			 ld->row_width - ld->row_border_left - ld->row_border_right, row_oh,
			 ld->row_overlay_center, 0, 0,
			 area, row_oh,
			 ld->row_stretch, TRUE);

	w = 0;
	for (i = 0; i < ld->column_count; i++)
		{
		gint c;
		gint s;
		gint p;

		s = w;

		if (ld->columns_right_justify)
			{
			p = (ld->column_count - 1) - i;
			}
		else
			{
			p = i;
			}

		if (w >= area)
			{
			ld->column_real_widths[p] = 0;
			continue;
			}

		if (i == ld->column_count - 1)
			{
			c = area - w;
			w += c;
			}
		else
			{
			if (ld->column_flags[p] & UI_LIST_COLUMN_SIZE_PROPORTIONAL)
				{
				c = ((float)ld->column_widths[p] / 100.0 * ld->region_width) - ld->divider_width;
				if (c < 1) c = 1;
				}
			else
				{
				c = ld->column_widths[p];
				}
			w += c + ld->divider_width;
			}
		if (w >= area) c = area - s;
		ld->column_real_widths[p] = c;
		if (debug_mode) printf("column %d with width of %d (%d)\n", p, c, ld->column_widths[p]);
		}

	/* vertical centering offsets */
	ld->text_voff = MAX((ld->row_height - ld->font->char_height) / 2, 0);
	ld->divider_voff = MAX((ld->row_height - ld->divider_height) / 2, 0);
	ld->flag_voff = MAX((ld->row_height - ld->flag_height) / 2, 0);

	list_length_sync(ld, 0, TRUE);
}

static gint list_rows_visible(ListData *ld)
{
	return (ld->region_height / ld->row_height);
}

static gint list_scroll_clamp(ListData *ld)
{
	gint vis;
	gint old_s, old_e;

	vis = list_rows_visible(ld);

	old_s = ld->row_start;
	old_e = ld->row_end;

	ld->row_start = CLAMP(ld->row_start, 0, MAX(0, ld->row_count - vis));
	ld->row_end = CLAMP(ld->row_start + vis - 1, ld->row_start, ld->row_count - 1);

	list_scroll_update_widgets(ld);

	return (ld->row_start != old_s || ld->row_end != old_e);
}

static gint list_length_sync(ListData *ld, gint length, gint force)
{

	if (ld->row_count == length && !force) return FALSE;
	if (!force) ld->row_count = length;

	if (ld->focus_row >= ld->row_count)
		{
		ld->focus_row = ld->row_count - 1;
		}
	else if (ld->focus_row < 0 && ld->row_count > 0)
		{
		ld->focus_row = 0;
		}

	if (list_scroll_clamp(ld))
		{
		gint i;
		list_rows_free(ld);

		if (ld->row_count > 0)
		    for (i = 0; i <= ld->row_end - ld->row_start; i++)
			{
			ld->row_list = g_list_prepend(ld->row_list, list_row_new(ld->column_count));
			}

		return TRUE;
		}

	return FALSE;
}

static gint list_row_sync(ListData *ld, UIData *ui, const gchar *key, gint row)
{
	ListCallbackData *cd;
	ListRowData *rd;

	rd = g_list_nth_data(ld->row_list, row - ld->row_start);
	if (!rd) return FALSE;

	cd = ui_get_registered_callbacks(ui, key, type_id);
	if (!cd || !cd->row_info_func) return FALSE;

	list_row_clear_text(rd);
	return cd->row_info_func(ld, key, row, rd, cd->row_info_data);
}

static gint list_row_sync_n(ListData *ld, UIData *ui, const gchar *key,
			    gint start, gint end, ListCallbackData *cd)
{
	GList *work;
	gint row;
	gint ret = FALSE;

	if (!cd) cd = ui_get_registered_callbacks(ui, key, type_id);
	if (!cd || !cd->row_info_func) return FALSE;

	row = ld->row_start + start;

	work = g_list_nth(ld->row_list, start);
	while (work && start <= end)
		{
		ListRowData *rd = work->data;

		list_row_clear_text(rd);
		ret |= cd->row_info_func(ld, key, row, rd, cd->row_info_data);
		work = work->next;
		start++;
		row++;
		}

	return ret;
}

static gint list_row_sync_all(ListData *ld, UIData *ui, const gchar *key, ListCallbackData *cd)
{
	gint row;
	GList *work;
	gint ret = FALSE;

	if (!cd) cd = ui_get_registered_callbacks(ui, key, type_id);
	if (!cd || !cd->row_info_func) return FALSE;

	row = ld->row_start;
	work = ld->row_list;
	while (work)
		{
		ListRowData *rd = work->data;
		work = work->next;

		ret |= cd->row_info_func(ld, key, row, rd, cd->row_info_data);

		row++;
		}

	return ret;
}

/*
 *-----------------------------
 * draw, etc.
 *-----------------------------
 */

static void list_draw_text(ListData *ld, GdkPixbuf *pb, const gchar *text, gint x, gint y,
			   gint w, gint right_justify, gint alpha)
{
	gint p;

	if (!text) return;

	if (right_justify)
		{
		p = w - font_string_length(ld->font, text);
		if (p < 0) p = 0;
		}
	else
		{
		p = 0;
		}

	font_draw(ld->font, text, 0, x + p, y,
		  w - p, MIN(ld->row_height - ld->text_voff, ld->font->char_height), 0,
		  ld->text_r, ld->text_g, ld->text_b, ld->text_a,
		  pb, ld->ui, alpha);
}

static void list_draw_cell(ListData *ld, GdkPixbuf *pb, gint x, gint y, gint row, gint column, gint divider)
{
	ListRowData *rd;
	const gchar *text;
	gint w;
	gint alpha;

	rd = g_list_nth_data(ld->row_list, row);
	if (!rd) return;

	alpha = (rd->sensitive) ? 255 : 86;

	w = ld->column_real_widths[column];
	text = list_row_text_get(rd, column);
	if (text) list_draw_text(ld, pb, text, ld->x + x, ld->y + y + ld->text_voff, w,
				 (ld->column_flags[column] & UI_LIST_COLUMN_JUSTIFY_RIGHT),
				 alpha);
	if (divider && ld->divider_overlay)
		{
		pixbuf_copy_area_alpha(ld->divider_overlay, 0, 0,
				       pb, ld->x + x + w, ld->y + y + ld->divider_voff,
				       ld->divider_width, ld->divider_height, alpha);
		}
	if (column == ld->flag_column && ld->flag_overlay && rd->flag_mask >= 0 && rd->flag_mask < ld->flag_sections)
		{
		pixbuf_copy_area_alpha(ld->flag_overlay, 0, rd->flag_mask * ld->flag_height,
				       pb, ld->x + x, ld->y + y + ld->flag_voff,
				       MIN(ld->flag_width, w), MIN(ld->flag_height, ld->row_height), alpha);
		}
}

static void list_draw_row_image(ListData *ld, GdkPixbuf *pb, gint y_off, gint x, gint y)
{
	pixbuf_copy_area_alpha(ld->row_overlay, 0, y_off,
		       pb, ld->x + x, ld->y + y, ld->row_border_left, ld->row_height, 255);
	pixbuf_copy_area_alpha(ld->row_overlay_center, 0, y_off,
		       pb, ld->x + x + ld->row_border_left, ld->y + y,
		       ld->region_width - ld->row_border_left - ld->row_border_right, ld->row_height, 255);
	pixbuf_copy_area_alpha(ld->row_overlay, ld->row_width - ld->row_border_right, y_off,
		       pb, ld->x + x + ld->region_width - ld->row_border_right, ld->y + y,
		       ld->row_border_right, ld->row_height, 255);
}

static void list_draw_row(ListData *ld, GdkPixbuf *pb, UIData *ui, gint row, gint render)
{
	gint x, y;
	gint i;
	gint row_has_alpha;
	gint pressed;
	gint prelit;

	x = ld->border_left;
	y = row * ld->row_height + ld->border_top;

	row_has_alpha = gdk_pixbuf_get_has_alpha(ld->row_overlay);

	if (row_has_alpha)
		{
		pixbuf_copy_area(ld->pixbuf, x, y,
				 pb, ld->x + x, ld->y + y, ld->region_width, ld->row_height,
				 FALSE);
		}

	pressed = (ld->row_has_press && row == ld->row_press - ld->row_start);
	prelit = (ld->row_has_prelight && row == ld->row_prelight - ld->row_start);

	if (row_has_alpha || (!pressed && !prelit))
		{
		list_draw_row_image(ld, pb, 0, x, y);
		}
	if (pressed)
		{
		list_draw_row_image(ld, pb, ld->row_height, x, y);
		}
	if (prelit && (!row_has_alpha || slik_transparency_force))
		{
		list_draw_row_image(ld, pb, ld->row_height * (ld->row_has_press + 1), x, y);
		}

	x += ld->row_border_left;
	i = 0;
	while (i < ld->column_count && ld->column_real_widths[i] > 0)
		{
		gint divider;

		divider = !(i == ld->column_count - 1 || ld->column_real_widths[i+1] < 1);
		list_draw_cell(ld, pb, x, y, row, i, divider);
		x += ld->column_real_widths[i] + ld->divider_width;
		i++;
		}

	x = ld->border_left;

	if (prelit && (row_has_alpha && !slik_transparency_force))
		{
		list_draw_row_image(ld, pb, ld->row_height * (ld->row_has_press + 1), x, y);
		}

	if (render && ui)
		{
		ui_display_render_area(ui, ld->x + ld->border_left, ld->y + y,
				       ld->region_width, ld->row_height, ld->wd);
		}
}

static void list_draw_empty_rows(ListData *ld, GdkPixbuf *pb, UIData *ui, gint render)
{
	gint vis;
	gint y;

	vis = list_rows_visible(ld);

	y = 0;
	if (ld->row_count > 0) y += vis < 1 ? 0 : ld->row_height * (ld->row_end - ld->row_start + 1);
	if (y >= ld->region_height) return;

	pixbuf_copy_area(ld->pixbuf, ld->border_left, ld->border_top + y,
			 pb, ld->x + ld->border_left, ld->y + ld->border_top + y,
			 ld->region_width, ld->region_height - y, FALSE);

	if (render && ui)
		{
		ui_display_render_area(ui, ld->x + ld->border_left, ld->y + ld->border_top + y,
				      ld->region_width, ld->region_height - y, ld->wd);
		}
}

static void list_draw_border(ListData *ld, GdkPixbuf *pb)
{
	pixbuf_copy_area(ld->pixbuf, 0, 0,
			 pb, ld->x, ld->y,
			 ld->width, ld->border_top, FALSE);
	pixbuf_copy_area(ld->pixbuf, 0, ld->border_top,
			 pb, ld->x, ld->y + ld->border_top,
			 ld->border_left, ld->region_height, FALSE);
	pixbuf_copy_area(ld->pixbuf, ld->width - ld->border_right, ld->border_top,
			 pb, ld->x + ld->width - ld->border_right, ld->y + ld->border_top,
			 ld->border_right, ld->region_height, FALSE);
	pixbuf_copy_area(ld->pixbuf, 0, ld->height - ld->border_bottom,
			 pb, ld->x, ld->y + ld->height - ld->border_bottom,
			 ld->width, ld->border_bottom, FALSE);
}

static void list_redraw_row(ListData *ld, GdkPixbuf *pb, UIData *ui, gint row)
{
	if (ld->row_count < 1 || row < 0 || row > ld->row_end - ld->row_start + 1) return;
	list_draw_row(ld, pb, ui, row, TRUE);
}

static void list_draw_rows(ListData *ld, GdkPixbuf *pb, UIData *ui, gint start, gint end, gint render)
{
	gint row;

	row = start;
	while (row <= end)
		{
		list_draw_row(ld, pb, NULL, row, FALSE);
		row++;
		}

	if (render && ui)
		{
		ui_display_render_area(ui, ld->x + ld->border_left, ld->y + ld->border_top + start * ld->row_height,
				       ld->region_width, (end - start + 1) * ld->row_height, ld->wd);
		}
}

static void list_draw_all_rows(ListData *ld, GdkPixbuf *pb, UIData *ui, gint render)
{
	if (ld->row_count > 0 && list_rows_visible(ld) > 0)
		{
		list_draw_rows(ld, pb, ui, 0, ld->row_end - ld->row_start, render);
		}
	list_draw_empty_rows(ld, pb, ui, render);
}

static void list_draw_all(ListData *ld, GdkPixbuf *pb, UIData *ui)
{
	list_draw_border(ld, pb);
	list_draw_all_rows(ld, pb, ui, FALSE);

	ui_display_render_area(ui, ld->x, ld->y, ld->width, ld->height, ld->wd);
}

/*
 *-----------------------------
 * scrolling
 *-----------------------------
 */

static void list_row_scroll_by_row(ListData *ld, GdkPixbuf *pb, UIData *ui, const gchar *key, gint rows)
{
	gint vis;
	gint old_s;

	old_s = ld->row_start;

	ld->row_start += rows;
	list_scroll_clamp(ld);

	if (old_s == ld->row_start) return;

	vis = list_rows_visible(ld);

	if (abs(old_s - ld->row_start) < vis)
		{
		GList *link;
		GList *tail;

		if (ld->row_start > old_s)
			{
			link = g_list_nth(ld->row_list, abs(ld->row_start - old_s));
			}
		else
			{
			link = g_list_nth(ld->row_list, vis - abs(ld->row_start - old_s));
			}

		tail = link->prev;
		tail->next = NULL;
		link->prev = NULL;

		tail = g_list_last(link);
		tail->next = ld->row_list;
		ld->row_list->prev = tail;

		ld->row_list = link;

		if (ld->row_start > old_s)
			{
			list_row_sync_n(ld, ui, key, vis - (ld->row_start - old_s), vis - 1, NULL);
			}
		else
			{
			list_row_sync_n(ld, ui, key, 0, old_s - ld->row_start - 1, NULL);
			}
		}
	else
		{
		list_row_sync_all(ld, ui, key, NULL);
		}

	if (ld->focus_row < ld->row_start) ld->focus_row = ld->row_start;
	if (ld->focus_row > ld->row_end) ld->focus_row = ld->row_end;

	if (ld->row_prelight >= 0)
		{
		ld->row_prelight += ld->row_start - old_s;
		}

	list_draw_rows(ld, pb, ui, 0, ld->row_end - ld->row_start, TRUE);
}

static void list_row_scroll(ListData *ld, GdkPixbuf *pb, UIData *ui, const gchar *key, gfloat val)
{
	gint row;
	gint vis;

	vis = list_rows_visible(ld);

	if (ld->row_count <= vis) return;

	row = CLAMP(val * (ld->row_count - vis), 0, ld->row_count - vis);

	list_row_scroll_by_row(ld, pb, ui, key, row - ld->row_start);
}

/*
 *-----------------------------
 * selection, misc
 *-----------------------------
 */

static void list_row_prelight_set(ListData *ld, GdkPixbuf *pb, UIData *ui, gint row)
{
	gint old_r;

	if (ld->row_prelight == row) return;

	old_r = ld->row_prelight;
	ld->row_prelight = row;

	if (row >= 0) ld->row_press = -1;

	if (old_r >= 0) list_redraw_row(ld, pb, ui, old_r - ld->row_start);
	if (ld->row_prelight >= 0) list_redraw_row(ld, pb, ui, ld->row_prelight - ld->row_start);
}

static void list_row_press_set(ListData *ld, GdkPixbuf *pb, UIData *ui, gint row)
{
	gint old_r;

	if (ld->row_press == row) return;

	old_r = ld->row_press;
	ld->row_press = row;

	if (row >= 0) ld->row_prelight = -1;

	if (old_r >= 0) list_redraw_row(ld, pb, ui, old_r - ld->row_start);
	if (ld->row_press >= 0) list_redraw_row(ld, pb, ui, ld->row_press - ld->row_start);
}

static gint list_test_proximity(ListData *ld, gint x, gint y)
{
	if (!ld) return FALSE;

	/* we only test region that is useful */
	if (x < ld->x + ld->border_left || x >= ld->x + ld->border_left + ld->region_width ||
	    y < ld->y + ld->border_top || y >= ld->y + ld->border_top + ld->region_height) return FALSE;

	return TRUE;
}

static gint list_row_find_proximity(ListData *ld, gint x, gint y)
{
	if (ld->row_count < 1 || ld->row_height < 1 ||
	    y < ld->y + ld->border_top ||
	    y >= ld->y + ld->border_top + (ld->row_end - ld->row_start + 1) * ld->row_height) return -1;

	return ld->row_start + ((y - ld->y - ld->border_top) / ld->row_height);
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static void list_draw(gpointer data, const gchar *key, gint update, gint force, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld = data;

	if (update)
		{
		ListCallbackData *cd;
		gint length = 0;

		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->length_request_func)
                        {
			length = cd->length_request_func(ld, key, cd->length_request_data);
                        }
		list_length_sync(ld, length, FALSE);
		list_row_sync_all(ld, ui, key, cd);
		}
	if (force)
		{
		list_draw_all(ld, pb, ui);
		}
	else if (update)
		{
		list_draw_all_rows(ld, pb, ui, TRUE);
		}
}

static void list_reset(gpointer data, const gchar *key, GdkPixbuf *pb, UIData *ui)
{
        ListData *ld = data;

	list_row_prelight_set(ld, pb, ui, -1);
}

static void list_motion(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld = data;
	gint old_prelight;

	if (ld->pressed)
		{
		gint row;

		row = list_row_find_proximity(ld, x, y);
		if (row == ld->press_row || ld->menu_style_select)
			{
			gint changed = (row != ld->row_press);

			list_row_press_set(ld, pb, ui, row);

			if (changed && ld->menu_style_select)
				{
				ListCallbackData *cd;
	
				cd = ui_get_registered_callbacks(ui, key, type_id);
				if (cd && cd->menu_move_func)
					{
					cd->menu_move_func(ld, key, row, FALSE, FALSE, cd->menu_move_data);
					}
				}

			}
		else
			{
			list_row_press_set(ld, pb, ui, -1);
			}

		return;
		}

	old_prelight = ld->row_prelight;

	if (list_test_proximity(ld, x, y))
		{
		gint row;

		row = list_row_find_proximity(ld, x, y);
		list_row_prelight_set(ld, pb, ui, row);
		}
	else if (ld->row_prelight != -1)
		{
		list_row_prelight_set(ld, pb, ui, -1);
		}

	if (old_prelight != ld->row_prelight && ld->menu_style_select)
		{
		ListCallbackData *cd;
	
		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->menu_move_func)
			{
			cd->menu_move_func(ld, key, ld->row_prelight, FALSE, FALSE, cd->menu_move_data);
			}
		}
}

static gint list_press(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld = data;

	if (list_test_proximity(ld, x, y))
		{
		ListRowData *rd;
		GdkEvent *event;
		guint32 t;
		gint row;

		row = list_row_find_proximity(ld, x, y);
		if (row == -1) return FALSE;

		ld->pressed = TRUE;
		ld->press_x = x;
		ld->press_y = y;
		ld->in_drag = FALSE;

		/* peek at current event's time */
		event = gtk_get_current_event();
		if (event)
			{
			t = gdk_event_get_time(event);
			gdk_event_free(event);
			}
		else
			{
			t = 0;
			}

		rd = g_list_nth_data(ld->row_list, row - ld->row_start);
		if (rd)
			{
			ListCallbackData *cd;

			cd = ui_get_registered_callbacks(ui, key, type_id);

			if (row == ld->press_row && t - ld->click_time < UI2_LIST_SELECT_TIME)
				{
				if (cd && cd->row_select_func)
					{
					cd->row_select_func(ld, key, row, NULL, cd->row_select_data);
					}
				t = 0;
				}
			}
		else
			{
			return FALSE;
			}

		if (row != ld->focus_row)
			{
			gint old_row;

			old_row = ld->focus_row;
			ld->focus_row = row;
			
			if (old_row >= 0 && old_row >= ld->row_start && old_row <= ld->row_end)
				{
				list_draw_row(ld, pb, ui, old_row - ld->row_start, TRUE);
				}

			if (ld->menu_style_select)
				{
				ListCallbackData *cd;
	
				cd = ui_get_registered_callbacks(ui, key, type_id);
				if (cd && cd->menu_move_func)
					{
					cd->menu_move_func(ld, key, row, FALSE, FALSE, cd->menu_move_data);
					}
				}
			}

		ld->press_row = row;
		list_row_press_set(ld, pb, ui, row);
		ld->click_time = t;

		return TRUE;
		}

	return FALSE;
}

static void list_release(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld = data;

	if (!ld->pressed) return;

	if (list_test_proximity(ld, x, y))
		{
		gint row;

		row = list_row_find_proximity(ld, x, y);
		if (row != ld->row_press)
			{
			list_row_press_set(ld, pb, ui, -1);
			}
		else if (row >= 0)
			{
			ListCallbackData *cd;

			cd = ui_get_registered_callbacks(ui, key, type_id);

			if (cd && cd->row_click_func)
				{
				cd->row_click_func(ld, key, row, NULL, 1, cd->row_click_data);
				}
			}

		list_row_prelight_set(ld, pb, ui, row);
		}
	else
		{
		list_row_press_set(ld, pb, ui, -1);
		}

	ld->pressed = FALSE;
	ld->in_drag = FALSE;
}

static void list_back_set(gpointer data, GdkPixbuf *pb)
{
	ListData *ld = data;

	list_sync(ld);

	if (!ld->overlay || gdk_pixbuf_get_has_alpha(ld->overlay))
		{
		pixbuf_copy_area(pb, ld->x, ld->y,
				 ld->pixbuf, 0, 0, ld->width, ld->height, TRUE);
		}

	pixbuf_copy_fill_border_alpha(ld->overlay, ld->pixbuf,
				      0, 0,
				      gdk_pixbuf_get_width(ld->pixbuf), gdk_pixbuf_get_height(ld->pixbuf),
				      ld->border_left, FALSE,
				      ld->border_right, FALSE,
				      ld->border_top, FALSE,
				      ld->border_bottom, FALSE,
				      ld->stretch, 255);
}

static gint list_key_event_cb(gpointer widget, const gchar *key, GdkEventKey *event, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld = widget;
	gint ret = FALSE;
	gint new_row;
	gint old_row;

	if (event->state & GDK_CONTROL_MASK) return FALSE;

	old_row = new_row = ld->focus_row;

	switch (event->keyval)
		{
		case GDK_space:
			if (new_row >= 0)
				{
				ListCallbackData *cd;

				cd = ui_get_registered_callbacks(ui, key, type_id);
				if (cd && cd->row_click_func)
					{
					cd->row_click_func(ld, key, new_row, NULL, 0, cd->row_click_data);
					}
				}
			ret = TRUE;
			break;
		case GDK_Return:
		case GDK_KP_Enter:
			if (new_row >= 0)
				{
				ListCallbackData *cd;

				cd = ui_get_registered_callbacks(ui, key, type_id);
				if (cd && cd->row_select_func)
					{
					cd->row_select_func(ld, key, new_row, NULL, cd->row_select_data);
					}
				}
			ret = TRUE;
			break;
		case GDK_Left:
		case GDK_KP_Left:
		case GDK_Right:
		case GDK_KP_Right:
			if (ld->menu_style_select)
				{
				ListCallbackData *cd;

				cd = ui_get_registered_callbacks(ui, key, type_id);
				if (cd && cd->menu_move_func)
					{
					gint up;

					up = (event->keyval == GDK_Right || event->keyval == GDK_KP_Right);
					ret = cd->menu_move_func(ld, key, new_row, TRUE, up, cd->menu_move_data);
					}
				}
			break;
		case GDK_Up:
		case GDK_KP_Up:
			new_row--;
			break;
		case GDK_Down:
		case GDK_KP_Down:
			new_row++;
			break;
		case GDK_Page_Up:
		case GDK_KP_Page_Up:
			new_row -= MAX((list_rows_visible(ld) - 1), 1);
			ret = TRUE;
			break;
		case GDK_Page_Down:
		case GDK_KP_Page_Down:
			new_row += MAX((list_rows_visible(ld) - 1), 1);
			ret = TRUE;
			break;
		case GDK_Home:
		case GDK_KP_Home:
			new_row = 0;
			ret = TRUE;
			break;
		case GDK_End:
		case GDK_KP_End:
			new_row = ld->row_count - 1;
			ret = TRUE;
			break;
		default:
			break;
		}

	if (new_row > ld->row_count - 1)
		{
		new_row = ld->row_count - 1;
		}
	else if (new_row < 0 && ld->row_count > 0)
		{
		new_row = 0;
		}

	ret = (ret || (ld->focus_row != new_row));
	ld->focus_row = new_row;

	if (new_row >= 0)
		{
		if (new_row < ld->row_start)
			{
			list_row_scroll_by_row(ld, pb, ui, key, new_row - ld->row_start);
			}
		else if (new_row > ld->row_end)
			{
			list_row_scroll_by_row(ld, pb, ui, key, new_row - ld->row_end);
			}
		else
			{
			/* scrolling handles redraws, no scroll do it ourselves */
			if (old_row >= ld->row_start && old_row <= ld->row_end)
				{
				list_draw_row(ld, pb, ui, old_row - ld->row_start, TRUE);
				}
			list_draw_row(ld, pb, ui, new_row - ld->row_start, TRUE);
			}

		if (ld->menu_style_select)
			{
			ListCallbackData *cd;

			cd = ui_get_registered_callbacks(ui, key, type_id);
			if (cd && cd->menu_move_func)
				{
				cd->menu_move_func(ld, key, new_row, FALSE, FALSE, cd->menu_move_data);
				}
			}
		}

	return ret;
}

static gint list_focus_draw_cb(gpointer widget, const gchar *key,
			       gint x, gint y, gint w, gint h, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld = widget;
	gint rx, ry, rw, rh;

	if (ld->focus_row < 0 ||
	    ld->focus_row < ld->row_start ||
	    ld->focus_row > ld->row_end) return TRUE;

	rx = ld->border_left;
	ry = (ld->focus_row - ld->row_start) * ld->row_height + ld->border_top;
	rw = ld->region_width;
	rh = ld->row_height;

	ui_display_draw_focus(ui, pb, ld->x + rx, ld->y + ry, rw, rh,
			      x, y, w, h, NULL);

	return TRUE;
}

static gint list_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	ListData *ld = widget;

	*x = ld->x;
	*y = ld->y;
	*w = ld->width;
	*h = ld->height;

	return TRUE;
}

static void list_set_coord(gpointer widget, gint x, gint y)
{
	ListData *ld = widget;

	ld->x = x;
	ld->y = y;
}

static void list_set_size(gpointer widget, gint dev_w, gint dev_h)
{
	ListData *ld = widget;
	UIData *ui;
	WidgetData *wd;

	if (!ld->sizeable) return;

	ui = skin_get_ui(ld->skin);
	if (ui)
		{
		wd = ui_widget_get_by_widget(ui, ld);
		}
	else
		{
		wd = NULL;
		}

	if (!wd || !wd->anchor_right)
		{
		ld->width = MAX(ld->width + dev_w,
			       	ld->border_left + ld->border_right + ld->row_border_left + ld->row_border_right + 4);
		}
	if (!wd || !wd->anchor_bottom)
		{
		ld->height = MAX(ld->height + dev_h, ld->border_top + ld->border_bottom + ld->row_height);
		}

	list_sync(ld);
}

static WidgetData *list_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	ListData *ld;
	gint x, y;
	gint w, h;
	gint sizeable;
	gint border_top;
	gint border_right;
	gint border_bottom;
	gint border_left;
	gint center_stretch;
	gint columns;
	gint columns_right_justify;
	gchar *filename;
	gchar *row_filename;
	gchar *divider_filename;
	gchar *flag_filename;
	gchar *text_filename;
	const gchar *font_desc;
	gint r, g, b, a;
	gint flag_sections;
	gint flag_column;
	gint row_prelight;
	gint row_press;
	gint row_stretch;
	gint row_border_left;
	gint row_border_right;
	gint text_extended;


	/* req */
	if (!key_list_read_int(list, "x", &x)) return NULL;
	if (!key_list_read_int(list, "y", &y)) return NULL;

	if (!key_list_read_int(list, "width", &w)) return NULL;
	if (!key_list_read_int(list, "height", &h)) return NULL;

	if (!key_list_read_int(list, "border_top", &border_top)) return NULL;
	if (!key_list_read_int(list, "border_right", &border_right)) return NULL;
	if (!key_list_read_int(list, "border_bottom", &border_bottom)) return NULL;
	if (!key_list_read_int(list, "border_left", &border_left)) return NULL;

	if (!key_list_read_int(list, "row_border_left", &row_border_left)) return NULL;
	if (!key_list_read_int(list, "row_border_right", &row_border_right)) return NULL;

	if (!key_list_read_int(list, "columns", &columns)) return NULL;
	if (columns < 1) return NULL;

	filename = key_list_read_path(list, "image", skin_dir);
	row_filename = key_list_read_path(list, "row_image", skin_dir);
	text_filename = key_list_read_path(list, "text_image", skin_dir);
	font_desc = key_list_read_chars(list, "text_font", NULL);
	if (!filename || !row_filename || (!text_filename && !font_desc) )
		{
		g_free(filename);
		g_free(row_filename);
		g_free(text_filename);
		return NULL;
		}

	/* opt */
	sizeable = key_list_read_bool(list, "sizeable");
	center_stretch = key_list_read_bool(list, "center_stretch");

	row_prelight = key_list_read_bool(list, "row_prelight");
	row_press = key_list_read_bool(list, "row_pressable");
	row_stretch = key_list_read_bool(list, "row_stretch");

	divider_filename = key_list_read_path(list, "divider_image", skin_dir);
	flag_filename = key_list_read_path(list, "flag_image", skin_dir);
	if (!key_list_read_int(list, "flag_sections", &flag_sections)) flag_sections = 1;
	if (!key_list_read_int(list, "flag_column", &flag_column)) flag_column = 0;

	columns_right_justify = key_list_read_bool(list, "columns_right_justify");

	text_extended = key_list_read_bool(list, "text_extended");

	if (!key_list_read_int(list, "text_red", &r)) r = 0;
	if (!key_list_read_int(list, "text_green", &g)) g = 0;
	if (!key_list_read_int(list, "text_blue", &b)) b = 0;
	if (!key_list_read_int(list, "text_alpha", &a)) a = 255;

	ld = list_new(util_pixbuf_new_from_file(filename), x, y, w, h, sizeable, columns,
		      border_top, border_right, border_bottom, border_left, center_stretch);
	if (ld)
		{
		gint i;

		list_image_row(ld, util_pixbuf_new_from_file(row_filename), row_press, row_prelight,
			       row_border_left, row_border_right, row_stretch,
			       divider_filename ? util_pixbuf_new_from_file(divider_filename) : NULL);

		if (text_filename)
			{
			list_set_font(ld, NULL, util_pixbuf_new_from_file(text_filename), text_extended, r, g, b, a);
			}
		else
			{
			list_set_font(ld, font_desc, NULL, FALSE, r, g, b, a);
			}

		if (flag_filename) list_image_row_flag(ld, util_pixbuf_new_from_file(flag_filename), flag_sections, flag_column);

		list_set_column_justify(ld, columns_right_justify);

		for (i = 0; i < columns; i++)
			{
			gchar *buf;
			gint length;
			ListColumnFlags flags = 0;
			const gchar *key;

			buf = g_strdup_printf("column_%d_key", i);
			key = key_list_read_chars(list, buf, NULL);
			g_free(buf);

			buf = g_strdup_printf("column_%d_width", i);
			if (!key_list_read_int(list, buf, &length)) length = 1;
			g_free(buf);

			buf = g_strdup_printf("column_%d_proportional", i);
			if (key_list_read_bool(list, buf))
				flags |= UI_LIST_COLUMN_SIZE_PROPORTIONAL;
			else
				flags |= UI_LIST_COLUMN_SIZE_FIXED;
			g_free(buf);

			buf = g_strdup_printf("column_%d_right_justify", i);
			if (key_list_read_bool(list, buf)) flags |= UI_LIST_COLUMN_JUSTIFY_RIGHT;
			g_free(buf);

			list_set_column_attributes(ld, i, length, flags, key);
			}

		if (!ld->row_overlay || !ld->font)
			{
			if (!ld->font) printf("list warning: failed to load font.\n");
			if (!ld->row_overlay) printf("list warning: failed to load row image.\n");

			list_free(ld);
			ld = NULL;
			}
		}

	if (ld)
		{
		wd = list_register(skin, ld, key, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			ui_widget_set_data(wd, "row_image", row_filename);
			ui_widget_set_data(wd, "text_image", text_filename);
			ui_widget_set_data(wd, "divider_image", divider_filename);
			ui_widget_set_data(wd, "flag_image", flag_filename);
			}
		}

	g_free(filename);
	g_free(row_filename);
	g_free(text_filename);
	g_free(divider_filename);
	g_free(flag_filename);

	return wd;
}

/*
 *-----------------------------
 * register ui / app side
 *-----------------------------
 */

WidgetData *list_register(SkinData *skin, ListData *ld, const gchar *key, const gchar *text_id)
{
	if (!ld || !ld->font)
		{
		printf("warning: attempt to register list \"%s\" will NULL font.\n", key);
		}
	if (ld) ld->skin = skin;
	
	return skin_register_widget(skin, key, text_id, type_id, ld);
}

RegisterData *list_register_key(const gchar *key, UIData *ui,
				gint (*length_request_func)(ListData *list, const gchar *key, gpointer data), gpointer length_request_data,
				gint (*row_info_func)(ListData *list, const gchar *key, gint row, ListRowData *rd, gpointer data), gpointer row_info_data,
				void (*row_click_func)(ListData *list, const gchar *key, gint row, ListRowData *rd, gint button, gpointer data), gpointer row_click_data,
				void (*row_select_func)(ListData *list, const gchar *key, gint row, ListRowData *rd, gpointer data), gpointer row_select_data,
				gint (*row_move_func)(ListData *list, const gchar *key, gint source_row, gint dest_row, gpointer data), gpointer row_move_data)
{
	ListCallbackData *cd;

	list_type_init();

        cd = g_new0(ListCallbackData, 1);

	cd->length_request_func = length_request_func;
	cd->row_info_func = row_info_func;
	cd->row_click_func = row_click_func;
	cd->row_select_func = row_select_func;
	cd->row_move_func = row_move_func;

	cd->length_request_data = length_request_data;
	cd->row_info_data = row_info_data;
	cd->row_click_data = row_click_data;
	cd->row_select_data = row_select_data;
	cd->row_move_data = row_move_data;

	cd ->menu_move_func = NULL;
	cd ->menu_move_data = NULL;

	return ui_register_key(ui, key, type_id, cd, sizeof(ListCallbackData));
}

void list_register_menu_funcs(const gchar *key, UIData *ui,
			      gint (*func)(ListData *list, const gchar *key, gint row, gint activated, gint up, gpointer data),
			      gpointer data)
{
	ListCallbackData *cd;

	cd = ui_get_registered_callbacks(ui, key, type_id);

	if (cd)
		{
		cd ->menu_move_func = func;
		cd ->menu_move_data = data;
		}
}

/*
 *-----------------------------
 * app funcs
 *-----------------------------
 */

static void list_row_insert_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld;
        gint row;

	ld = wd->widget;
        row = GPOINTER_TO_INT(data);

	if (list_length_sync(ld, ld->row_count + 1, FALSE))
		{
		list_row_sync_n(ld, ui, wd->key, 0, ld->row_end - ld->row_start, NULL);
		list_draw_rows(ld, pb, ui, 0, ld->row_end - ld->row_start, TRUE);
		}
	else if (row < ld->row_start)
		{
		list_row_scroll_by_row(ld, pb, ui, wd->key, 1);
		}
	else if (row <= ld->row_end)
		{
		list_row_sync_n(ld, ui, wd->key, row - ld->row_start, ld->row_end - ld->row_start, NULL);
		list_draw_rows(ld, pb, ui, row - ld->row_start, ld->row_end - ld->row_start, TRUE);
		}
}

gint list_row_insert(const gchar *key, UIData *ui, gint row)
{
	return skin_widget_for_each_key(ui, key, type_id, list_row_insert_cb, GINT_TO_POINTER(row));
}

static void list_row_remove_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld;
        gint row;

	ld = wd->widget;
        row = GPOINTER_TO_INT(data);

	if (ld->row_count > 0)
		{
		if (list_length_sync(ld, ld->row_count - 1, FALSE))
			{
			list_row_sync_n(ld, ui, wd->key, 0, ld->row_end - ld->row_start, NULL);
			list_draw_all_rows(ld, pb, ui, TRUE);
			}
		else if (row < ld->row_start)
			{
			list_row_scroll_by_row(ld, pb, ui, wd->key, -1);
			}
		else if (row <= ld->row_end)
			{
			list_row_sync_n(ld, ui, wd->key, row - ld->row_start, ld->row_end - ld->row_start, NULL);
			list_draw_rows(ld, pb, ui, row - ld->row_start, ld->row_end - ld->row_start, TRUE);
			list_draw_empty_rows(ld, pb, ui, TRUE);
			}
		}
}

gint list_row_remove(const gchar *key, UIData *ui, gint row)
{
	return skin_widget_for_each_key(ui, key, type_id, list_row_remove_cb, GINT_TO_POINTER(row));
}

static void list_row_update_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld;
        gint row;

	ld = wd->widget;
        row = GPOINTER_TO_INT(data);

	if (row < ld->row_start || row > ld->row_end) return;

	list_row_sync(ld, ui, wd->key, row);
	list_redraw_row(ld, pb, ui, row - ld->row_start);
}

gint list_row_update(const gchar *key, UIData *ui, gint row)
{
	return skin_widget_for_each_key(ui, key, type_id, list_row_update_cb, GINT_TO_POINTER(row));
}

static void list_refresh_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld;
	ListCallbackData *cd;
	gint length = 0;

	ld = wd->widget;

	cd = ui_get_registered_callbacks(ui, wd->key, type_id);
	if (cd && cd->length_request_func)
		{
		length = cd->length_request_func(ld, wd->key, cd->length_request_data);
		}

	ld->row_count = length;
	list_length_sync(ld, length, TRUE);
	list_row_sync_n(ld, ui, wd->key, 0, ld->row_end - ld->row_start, cd);
	list_draw_all_rows(ld, pb, ui, FALSE);

	ui_display_render_area(ui, ld->x + ld->border_left, ld->y + ld->border_top,
			       ld->region_width, ld->region_height, ld->wd);
}

gint list_refresh(const gchar *key, UIData *ui)
{
	return skin_widget_for_each_key(ui, key, type_id, list_refresh_cb, NULL);
}

static void list_scroll_row_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld;
	gint rows;

	ld = wd->widget;
	rows = GPOINTER_TO_INT(data);

	list_row_scroll_by_row(ld, pb, ui, wd->key, rows);
}

gint list_scroll_row(const gchar *key, UIData *ui, gint rows)
{
	return skin_widget_for_each_key(ui, key, type_id, list_scroll_row_cb, GINT_TO_POINTER(rows));
}

static void list_scroll_to_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	ListData *ld;
	gint row;

	ld = wd->widget;
	row = GPOINTER_TO_INT(data);

	list_row_scroll_by_row(ld, pb, ui, wd->key, row - ld->row_start);
}

gint list_scroll_to(const gchar *key, UIData *ui, gint row)
{
	return skin_widget_for_each_key(ui, key, type_id, list_scroll_to_cb, GINT_TO_POINTER(row));
}

void list_set_menu_style(ListData *ld, gint menu_style)
{
	if (ld) ld->menu_style_select = menu_style;
}

/*
 *-----------------------------
 * internal signals
 *-----------------------------
 */

static void list_scroll_timeout_cancel(ListData *ld)
{
	if (ld->timeout_id != -1)
		{
		g_source_remove(ld->timeout_id);
		ld->timeout_id = -1;
		}
	ld->scroll_count = 0;
}

static gint list_scroll_button_down_delay(gpointer data)
{
	ListData *ld = data;
	
	list_row_scroll_by_row(ld, skin_get_pixbuf(ld->ui->skin), ld->ui, ld->wd->key, 1);

	if (ld->scroll_count < UI2_LIST_SCROLL_SPEEDUP)
		{
		ld->scroll_count++;
		if (ld->scroll_count >= UI2_LIST_SCROLL_SPEEDUP)
			{
			ld->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, UI2_LIST_SCROLL_DELAY_FAST,
							    list_scroll_button_down_delay, ld, NULL);
			return FALSE;
			}
		}

	return TRUE;
}

static void list_scroll_button_down_press(ButtonData *button, const gchar *key, gpointer data)
{
	ListData *ld = data;

	list_row_scroll_by_row(ld, skin_get_pixbuf(ld->ui->skin), ld->ui, ld->wd->key, 1);
	list_scroll_timeout_cancel(ld);
	ld->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, UI2_LIST_SCROLL_DELAY,
					    list_scroll_button_down_delay, ld, NULL);
}

static void list_scroll_button_down_release(ButtonData *button, const gchar *key, gpointer data)
{
	ListData *ld = data;

	list_scroll_timeout_cancel(ld);
}

static gint list_scroll_button_up_delay(gpointer data)
{
	ListData *ld = data;
	
	list_row_scroll_by_row(ld, skin_get_pixbuf(ld->ui->skin), ld->ui, ld->wd->key, -1);

	if (ld->scroll_count < UI2_LIST_SCROLL_SPEEDUP)
		{
		ld->scroll_count++;
		if (ld->scroll_count >= UI2_LIST_SCROLL_SPEEDUP)
			{
			ld->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, UI2_LIST_SCROLL_DELAY_FAST,
							    list_scroll_button_up_delay, ld, NULL);
			return FALSE;
			}
		}

	return TRUE;
}

static void list_scroll_button_up_press(ButtonData *button, const gchar *key, gpointer data)
{
	ListData *ld = data;

	list_row_scroll_by_row(ld, skin_get_pixbuf(ld->ui->skin), ld->ui, ld->wd->key, -1);
	list_scroll_timeout_cancel(ld);
	ld->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, UI2_LIST_SCROLL_DELAY,
					    list_scroll_button_up_delay, ld, NULL);
}

static void list_scroll_button_up_release(ButtonData *button, const gchar *key, gpointer data)
{
	ListData *ld = data;

	list_scroll_timeout_cancel(ld);
}

static gfloat list_scroll_position(ListData *ld)
{
	gint vis = list_rows_visible(ld);

	if (ld->row_start == 0 || ld->row_count <= vis) return 0.0;

	return (float)ld->row_start / (ld->row_count - vis);
}

static gfloat list_scroll_slider_status(SliderData *slider, const gchar *key, gpointer data)
{
	ListData *ld = data;

	return list_scroll_position(ld);
}

static gint list_scroll_idle_cb(gpointer data)
{
	ListData *ld = data;

	if (ld->scroll_idle_id == -1) return FALSE;

	list_row_scroll(ld, skin_get_pixbuf(ld->ui->skin), ld->ui, ld->wd->key, ld->scroll_idle_value);
	
	ld->scroll_idle_id = -1;
	return FALSE;
}

static void list_scroll_slider_drag(SliderData *slider, const gchar *key, gfloat value, gpointer data)
{
	ListData *ld = data;

	ld->scroll_idle_value = value;
	if (ld->scroll_idle_id == -1) ld->scroll_idle_id = gtk_idle_add(list_scroll_idle_cb, ld);
}

static void list_init(gpointer widget, const gchar *key, UIData *ui)
{
	ListData *ld = widget;
	RegisterData *rd;
	gchar *buf;
	WidgetData *wd;

	wd = ui_widget_get_by_widget(ui, widget);
	ld->wd = wd;
	ld->ui = ui;

	buf = g_strdup_printf("list_%s_scroll_down", key);
	rd = button_register_key(buf, ui,
				 NULL, NULL,
				 NULL, NULL,
				 list_scroll_button_down_press, ld,
				 list_scroll_button_down_release, ld);
	rd->private = TRUE;
	rd->private_widget = ld;
	g_free(buf);

	buf = g_strdup_printf("list_%s_scroll_up", key);
	rd = button_register_key(buf, ui,
				 NULL, NULL,
				 NULL, NULL,
				 list_scroll_button_up_press, ld,
				 list_scroll_button_up_release, ld);
	rd->private = TRUE;
	rd->private_widget = ld;
	g_free(buf);

	buf = g_strdup_printf("list_%s_scroll", key);
	rd = slider_register_key(buf, ui,
				 list_scroll_slider_status, ld,
				 list_scroll_slider_drag, ld,
				 NULL, NULL,
				 list_scroll_slider_drag, ld);
	rd->private = TRUE;
	rd->private_widget = ld;
	g_free(buf);
}

static void list_scroll_update_widgets(ListData *ld)
{
	gchar *buf;
	gint total;
	gint vis;

	if (!ld->ui) return;

	vis = list_rows_visible(ld);
	total = ld->row_count - vis;

	buf = g_strdup_printf("list_%s_scroll", ld->wd->key);
	slider_value_set(buf, ld->ui, list_scroll_position(ld));
	slider_step_size_set(buf, ld->ui,
			     (total > 0) ? (1.0 / (float)total) : 0.1,
			     (vis > 1) ? vis - 1 : vis);
	g_free(buf);
}

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType list_type_id(void)
{
	return type_id;
}

void list_type_init(void)
{
	WidgetObjectData *od;

	if (type_id != -1) return;

	od = ui_widget_type_new("list");
	type_id = od->type;

	od->func_draw = list_draw;
	od->func_reset = list_reset;
	od->func_press = list_press;
	od->func_release = list_release;
	od->func_motion = list_motion;
	od->func_back = list_back_set;
	od->func_free = list_free_cb;

	od->func_focus_key_event = list_key_event_cb;
	od->func_focus_draw = list_focus_draw_cb;

	od->func_get_geometry = list_get_geometry;
	od->func_set_coord = list_set_coord;
	od->func_set_size = list_set_size;

	od->func_parse = list_parse;
	od->func_init = list_init;

	list_type_init_edit(od);
}

