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

#ifndef UI_PIXBUF_OPS_H
#define UI_PIXBUF_OPS_H


void pixbuf_copy_area(GdkPixbuf *src, gint sx, gint sy,
		      GdkPixbuf *dest, gint dx, gint dy,
		      gint w, gint h, gint alpha_too);

void pixbuf_copy_area_alpha(GdkPixbuf *src, gint sx, gint sy,
			    GdkPixbuf *dest, gint dx, gint dy,
			    gint w, gint h, gint alpha_modifier);

void pixbuf_copy_area_alpha_with_clipping(GdkPixbuf *src, gint sx, gint sy,
					  GdkPixbuf *dest, gint dx, gint dy,
					  gint w, gint h, gint alpha_modifier,
					  gint clip_x, gint clip_y, gint clip_w, gint clip_h, GdkPixbuf *clip_pb);

void pixbuf_copy_fill(GdkPixbuf *src, gint sx, gint sy, gint sw, gint sh,
			   GdkPixbuf *dest, gint dx, gint dy, gint dw, gint dh,
			   gint stretch, gint alpha_too);

void pixbuf_copy_fill_alpha(GdkPixbuf *src, gint sx, gint sy, gint sw, gint sh,
			    GdkPixbuf *dest, gint dx, gint dy, gint dw, gint dh,
			    gint stretch, gint alpha_modifier);

void pixbuf_copy_fill_alpha_offset(GdkPixbuf *src, gint sx, gint sy, gint sw, gint sh,
				   GdkPixbuf *dest, gint dx, gint dy, gint dw, gint dh,
				   gint stretch, gint alpha_modifier,
				   gint offset_x, gint offset_y);

void pixbuf_copy_fill_border(GdkPixbuf *src, GdkPixbuf *dest,
			     gint x, gint y, gint width, gint height,
			     gint border_left, gint left_stretch,
			     gint border_right, gint right_stretch,
			     gint border_top, gint top_stretch,
			     gint border_bottom, gint bottom_stretch,
			     gint stretch_center, gint alpha_too);

void pixbuf_copy_fill_border_alpha(GdkPixbuf *src, GdkPixbuf *dest,
				   gint x, gint y, gint width, gint height,
				   gint border_left, gint left_stretch,
				   gint border_right, gint right_stretch,
				   gint border_top, gint top_stretch,
				   gint border_bottom, gint bottom_stretch,
				   gint stretch_center,
				   gint alpha_modifier);

/* great for copying only alpha, channel is 0 - 3 for r, g, b, a */
void pixbuf_copy_channel(GdkPixbuf *src, gint sx, gint sy,
			 GdkPixbuf *dest, gint dx, gint dy,
			 gint w, gint h, guint channel);

void pixbuf_copy_rotate_alpha(GdkPixbuf *src, gint offset_x, gint offset_y,
			      GdkPixbuf *dest, gint center_x, gint center_y,
			      double theta);

void pixbuf_copy_rotate_alpha_with_clipping(GdkPixbuf *src, gint offset_x, gint offset_y,
					    GdkPixbuf *dest, gint center_x, gint center_y,
					    double theta,
					    gint clip_x, gint clip_y, gint clip_w, gint clip_h,
					    GdkPixbuf *clip_pb);


/* simple rotation, rotation is 90, 180, or 270 only, all others ignored. */
void pixbuf_copy_area_rotate_90(GdkPixbuf *src, gint sx, gint sy,
			 	GdkPixbuf *dest, gint dx, gint dy,
				gint w, gint h, gint alpha_too, gint rotation);

void pixbuf_copy_area_rotate_90_alpha(GdkPixbuf *src, gint sx, gint sy,
				      GdkPixbuf *dest, gint dx, gint dy,
				      gint w, gint h, gint alpha_modifier, gint rotation);


void pixbuf_draw_rect_fill(GdkPixbuf *pb,
			   gint x, gint y, gint w, gint h,
			   gint r, gint g, gint b, gint a);

void pixbuf_draw_rect(GdkPixbuf *pb,
		      gint x, gint y, gint w, gint h,
		      gint r, gint g, gint b, gint a,
		      gint left, gint right, gint top, gint bottom);

void pixbuf_draw_rect_shift(GdkPixbuf *pb,
			   gint x, gint y, gint w, gint h,
			   gint r, gint g, gint b, gint strength);
guchar pixel_shift(gint p, gint saturation, gint adjustment, gint strength);

void pixbuf_alpha_adjust(GdkPixbuf *pb, gint x, gint y, gint w, gint h, gint a);
void pixbuf_alpha_force_to_bw(GdkPixbuf *pb, guint8 threshhold);

/* this is not meant to be high speed,
   substitute NULL for anything you don't need */
void pixbuf_pixel_get(GdkPixbuf *pb, gint x, gint y, gint *r, gint *g, gint *b, gint *a);

/* nor this */
gint pixbuf_pixel_is_visible(GdkPixbuf *pb, gint x, gint y);

/* analagous to draw point, using the color from the pixel in the source */
void pixbuf_copy_point(GdkPixbuf *src, gint sx, gint sy,
		       GdkPixbuf *dest, gint dx, gint dy, gint use_alpha);
void pixbuf_copy_point_at_alpha(GdkPixbuf *src, gint sx, gint sy,
				GdkPixbuf *dest, gint dx, gint dy, gint alpha);

/* like point above, but a line instead (not antialiased) */
void pixbuf_copy_line(GdkPixbuf *src, gint sx1, gint sy1, gint sx2, gint sy2,
		      GdkPixbuf *dest, gint dx, gint dy, gint use_alpha);



#endif


