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

#ifndef UI2_TEXT_H
#define UI2_TEXT_H


#define SLIK_DEFAULT_FONT "Sans 12"


typedef struct _FontData FontData;
struct _FontData
{
	GdkPixbuf *overlay;
	gint char_width;
	gint char_height;
	gint extended;	/* to support chars above 128 */
	gint fill;	/* pad image fonts with spaces at end */

	gchar *text_description;
	PangoFontDescription *font_description;

	gint ref;	/* ref count */
};

typedef struct _TextData TextData;
struct _TextData
{
	FontData *font;

	GdkPixbuf *pixbuf;
	gint width;
	gint height;
	gint x;
	gint y;
	gint sizeable;
	gchar *text;
	gint length;
	gint offset;
	gint scroll;
	gint autoscroll;
	gint autoscroll_reverse_count;
	gint autoscroll_delay;

	guint8 r;
	guint8 g;
	guint8 b;
	guint8 a;

	gint rotation;	/* only supports 0, 90, 180, 270 - all others are read as 0 */

	gint scroll_cb_id;
	SkinData *skin;
};


FontData *font_new(GdkPixbuf *pb, gint extended);
FontData *font_new_from_data(gchar **data, gint extended);
FontData *font_new_from_file(const gchar *file, gint extended);
FontData *font_new_from_x(const gchar *description);

void font_ref(FontData *font);
void font_unref(FontData *font);

WidgetData *font_register(SkinData *skin, FontData *font, const gchar *key, const gchar *text_id);
WidgetData *font_register_to_skin(const gchar *key, gchar **data, 
				  gint extended,
				  SkinData *skin, const gchar *text_id);

/* font_draw and string_length only exported for use by other widgets, do not use in app! */
void font_draw(FontData *font, const gchar *text, gint offset,
	       gint x, gint y, gint w, gint h, gint rotation,
	       guint8 r, guint8 g, guint8 b, guint8 a,
	       GdkPixbuf *pb, UIData *ui, guint8 alpha);
gint font_string_length(FontData *font, const gchar *text);

#define font_rotation_vertical(x) (x == 90 || x == 270)

WidgetType font_type_id(void);
void font_type_init(void);


TextData *text_new(FontData *font, gint x, gint y,
		   gint width, gint sizeable, gint rotation);
TextData *text_new_from_data(gchar **data, gint x, gint y,
			     gint width, gint sizeable, gint rotation, gint extended);
TextData *text_new_from_file(const gchar *file, gint x, gint y,
			     gint width, gint sizeable, gint rotation, gint extended);

TextData *text_new_from_x(const gchar *description, gint x, gint y,
			  gint width, gint sizeable, gint rotation);
void text_set_color(TextData *text, guint8 r, guint8 g, guint8 b, guint8 a);

void text_free(TextData *text);

gint text_set_text(const gchar *key, UIData *ui, const gchar *new_text);

gint text_set_autoscroll(const gchar *key, UIData *ui, gint enable);

gint text_set_scroll_speed(const gchar *key, UIData *ui, gint delay);

WidgetData *text_register(SkinData *skin, TextData *text, const gchar *key, const gchar *text_id);
WidgetData *text_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				  gint width, gint sizeable, gint rotation, gint extended,
				  SkinData *skin, const gchar *text_id);

RegisterData *text_register_key(const gchar *key, UIData *ui,
				const gchar *(*status_get_func)(TextData *text, const gchar *key, gpointer data), gpointer status_get_data);


WidgetType text_type_id(void);
void text_type_init(void);


#endif

