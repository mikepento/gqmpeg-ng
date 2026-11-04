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
#include "ui2_text.h"
#include "ui2_text_edit.h"

#include "ui2_display.h"
#include "ui2_main.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_pixbuf_ops.h"


/* pixel increment */
#define TEXT_SCROLL_INCREMENT 2
/* increments are per second */
#define TEXT_DEFAULT_DELAY 24
/* number of increments to wait before reversing scroll direction */
#define TEXT_DEFAULT_AUTOSCROLL_REVERSE 8

typedef struct _TextCallbackData TextCallbackData;
struct _TextCallbackData
{
	const gchar *(*status_get_func)(TextData *text, const gchar *key, gpointer data);
	gpointer status_get_data;
};


/* table to convert iso_8859 chars to similar ascii counterpart
 * only used in font_draw_char()
 */
static const gchar iso_ascii[]=
{
	' ','|','c','L','c','Y','|','S','\"','c',' ','<','!','-','r','~',
	'o',' ','2','3','\'','u','p','.',',','1',' ','>',' ',' ',' ','?',
	'A','A','A','A','A','A','A','C','E','E','E','E','I','I','I','I',
	'D','N','O','O','O','O','O','x','0','U','U','U','U','Y','P','B',
	'a','a','a','a','a','a','a','c','e','e','e','e','i','i','i','i',
	'o','n','o','o','o','o','o','/','o','u','u','u','u','y','p','y'
};


static WidgetType type_id_font = -1;

/*
 *-----------------------------
 * font new / free
 *-----------------------------
 */

FontData *font_new(GdkPixbuf *pb, gint extended)
{
	FontData *font;
	gint width;
	gint height;

	font_type_init();

	if (!pb) return NULL;

	font = g_new0(FontData, 1);

	font->overlay = util_size_pixbuf(pb, TRUE);
	font->extended = extended;
	font->fill = TRUE;

	font->text_description = NULL;
	font->font_description = NULL;

	width = gdk_pixbuf_get_width(font->overlay);
	height = gdk_pixbuf_get_height(font->overlay);

	font->char_width = width / 32;

	if (extended)
		{
		font->char_height = height / 6;
		}
	else
		{
		font->char_height = height / 3;
		}

	font->ref = 1;

	return font;
}

FontData *font_new_from_data(gchar **data, gint extended)
{
	GdkPixbuf *pb;

	pb = gdk_pixbuf_new_from_xpm_data((const char **)data);

	return font_new(pb, extended);
}

FontData *font_new_from_file(const gchar *file, gint extended)
{
	GdkPixbuf *pb;

	pb = util_pixbuf_new_from_file(file);

	return font_new(pb, extended);
}

static gchar *x_font_find_field(const gchar *text, gint n)
{
	const gchar *ptr;
	gint l;

	ptr = text;
	while (n >= 0 && *ptr != '\0')
		{
		if (*ptr == '-') n--;
		ptr++;
		}

	if (n >= 0 || *ptr == '*') return NULL;

	l = 0;
	while (ptr[l] != '\0' && ptr[l] != '-') l++;

	if (l < 1) return NULL;

	return g_strndup(ptr, l);
}

/*
 * This is to pull the dpi used by pango, and scale the point size so that
 * the pixel size is always equal to that on a 96 dpi monitor
 */
static gint x_font_convert_points_by_dpi(gint points)
{
	GtkSettings *settings;
	GObjectClass *klass;
	gint dpi = -1;

	settings = gtk_settings_get_default();
	klass = G_OBJECT_CLASS(GTK_SETTINGS_GET_CLASS(settings));
	if (g_object_class_find_property(klass, "gtk-xft-dpi"))
		{
		static gint debugged = FALSE;
		int int_dpi;

		g_object_get(settings, "gtk-xft-dpi", &int_dpi, NULL);
		dpi = int_dpi / PANGO_SCALE;

		if (debug_mode && !debugged)
			{
			printf("pango is using a dpi of %d\n", dpi);
			debugged = TRUE;
			}
		}

	if (dpi < 25)
		{
		static gint warned = FALSE;
		gint fallback_dpi = 96;

		/* not set or invalid, just assume the default */

		if (!warned)
			{
			if (dpi == -1)
				{
				printf("pango dpi unknown, assuming %d\n", fallback_dpi);
				}
			else
				{
				printf("pango dpi reported as %d ignored, assuming %d\n", dpi, fallback_dpi);
				}
			warned = TRUE;
			}

		dpi = fallback_dpi;
		}

	return points * 96 / dpi;
}

static gchar *x_font_convert_to_pango_string(const gchar *description)
{
	gchar *result;
	GString *str = NULL;
	const gchar *ptr;
	gint c = 0;
	gchar *family;
	gchar *weight;
	gchar *slant;
	gchar *size;
	gint space;

	if (!description) return NULL;

	ptr = description;
	if (*ptr != '-') return NULL;
	while (*ptr != '\0')
		{
		if (*ptr == '-') c++;
		ptr++;
		}

	if (c != 14) return NULL;

	family = x_font_find_field(description, 1);
	weight = x_font_find_field(description, 2);
	slant  = x_font_find_field(description, 3);
	size   = x_font_find_field(description, 6);

	str = g_string_new("");
	space = FALSE;

	g_string_append(str, (family && strlen(family) > 0) ? family : "Sans");
	space = TRUE;

	if (weight)
		{
		if (space) g_string_append(str, " ");
		g_string_append(str, weight);
		space = TRUE;
		}

	if (slant &&
	    (strcasecmp(slant, "i") == 0 || strcasecmp(slant, "o") == 0))
		{
		if (space) g_string_append(str, " ");
		g_string_append(str, "italic");
		space = TRUE;
		}

	if (size)
		{
		gint s;

		s = strtol(size, NULL, 10);
		if (s < 8) s = 10;

		/* pango size is in points, we are coming from pixels, try to convert
		 * (there is no exact correlation? but this seems to work ok - for me.)
		 */
		s = s * 75 / 96;

		if (space) g_string_append(str, " ");
		g_string_append_printf(str, "%d", s);
		}
	else
		{
		printf("warning: font description contains no size information [%s]\n", description);
		}

	g_free(family);
	g_free(weight);
	g_free(slant);
	g_free(size);

	result = str->str;
	g_string_free(str, FALSE);

	if (debug_mode)
		{
		printf("Pango font description \"%s\"\n   auto converted from \"%s\"\n", result, description);
		}

	return result;
}

FontData *font_new_from_x(const gchar *description)
{
	FontData *font;

	font_type_init();

	font = g_new0(FontData, 1);

	font->overlay = NULL;
	font->extended = FALSE;
	font->fill = FALSE;

	font->text_description = g_strdup(description);
	font->font_description = NULL;

	if (font->text_description)
		{
		gchar *converted;

		converted = x_font_convert_to_pango_string(font->text_description);
		if (converted)
			{
			font->font_description = pango_font_description_from_string(converted);
			g_free(converted);
			}
		else
			{
			font->font_description = pango_font_description_from_string(font->text_description);
			}
		}
	if (!font->font_description)
		{
		printf("Warning: unable to load font \"%s\"\n", font->text_description);
		font->font_description = pango_font_description_from_string(SLIK_DEFAULT_FONT);
		}
	if (!font->font_description)
		{
		font->font_description = pango_font_description_from_string("fixed");
		}

	if (font->font_description)
		{
		PangoContext *context;
		PangoFontMetrics *metrics;
		gint points;

		/* Convert point size based on dpi used by pango,
		 * so that pixel size is always equivelent regardless of dpi setting.
		 */
		points = pango_font_description_get_size(font->font_description);
		points = x_font_convert_points_by_dpi(points);
		if (points > 0) pango_font_description_set_size(font->font_description, points);

		context = gdk_pango_context_get_for_screen(gdk_screen_get_default());
		metrics = pango_context_get_metrics(context, font->font_description,
						    pango_context_get_language(context));

		font->char_height = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
						 pango_font_metrics_get_descent(metrics));

		pango_font_metrics_unref(metrics);
		g_object_unref(context);
		}
	else
		{
		printf("Warning: unable to load any font\n");

		font->char_height = 1;
		}

	font->char_width = 1;

	font->ref = 1;

	return font;
}

static void font_free(FontData *font)
{
	if (!font) return;

	if (font->overlay) gdk_pixbuf_unref(font->overlay);

	if (font->font_description) pango_font_description_free(font->font_description);
	g_free(font->text_description);

	g_free(font);
}

void font_ref(FontData *font)
{
	if (font) font->ref++;
}

void font_unref(FontData *font)
{
	if (font)
		{
		font->ref--;
		if (font->ref < 1) font_free(font);
		}
}

static void font_unref_cb(gpointer data)
{
	font_unref((FontData *)data);
}

/*
 *-----------------------------
 * draw
 *-----------------------------
 */

static void font_draw_char(FontData *font, guint8 c,
			   gint x, gint y,
			   gint xp, gint offset, gint width, gint total_width, gint rotation,
			   GdkPixbuf *pb, guint8 alpha)
{
	gint px, py;

	if (font->extended)
		{
		if (c >= 32 && c < 128)
			c -= 32;
		else if (c >= 160 && c < 255)
			c -= 64;
		else
			c = 0;
		}
	else
		{
		if (c >= 32 && c < 128)
			c -= 32;
		else if (c >= 160 && c < 255)
			/* translate to a limited set */
			c = iso_ascii[c - 160] - 32;
		else
			c = 0;
		}

	py = c / 32;
	px = c - (py * 32);

	px = px * font->char_width;
	py = py * font->char_height;

	switch (rotation)
		{
		case 0:
			pixbuf_copy_area_alpha(font->overlay, px + offset, py,
					       pb, x + xp, y,
			 		       width, font->char_height, alpha);
			break;
		case 90:
			pixbuf_copy_area_rotate_90_alpha(font->overlay, px + offset, py,
							 pb, x, y + xp,
							 width, font->char_height, alpha, rotation);
			break;
		case 180:
			pixbuf_copy_area_rotate_90_alpha(font->overlay, px + offset, py,
							 pb, x + total_width - xp - width, y,
							 width, font->char_height, alpha, rotation);
			break;
		case 270:
			pixbuf_copy_area_rotate_90_alpha(font->overlay, px + offset, py,
							 pb, x, y + total_width - xp - width,
							 width, font->char_height, alpha, rotation);
			break;
		}
}

static void font_draw_string(FontData *font, const gchar *text, gint offset,
			     gint x, gint y, gint w, gint h, gint rotation,
			     GdkPixbuf *pb, gint alpha)
{
	gint xp;
	gint xo;
	gint x_end;
	gint i;
	gint l;

	l = strlen(text);

	if (offset > 0)
		{
		i = offset / font->char_width;
		xo = offset - (i * font->char_width);
		}
	else
		{
		i = 0;
		xo = 0;
		}
	xp = 0;
	x_end = w;
	
	while (i < l && xp < x_end)
		{
		gint xw;

		xw = MIN(font->char_width - xo, x_end - xp);

		font_draw_char(font, text[i], x, y, xp, xo, xw, w, rotation, pb, alpha);

		xp+= xw;
		xo = 0;
		i++;
		}
	
	/* fill remainder of field with spaces */
	while (font->fill && xp < x_end)
		{
		gint xw;

		xw = MIN(font->char_width - xo, x_end - xp);

		font_draw_char(font, ' ', x, y, xp, xo, xw, w, rotation, pb, alpha);

		xp += xw;
		xo = 0;
		}
}

static void pixbuf_font_rotate_0(guchar *s_pix, gint srs, gint s_step, gint s_alpha, gint sx, gint sy, gint w, gint h,
				 guchar *d_pix, gint drs, gint d_step, gint d_alpha, gint dx, gint dy,
				 guint8 r, guint8 g, guint8 b, guint8 a)
{
	guchar *sp;
        guchar *dp;
	gint i, j;

	for (i = 0; i < h; i++)
		{
		sp = s_pix + (sy + i) * srs + sx * s_step;
		dp = d_pix + (dy + i) * drs + dx * d_step;
		for (j = 0; j < w; j++)
			{
			if (*sp)
				{
#if 0
				*dp = (r * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (g * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (b * a + *dp * (256-a)) >> 8;
#endif
				guint8 asub;

				asub = a * sp[0] / 255;
				*dp = (r * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[1] / 255;
				*dp = (g * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[2] / 255;
				*dp = (b * asub + *dp * (256-asub)) >> 8;

				dp++;
				if (d_alpha) dp++;
				}
			else
				{
				dp += d_step;
				}

			sp += s_step;
			}
		}
}

static void pixbuf_font_rotate_180(guchar *s_pix, gint srs, gint s_step, gint s_alpha, gint sx, gint sy, gint w, gint h,
				   guchar *d_pix, gint drs, gint d_step, gint d_alpha, gint dx, gint dy,
				   guint8 r, guint8 g, guint8 b, guint8 a)
{
	guchar *sp;
        guchar *dp;
	gint i, j;

	for (i = 0; i < h; i++)
		{
		sp = s_pix + (sy + i) * srs + sx * s_step;
		dp = d_pix + (dy + h - i - 1) * drs + (dx + w - 1) * d_step;
		for (j = 0; j < w; j++)
			{
			if (*sp)
				{
#if 0
				*dp = (r * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (g * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (b * a + *dp * (256-a)) >> 8;
#endif
				guint8 asub;

				asub = a * sp[0] / 255;
				*dp = (r * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[1] / 255;
				*dp = (g * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[2] / 255;
				*dp = (b * asub + *dp * (256-asub)) >> 8;

				dp -= 2;
				}

			dp -= d_step;

			sp += s_step;
			}
		}
}

static void pixbuf_font_rotate_90(guchar *s_pix, gint srs, gint s_step, gint s_alpha, gint sx, gint sy, gint w, gint h,
				  guchar *d_pix, gint drs, gint d_step, gint d_alpha, gint dx, gint dy,
				  guint8 r, guint8 g, guint8 b, guint8 a)
{
	guchar *sp;
        guchar *dp;
	gint i, j;

	for (i = 0; i < w; i++)
		{
		sp = s_pix + sy * srs + (sx + i) * s_step;
		dp = d_pix + (dy + i) * drs + (dx + h - 1) * d_step;
		for (j = 0; j < h; j++)
			{
			if (*sp)
				{
#if 0
				*dp = (r * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (g * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (b * a + *dp * (256-a)) >> 8;
#endif
				guint8 asub;

				asub = a * sp[0] / 255;
				*dp = (r * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[1] / 255;
				*dp = (g * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[2] / 255;
				*dp = (b * asub + *dp * (256-asub)) >> 8;

				dp -= 2;
				}

			dp -= d_step;

			sp += srs;
			}
		}
}

static void pixbuf_font_rotate_270(guchar *s_pix, gint srs, gint s_step, gint s_alpha, gint sx, gint sy, gint w, gint h,
				   guchar *d_pix, gint drs, gint d_step, gint d_alpha, gint dx, gint dy,
				   guint8 r, guint8 g, guint8 b, guint8 a)
{
	guchar *sp;
        guchar *dp;
	gint i, j;

	for (i = 0; i < w; i++)
		{
		sp = s_pix + sy * srs + (sx + i) * s_step;
		dp = d_pix + (dy + w - i - 1) * drs + dx * d_step;
		for (j = 0; j < h; j++)
			{
			if (*sp)
				{
#if 0
				*dp = (r * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (g * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (b * a + *dp * (256-a)) >> 8;
#endif
				guint8 asub;

				asub = a * sp[0] / 255;
				*dp = (r * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[1] / 255;
				*dp = (g * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[2] / 255;
				*dp = (b * asub + *dp * (256-asub)) >> 8;

				dp++;
				if (d_alpha) dp++;
				}
			else
				{
				dp += d_step;
				}

			sp += srs;
			}
		}
}

static void pixbuf_copy_font(GdkPixbuf *src, gint sx, gint sy,
			     GdkPixbuf *dest, gint dx, gint dy,
			     gint w, gint h, gint rotation,
			     guint8 r, guint8 g, guint8 b, guint8 a)
{
	gint s_alpha;
	gint d_alpha;
	gint sw, sh, srs;
	gint s_step;
	gint dw, dh, drs;
	gint d_step;
	guchar *s_pix;
        guchar *d_pix;

	if (!src || !dest) return;

	sw = gdk_pixbuf_get_width(src);
	sh = gdk_pixbuf_get_height(src);

	if (sx < 0 || sx + w > sw) return;
	if (sy < 0 || sy + h > sh) return;

	dw = gdk_pixbuf_get_width(dest);
	dh = gdk_pixbuf_get_height(dest);

	if (rotation == 90 || rotation == 270)
		{
		if (dx < 0 || dx + h > dw) return;
		if (dy < 0 || dy + w > dh) return;
		}
	else
		{
		if (dx < 0 || dx + w > dw) return;
		if (dy < 0 || dy + h > dh) return;
		}

	s_alpha = gdk_pixbuf_get_has_alpha(src);
	d_alpha = gdk_pixbuf_get_has_alpha(dest);
	srs = gdk_pixbuf_get_rowstride(src);
	drs = gdk_pixbuf_get_rowstride(dest);
	s_pix = gdk_pixbuf_get_pixels(src);
	d_pix = gdk_pixbuf_get_pixels(dest);

	s_step = (s_alpha) ? 4 : 3;
	d_step = (d_alpha) ? 4 : 3;

	switch (rotation)
		{
		case 0:
			pixbuf_font_rotate_0(s_pix, srs, s_step, s_alpha, sx, sy, w, h,
					     d_pix, drs, d_step, d_alpha, dx, dy,
					     r, g, b, a);
			break;
		case 90:
			pixbuf_font_rotate_90(s_pix, srs, s_step, s_alpha, sx, sy, w, h,
					      d_pix, drs, d_step, d_alpha, dx, dy,
					      r, g, b, a);
			break;
		case 180:
			pixbuf_font_rotate_180(s_pix, srs, s_step, s_alpha, sx, sy, w, h,
					       d_pix, drs, d_step, d_alpha, dx, dy,
					       r, g, b, a);
			break;
		case 270:
			pixbuf_font_rotate_270(s_pix, srs, s_step, s_alpha, sx, sy, w, h,
					       d_pix, drs, d_step, d_alpha, dx, dy,
					       r, g, b, a);
			break;
		}
}

static void font_draw_string_x(FontData *font, const gchar *text, gint offset,
			       gint x, gint y, gint w, gint h, gint rotation,
			       guint8 r, guint8 g, guint8 b, guint8 a,
			       GdkPixbuf *pb, UIData *ui)
{
	PangoLayout *layout;
	GdkGC *gc;
	GdkPixbuf *buffer_pixbuf;
	GdkPixmap *buffer_pixmap;
	GdkRectangle rect;

	skin_get_scratch(ui->skin, w, h, &buffer_pixbuf, &buffer_pixmap);

	gc = gdk_gc_new(ui->display->window);

	gdk_gc_copy(gc, ui->display->style->black_gc);
	gdk_draw_rectangle(buffer_pixmap, gc, TRUE, 0, 0, w, h);

	gdk_gc_copy(gc, ui->display->style->white_gc);
	rect.x = 0;
	rect.y = 0;
	rect.width = w;
	rect.height = h;
	gdk_gc_set_clip_rectangle(gc, &rect);
#if 0
	gdk_draw_string(buffer_pixmap, font->font, gc, 0 - offset, font->font->ascent, text);
#endif
	layout = gtk_widget_create_pango_layout(ui->display, NULL);
	pango_layout_set_font_description(layout, font->font_description);
	pango_layout_set_text(layout, text, -1);
	gdk_draw_layout(buffer_pixmap, gc, 0 - offset, 0, layout);
	g_object_unref(G_OBJECT(layout));

	gdk_pixbuf_get_from_drawable(buffer_pixbuf, buffer_pixmap,
		       		     gdk_drawable_get_colormap(ui->display->window),
				     0, 0, 0, 0, w, h);

	pixbuf_copy_font(buffer_pixbuf, 0, 0,
			 pb, x, y, w, h, rotation,
			 r, g, b, a);

	g_object_unref(gc);
}

void font_draw(FontData *font, const gchar *text, gint offset,
	       gint x, gint y, gint w, gint h, gint rotation,
	       guint8 r, guint8 g, guint8 b, guint8 a,
	       GdkPixbuf *pb, UIData *ui, guint8 alpha)
{
	if (!font || !text) return;

	if (font_rotation_vertical(rotation))
		{
		gint t;
		t = w;
		w = h;
		h = t;
		}

	if (font->overlay)
		{
		font_draw_string(font, text, offset,
				 x, y, w, h, rotation, pb, alpha);
		}
	else if (font->font_description)
		{
		font_draw_string_x(font, text, offset,
				   x, y, w, h, rotation,
				   r, g, b, ((a * alpha) >> 8),
				   pb, ui);
		}
	else
		{
		printf("ui2_text.c warning: attempt to draw font with NULL overlay and font description\n");
		}
}

gint font_string_length(FontData *font, const gchar *text)
{
	if (!font || !text) return 0;

	if (font->overlay)
		{
		return strlen(text) * font->char_width;
		}
	if (font->font_description)
		{
		gint width;

		PangoLayout *layout;
		PangoContext *context;

		context = gdk_pango_context_get_for_screen(gdk_screen_get_default());

		layout = pango_layout_new(context);
		pango_layout_set_font_description(layout, font->font_description);
		pango_layout_set_text(layout, text, -1);

		pango_layout_get_pixel_size(layout, &width, NULL);

		g_object_unref(context);
		g_object_unref(layout);

		return width;
		}

	return 0;
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static gint font_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	FontData *font = widget;

	/* fonts have no geometry, they are not directly displayed */
	*x = 0;
	*y = 0;
	*w = font->char_width;
	*h = font->char_height;

	return TRUE;
}

static WidgetData *font_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	FontData *font;
	const gchar *text_id;
	gchar *filename;
	const gchar *description;
	gint extended;

	/* req */
	filename = key_list_read_path(list, "image", skin_dir);

	/* opt */
	extended = key_list_read_bool(list, "extended");
	description = key_list_read_chars(list, "description", NULL);
	text_id = key_list_read_chars(list, "id", NULL);

	if (filename)
		{
		font = font_new_from_file(filename, extended);
		}
	else
		{
		font = font_new_from_x(description);
		}

	if (font)
		{
		wd = font_register(skin, font, key, text_id);
		font_unref(font);

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

WidgetData *font_register(SkinData *skin, FontData *font, const gchar *key, const gchar *text_id)
{
	font_ref(font);
	return skin_register_widget(skin, key, text_id, type_id_font, font);
}

WidgetData *font_register_to_skin(const gchar *key, gchar **data, 
				  gint extended,
				  SkinData *skin, const gchar *text_id)
{
	FontData *font;

	font = font_new_from_data(data, extended);
	return font_register(skin, font, key, text_id);
}

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType font_type_id(void)
{
	return type_id_font;
}

void font_type_init(void)
{
	WidgetObjectData *od;

	if (type_id_font != -1) return;

	od = ui_widget_type_new("font");
	type_id_font = od->type;
	od->is_visible = FALSE;

	od->func_free = font_unref_cb;

	od->func_get_geometry = font_get_geometry;

	od->func_parse = font_parse;

	font_type_init_edit(od);
}

/*
 *------------
 * text side
 *------------
 */

static WidgetType type_id = -1;

static void text_scroll_update_set(TextData *text, gint enable);

/*
 *-----------------------------
 * new / free
 *-----------------------------
 */

TextData *text_new(FontData *font, gint x, gint y,
		   gint width, gint sizeable, gint rotation)
{
	TextData *text;

	font_type_init();
	text_type_init();

	if (!font || width < 1) return NULL;

	util_size(&x);
	util_size(&y);
	util_size(&width);
	/* FIXME: X font does not support double size properly! */

	text = g_new0(TextData, 1);

	text->font = font;
	font_ref(text->font);

	text->rotation = rotation;
	if (font_rotation_vertical(text->rotation))
		{
		text->width = text->font->char_height;;
		text->height = width;
		}
	else
		{
		text->width = width;
		text->height = text->font->char_height;
		if (text->rotation != 0 && text->rotation != 180)
			{
			printf("unkown/unsupported text rotation value %d, using default (0)\n", text->rotation);
			text->rotation = 0;
			}
		}
			
	text->x = x;
	text->y = y;
	text->text = NULL;
	text->sizeable = sizeable;

	text->length = 0;
	text->offset = 0;

	text->r = 0;
	text->g = 0;
	text->b = 0;
	text->a = 255;

	text->autoscroll_delay = TEXT_DEFAULT_DELAY;
	text->autoscroll = FALSE;
	text->autoscroll_reverse_count = 0;

	text->scroll = 0;
	text->scroll_cb_id = -1;

	text->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, text->width, text->height);

	return text;
}

TextData *text_new_from_data(gchar **data, gint x, gint y,
			     gint width, gint sizeable, gint rotation, gint extended)
{
	TextData *text;
	FontData *font;

	font = font_new_from_data(data, extended);

	text = text_new(font, x, y, width, sizeable, rotation);
	font_unref(font);
	return text;
}

TextData *text_new_from_file(const gchar *file, gint x, gint y,
			     gint width, gint sizeable, gint rotation, gint extended)
{
	TextData *text;
	FontData *font;

	font = font_new_from_file(file, extended);

	text = text_new(font, x, y, width, sizeable, rotation);
	font_unref(font);
	return text;
}

TextData *text_new_from_x(const gchar *description, gint x, gint y,
			  gint width, gint sizeable, gint rotation)
{
	TextData *text;
	FontData *font;

	font = font_new_from_x(description);

	text = text_new(font, x, y, width, sizeable, rotation);
	font_unref(font);
	return text;
}

void text_set_color(TextData *text, guint8 r, guint8 g, guint8 b, guint8 a)
{
	if (!text) return;

	util_color(&r, &g, &b, &a);

	text->r = r;
	text->g = g;
	text->b = b;
	text->a = a;
}

void text_free(TextData *text)
{
	if (!text) return;

	text_scroll_update_set(text, FALSE);

	font_unref(text->font);
	if (text->pixbuf) gdk_pixbuf_unref(text->pixbuf);
	g_free(text->text);
	g_free(text);
}

static void text_free_cb(gpointer data)
{
	text_free((TextData *)data);
}

/*
 *-----------------------------
 * draw
 *-----------------------------
 */

static void text_draw_real(TextData *text, GdkPixbuf *pb, UIData *ui)
{
	gint width;

	if (!text) return;

	if (font_rotation_vertical(text->rotation))
		{
		width = text->height;
		}
	else
		{
		width = text->width;
		}
	
	if (text->offset >= text->length - width) text->offset = text->length - width;
	if (text->offset < 0) text->offset = 0;

	pixbuf_copy_area(text->pixbuf, 0, 0, pb, text->x, text->y, text->width, text->height, FALSE);

	font_draw(text->font, text->text, text->offset,
		  text->x, text->y, text->width, text->height, text->rotation,
		  text->r, text->g, text->b, text->a,
		  pb, ui, 255);

	ui_display_render_area(ui, text->x, text->y, text->width, text->height, NULL);

	text_scroll_update_set(text, TRUE);
}

static void text_set_text_real(TextData *text, const gchar *new_text, GdkPixbuf *pb, UIData *ui)
{
	g_free(text->text);
	
	if (new_text)
		{
		text->text = g_strdup(new_text);
		}
	else
		{
		text->text = NULL;
		}

	text->length = font_string_length(text->font, text->text);
	text->offset = 0;
	text->autoscroll_reverse_count = 0;
	text->scroll = -1;

	text_draw_real(text, pb, ui);
}

static gint text_scroll_update_cb(gpointer data)
{
	TextData *text = data;
	GdkPixbuf *pb;
	UIData *ui;
	gint old_offset;

	if (!text) return FALSE;
	if (!text->text || !text->skin) return TRUE;
	if (text->scroll_cb_id == -1) return FALSE;

	pb = skin_get_pixbuf(text->skin);
	ui = skin_get_ui(text->skin);

	if (!pb) return TRUE;

	old_offset = text->offset;

	if (text->scroll > 0)
		{
		gint width;

		if (font_rotation_vertical(text->rotation))
			{
			width = text->height;
			}
		else
			{
			width = text->width;
			}
		
		if (text->offset < text->length - width && text->length > width)
			{
			text->offset += TEXT_SCROLL_INCREMENT;
			if (text->offset > text->length) text->offset = text->length;
			}
		else if (text->autoscroll)
			{
			text->autoscroll_reverse_count++;
			if (text->autoscroll_reverse_count > TEXT_DEFAULT_AUTOSCROLL_REVERSE)
				{
				text->autoscroll_reverse_count = 0;
				text->scroll = -1;
				}
			}
		}
	else
		{
		if (text->offset > 0)
			{
			text->offset -= TEXT_SCROLL_INCREMENT;
			if (text->offset < 0) text->offset = 0;
			}
		else if (text->autoscroll)
			{
			text->autoscroll_reverse_count++;
			if (text->autoscroll_reverse_count > TEXT_DEFAULT_AUTOSCROLL_REVERSE)
				{
				text->autoscroll_reverse_count = 0;
				text->scroll = 1;
				}
			}
		}

	if (text->offset != old_offset) text_draw_real(text, pb, ui);

	return TRUE;
}

static void text_scroll_update_set(TextData *text, gint enable)
{
	if (enable && text->scroll_cb_id == -1)
		{
		text->scroll_cb_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000 / text->autoscroll_delay,
							text_scroll_update_cb, text, NULL);
		}
	else if (!enable && text->scroll_cb_id != -1)
		{
		g_source_remove(text->scroll_cb_id);
		text->scroll_cb_id = -1;
		}
}

/*
 *-----------------------------
 * ui funcs
 *-----------------------------
 */

static void text_draw(gpointer data, const gchar *key, gint update, gint force, GdkPixbuf *pb, UIData *ui)
{
	TextData *text = data;

	if (update)
		{
		TextCallbackData *cd;

		cd = ui_get_registered_callbacks(ui, key, type_id);
		if (cd && cd->status_get_func)
			{
			const gchar *new_text;
			new_text = cd->status_get_func(text, key, cd->status_get_data);
			text_set_text_real(text, new_text, pb, ui);
			}
		else
			{
			const gchar *new_text;
			new_text = ui_widget_get_data_by_widget(ui, text);
			text_set_text_real(text, new_text, pb, ui);
			}
		}
	else if (force)
		{
		text_draw_real(text, pb, ui);
		}
}

static void text_reset(gpointer data, const gchar *key, GdkPixbuf *pb, UIData *ui)
{
	TextData *text = data;

	if (!text) return;

	text->scroll = -1;
}

static void text_motion(gpointer data, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui)
{
	TextData *text = data;

	if (!text) return;

	if (text->autoscroll) return;

	if (x >= text->x && x < text->x + text->width &&
	    y >= text->y && y < text->y + text->height)
		{
		text->scroll = 1;
		}
	else
		{
		text->scroll = -1;
		}
}

static void text_back_set(gpointer data, GdkPixbuf *pb)
{
	TextData *text = data;

	if (!text) return;

	pixbuf_copy_area(pb, text->x, text->y, text->pixbuf, 0, 0,
			 text->width, text->height, FALSE);
}

static gint text_get_geometry(gpointer widget, gint *x, gint *y, gint *w, gint *h)
{
	TextData *text = widget;

	*x = text->x;
	*y = text->y;
	*w = text->width;
	*h = text->height;

	return TRUE;
}

static void text_set_coord(gpointer widget, gint x, gint y)
{
	TextData *text = widget;

	text->x = x;
	text->y = y;
}

static void text_set_size(gpointer widget, gint dev_w, gint dev_h)
{
	TextData *text = widget;
	gint old;

	if (!text->sizeable) return;

	if (font_rotation_vertical(text->rotation))
		{
		old = text->height;

		text->height += dev_h;
		if (text->height < 1) text->height = 1;

		if (old == text->height) return;
		}
	else
		{
		old = text->width;

		text->width += dev_w;
		if (text->width < 1) text->width = 1;

		if (old == text->width) return;
		}

	if (text->pixbuf) gdk_pixbuf_unref(text->pixbuf);
	text->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, text->width, text->height);
}

static WidgetData *text_parse(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit)
{
	WidgetData *wd = NULL;
	TextData *text;
	gchar *filename;
	const gchar *font_id;
	gint x, y;
	gint width;
	gint length;
	gint extended;
	gint sizeable;
	gint rotation;
	const gchar *description;
	gint r, g, b, a;

	/* req */
	if (!key_list_read_int(list, "x", &x)) return NULL;
	if (!key_list_read_int(list, "y", &y)) return NULL;

	if (!key_list_read_int(list, "width", &width))
		{
		/* backwards compatibility, old format specified a length in chars */
		if (!key_list_read_int(list, "length", &length)) return NULL;
		width = 1;
		}
	else
		{
		length = 0;
		}

	filename = key_list_read_path(list, "image", skin_dir);
	description = key_list_read_chars(list, "description", NULL);
	font_id = key_list_read_chars(list, "font", NULL);
	if (!filename && !description && !font_id) return NULL;

	/* opt */
	extended = key_list_read_bool(list, "extended");
	sizeable = key_list_read_bool(list, "sizeable");

	if (!key_list_read_int(list, "rotation", &rotation)) rotation = 0;

	if (!key_list_read_int(list, "red", &r)) r = 0;
	if (!key_list_read_int(list, "green", &g)) g = 0;
	if (!key_list_read_int(list, "blue", &b)) b = 0;
	if (!key_list_read_int(list, "alpha", &a)) a = 255;

	if (filename)
		{
		text = text_new_from_file(filename, x, y, width, sizeable, rotation, extended);

		/* backwards compatibility, width is now in pixels instead of chars */
		if (text && length > 0)
			{
			text->width = length * text->font->char_width;
			gdk_pixbuf_unref(text->pixbuf);
			text->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, text->width, text->height);
			}
		}
	else if (description)
		{
		text = text_new_from_x(description, x, y, width, sizeable, rotation);
		}
	else
		{
		WidgetData *font_wd;
		FontData *font;

		font_wd = skin_widget_get_by_key(skin, font_id, font_type_id());
		if (!font_wd)
			{
			printf("font not found registered as \"%s\"\n", font_id);
			return NULL;
			}

		font = font_wd->widget;

		/* backwards compatibility, width is now pixels instead of chars */
		if (length > 0) width = length * font->char_width;

		text = text_new(font, x, y, width, sizeable, rotation);
		}

	if (text)
		{
		text_set_color(text, r, g, b, a);
		wd = text_register(skin, text, key, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			ui_widget_set_data(wd, "font_id", font_id);
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

WidgetData *text_register(SkinData *skin, TextData *text, const gchar *key, const gchar *text_id)
{
	text->skin = skin;
	return skin_register_widget(skin, key, text_id, type_id, text);
}

WidgetData *text_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				  gint width, gint sizeable, gint rotation, gint extended,
				  SkinData *skin, const gchar *text_id)
{
	TextData *text;

	text = text_new_from_data(data, x, y, width, sizeable, rotation, extended);
	return text_register(skin, text, key, text_id);
}

RegisterData *text_register_key(const gchar *key, UIData *ui,
				const gchar *(*status_get_func)(TextData *text, const gchar *key, gpointer data), gpointer status_get_data)
{
	TextCallbackData *cd;

	font_type_init();
	text_type_init();

	cd = g_new0(TextCallbackData, 1);

	cd->status_get_func = status_get_func;
	cd->status_get_data = status_get_data;

	return ui_register_key(ui, key, type_id, cd, sizeof(TextCallbackData));
}

/*
 *-----------------------------
 * app funcs
 *-----------------------------
 */

static void text_set_text_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	TextData *text;
	const gchar *new_text;

	text = wd->widget;
	new_text = (const gchar *)data;

	text_set_text_real(text, new_text, pb, ui);
}

gint text_set_text(const gchar *key, UIData *ui, const gchar *new_text)
{
	return skin_widget_for_each_key(ui, key, type_id, text_set_text_cb, (gchar *)new_text);
}

static void text_set_autoscroll_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	TextData *text;

	text = wd->widget;
	text->autoscroll = GPOINTER_TO_INT(data);
}

gint text_set_autoscroll(const gchar *key, UIData *ui, gint enable)
{
	return skin_widget_for_each_key(ui, key, type_id, text_set_autoscroll_cb, GINT_TO_POINTER(enable));
}

static void text_set_scroll_speed_cb(WidgetData *wd, gpointer data, GdkPixbuf *pb, UIData *ui)
{
	TextData *text;
	gint val;

	text = wd->widget;
	val = GPOINTER_TO_INT(data);
	if (val < 0) val = 1;
	text->autoscroll_delay = val;

	text_scroll_update_set(text, FALSE);
	text_scroll_update_set(text, TRUE);
}

gint text_set_scroll_speed(const gchar *key, UIData *ui, gint delay)
{
	return skin_widget_for_each_key(ui, key, type_id, text_set_scroll_speed_cb, GINT_TO_POINTER(delay));
}

/*
 *-----------------------------
 * init
 *-----------------------------
 */

WidgetType text_type_id(void)
{
	return type_id;
}

void text_type_init(void)
{
	WidgetObjectData *od;

	if (type_id != -1) return;

	font_type_init();

	od = ui_widget_type_new("text");
	type_id = od->type;

	od->func_draw = text_draw;
	od->func_reset = text_reset;
	od->func_motion = text_motion;
	od->func_back = text_back_set;
	od->func_free = text_free_cb;

	od->func_get_geometry = text_get_geometry;
	od->func_set_coord = text_set_coord;
	od->func_set_size = text_set_size;

	od->func_parse = text_parse;

	text_type_init_edit(od);
}
