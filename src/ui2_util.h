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

#ifndef UI2_UTIL_H
#define UI2_UTIL_H


void util_size(gint *n);

void util_color(guint8 *r, guint8 *g, guint8 *b, guint8 *a);

GdkPixbuf *util_pixbuf_new_from_file(const gchar *path);

GdkPixbuf *util_size_pixbuf(GdkPixbuf *pb, gint apply_adjustments);

gint util_clip_region(gint x, gint y, gint w, gint h,
		      gint clip_x, gint clip_y, gint clip_w, gint clip_h,
		      gint *rx, gint *ry, gint *rw, gint *rh);

void util_pixbuf_copy_border_clipped(GdkPixbuf *src,
				     gint border_left, gint border_right,
				     gint border_top, gint border_bottom,
				     GdkPixbuf *dest, gint x, gint y, gint w, gint h,
				     gint clip_x, gint clip_y, gint clip_w, gint clip_h,
				     gint alpha);

gint util_pixbuf_fill_from_root_window(GdkPixbuf *pb, gint x, gint y, gint get_all);
GdkPixbuf *util_pixbuf_from_root_window(gint x, gint y, gint w, gint h, gint get_all);


gint ui_states_save(const gchar *path);
gint ui_states_load(const gchar *path);

void ui_state_set(const gchar *skin_name, const gchar *window_id, gint x, gint y, gint w, gint h);
gint ui_state_get(const gchar *skin_name, const gchar *window_id, gint *x, gint *y, gint *w, gint *h);

void ui_state_set_active(const gchar *skin_name, const gchar *window_id, gint active);
gint ui_state_get_active(const gchar *skin_name, const gchar *window_id);


#endif

