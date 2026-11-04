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

#ifndef UI2_DIAL_H
#define UI2_DIAL_H


typedef struct _DialData DialData;
struct _DialData
{
	GdkPixbuf *overlay;
	GdkPixbuf *pixbuf;
	GdkPixbuf *pb_normal;
	GdkPixbuf *pb_pressed;
	GdkPixbuf *pb_prelit;
	gint center_x;
	gint center_y;
	gint x;
	gint y;
	gint width;
	gint height;
	gint clip_mask;
	gint reversed;

	gint fixed_handle;
	gint fixed_radius;

	gint offset_x;
	gint offset_y;
	gint handle_width;
	gint handle_height;

	gint angle_start;
	gint angle_end;
	gint angle_length;
	gfloat position;
	gfloat increment;
	gfloat value;

	gint has_prelight;
	gint has_press;
	gint pushed;
	gint prelit;

	gfloat press_angle;

	gint idle_id;

	gchar *item_key;

	SkinData *skin;
};


DialData *dial_new(GdkPixbuf *pb, gint x, gint y,
		   gint center_x, gint center_y,
		   gint angle_start, gint angle_end, gint offset_x, gint offset_y,
		   gint has_press, gint has_prelight, gint reversed,
		   gint clip_w, gint clip_h, GdkPixbuf *clip_pb,
		   const gchar *item_key);
DialData *dial_new_from_data(gchar **data, gint x, gint y,
			     gint center_x, gint center_y,
			     gint angle_start, gint angle_end, gint offset_x, gint offset_y,
			     gint has_press, gint has_prelight, gint reversed,
			     gint clip_w, gint clip_h, gchar **clip_data,
			     const gchar *item_key);
DialData *dial_new_from_file(const gchar *file, gint x, gint y,
			     gint center_x, gint center_y,
			     gint angle_start, gint angle_end, gint offset_x, gint offset_y,
			     gint has_press, gint has_prelight, gint reversed,
			     gint clip_w, gint clip_h, const gchar *clip_file,
			     const gchar *item_key);
void dial_set_fixed_handle(DialData *dial, gint fixed, gint radius);
void dial_free(DialData *dial);

WidgetData *dial_register(SkinData *skin, DialData *dial, const gchar *key, const gchar *text_id);
WidgetData *dial_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				  gint center_x, gint center_y,
				  gint angle_start, gint angle_end, gint offset_x, gint offset_y,
				  gint has_press, gint has_prelight, gint reversed,
				  gint clip_w, gint clip_h, gchar **clip_data,
				  const gchar *item_key,
				  SkinData *skin, const gchar *text_id);

RegisterData *dial_register_key(const gchar *key, UIData *ui,
				gfloat (*status_get_func)(DialData *dial, const gchar *key, gpointer data), gpointer status_get_data,
				void (*dial_press_func)(DialData *dial, const gchar *key, gfloat value, gpointer data), gpointer dial_press_data,
				void (*dial_release_func)(DialData *dial, const gchar *key, gfloat value, gpointer data), gpointer dial_release_data,
				void (*dial_drag_func)(DialData *dial, const gchar *key, gfloat value, gpointer data), gpointer dial_drag_data);
gint dial_value_set(const gchar *key, UIData *ui, gfloat value);
gfloat dial_value_get(const gchar *key, UIData *ui);


WidgetType dial_type_id(void);
void dial_type_init(void);


#endif




