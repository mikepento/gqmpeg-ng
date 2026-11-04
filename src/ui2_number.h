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

#ifndef UI2_NUMBER_H
#define UI2_NUMBER_H


typedef struct _DigitData DigitData;
struct _DigitData
{
        GdkPixbuf *overlay;
	gint width;
	gint height;

	gint ref;
};

typedef struct _NumberData NumberData;
struct _NumberData
{
        GdkPixbuf *pixbuf;
	DigitData *digits;
	gint x;
	gint y;
	gint length;
	gint zeros;
	gint centered;

	gint rotation;
	
	gint value;
};


DigitData *digit_new(GdkPixbuf *pb);
DigitData *digit_new_from_data(gchar **data);
DigitData *digit_new_from_file(const gchar *file);

#define digit_rotation_vertical(x) (x == 90 || x == 270)

void digit_ref(DigitData *digit);
void digit_unref(DigitData *digit);

WidgetType digit_type_id(void);
void digit_type_init(void);

WidgetData *digit_register(SkinData *skin, DigitData *digit, const gchar *key, const gchar *text_id);
WidgetData *digit_register_to_skin(const gchar *key, gchar **data,
				   SkinData *skin, const gchar *text_id);

NumberData *number_new(DigitData *digit, gint x, gint y,
		       gint length, gint zeros, gint centered, gint rotation);
void number_free(NumberData *number);

WidgetData *number_register(SkinData *skin, NumberData *number, const gchar *key, const gchar *text_id);
WidgetData *number_register_to_skin(const gchar *key, DigitData *digit, gint x, gint y,
				    gint length, gint zeros, gint centered, gint rotation,
				    SkinData *skin, const gchar *text_id);

RegisterData *number_register_key(const gchar *key, UIData *ui,
				  gint (*status_get_func)(NumberData *number, const gchar *key, gpointer data), gpointer status_get_data);
gint number_value_get(const gchar *key, UIData *ui);
gint number_value_set(const gchar *key, UIData *ui, gint n);


WidgetType number_type_id(void);
void number_type_init(void);


#endif



