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

#ifndef UI2_SLIDER_H
#define UI2_SLIDER_H


typedef struct _SliderData SliderData;
struct _SliderData
{
	GdkPixbuf *overlay;
	GdkPixbuf *pixbuf;
	gint x;
	gint y;
	gint width;
	gint height;
	gint vertical;
	gint reversed;
	gint handle_width;
	gint handle_height;
	gint handle_size;
	gint length;
	gint position;
	gfloat value;
	gint has_press;
	gint has_prelight;
	gint pushed;
	gint prelit;
	gchar *item_key;
	SkinData *skin;

	gint sizeable;
	gint real_length;
	gint border_1;	/* top or left border (dep. on vertical flag) */
	gint border_2;	/* bottom or right border */
	gint stretch;

	gfloat step_size;	/* amount to increment one step */
	gint page_size;		/* number of steps (above) in one page */

	gint key_timer_id;

	WidgetData *wd;
};


SliderData *slider_new(GdkPixbuf *pb, gint x, gint y,
		       gint length, gint vertical, gint reversed,
		       gint has_press, gint prelight,
		       const gchar *item_key);
SliderData *slider_new_from_data(gchar **data, gint x, gint y,
				 gint length, gint vertical, gint reversed,
				 gint has_press, gint prelight,
				 const gchar *item_key);
SliderData *slider_new_from_file(const gchar *file, gint x, gint y,
				 gint length, gint vertical, gint reversed,
				 gint has_press, gint prelight,
				 const gchar *item_key);
void slider_set_attributes(SliderData *slider, gint sizeable,
			   gint real_length, gint border_1, gint border_2, gint stretch);
void slider_free(SliderData *slider);

WidgetData *slider_register(SkinData *skin, SliderData *slider, const gchar *key, const gchar *text_id);
WidgetData *slider_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				    gint length, gint vertical, gint reversed,
				    gint has_press, gint prelight,
				    const gchar *item_key,
				    SkinData *skin, const gchar *text_id);

RegisterData *slider_register_key(const gchar *key, UIData *ui,
				  gfloat (*status_get_func)(SliderData *slider, const gchar *key, gpointer data), gpointer status_get_data,
				  void (*slider_press_func)(SliderData *slider, const gchar *key, gfloat value, gpointer data), gpointer slider_press_data,
				  void (*slider_release_func)(SliderData *slider, const gchar *key, gfloat value, gpointer data), gpointer slider_release_data,
				  void (*slider_drag_func)(SliderData *slider, const gchar *key, gfloat value, gpointer data), gpointer slider_drag_data);
gint slider_value_set(const gchar *key, UIData *ui, gfloat value);
gfloat slider_value_get(const gchar *key, UIData *ui);

gint slider_step_size_set(const gchar *key, UIData *ui, gfloat step_size, gint page_size);


WidgetType slider_type_id(void);
void slider_type_init(void);

/* this is private for use only by ui2_slider_edit.c */
void slider_sync_overlay(SliderData *slider);


#endif







