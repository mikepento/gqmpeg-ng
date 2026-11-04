/*
 * GQmpeg
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "gqmpeg.h"
#include "skin_wamp.h"

#include "ui2_skin.h"
#include "ui2_button.h"
#include "ui2_item.h"
#include "ui2_number.h"
#include "ui2_slider.h"
#include "ui2_text.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_fileops.h"
#include "ui_pixbuf_ops.h"


/* table to convert the fonts */
static gint letter_convert[] = { 30,48,47,61,60,57,56,47,44,45,66,50,58,46,58,52,
                                 31,32,33,34,35,36,37,38,39,40,-1,-1,-1,59,-1,-1,

                                 27,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,11,12,13,14,
                                 15,16,17,18,19,20,21,22,23,24,25,53,51,54,55,49,

                                 47,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,11,12,13,14,
                                 15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1 };


static TextData *wamp_text(gchar *path, gint l, gint x, gint y, SkinData *skin)
{
	FontData *font;
	TextData *text;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;
	gint i;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return NULL;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 160, 18);

	for (i=0; i < 96; i++)
		{
		gint cp = letter_convert[i];
		gint xc, yc;
		gint xp, yp;
		yp = i / 32;
		xp = i - (yp * 32);
		if (cp >= 0)
			{
			yc = cp / 31;
			xc = cp - (yc * 31);
			}
		else
			{
			yc = 0;
			xc = 29;
			}
		pixbuf_copy_area(pb_temp, xc * 5, yc * 6, pb, xp * 5, yp * 6, 5, 6, TRUE);
		}

	gdk_pixbuf_unref(pb_temp);

	/* backwards compatibility, new width is pixels instead of chars,
	 * winamp chars are 5 pixels wide */
	l *= 5;

	font = font_new(pb, FALSE);
	text = text_new(font, x, y, l, FALSE, 0);
	font_unref(font);
	text_register(skin, text, "title", NULL);

	return text;
}

static DigitData *wamp_digit(gchar *path, GdkPixbuf *back, SkinData *skin)
{
	DigitData *digit;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;
	gint i;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return NULL;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 132, 13);

	for (i=0; i < 11; i++)
		{
		pixbuf_copy_area(pb_temp, i * 9, 0, pb, (i * 12) + 3, 0, 9, 13, TRUE);
		pixbuf_copy_area(back, 87, 26, pb, i * 12, 0, 3, 13, TRUE);
		}

	gdk_pixbuf_unref(pb_temp);

	digit = digit_new(pb);
	digit_register(skin, digit, "wamp", NULL);
	digit_unref(digit);

	return digit;
}

static DigitData *wamp_digit_from_text(gchar *path, SkinData *skin)
{
	DigitData *digit;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;
	gint i;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return NULL;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 55, 6);

	for (i=0; i<10; i++)
		{
		pixbuf_copy_area(pb_temp, i * 5, 6, pb, i * 5, 0, 5, 6, TRUE);
		}
	pixbuf_copy_area(pb_temp, 150, 0, pb, 50, 0, 5, 6, TRUE);

	gdk_pixbuf_unref(pb_temp);

	digit = digit_new(pb);
	digit_register(skin, digit, "wamp_text", NULL);
	digit_unref(digit);

	return digit;
}

static WidgetData *wamp_button(GdkPixbuf *pb_temp, gint lit, gint x, gint y, gint ox, gint oy, gint lx, gint ly,
			SkinData *skin, const gchar *key)
{
	ButtonData *button;
	GdkPixbuf *pb;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, lx * (2 + (2 * lit)) , ly);

	pixbuf_copy_area(pb_temp, ox, oy, pb, 0, 0, lx, ly, TRUE);
	pixbuf_copy_area(pb_temp, ox, oy +ly, pb, lx, 0, lx, ly, TRUE);

	if (lit)
		{
		pixbuf_copy_area(pb_temp, ox, oy + (ly * 2), pb, lx * 2, 0, lx, ly, TRUE);
		pixbuf_copy_area(pb_temp, ox, oy + (ly * 3), pb, lx * 3, 0, lx, ly, TRUE);
		}

	button = button_new(pb, x, y, FALSE, lit, NULL);
	return button_register(skin, button, key, NULL);
}

static WidgetData *wamp_button_v(GdkPixbuf *pb_temp, gint lit, gint x, gint y, gint ox, gint oy, gint lx, gint ly,
			  SkinData *skin, const gchar *key)
{
	ButtonData *button;
	GdkPixbuf *pb;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, lx * (2 + (2 * lit)) , ly);

	pixbuf_copy_area(pb_temp, ox, oy, pb, 0, 0, lx, ly, TRUE);
	pixbuf_copy_area(pb_temp, ox + lx, oy, pb, lx, 0, lx, ly, TRUE);

	if (lit)
		{
		pixbuf_copy_area(pb_temp, ox, oy + (ly * 2), pb, lx * 2, 0, lx, ly, TRUE);
		pixbuf_copy_area(pb_temp, ox, oy + (ly * 3), pb, lx * 3, 0, lx, ly, TRUE);
		}

	button = button_new(pb, x, y, FALSE, lit, NULL);
	return button_register(skin, button, key, NULL);
}

static void wamp_item_monoster(gchar *path, SkinData *skin)
{
	ItemData *item;
	gint width, height;
	gint s1, s2;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return;

	width = gdk_pixbuf_get_width(pb_temp);
	height = gdk_pixbuf_get_height(pb_temp);

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 56, 36);

	if (width == 58 || (slik_double_size && width == 116))
		{
		s1 = 1;
		s2 = 29;
		}
	else
		{
		s1 = 0;
		s2 = 28;
		}
	pixbuf_copy_area(pb_temp, s2, 12, pb, 0, 0, 28, 12, TRUE);
	pixbuf_copy_area(pb_temp, s1, 12, pb, 28, 0, 28, 12, TRUE);
	pixbuf_copy_area(pb_temp, s2, 0, pb, 0, 12, 28, 12, TRUE);
	pixbuf_copy_area(pb_temp, s1, 12, pb, 28, 12, 28, 12, TRUE);
	pixbuf_copy_area(pb_temp, s2, 12, pb, 0, 24, 28, 12, TRUE);
	pixbuf_copy_area(pb_temp, s1, 0, pb, 28, 24, 28, 12, TRUE);

	gdk_pixbuf_unref(pb_temp);

	item = item_new(pb, 212, 41, 3);	
	item_register(skin, item, "stereo", NULL);
}

static void wamp_item_status(gchar *path, SkinData *skin)
{
	ItemData *item;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 9, 27);

	pixbuf_copy_area(pb_temp, 18, 0, pb, 0, 0, 9, 9, TRUE);
	pixbuf_copy_area(pb_temp, 9, 0, pb, 0, 9, 9, 9, TRUE);
	pixbuf_copy_area(pb_temp, 0, 0, pb, 0, 18, 9, 9, TRUE);

	gdk_pixbuf_unref(pb_temp);

	item = item_new(pb, 26, 28, 3);
	item_register(skin, item, "status", NULL);
}

static void wamp_button_alt(GdkPixbuf *pb_temp, gint x, gint y, gint ox, gint oy, gint lx, gint ly,
			    SkinData *skin, const gchar *key)
{
	ButtonData *button;
	GdkPixbuf *pb;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, lx * 4, ly);

	pixbuf_copy_area(pb_temp, ox, oy, pb, 0, 0, lx, ly, TRUE);
	pixbuf_copy_area(pb_temp, ox, oy +ly, pb, lx, 0, lx, ly, TRUE);
	pixbuf_copy_area(pb_temp, ox + lx * 2, oy, pb, lx * 2, 0, lx, ly, TRUE);
	pixbuf_copy_area(pb_temp, ox + lx * 2, oy + ly, pb, lx * 3, 0, lx, ly, TRUE);

	button = button_new(pb, x, y, FALSE, TRUE, NULL);
	button_register(skin, button, key, NULL);
}

static void wamp_button_small(GdkPixbuf *pb_temp, gint x, gint y, gint ox, gint oy,
			      SkinData *skin, const gchar *key)
{
	ButtonData *button;
	GdkPixbuf *pb;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 20, 10);

	pixbuf_copy_area(pb_temp, ox, oy, pb, 0, 0, 10, 10, TRUE);
	pixbuf_copy_area(pb_temp, ox, oy, pb, 10, 0, 10, 10, TRUE);

	button = button_new(pb, x, y, FALSE, FALSE, NULL);
	button_register(skin, button, key, NULL);
}

static void wamp_slider(gchar *path, gint prelight, gint length, gint x, gint y, SkinData *skin)
{
	SliderData *slider;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 306, 10);

	pixbuf_copy_area(pb_temp, 0, 0, pb, 0, 0, 277, 10, TRUE);
	pixbuf_copy_area(pb_temp, 278, 0, pb, 277, 0, 29, 10, TRUE);

	gdk_pixbuf_unref(pb_temp);

	slider = slider_new(pb, x, y, length, FALSE, FALSE, TRUE, prelight, NULL);
	slider_register(skin, slider, "position", NULL);
}

static void wamp_vol_slider(gchar *path, gint prelight, gint length, gint x, gint y, SkinData *skin)
{
	SliderData *slider;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 88, 11);

	pixbuf_copy_area(pb_temp, 3, 1, pb, 0, 0, 60, 11, TRUE);
	pixbuf_copy_area(pb_temp, 0, 422, pb, 60, 0, 14, 11, TRUE);
	pixbuf_copy_area(pb_temp, 15, 422, pb, 74, 0, 14, 11, TRUE);

	gdk_pixbuf_unref(pb_temp);

	slider = slider_new(pb, x, y, length, FALSE, FALSE, TRUE, prelight, "wamp_volume");
	slider_register(skin, slider, "volume", NULL);
}

static void wamp_bal_slider(gchar *path, gint prelight, gint length, gint x, gint y, SkinData *skin)
{
	SliderData *slider;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 60, 11);

	pixbuf_copy_area(pb_temp, 11, 1, pb, 0, 0, 32, 11, TRUE);
	pixbuf_copy_area(pb_temp, 0, 422, pb, 32, 0, 14, 11, TRUE);
	pixbuf_copy_area(pb_temp, 15, 422, pb, 46, 0, 14, 11, TRUE);

	gdk_pixbuf_unref(pb_temp);

	slider = slider_new(pb, x, y, length, FALSE, FALSE, TRUE, prelight, "wamp_balance");
	slider_register(skin, slider, "balance", NULL);
}

static void wamp_vol_item(gchar *path, SkinData *skin)
{
	ItemData *item;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;
	gint i;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 68, 336);

	for(i=0; i < 28; i++)
		{
		pixbuf_copy_area(pb_temp, 0, 1 + i * 15, pb, 0, i * 12, 68, 12, TRUE);
		}

	gdk_pixbuf_unref(pb_temp);

	item = item_new(pb, 107, 58, 28);
	item_register(skin, item, "wamp_volume", NULL);
}

static void wamp_bal_item(gchar *path, SkinData *skin)
{
	ItemData *item;
	GdkPixbuf *pb;
	GdkPixbuf *pb_temp;
	gint i;

	pb_temp = util_pixbuf_new_from_file(path);
	if (!pb_temp) return;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 38, 336);

	for(i=0; i < 14; i++)
		{
		pixbuf_copy_area(pb_temp, 9, 1 + i * 30, pb, 0, 156 - i * 12, 38, 12, TRUE);
		}
	for(i=0; i < 14; i++)
		{
		pixbuf_copy_area(pb_temp, 9, 1 + i * 30, pb, 0, 168 + i * 12, 38, 12, TRUE);
		}

	gdk_pixbuf_unref(pb_temp);

	item = item_new(pb, 177, 58, 28);
	item_register(skin, item, "wamp_balance", NULL);
}

static void wamp_number(DigitData *digit, gint length, gint zeros, gint center,
			gint x, gint y, SkinData *skin, const gchar *key)
{
	NumberData *number;

	number = number_new(digit, x, y, length, zeros, center, 0);
	number_register(skin, number, key, NULL);
}

SkinData *skin_load_wamp_normal(const gchar *path)
{
	SkinData *s;
	DigitData *digit_s;
	DigitData *digit_l;
	TextData *text;
	GdkPixbuf *pb;
	gchar *file;

	file = g_strconcat(path, "/main.bmp", NULL);
	pb = util_pixbuf_new_from_file(file);
	g_free(file);

	if (!pb) return NULL;

	s = skin_new();

	s->real_overlay = pb;

	file = g_strconcat(path, "/titlebar.bmp", NULL);
	pb = util_pixbuf_new_from_file(file);
	g_free(file);

	if (pb)
		{
		pixbuf_copy_area(pb, 27, 0, s->real_overlay, 0, 0, 275, 14, TRUE);
		gdk_pixbuf_unref(pb);
		}

	file = g_strconcat(path, "/text.bmp", NULL);
	text = wamp_text(file, 26, 133, 27, s);

	digit_s = wamp_digit_from_text(file, s);
	g_free(file);

	file = g_strconcat(path, "/numbers.bmp", NULL);
	if (!isfile(file))
		{
		g_free(file);
		file = g_strconcat(path, "/nums_ex.bmp", NULL);
		}
	digit_l = wamp_digit(file, s->real_overlay, s);
	g_free(file);

	file = g_strconcat(path, "/cbuttons.bmp", NULL);
	pb = util_pixbuf_new_from_file(file);
	g_free(file);

	if (pb)
		{
		wamp_button(pb, FALSE, 16, 88, 0, 0, 22, 18, s, "prev");
		wamp_button(pb, FALSE, 38, 88, 22, 0, 24, 18, s, "play");
		wamp_button(pb, FALSE, 62, 88, 46, 0, 23, 18, s, "pause");
		wamp_button(pb, FALSE, 85, 88, 69, 0, 22, 18, s, "stop");
		wamp_button(pb, FALSE, 107, 88, 91, 0, 22, 18, s, "next");
		/* we are cheating a little and using the ff slot for a second playlist */
		wamp_button(pb, FALSE, 136, 89, 114, 0, 22, 16, s, "playlist");
		gdk_pixbuf_unref(pb);
		}

	file = g_strconcat(path, "/shufrep.bmp", NULL);
	pb = util_pixbuf_new_from_file(file);
	g_free(file);

	if (pb)
		{
		wamp_button(pb, TRUE, 165, 89, 29, 0, 46, 15, s, "shuffle");
		wamp_button(pb, TRUE, 211, 89, 0, 0, 29, 15, s, "repeat");
		wamp_button_alt(pb, 242, 58, 23, 61, 23, 12, s, "playlist");
		wamp_button_alt(pb, 219, 58, 0, 61, 23, 12, s, "mixer");
		gdk_pixbuf_unref(pb);
		}

	file = g_strconcat(path, "/titlebar.bmp", NULL);
	pb = util_pixbuf_new_from_file(file);
	g_free(file);

	if (pb)
		{
		WidgetData *wd;
		wamp_button(pb, FALSE, 244, 3, 9, 0, 9, 9, s, "iconify");
		wamp_button(pb, FALSE, 264, 3, 18, 0, 9, 9, s, "exit");
		wd = wamp_button_v(pb, FALSE, 254, 3, 0, 18, 9, 9, s, "skin_toggle");
		ui_widget_set_data(wd, "data", "skindata_alt");
		wamp_button(pb, FALSE, 6, 3, 0, 0, 9, 9, s, "config");
		gdk_pixbuf_unref(pb);
		}

	file = g_strconcat(path, "/monoster.bmp", NULL);
	wamp_item_monoster(file, s);
	g_free(file);

	wamp_number(digit_l, 2, FALSE, FALSE, 45, 26, s , "minute");
	wamp_number(digit_l, 2, TRUE, FALSE, 75, 26, s, "second");

	wamp_number(digit_s, 3, FALSE, FALSE, 112, 27, s, "song");
	wamp_number(digit_s, 3, FALSE, FALSE, 112, 43, s, "input_rate");
	wamp_number(digit_s, 2, FALSE, FALSE, 157, 43, s, "input_hz");

	file = g_strconcat(path, "/playpaus.bmp", NULL);
	wamp_item_status(file, s);
	g_free(file);

	file = g_strconcat(path, "/posbar.bmp", NULL);
	wamp_slider(file, FALSE, 248, 16, 72, s);
	g_free(file);

	file = g_strconcat(path, "/volume.bmp", NULL);
	wamp_vol_item(file, s);
	wamp_vol_slider(file, FALSE, 60, 110, 58, s);
	g_free(file);

	file = g_strconcat(path, "/balance.bmp", NULL);
	if (!isfile(file))
		{
		g_free(file);
		file = g_strconcat(path, "/volume.bmp", NULL);
		}
	wamp_bal_item(file, s);
	wamp_bal_slider(file, FALSE, 32, 180, 58, s);
	g_free(file);

	s->real_overlay = util_size_pixbuf(s->real_overlay, TRUE);
	s->width = gdk_pixbuf_get_width(s->real_overlay);
	s->height = gdk_pixbuf_get_height(s->real_overlay);

	s->width_def = s->width_min = s->width_max = s->width;
	s->height_def = s->height_min = s->height_max = s->height;

	if (show_extra_winamp_info)
		{
		wamp_number(digit_s, 2, FALSE, FALSE, 52, 48, s, "cpu");
		wamp_number(digit_s, 5, FALSE, FALSE, 72, 48, s, "frame");

		/* draw the labels */
		if (text)
			{
			gint cw = 5;
			gint ch = 6;
			gint cx = 27;
			gint cy = 48;

			util_size(&cw);
			util_size(&ch);
			util_size(&cx);
			util_size(&cy);
			pixbuf_copy_area(text->font->overlay, cw * 3, ch, s->real_overlay, cx, cy, cw, ch, FALSE); /* c */
			pixbuf_copy_area(text->font->overlay, cw * 16, ch, s->real_overlay, cx + cw, cy, cw, ch, FALSE); /* p */
			pixbuf_copy_area(text->font->overlay, cw * 21, ch, s->real_overlay, cx + cw * 2, cy, cw, ch, FALSE); /* u */
			pixbuf_copy_area(text->font->overlay, cw * 5, 0, s->real_overlay, cx + cw * 3, cy, cw, ch, FALSE); /* % */
			pixbuf_copy_area(text->font->overlay, 0, 0, s->real_overlay, cx + cw * 4, cy, cw, ch, FALSE); /* ' ' */
			}

		}

	return s;
}

SkinData *skin_load_wamp_small(const gchar *path)
{
	SkinData *s;
	DigitData *digit;
	GdkPixbuf *pb;
	gchar *file;

	file = g_strconcat(path, "/titlebar.bmp", NULL);
	pb = util_pixbuf_new_from_file(file);
	g_free(file);
	if (!pb) return NULL;

	s = skin_new();

	s->real_overlay = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 275, 14);

	pixbuf_copy_area(pb, 27, 29, s->real_overlay, 0, 0, 275, 14, TRUE);
	gdk_pixbuf_unref(pb);

	file = g_strconcat(path, "/text.bmp", NULL);
#if 0
	wamp_text(file, 22, 132, 27, s);
#endif

	digit = wamp_digit_from_text(file, s);
	g_free(file);

	file = g_strconcat(path, "/titlebar.bmp", NULL);
	pb = util_pixbuf_new_from_file(file);
	g_free(file);

	if (pb)
		{
		wamp_button_small(pb, 166, 2, 192, 31, s, "prev");
		wamp_button_small(pb, 176, 2, 202, 31, s, "play");
		wamp_button_small(pb, 186, 2, 212, 31, s, "pause");
		wamp_button_small(pb, 196, 2, 222, 31, s, "stop");
		wamp_button_small(pb, 206, 2, 232, 31, s, "next");
		wamp_button_small(pb, 216, 2, 242, 31, s, "playlist");

		wamp_button(pb, FALSE, 244, 3, 9, 0, 9, 9, s, "iconify");
		wamp_button(pb, FALSE, 264, 3, 18, 0, 9, 9, s, "exit");
		wamp_button_v(pb, FALSE, 254, 3, 0, 27, 9, 9, s, "skin_toggle");
		wamp_button(pb, FALSE, 6, 3, 0, 0, 9, 9, s, "config");
		gdk_pixbuf_unref(pb);
		}

	wamp_number(digit, 2, FALSE, FALSE, 133, 4, s, "minute");
	wamp_number(digit, 2, TRUE, FALSE, 148, 4, s, "second");

	s->real_overlay = util_size_pixbuf(s->real_overlay, TRUE);
	s->width = gdk_pixbuf_get_width(s->real_overlay);
	s->height = gdk_pixbuf_get_height(s->real_overlay);

	s->width_def = s->width_min = s->width_max = s->width;
	s->height_def = s->height_min = s->height_max = s->height;

	return s;

	return NULL;
}


