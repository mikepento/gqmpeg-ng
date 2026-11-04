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

#ifndef UI2_BUTTON_H
#define UI2_BUTTON_H


typedef struct _ButtonData ButtonData;
struct _ButtonData
{
        GdkPixbuf *overlay;
        GdkPixbuf *pixbuf;
	gint sections;
	gint has_light;
	gint has_prelight;
	gint pushed;
	gint active;
	gint lit;
	gint prelit;
	gint width;
	gint height;
	gint clip_mask;
	gint x;
	gint y;
	WidgetData *wd;
};


ButtonData *button_new(GdkPixbuf *pb, gint x, gint y,
		       gint prelight, gint light, GdkPixbuf *clip_pb);
ButtonData *button_new_from_file(const gchar *file, gint x, gint y,
				 gint prelight, gint light, const gchar *clip_file);
ButtonData *button_new_from_data(gchar **data, gint x, gint y,
				 gint prelight, gint light, gchar **clip_data);
void button_free(ButtonData *button);

WidgetData *button_register(SkinData *skin, ButtonData *button, const gchar *key, const gchar *text_id);
WidgetData *button_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				    gint prelight, gint light, gchar **clip_data,
				    SkinData *skin, const gchar *text_id);

RegisterData *button_register_key(const gchar *key, UIData *ui,
				  gint (*status_get_func)(ButtonData *button, const gchar *key, gpointer data), gpointer status_get_data,
				  void (*button_click_func)(ButtonData *button, const gchar *key, gpointer data), gpointer button_click_data,
				  void (*button_press_func)(ButtonData *button, const gchar *key, gpointer data), gpointer button_press_data,
				  void (*button_release_func)(ButtonData *button, const gchar *key, gpointer data), gpointer button_release_data);
gint button_state_set(const gchar *key, UIData *ui, gint state);
gint button_state_get(const gchar *key, UIData *ui);


WidgetType button_type_id(void);
void button_type_init(void);


#endif

