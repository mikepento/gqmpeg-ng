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
#include "ui2_number.h"
#include "ui2_number_edit.h"

#include "ui2_display.h"
#include "ui2_main.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"


/*
 *------------
 * digit side
 *------------
 */

static WidgetType type_id_digit = -1;


/*
 *-----------------------------
 * new / free
 *-----------------------------
 */

DigitData *digit_new(GdkPixbuf *pb)
{
	DigitData *digit;

	digit_type_init();

	if (!pb) return NULL;

	digit = g_new0(DigitData, 1);

	digit->overlay = util_size_pixbuf(pb, TRUE);
	digit->width = gdk_pixbuf_get_width(digit->overlay) / 11;
	digit->height = gdk_pixbuf_get_height(digit->overlay);

	digit->ref = 1;

	return digit;
}

DigitData *digit_new_from_data(gchar **data)
{
	GdkPixbuf *pb;

	pb = gdk_pixbuf_new_from_xpm_data((const char **)data);

	return digit_new(pb);
}

DigitData *digit_new_from_file(const gchar *file)
{
	GdkPixbuf *pb;

	pb = util_pixbuf_new_from_file(file);

	return digit_new(pb);
}

static void digit_free(DigitData *digit)
{
	if (!digit) return;
	if (digit->overlay) gdk_pixbuf_unref(digit->overlay);
	g_free(digit);
}

void digit_ref(DigitData *digit)
{
	if (digit) digit->ref++;
}

void digit_unref(DigitData *digit)
{
	if (digit)
		{
		digit->ref--;
		if (digit->ref < 1) digit_free(digit);
		}
}

static void digit_unref_cb(gpointer data)
{
	digit_unref((DigitData *)data);
}

/*
 *-----------------------------
 * draw
 *-----------------------------
 */

static void digit_draw(DigitData *digit, gint n, gint x, gint y, gint offset, gint length, gint rotation, GdkPixbuf *pb)
{
	if (!digit) return;

	if (n < 0 || n > 10) n = 10;
	switch (rotation)
		{
		case 0:
			pixbuf_copy_area_alpha(digit->overlay, digit->width * n, 0,
					       pb, x + offset, y, digit->width, digit->height, 255);
			break;
		case 90:
			pixbuf_copy_area_rotate_90_alpha(digit->overlay, digit->width * n, 0,
							 pb, x, y + offset,
							 digit->width, digit->height, 255, rotation);
			break;
		case 180:
			pixbuf_copy_area_rotate_90_alpha(digit->overlay, digit->width * n, 0,
							 pb, x + length - offset - digit->width, y,
							 digit->width, digit->height, 255, rotation);
			break;
		case 270:
			pixbuf_copy_area_rotate_90_alpha(digit->overlay, digit->width * n, 0,
							 pb, x, y + length - offset - digit->width,
							 digit->width, digit->height, 255, rotation);
			break;
		}
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static gint digit_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	DigitData *digit = widget;

	/* digits have no geometry, they are not directly displayed */
	*x = 0;
	*y = 0;
	*w = digit->width;
	*h = digit->height;

	return TRUE;
}

static WidgetData *digit_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	DigitData *digit;
	const gchar *text_id;
	gchar *filename;

	/* req */
	filename = key_list_read_path(list, "image", skin_dir);
	if (!filename) return NULL;

	/* opt */
	text_id = key_list_read_chars(list, "id", NULL);

	digit = digit_new_from_file(filename);

	if (digit)
		{
		wd = digit_register(skin, digit, key, text_id);
		digit_unref(digit);

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
 * register ui
 *-----------------------------
 */

WidgetData *digit_register(SkinData *skin, DigitData *digit, const gchar *key, const gchar *text_id)
{
	digit_ref(digit);
	return skin_register_widget(skin, key, text_id, type_id_digit, digit);
}

WidgetData *digit_register_to_skin(const gchar *key, gchar **data,
				   SkinData *skin, const gchar *text_id)
{
	DigitData *digit;

	digit = digit_new_from_data(data);
	return digit_register(skin, digit, key, text_id);
}

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType digit_type_id(void)
{
	return type_id_digit;
}

void digit_type_init(void)
{
	WidgetObjectData *od;

	if (type_id_digit != -1) return;

	od = ui_widget_type_new("digit");
	type_id_digit = od->type;
	od->is_visible = FALSE;

	od->func_free = digit_unref_cb;

	od->func_get_geometry = digit_get_geometry;

	od->func_parse = digit_parse;

	digit_type_init_edit(od);
}


/*
 *------------
 * number side
 *------------
 */


typedef struct _NumberCallbackData NumberCallbackData;
struct _NumberCallbackData
{
	gint (*status_get_func)(NumberData *number, const gchar *key, gpointer data);
	gpointer status_get_data;
};

static WidgetType type_id_number = -1;


/*
 *-----------------------------
 * new / free
 *-----------------------------
 */

NumberData *number_new(DigitData *digit, gint x, gint y,
		       gint length, gint zeros, gint centered, gint rotation)
{
	NumberData *number;

	digit_type_init();
	number_type_init();

	if (!digit || length < 1) return NULL;

	number = g_new0(NumberData, 1);

	util_size(&x);
	util_size(&y);

	number->digits = digit;
	digit_ref(number->digits);
	number->length = length;
	number->zeros = zeros;
	number->centered = centered;
	number->x = x;
	number->y = y;

	if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
		{
		printf("ui2_number.c warning: invalid rotation value, using default (0)\n");
		rotation = 0;
		}
	number->rotation = rotation;

	number->value = 0;

	if (digit_rotation_vertical(number->rotation))
		{
		number->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, digit->height, digit->width * length);
		}
	else
		{
		number->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, digit->width * length, digit->height);
		}
	return number;
}

void number_free(NumberData *number)
{
	if (!number) return;

	digit_unref(number->digits);
	if (number->pixbuf) gdk_pixbuf_unref(number->pixbuf);
	g_free(number);
}

static void number_free_cb(gpointer data)
{
	number_free((NumberData *)data);
}

/*
 *-----------------------------
 * draw
 *-----------------------------
 */

static void number_draw_real(NumberData *number, gint n, GdkPixbuf *pb, UIData *ui)
{
	gint i, d;
	gint t = 1;
	gint z;
	gint length;
	gint w, h;
	DigitData *digit;

	if (!number) return;

	n = abs(n);
	number->value = n;

	digit = number->digits;

	w = gdk_pixbuf_get_width(number->pixbuf);
	h = gdk_pixbuf_get_height(number->pixbuf);
	pixbuf_copy_area(number->pixbuf, 0, 0, pb, number->x, number->y, w, h, FALSE);

	length = number->length * digit->width;
	z = number->zeros;

	for (i=0; i< number->length - 1; i++) t *= 10;

	if (number->value >= t * 10)
		{
		/* exceeding length, just force to all 9 */
		for (i = 0; i < number->length; i++)
			{
			digit_draw(digit, 9, number->x, number->y, i * digit->width, length, number->rotation, pb);
			}
		}
	else if (number->centered && n < t)
		{
		gint offset = 0;
		gint b = FALSE;
		for (i=number->length; i > 0; i--)
			{
			d = n / t;
			n -= d * t;
			t = t / 10;
			if (!(d == 0 && i>1 && !b))
				{
				if (!b)
					{
					offset = (number->length * digit->width / 2) + (i * digit->width / 2);
					b = TRUE;
					}
				digit_draw(digit, d, number->x, number->y,
					   offset - (i * digit->width), length, number->rotation, pb);
				}
			}
		}
	else
		{
		for (i=number->length; i > 0; i--)
			{
			d = n / t;
			n -= d * t;
			t = t / 10;
			if (d == 0 && i>1 && (!z))
				{
				d = -1;
				}
			else
				{
				z = TRUE;
				}
			digit_draw(digit, d, number->x, number->y,
				   (number->length - i) * digit->width, length, number->rotation, pb);
			}
		}

	ui_display_render_area(ui, number->x, number->y, w, h, NULL);
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static void number_draw(gpointer data, const gchar *key, gint update, gint force, GdkPixbuf *pb, UIData *ui)
{
	NumberData *number = data;
	gint n;

	n = number->value;
	if (update)
		{
		NumberCallbackData *cd;

		cd = ui_get_registered_callbacks(ui, key, type_id_number);
		if (cd && cd->status_get_func)
			{
			n = cd->status_get_func(number, key, cd->status_get_data);
			}

		}
	if (force || n != number->value)
		{
		number_draw_real(number, n, pb, ui);
		}
}


static void number_back_set(gpointer data, GdkPixbuf *pb)
{
	NumberData *number = data;

	if (!number) return;

	pixbuf_copy_area(pb, number->x, number->y, number->pixbuf, 0, 0,
			 gdk_pixbuf_get_width(number->pixbuf), gdk_pixbuf_get_height(number->pixbuf), FALSE);
}

static gint number_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	NumberData *number = widget;

	*x = number->x;
	*y = number->y;

	if (digit_rotation_vertical(number->rotation))
		{
		*w = number->digits->height;
		*h = number->digits->width * number->length;
		}
	else
		{
		*w = number->digits->width * number->length;
		*h = number->digits->height;
		}

	return TRUE;
}

static void number_set_coord(gpointer widget, gint x, gint y)
{
	NumberData *number = widget;

	number->x = x;
	number->y = y;
}

static WidgetData *number_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	NumberData *number;
	DigitData *digit;
	gchar *filename;
	const gchar *digit_id;
	gint x, y;
	gint length;
	gint zeros;
	gint center;
	gint rotation;

	/* req */
	if (!key_list_read_int(list, "x", &x)) return NULL;
	if (!key_list_read_int(list, "y", &y)) return NULL;
	if (!key_list_read_int(list, "length", &length)) return NULL;
	filename = key_list_read_path(list, "image", skin_dir);
	digit_id = key_list_read_chars(list, "digit", NULL);
	if (!filename && !digit_id) return NULL;

	/* opt */
	zeros = key_list_read_bool(list, "zeros");
	center = key_list_read_bool(list, "center");
	if (!key_list_read_int(list, "rotation", &rotation)) rotation = 0;

	if (filename)
		{
		digit = digit_new_from_file(filename);
		if (!digit)
			{
			printf("failed to load file \"%s\"\n", filename);
			}
		}
	else
		{
		WidgetData *dig_wd;

		dig_wd = skin_widget_get_by_key(skin, digit_id, digit_type_id());
		if (dig_wd)
			{
			digit = dig_wd->widget;
			digit_ref(digit);
			}
		else
			{
			printf("digit not found registered as \"%s\"\n", digit_id);
			return NULL;
			}
		}

	number = number_new(digit, x, y, length, zeros, center, rotation);

	if (number)
		{
		wd = number_register(skin, number, key, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			ui_widget_set_data(wd, "digit_id", digit_id);
			}
		}
	digit_unref(digit);

	g_free(filename);

	return wd;
}

/*
 *-----------------------------
 * register ui / app side
 *-----------------------------
 */

WidgetData *number_register(SkinData *skin, NumberData *number, const gchar *key, const gchar *text_id)
{
	return skin_register_widget(skin, key, text_id, type_id_number, number);
}

WidgetData *number_register_to_skin(const gchar *key, DigitData *digit, gint x, gint y,
				    gint length, gint zeros, gint centered, gint rotation,
				    SkinData *skin, const gchar *text_id)
{
	NumberData *number;

	number = number_new(digit, x, y, length, zeros, centered, rotation);
	return number_register(skin, number, key, text_id);
}

RegisterData *number_register_key(const gchar *key, UIData *ui,
				  gint (*status_get_func)(NumberData *number, const gchar *key, gpointer data), gpointer status_get_data)
{
	NumberCallbackData *cd;

	digit_type_init();
	number_type_init();

	cd = g_new0(NumberCallbackData, 1);

	cd->status_get_func = status_get_func;
	cd->status_get_data = status_get_data;

	return ui_register_key(ui, key, type_id_number, cd, sizeof(NumberCallbackData));
}

/*
 *-----------------------------
 * app funcs
 *-----------------------------
 */

gint number_value_get(const gchar *key, UIData *ui)
{
        WidgetData *wd;
        NumberData *number;

        wd = skin_widget_get_by_key(ui->skin, key, type_id_number);
        if (!wd) return 0;

        number = wd->widget;
        return number->value;
}

static void number_value_set_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	NumberData *number;
	gint n;

	number = wd->widget;
	n = GPOINTER_TO_INT(data);

	if (n != number->value) number_draw_real(number, n, pb, ui);
}

gint number_value_set(const gchar *key, UIData *ui, gint n)
{
	return skin_widget_for_each_key(ui, key, type_id_number, number_value_set_cb, GINT_TO_POINTER(n));
}

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType number_type_id(void)
{
	return type_id_number;
}

void number_type_init(void)
{
	WidgetObjectData *od;

	if (type_id_number != -1) return;

	digit_type_init();

	od = ui_widget_type_new("number");
	type_id_number = od->type;

	od->func_draw = number_draw;
	od->func_back = number_back_set;
	od->func_free = number_free_cb;

	od->func_get_geometry = number_get_geometry;
        od->func_set_coord = number_set_coord;

	od->func_parse = number_parse;

	number_type_init_edit(od);
}





