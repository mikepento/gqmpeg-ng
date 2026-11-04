/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#ifndef UI2_DECAL_H
#define UI2_DECAL_H


typedef struct _DecalData DecalData;
struct _DecalData
{
        GdkPixbuf *overlay;
	gint border_left;
	gint border_right;
	gint border_top;
	gint border_bottom;

	gint border_stretch;
	gint stretch;

	guint8 rect_r;
	guint8 rect_g;
	guint8 rect_b;
	gint   rect_fill;

	guint8 alpha;
	gint   sizeable;

	gint width;
	gint height;
	gint x;
	gint y;

	SkinData *skin;
};

DecalData *decal_new(gint x, gint y, gint width, gint height, gint sizeable, guint8 alpha);

void decal_set_image(DecalData *decal, GdkPixbuf *pb, gchar **data, const gchar *file,
		     gint stretch, gint border_stretch);
void decal_set_color(DecalData *decal, guint8 r, guint8 g, guint8 b, gint fill);
void decal_set_border(DecalData *decal, gint border_left, gint border_right,
		      gint border_top, gint border_bottom);

void decal_free(DecalData *decal);

WidgetData *decal_register(SkinData *skin, DecalData *decal, const gchar *key, const gchar *text_id);

WidgetType decal_type_id(void);
void decal_type_init(void);


#endif



