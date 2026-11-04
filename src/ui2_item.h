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

#ifndef UI2_ITEM_H
#define UI2_ITEM_H


typedef struct _ItemData ItemData;
struct _ItemData
{
        GdkPixbuf *overlay;
        GdkPixbuf *pixbuf;
	gint sections;
	gint width;
	gint height;
	gint x;
	gint y;
	gint current;
};


ItemData *item_new(GdkPixbuf *pb, gint x, gint y, gint sections);
ItemData *item_new_from_data(gchar **data, gint x, gint y, gint sections);
ItemData *item_new_from_file(const gchar *file, gint x, gint y, gint sections);
void item_free(ItemData *item);

WidgetData *item_register(SkinData *skin, ItemData *item, const gchar *key, const gchar *text_id);
WidgetData *item_register_to_skin(const gchar *key, gchar **data, gint x, gint y,
				  gint sections,
				  SkinData *skin, const gchar *text_id);

RegisterData *item_register_key(const gchar *key, UIData *ui,
				gint (*status_get_func)(ItemData *item, const gchar *key, gpointer data), gpointer status_get_data);

gint item_animation_step(const gchar *key, UIData *ui, gint report_only, gint reset);

gint item_section_get(const gchar *key, UIData *ui);
gint item_section_set(const gchar *key, UIData *ui, gint section);
gint item_section_set_by_percent(const gchar *key, UIData *ui, gfloat value);

gint item_section_by_percent(ItemData *item, gfloat value);


WidgetType item_type_id(void);
void item_type_init(void);


#endif



