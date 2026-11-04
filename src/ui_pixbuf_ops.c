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

#include <stdlib.h>
#include <math.h>

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "ui_pixbuf_ops.h"


/*
 * Copies src to dest, optionally applying alpha
 * but takes into account the source's alpha channel,
 * no source alpha results in a simple copy.
 */
static void _pixbuf_copy_area_alpha(GdkPixbuf *src, gint sx, gint sy,
				    GdkPixbuf *dest, gint dx, gint dy,
				    gint w, gint h, gint apply_alpha,
				    gint alpha_modifier, gint alpha_too)
{
	gint s_alpha;
	gint d_alpha;
	gint sw, sh, srs;
	gint dw, dh, drs;
	guchar *s_pix;
        guchar *d_pix;
	guchar *sp;
        guchar *dp;
	guchar r, g, b, a;
	gint i, j;

	if (!src || !dest) return;

	sw = gdk_pixbuf_get_width(src);
	sh = gdk_pixbuf_get_height(src);

	if (sx < 0 || sx + w > sw) return;
	if (sy < 0 || sy + h > sh) return;

	dw = gdk_pixbuf_get_width(dest);
	dh = gdk_pixbuf_get_height(dest);

	if (dx < 0 || dx + w > dw) return;
	if (dy < 0 || dy + h > dh) return;

	s_alpha = gdk_pixbuf_get_has_alpha(src);
	d_alpha = gdk_pixbuf_get_has_alpha(dest);
	srs = gdk_pixbuf_get_rowstride(src);
	drs = gdk_pixbuf_get_rowstride(dest);
	s_pix = gdk_pixbuf_get_pixels(src);
	d_pix = gdk_pixbuf_get_pixels(dest);

	if (s_alpha && apply_alpha)
		{
	        for (i = 0; i < h; i++)
			{
			sp = s_pix + (sy + i) * srs + sx * 4;
			dp = d_pix + (dy + i) * drs + (dx * (d_alpha ? 4 : 3));
			for (j = 0; j < w; j++)
				{
				r = *(sp++);
				g = *(sp++);
				b = *(sp++);
				a = (*(sp++) * alpha_modifier) >> 8;
				*dp = (r * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (g * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (b * a + *dp * (256-a)) >> 8;
				dp++;
				if (d_alpha) dp++;
				}
			}
		}
	else if (alpha_too && s_alpha && d_alpha)
		{
		/* I wonder if using memcopy would be faster? */
	        for (i = 0; i < h; i++)
			{
			sp = s_pix + (sy + i) * srs + (sx * 4);
			dp = d_pix + (dy + i) * drs + (dx * 4);
			for (j = 0; j < w; j++)
				{
				*(dp++) = *(sp++);	/* r */
				*(dp++) = *(sp++);	/* g */
				*(dp++) = *(sp++);	/* b */
				*(dp++) = *(sp++);	/* a */
				}
			}
		}
	else
		{
		/* I wonder if using memcopy would be faster? */
	        for (i = 0; i < h; i++)
			{
			sp = s_pix + (sy + i) * srs + (sx * (s_alpha ? 4 : 3));
			dp = d_pix + (dy + i) * drs + (dx * (d_alpha ? 4 : 3));
			for (j = 0; j < w; j++)
				{
				*(dp++) = *(sp++);	/* r */
				*(dp++) = *(sp++);	/* g */
				*(dp++) = *(sp++);	/* b */
				if (s_alpha) sp++;	/* a (?) */
				if (d_alpha) dp++;	/* a (?) */
				}
			}
		}
}

/*
 * Copies src to dest, ignoring alpha
 */
void pixbuf_copy_area(GdkPixbuf *src, gint sx, gint sy,
		      GdkPixbuf *dest, gint dx, gint dy,
		      gint w, gint h, gint alpha_too)
{
	_pixbuf_copy_area_alpha(src, sx, sy, dest, dx, dy, w, h, FALSE, 0, alpha_too);
}

/*
 * Copies src to dest, applying alpha
 * no source alpha results in a simple copy.
 */
void pixbuf_copy_area_alpha(GdkPixbuf *src, gint sx, gint sy,
			    GdkPixbuf *dest, gint dx, gint dy,
			    gint w, gint h, gint alpha_modifier)
{
	_pixbuf_copy_area_alpha(src, sx, sy, dest, dx, dy, w, h, TRUE, alpha_modifier, FALSE);
}

/*
 *  like above, but uses a clip region and/or mask
 */
void pixbuf_copy_area_alpha_with_clipping(GdkPixbuf *src, gint sx, gint sy,
					  GdkPixbuf *dest, gint dx, gint dy,
					  gint w, gint h, gint alpha_modifier,
					  gint clip_x, gint clip_y, gint clip_w, gint clip_h, GdkPixbuf *clip_pb)
{
	gint s_alpha;
	gint d_alpha;
	gint sw, sh, srs, s_step;
	gint dw, dh, drs, d_step;
	guchar *s_pix;
	guchar *d_pix;
	guchar *sp;
	guchar *dp;
	guchar *clip_pix;
	gint clip_rs;
	gint cx, cy;
	guchar *cp;
	guchar r, g, b, a;
	gint i, j;
	gint clip_x2, clip_y2;

	if (!src || !dest) return;

	if (clip_pb && gdk_pixbuf_get_has_alpha(clip_pb))
		{
		clip_w = MIN(clip_w, (gdk_pixbuf_get_width(clip_pb)));
		clip_h = MIN(clip_h, (gdk_pixbuf_get_height(clip_pb)));
		clip_pix = gdk_pixbuf_get_pixels(clip_pb);
		clip_rs = gdk_pixbuf_get_rowstride(clip_pb);
		}
	else
		{
		clip_pix = NULL;
		clip_rs = 0;
		}

	sw = gdk_pixbuf_get_width(src);
	sh = gdk_pixbuf_get_height(src);

	dw = gdk_pixbuf_get_width(dest);
	dh = gdk_pixbuf_get_height(dest);

	if (sx < 0 || sx + w > sw) return;
	if (sy < 0 || sy + h > sh) return;

	cx = clip_x;
	cy = clip_y;

	clip_x = MAX(clip_x, dx);
	clip_y = MAX(clip_y, dy);

	clip_w -= clip_x - cx;
	clip_h -= clip_y - cy;

	clip_x2 = MIN((clip_x + clip_w), (dx + w));
	clip_y2 = MIN((clip_y + clip_h), (dy + h));

	if (clip_x > dw || clip_y > dh || clip_x2 < 0 || clip_y2 < 0) return;
	
	if (clip_x < 0) clip_x = 0;
	if (clip_y < 0) clip_y = 0;
	if (clip_x2 >= dw) clip_x2 = dw -1;
	if (clip_y2 >= dh) clip_y2 = dh -1;

	cx = clip_x - cx;
	cy = clip_y - cy;

	sx += clip_x - dx;
	sy += clip_y - dy;
	dx = clip_x;
	dy = clip_y;
	w = clip_x2 - clip_x;
	h = clip_y2 - clip_y;

	if (sx < 0 || sx + w > sw) return;
	if (sy < 0 || sy + h > sh) return;

	s_alpha = gdk_pixbuf_get_has_alpha(src);
	d_alpha = gdk_pixbuf_get_has_alpha(dest);
	srs = gdk_pixbuf_get_rowstride(src);
	drs = gdk_pixbuf_get_rowstride(dest);
	s_pix = gdk_pixbuf_get_pixels(src);
	d_pix = gdk_pixbuf_get_pixels(dest);

	s_step = (s_alpha) ? 4 : 3;
	d_step = (d_alpha) ? 4 : 3;

	cp = 0;

	for (i = 0; i < h; i++)
		{
		sp = s_pix + (sy + i) * srs + sx * s_step;
		dp = d_pix + (dy + i) * drs + (dx * d_step);
		if (clip_pix) cp = clip_pix + (cy + i) * clip_rs + cx * 4;
		for (j = 0; j < w; j++)
			{
			if (!clip_pix || *(cp+3) )
				{
				r = *(sp++);
				g = *(sp++);
				b = *(sp++);
				a = (s_alpha) ? ((*(sp++) * alpha_modifier) >> 8) : 255;
				*dp = (r * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (g * a + *dp * (256-a)) >> 8;
				dp++;
				*dp = (b * a + *dp * (256-a)) >> 8;
				dp++;
				if (d_alpha) dp++;
				}
			else
				{
				sp += s_step;
				dp += d_step;
				}
			cp += 4;
			}
		}
}

/*
 * Copies src to dest, filling area by tiling ot stretching if dest size > src size
 */
static void _pixbuf_copy_fill_alpha(GdkPixbuf *src, gint sx, gint sy, gint sw, gint sh,
				    GdkPixbuf *dest, gint dx, gint dy, gint dw, gint dh,
				    gint stretch, gint apply_alpha, gint alpha_modifier, gint alpha_too,
				    gint offset_x, gint offset_y)
{
	if (sw < 1 || sh < 1 || dw < 1 || dh < 1) return;

	if (sw == dw && sh == dh && offset_x == 0 && offset_y == 0)
		{
		_pixbuf_copy_area_alpha(src, sx, sy, dest, dx, dy, sw, sh,
					apply_alpha, alpha_modifier, alpha_too);
		return;
		}
	if (stretch)
		{
		if (sw < 1 || sh < 1) return;
		if (dw < 1 || dh < 1) return;

		/* uh, is it this hard to figure out the offset? or is pixbuf_scale just plain broken
		 * when offets are not 0 ?
		 * (also note stretch does not support offsets)
		 */

		if (apply_alpha)
			{
			gdk_pixbuf_composite(src, dest, dx, dy, dw, dh,
					 (double)dx - (sx * (dw / sw)), (double)dy - (sy * (dh / sh)),
					 (double) dw / sw, (double) dh / sh, GDK_INTERP_NEAREST,
					 alpha_modifier);
			}
		else
			{
			gdk_pixbuf_scale(src, dest,  dx, dy, dw, dh,
					 (double)dx - (sx * (dw / sw)), (double)dy - (sy * (dh / sh)),
					 (double) dw / sw, (double) dh / sh, GDK_INTERP_NEAREST);
			}
		return;
		}
	else
		{
		gint x, y;

		/* tile */

		if (offset_x != 0) offset_x = offset_x % sw;
		if (offset_y != 0) offset_y = offset_y % sh;

		for (x = dx; x < dx + dw; x += sw)
		    for (y = dy; y < dy + dh; y += sh)
			{
			gint w, h;
			gint ox, oy;

			w = sw;
			h = sh;
			ox = sx;
			oy = sy;
			if (x == dx && offset_x > 0)
				{
				ox += offset_x;
				w -= offset_x;
				}
			if (y == dy && offset_y > 0)
				{
				oy += offset_y;
				h -= offset_y;
				}
			if (x + w > dx + dw) w = dx + dw - x;
			if (y + h > dy + dh) h = dy + dh - y;
			if (w > 0 && h > 0)
				{
				_pixbuf_copy_area_alpha(src, ox, oy, dest, x, y, w, h,
							apply_alpha, alpha_modifier, alpha_too);
				}
			}
		}
}

void pixbuf_copy_fill(GdkPixbuf *src, gint sx, gint sy, gint sw, gint sh,
			   GdkPixbuf *dest, gint dx, gint dy, gint dw, gint dh,
			   gint stretch, gint alpha_too)
{
	_pixbuf_copy_fill_alpha(src, sx, sy, sw, sh,
				dest, dx, dy, dw, dh,
				stretch, FALSE, 0, alpha_too, 0, 0);
}

void pixbuf_copy_fill_alpha(GdkPixbuf *src, gint sx, gint sy, gint sw, gint sh,
			    GdkPixbuf *dest, gint dx, gint dy, gint dw, gint dh,
			    gint stretch, gint alpha_modifier)
{
	_pixbuf_copy_fill_alpha(src, sx, sy, sw, sh,
				dest, dx, dy, dw, dh,
				stretch, TRUE, alpha_modifier, FALSE, 0, 0);
}

void pixbuf_copy_fill_alpha_offset(GdkPixbuf *src, gint sx, gint sy, gint sw, gint sh,
				   GdkPixbuf *dest, gint dx, gint dy, gint dw, gint dh,
				   gint stretch, gint alpha_modifier,
				   gint offset_x, gint offset_y)
{
	_pixbuf_copy_fill_alpha(src, sx, sy, sw, sh,
				dest, dx, dy, dw, dh,
				stretch, TRUE, alpha_modifier, FALSE,
				offset_x, offset_y);
}

static void _pixbuf_copy_fill_border(GdkPixbuf *src, GdkPixbuf *dest,
				     gint dx, gint dy, gint dw, gint dh,
				     gint border_left, gint left_stretch,
				     gint border_right, gint right_stretch,
				     gint border_top, gint top_stretch,
				     gint border_bottom, gint bottom_stretch,
				     gint stretch_center,
				     gint apply_alpha, gint alpha_modifier, gint alpha_too)
{
	gint sw, sh;
	gint n, m;

	if (!src || !dest) return;

	sw = gdk_pixbuf_get_width(src);
	sh = gdk_pixbuf_get_height(src);

	/* corners */
	_pixbuf_copy_area_alpha(src, 0, 0,
				dest, dx, dy,
				border_left, border_top,
				apply_alpha, alpha_modifier, alpha_too);
	_pixbuf_copy_area_alpha(src, sw - border_right, 0,
				dest, dx + dw - border_right, dy,
				border_right, border_top,
				apply_alpha, alpha_modifier, alpha_too);
	_pixbuf_copy_area_alpha(src, 0, sh - border_bottom,
				dest, dx, dy + dh - border_bottom,
				border_left, border_bottom,
				apply_alpha, alpha_modifier, alpha_too);
	_pixbuf_copy_area_alpha(src, sw - border_right, sh - border_bottom,
				dest, dx + dw - border_right, dy + dh - border_bottom,
				border_right, border_bottom,
				apply_alpha, alpha_modifier, alpha_too);

	/* sides */
	n = border_top + border_bottom;	/* vert borders tot. */
	m = border_left + border_right;	/* horz borders tot. */

	_pixbuf_copy_fill_alpha(src, 0, border_top, border_left, sh - n,
				dest, dx, dy + border_top, border_left, dh - n,
				left_stretch,
				apply_alpha, alpha_modifier, alpha_too, 0, 0);
	_pixbuf_copy_fill_alpha(src, sw - border_right, border_top, border_right, sh - n,
				dest, dx + dw - border_right, dy + border_top, border_right, dh - n,
				right_stretch,
				apply_alpha, alpha_modifier, alpha_too, 0, 0);
	_pixbuf_copy_fill_alpha(src, border_left, 0, sw - m, border_top,
				dest, dx + border_left, dy, dw - m, border_top,
				top_stretch,
				apply_alpha, alpha_modifier, alpha_too, 0, 0);
	_pixbuf_copy_fill_alpha(src, border_left, sh - border_bottom, sw - m, border_bottom,
				dest, dx + border_left, dy + dh - border_bottom, dw - m, border_bottom,
				bottom_stretch,
				apply_alpha, alpha_modifier, alpha_too, 0, 0);

	/* center field */
	_pixbuf_copy_fill_alpha(src, border_left, border_top, sw - m, sh - n,
				dest, dx + border_left, dy + border_top, dw - m, dh - n,
				stretch_center,
				apply_alpha, alpha_modifier, alpha_too, 0, 0);
}

void pixbuf_copy_fill_border(GdkPixbuf *src, GdkPixbuf *dest,
			     gint x, gint y, gint width, gint height,
			     gint border_left, gint left_stretch,
			     gint border_right, gint right_stretch,
			     gint border_top, gint top_stretch,
			     gint border_bottom, gint bottom_stretch,
			     gint stretch_center, gint alpha_too)
{
	_pixbuf_copy_fill_border(src, dest,
				 x, y, width, height,
				 border_left, left_stretch,
				 border_right, right_stretch,
				 border_top, top_stretch,
				 border_bottom, bottom_stretch,
				 stretch_center,
				 FALSE, 0, alpha_too);
}

void pixbuf_copy_fill_border_alpha(GdkPixbuf *src, GdkPixbuf *dest,
				   gint x, gint y, gint width, gint height,
				   gint border_left, gint left_stretch,
				   gint border_right, gint right_stretch,
				   gint border_top, gint top_stretch,
				   gint border_bottom, gint bottom_stretch,
				   gint stretch_center,
				   gint alpha_modifier)
{
	_pixbuf_copy_fill_border(src, dest,
				 x, y, width, height,
				 border_left, left_stretch,
				 border_right, right_stretch,
				 border_top, top_stretch,
				 border_bottom, bottom_stretch,
				 stretch_center,
				 TRUE, alpha_modifier, FALSE);
}

/*
 * Copies src channel to dest.
 * channel is 0, 1, 2, or 3 for
 * red, green, blue, alpha, respectively
 * obviously, if channel 3 is given and both do not have alpha, nothing happens.
 */
void pixbuf_copy_channel(GdkPixbuf *src, gint sx, gint sy,
			 GdkPixbuf *dest, gint dx, gint dy,
			 gint w, gint h, guint channel)
{
	gint s_alpha;
	gint d_alpha;
	gint sw, sh, srs;
	gint dw, dh, drs;
	guchar *s_pix;
        guchar *d_pix;
	guchar *sp;
        guchar *dp;
	gint i, j;
	gint s_step;
	gint d_step;

	if (!src || !dest) return;

	sw = gdk_pixbuf_get_width(src);
	sh = gdk_pixbuf_get_height(src);

	if (sx < 0 || sx + w > sw) return;
	if (sy < 0 || sy + h > sh) return;

	dw = gdk_pixbuf_get_width(dest);
	dh = gdk_pixbuf_get_height(dest);

	if (dx < 0 || dx + w > dw) return;
	if (dy < 0 || dy + h > dh) return;

	s_alpha = gdk_pixbuf_get_has_alpha(src);
	d_alpha = gdk_pixbuf_get_has_alpha(dest);
	srs = gdk_pixbuf_get_rowstride(src);
	drs = gdk_pixbuf_get_rowstride(dest);
	s_pix = gdk_pixbuf_get_pixels(src);
	d_pix = gdk_pixbuf_get_pixels(dest);

	if (channel < 0 ||
	    (channel > 2 && (!s_alpha || !d_alpha)) ) return;
	if (channel > 2) channel = 3;

	s_step = s_alpha ? 4 : 3;
	d_step = d_alpha ? 4 : 3;

        for (i = 0; i < h; i++)
		{
		sp = s_pix + (sy + i) * srs + (sx * s_step) + channel;
		dp = d_pix + (dy + i) * drs + (dx * d_step) + channel;
		for (j = 0; j < w; j++)
			{
			*dp = *sp;
			dp += d_step;
			sp += s_step;
			}
		}
	
}

static void buf_pixel_get(guchar *buf, gint x, gint y, guint8 *r, guint8 *g, guint8 *b, guint8 *a,
			  gint has_alpha, gint rowstride)
{
	guchar *p;

	p = buf + (y * rowstride) + (x * (has_alpha ? 4 : 3));

	*r = *(p++);
	*g = *(p++);
	*b = *(p++);
	*a = has_alpha ? *p : 255;
}

static void buf_pixel_set(guchar *buf, gint x, gint y, guint8 r, guint8 g, guint8 b, guint8 a,
			  gint has_alpha, gint rowstride)
{
	guchar *p;

	p = buf + (y * rowstride) + (x * (has_alpha ? 4 : 3));

	*p = (r * a + *p * (256 - a)) >> 8;
	p++;
	*p = (g * a + *p * (256 - a)) >> 8;
	p++;
	*p = (b * a + *p * (256 - a)) >> 8;
}

/*
 *---------------------------------------------------------------------------
 * The rotation code was borrowed from the afterstep clock (asclock), rot.c
 * and it has been adapted a bit.
 * Update 3/11/2000: Updated to work with GdkPixbuf buffers, and includes
 * source alpha channel support. Starting to look nothing like original code.
 * Only the pixel x, y location lines have remained the same.
 * Update 5/15/2000: Remove all division.. now use bitshifts, optimizations,
 * the with_clipping version no longer resembles the original at all.
 *
 * Still slower than it should be, IMHO.
 *---------------------------------------------------------------------------
 */

#define PI 3.14159

/*
 * Copies src to dest rotated, applying source alpha,
 * rotation is in degrees.
 * this version only works well if the source's center offset pixel is not
 * transparent (alpha value = 0) and is a basic shape.
 */
void pixbuf_copy_rotate_alpha(GdkPixbuf *src, gint offset_x, gint offset_y,
			      GdkPixbuf *dest, gint center_x, gint center_y,
			      double theta)
{
	guchar *s_buf;
	gint s_w, s_h;
	gint s_alpha, s_rs;
	gint s_step;

	guchar *d_buf;
	gint d_w, d_h;
	gint d_alpha, d_rs;

	int done, miss, missed=0;
	int goup;
	int goleft;

	int x, y;
	double xoff, yoff;

	s_buf = gdk_pixbuf_get_pixels(src);
	s_alpha = gdk_pixbuf_get_has_alpha(src);
	s_rs = gdk_pixbuf_get_rowstride(src);
	s_w = gdk_pixbuf_get_width(src);
	s_h = gdk_pixbuf_get_height(src);
	s_step = s_alpha ? 4 : 3;

	d_buf = gdk_pixbuf_get_pixels(dest);
	d_alpha = gdk_pixbuf_get_has_alpha(dest);
	d_rs = gdk_pixbuf_get_rowstride(dest);
	d_w = gdk_pixbuf_get_width(dest);
	d_h = gdk_pixbuf_get_height(dest);

	theta = - theta * PI / 180.0;
  
	xoff = center_x;
	yoff = center_y;

	done = FALSE;
	miss = FALSE;

	goup=TRUE;
	goleft=TRUE;
	x = center_x;
	y = center_y;
	while(!done) 
		{
		double ex, ey;
		int oldx, oldy;

		/* transform into old coordinate system */
		ex = offset_x-0.25 + ((x-xoff) * cos(theta) - (y-yoff) * sin(theta));
		ey = offset_y-0.25 + ((x-xoff) * sin(theta) + (y-yoff) * cos(theta));
		oldx = floor(ex);
		oldy = floor(ey);

		if(oldx >= -1 && oldx < s_w && oldy >= -1 && oldy < s_h)
			{
			if(x >= 0 && y >= 0 && x < d_w && y < d_h)
				{
				guchar tr, tg, tb, ta;
				guchar ag, bg, cg, dg;
				guchar *pp;

				/* calculate the strength of the according pixels */
				dg = (guchar)((ex - oldx) * (ey - oldy) * 255.0);
				cg = (guchar)((oldx + 1 - ex) * (ey - oldy) * 255.0);
				bg = (guchar)((ex - oldx) * (oldy + 1 - ey) * 255.0);
				ag = (guchar)((oldx + 1 - ex) * (oldy + 1 - ey) * 255.0);
	  
				tr = 0;
				tg = 0;
				tb = 0;
				ta = 0;

				pp = s_buf + (oldy * s_rs) + (oldx * s_step);

				if(oldx >= 0 && oldy >= 0)
					{
					guchar *pw;
					guchar a;

					pw = pp;
					a = s_alpha ? *(pw + 3) : 255;
					tr += (ag * *pw * a >> 16);
					pw++;
					tg += (ag * *pw * a >> 16);
					pw++;
					tb += (ag * *pw * a >> 16);
					ta += (ag * a >> 8);
					}
				if(oldx + 1 < s_w && oldy >= 0)
					{
					guchar *pw;
					guchar a;

					pw = pp + s_step;
					a = s_alpha ? *(pw + 3) : 255;
					tr += (bg * *pw * a >> 16);
					pw++;
					tg += (bg * *pw * a >> 16);
					pw++;
					tb += (bg * *pw * a >> 16);
					ta += (bg * a >> 8);
					}
				if(oldx >= 0 && oldy + 1 < s_h)
					{
					guchar *pw;
					guchar a;

					pw = pp + s_rs;
					a = s_alpha ? *(pw + 3) : 255;
					tr += (cg * *pw * a >> 16);
					pw++;
					tg += (cg * *pw * a >> 16);
					pw++;
					tb += (cg * *pw * a >> 16);
					ta += (cg * a >> 8);
					}
				if(oldx + 1 < s_w && oldy + 1 < s_h)
					{
					guchar *pw;
					guchar a;

					pw = pp + s_rs + s_step;
					a = s_alpha ? *(pw + 3) : 255;
					tr += (dg * *pw * a >> 16);
					pw++;
					tg += (dg * *pw * a >> 16);
					pw++;
					tb += (dg * *pw * a >> 16);
					ta += (dg * a >> 8);
					}
				buf_pixel_set(d_buf, x, y, tr, tg, tb, ta, d_alpha, d_rs);
				}
			missed = 0;
			miss = FALSE;
			}
		else
			{
			miss = TRUE;
			missed++;
			if(missed >= 15)
				{
				if(goup)
					{
					goup=FALSE;
					goleft=FALSE;
					x = center_x;
					y = center_y;
					}
				else
					{
					done = TRUE;
					}
				}
			}
		if(miss && (missed == 1))
			{
			goleft = !goleft;
			if(goup)
				y--;
			else
				y++;

			}
		else
			{
			if(goleft)
				x--;
			else
				x++;
			}
		}
}


/*
 * Copies src to dest rotated, applying source alpha,
 * rotation is in degrees.
 * the clipping is the part of dest to apply src, this may be
 * a little slower than above, but has less 'eccentricities'.
 */
void pixbuf_copy_rotate_alpha_with_clipping(GdkPixbuf *src, gint offset_x, gint offset_y,
					    GdkPixbuf *dest, gint center_x, gint center_y,
					    double theta,
					    gint clip_x, gint clip_y, gint clip_w, gint clip_h,
					    GdkPixbuf *clip_pb)
{
	guchar *s_buf;
	gint s_w, s_h;
	gint s_alpha, s_rs;
	gint s_step;

	guchar *d_buf;
	gint d_w, d_h;
	gint d_alpha, d_rs;

	guchar *clip_buf;
	guchar *clip_pp;
	gint clip_rs;

	gint x, y;
	double xoff, yoff;

	gint clip_x2;
	gint clip_y2;

	s_buf = gdk_pixbuf_get_pixels(src);
	s_alpha = gdk_pixbuf_get_has_alpha(src);
	s_rs = gdk_pixbuf_get_rowstride(src);
	s_w = gdk_pixbuf_get_width(src);
	s_h = gdk_pixbuf_get_height(src);
	s_step = s_alpha ? 4 : 3;

	d_buf = gdk_pixbuf_get_pixels(dest);
	d_alpha = gdk_pixbuf_get_has_alpha(dest);
	d_rs = gdk_pixbuf_get_rowstride(dest);
	d_w = gdk_pixbuf_get_width(dest);
	d_h = gdk_pixbuf_get_height(dest);

	theta = - theta * PI / 180.0;
  
	xoff = center_x;
	yoff = center_y;

	clip_x2 = clip_x + clip_w;
	clip_y2 = clip_y + clip_h;

	if (clip_pb && gdk_pixbuf_get_has_alpha(clip_pb))
		{
		clip_buf = gdk_pixbuf_get_pixels(clip_pb);
		clip_rs = gdk_pixbuf_get_rowstride(clip_pb);
		}
	else
		{
		clip_buf = NULL;
		clip_rs = 0;
		}

	clip_pp = NULL;

	if (clip_x < 0 || clip_x >= d_w) return;
	if (clip_y < 0 || clip_y >= d_h) return;
	if (clip_x2 >= d_w) clip_x2 = d_w -1;
	if (clip_y2 >= d_h) clip_y2 = d_h -1;

	for (y = clip_y; y < clip_y2; y++)
		{
		if (clip_buf) clip_pp = clip_buf + ((y - clip_y) * clip_rs) + 3;
		for (x = clip_x; x < clip_x2; x++)
			{
			double ex, ey;
			int oldx, oldy;

			/* transform into old coordinate system */
			ex = offset_x-0.25 + ((x-xoff) * cos(theta) - (y-yoff) * sin(theta));
			ey = offset_y-0.25 + ((x-xoff) * sin(theta) + (y-yoff) * cos(theta));
			oldx = floor(ex);
			oldy = floor(ey);

			if(oldx >= -1 && oldx < s_w && oldy >= -1 && oldy < s_h &&
			   (!clip_pp || *(clip_pp + ((x - clip_x) * 4))) )
				{
				guchar tr, tg, tb, ta;
				guchar ag, bg, cg, dg;
				guchar *pp;

				/* calculate the strength of the according pixels */
				dg = (guchar)((ex - oldx) * (ey - oldy) * 255.0);
				cg = (guchar)((oldx + 1 - ex) * (ey - oldy) * 255.0);
				bg = (guchar)((ex - oldx) * (oldy + 1 - ey) * 255.0);
				ag = (guchar)((oldx + 1 - ex) * (oldy + 1 - ey) * 255.0);
	  
				tr = 0;
				tg = 0;
				tb = 0;
				ta = 0;

				pp = s_buf + (oldy * s_rs) + (oldx * s_step);

				if(oldx >= 0 && oldy >= 0)
					{
					guchar *pw;
					guchar a;

					pw = pp;
					a = s_alpha ? *(pw + 3) : 255;
					tr += (ag * *pw * a >> 16);
					pw++;
					tg += (ag * *pw * a >> 16);
					pw++;
					tb += (ag * *pw * a >> 16);
					ta += (ag * a >> 8);
					}
				if(oldx + 1 < s_w && oldy >= 0)
					{
					guchar *pw;
					guchar a;

					pw = pp + s_step;
					a = s_alpha ? *(pw + 3) : 255;
					tr += (bg * *pw * a >> 16);
					pw++;
					tg += (bg * *pw * a >> 16);
					pw++;
					tb += (bg * *pw * a >> 16);
					ta += (bg * a >> 8);
					}
				if(oldx >= 0 && oldy + 1 < s_h)
					{
					guchar *pw;
					guchar a;

					pw = pp + s_rs;
					a = s_alpha ? *(pw + 3) : 255;
					tr += (cg * *pw * a >> 16);
					pw++;
					tg += (cg * *pw * a >> 16);
					pw++;
					tb += (cg * *pw * a >> 16);
					ta += (cg * a >> 8);
					}
				if(oldx + 1 < s_w && oldy + 1 < s_h)
					{
					guchar *pw;
					guchar a;

					pw = pp + s_rs + s_step;
					a = s_alpha ? *(pw + 3) : 255;
					tr += (dg * *pw * a >> 16);
					pw++;
					tg += (dg * *pw * a >> 16);
					pw++;
					tb += (dg * *pw * a >> 16);
					ta += (dg * a >> 8);
					}
				buf_pixel_set(d_buf, x, y, tr, tg, tb, ta, d_alpha, d_rs);
				}
			}
		}
}

/*
 * These rotate in increments of 90 degrees, and are analogous to the non
 * rotated pixbuf_copy_area{_alpha} utils.
 */

static void _pixbuf_copy_area_alpha_rotate_90(GdkPixbuf *src, gint sx, gint sy,
					      GdkPixbuf *dest, gint dx, gint dy,
					      gint w, gint h, gint apply_alpha,
					      gint alpha_modifier, gint alpha_too, gint rotation)
{
	gint s_alpha, s_step;
	gint d_alpha, d_step;
	gint sw, sh, srs;
	gint dw, dh, drs;
	guchar *s_pix;
	guchar *d_pix;
	guchar *sp;
	guchar *dp;
	gint i, j;

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

	s_step = s_alpha ? 4 : 3;
	d_step = d_alpha ? 4 : 3;

	if (s_alpha && apply_alpha)
		{
		guchar a;
		switch (rotation)
			{
			case 90:
				for (i = 0; i < w; i++)
					{
					sp = s_pix + sy * srs + (sx + i) * s_step;
					dp = d_pix + (dy + i) * drs + (dx + h - 1) * d_step;
					for (j = 0; j < h; j++)
						{
						a = (*(sp+3) * alpha_modifier) >> 8;
						*dp = (*sp * a + *dp * (256-a)) >> 8;
						*(dp+1) = (*(sp+1) * a + *(dp+1) * (256-a)) >> 8;
						*(dp+2) = (*(sp+2) * a + *(dp+2) * (256-a)) >> 8;
						sp += srs;
						dp -= d_step;
						}
					}
				break;
			case 180:
				for (i = 0; i < h; i++)
					{
					sp = s_pix + (sy + i) * srs + sx * s_step;
					dp = d_pix + (dy + h - i - 1) * drs + (dx + w - 1) * d_step;
					for (j = 0; j < w; j++)
						{
						a = (*(sp+3) * alpha_modifier) >> 8;
						*dp = (*sp * a + *dp * (256-a)) >> 8;
						*(dp+1) = (*(sp+1) * a + *(dp+1) * (256-a)) >> 8;
						*(dp+2) = (*(sp+2) * a + *(dp+2) * (256-a)) >> 8;
						sp += s_step;
						dp -= d_step;
						}
					}
				break;
			case 270:
				for (i = 0; i < w; i++)
					{
					sp = s_pix + sy * srs + (sx + i) * s_step;
					dp = d_pix + (dy + w - i - 1) * drs + dx * d_step;
					for (j = 0; j < h; j++)
						{
						a = (*(sp+3) * alpha_modifier) >> 8;
						*dp = (*sp * a + *dp * (256-a)) >> 8;
						*(dp+1) = (*(sp+1) * a + *(dp+1) * (256-a)) >> 8;
						*(dp+2) = (*(sp+2) * a + *(dp+2) * (256-a)) >> 8;
						sp += srs;
						dp += d_step;
						}
					}
				break;
			}
		}
	else
		{
		alpha_too = (alpha_too && s_alpha && d_alpha);

		switch (rotation)
			{
			case 90:
				for (i = 0; i < w; i++)
					{
					sp = s_pix + sy * srs + (sx + i) * s_step;
					dp = d_pix + (dy + i) * drs + (dx + h - 1) * d_step;
					for (j = 0; j < h; j++)
						{
						*(dp) = *(sp);		/* r */
						*(dp+1) = *(sp+1);	/* g */
						*(dp+2) = *(sp+2);	/* b */
						if (alpha_too)		/* a ? */
							{
							*(dp+3) = *(sp+3);
							}
						sp += srs;
						dp -= d_step;
						}
					}
				break;
			case 180:
				for (i = 0; i < h; i++)
					{
					sp = s_pix + (sy + i) * srs + sx * s_step;
					dp = d_pix + (dy + h - i - 1) * drs + (dx + w - 1) * d_step;
					for (j = 0; j < w; j++)
						{
						*(dp) = *(sp);		/* r */
						*(dp+1) = *(sp+1);	/* g */
						*(dp+2) = *(sp+2);	/* b */
						if (alpha_too)		/* a ? */
							{
							*(dp+3) = *(sp+3);
							}
						sp += s_step;
						dp -= d_step;
						}
					}
				break;
			case 270:
				for (i = 0; i < w; i++)
					{
					sp = s_pix + sy * srs + (sx + i) * s_step;
					dp = d_pix + (dy + w - i - 1) * drs + dx * d_step;
					for (j = 0; j < h; j++)
						{
						*(dp) = *(sp);		/* r */
						*(dp+1) = *(sp+1);	/* g */
						*(dp+2) = *(sp+2);	/* b */
						if (alpha_too)		/* a ? */
							{
							*(dp+3) = *(sp+3);
							}
						sp += srs;
						dp += d_step;
						}
					}
				break;
			}
		}
}

void pixbuf_copy_area_rotate_90(GdkPixbuf *src, gint sx, gint sy,
				GdkPixbuf *dest, gint dx, gint dy,
				gint w, gint h, gint alpha_too, gint rotation)
{
	_pixbuf_copy_area_alpha_rotate_90(src, sx, sy, dest, dx, dy,
					  w, h, FALSE, 0, alpha_too, rotation);
}

void pixbuf_copy_area_rotate_90_alpha(GdkPixbuf *src, gint sx, gint sy,
				      GdkPixbuf *dest, gint dx, gint dy,
				      gint w, gint h, gint alpha_modifier, gint rotation)
{
	_pixbuf_copy_area_alpha_rotate_90(src, sx, sy, dest, dx, dy,
					  w, h, TRUE, alpha_modifier, FALSE, rotation);
}


/*
 * Fills region of pixbuf at x,y over w,h
 * with colors red (r), green (g), blue (b)
 * applying alpha (a), use a=255 for solid.
 */
void pixbuf_draw_rect_fill(GdkPixbuf *pb,
			   gint x, gint y, gint w, gint h,
			   gint r, gint g, gint b, gint a)
{
	gint p_alpha;
	gint pw, ph, prs;
	guchar *p_pix;
	guchar *pp;
	gint i, j;

	if (!pb) return;

	pw = gdk_pixbuf_get_width(pb);
	ph = gdk_pixbuf_get_height(pb);

	if (x < 0 || x + w > pw) return;
	if (y < 0 || y + h > ph) return;

	p_alpha = gdk_pixbuf_get_has_alpha(pb);
	prs = gdk_pixbuf_get_rowstride(pb);
	p_pix = gdk_pixbuf_get_pixels(pb);

        for (i = 0; i < h; i++)
		{
		pp = p_pix + (y + i) * prs + (x * (p_alpha ? 4 : 3));
		for (j = 0; j < w; j++)
			{
			*pp = (r * a + *pp * (256-a)) >> 8;
			pp++;
			*pp = (g * a + *pp * (256-a)) >> 8;
			pp++;
			*pp = (b * a + *pp * (256-a)) >> 8;
			pp++;
			if (p_alpha) pp++;
			}
		}
}

void pixbuf_draw_rect(GdkPixbuf *pb,
		      gint x, gint y, gint w, gint h,
		      gint r, gint g, gint b, gint a,
		      gint left, gint right, gint top, gint bottom)
{
	pixbuf_draw_rect_fill(pb, x + left, y, w - left - right, top,
			      r, g, b ,a);
	pixbuf_draw_rect_fill(pb, x + w - right, y, right, h,
			      r, g, b ,a);
	pixbuf_draw_rect_fill(pb, x + left, y + h - bottom, w - left - right, bottom,
			      r, g, b ,a);
	pixbuf_draw_rect_fill(pb, x, y, left, h,
			      r, g, b ,a);
}

guchar pixel_shift(gint p, gint saturation, gint adjustment, gint strength)
{
	gint t;

	if (adjustment >= 128)
		{
		t = CLAMP(saturation + saturation * (adjustment - 128) / 127, 0, 255);
		}
	else
		{
		t = saturation * adjustment / 128;
		}
	p = (p * (255-strength) + t * strength) / 255;
                                                                                                                        
	return (guchar)p;
}

/*
 * Shifts color hue in region, attempting to maintain intensity.
 */
void pixbuf_draw_rect_shift(GdkPixbuf *pb,
			    gint x, gint y, gint w, gint h,
			    gint r, gint g, gint b, gint strength)
{
	gint p_alpha;
	gint pw, ph, prs;
	guchar *p_pix;
	guchar *pp;
	gint i, j;

	if (!pb) return;

	pw = gdk_pixbuf_get_width(pb);
	ph = gdk_pixbuf_get_height(pb);

	if (x < 0 || x + w > pw) return;
	if (y < 0 || y + h > ph) return;

	p_alpha = gdk_pixbuf_get_has_alpha(pb);
	prs = gdk_pixbuf_get_rowstride(pb);
	p_pix = gdk_pixbuf_get_pixels(pb);

	for (i = 0; i < h; i++)
		{
		pp = p_pix + (y + i) * prs + (x * (p_alpha ? 4 : 3));
		for (j = 0; j < w; j++)
			{
			gint s;

			s = (pp[0] + pp[1] + pp[2]) / 3;

			*pp = pixel_shift(*pp, s, r, strength);
                        pp++;
			*pp = pixel_shift(*pp, s, g, strength);
                        pp++;
			*pp = pixel_shift(*pp, s, b, strength);
			pp++;
			if (p_alpha) pp++;
			}
		}
}

static guchar pixel_adjust(gint p, gint a)
{
	p += a;
	if (p < 0) p = 0;
	if (p > 255) p = 255;

	return (guchar)p;
}

/*
 * Adjust the alpha of region of pixbuf at x,y over w,h
 * by amount a, use negative to decrease alpha
 */
void pixbuf_alpha_adjust(GdkPixbuf *pb, gint x, gint y, gint w, gint h, gint a)
{
	gint pw, ph, prs;
	guchar *p_pix;
	guchar *pp;
	gint i, j;

	if (!pb || !gdk_pixbuf_get_has_alpha(pb)) return;

	pw = gdk_pixbuf_get_width(pb);
	ph = gdk_pixbuf_get_height(pb);

	if (x < 0 || x + w > pw) return;
	if (y < 0 || y + h > ph) return;

	prs = gdk_pixbuf_get_rowstride(pb);
	p_pix = gdk_pixbuf_get_pixels(pb);

        for (i = 0; i < h; i++)
		{
		pp = p_pix + (y + i) * prs + (x * 4);
		for (j = 0; j < w; j++)
			{
			pp++;
			pp++;
			pp++;
			*pp = pixel_adjust(*pp, a);
			pp++;
			}
		}
}

/* force alpha to 0 or 255, cutoff is level threshhold */
void pixbuf_alpha_force_to_bw(GdkPixbuf *pb, guint8 threshhold)
{
	gint pw, ph, prs;
	guchar *p_pix;
	guchar *pp;
	gint i, j;

	if (!pb || !gdk_pixbuf_get_has_alpha(pb)) return;

	pw = gdk_pixbuf_get_width(pb);
	ph = gdk_pixbuf_get_height(pb);

	prs = gdk_pixbuf_get_rowstride(pb);
	p_pix = gdk_pixbuf_get_pixels(pb);

        for (i = 0; i < ph; i++)
		{
		pp = p_pix + i * prs + 3;
		for (j = 0; j < pw; j++)
			{
			*pp = (*pp < threshhold) ? 0 : 255;
			pp += 4;
			}
		}
}

void pixbuf_pixel_get(GdkPixbuf *pb, gint x, gint y, gint *r, gint *g, gint *b, gint *a)
{
	guint8 nr, ng, nb, na;

	if (x < 0 || x >= gdk_pixbuf_get_width(pb) ||
	    y < 0 || y >= gdk_pixbuf_get_height(pb))
		{
		nr = ng = nb = na = 0;
		}
	else
		{
		guchar *buf;
		gint has_alpha;
		gint rowstride;
		buf = gdk_pixbuf_get_pixels(pb);
		has_alpha = gdk_pixbuf_get_has_alpha(pb);
		rowstride = gdk_pixbuf_get_rowstride(pb);
		buf_pixel_get(buf, x, y, &nr, &ng, &nb, &na, has_alpha, rowstride);
		}

	if (r) *r = (gint)nr;
	if (g) *g = (gint)ng;
	if (b) *b = (gint)nb;
	if (a) *a = (gint)na;
}

gint pixbuf_pixel_is_visible(GdkPixbuf *pb, gint x, gint y)
{
	guint8 nr, ng, nb, na;

	if (x < 0 || x >= gdk_pixbuf_get_width(pb) ||
	    y < 0 || y >= gdk_pixbuf_get_height(pb)) return FALSE;

	if (!gdk_pixbuf_get_has_alpha(pb)) return FALSE;

	buf_pixel_get(gdk_pixbuf_get_pixels(pb), x, y, &nr, &ng, &nb, &na,
		      TRUE, gdk_pixbuf_get_rowstride(pb));

	return ( (na > 0) );
}

static void pixbuf_copy_point_real(GdkPixbuf *src, gint sx, gint sy,
				   GdkPixbuf *dest, gint dx, gint dy,
				   gint use_alpha, gint at_alpha, gint alpha)
{
	guint8 r, g, b, a;

	if (sx < 0 || sx >= gdk_pixbuf_get_width(src)) return;
	if (sy < 0 || sy >= gdk_pixbuf_get_height(src)) return;

	if (dx < 0 || dx >= gdk_pixbuf_get_width(dest)) return;
	if (dy < 0 || dy >= gdk_pixbuf_get_height(dest)) return;

	buf_pixel_get(gdk_pixbuf_get_pixels(src), sx, sy, &r, &g, &b, &a, 
		      gdk_pixbuf_get_has_alpha(src), gdk_pixbuf_get_rowstride(src));

	if (at_alpha)
		{
		buf_pixel_set(gdk_pixbuf_get_pixels(dest), dx, dy, r, g, b, alpha * a >> 8,
			      gdk_pixbuf_get_has_alpha(dest), gdk_pixbuf_get_rowstride(dest));
		}
	else
		{
		buf_pixel_set(gdk_pixbuf_get_pixels(dest), dx, dy, r, g, b, use_alpha ? a : 255,
			      gdk_pixbuf_get_has_alpha(dest), gdk_pixbuf_get_rowstride(dest));
		}
}

void pixbuf_copy_point(GdkPixbuf *src, gint sx, gint sy,
		       GdkPixbuf *dest, gint dx, gint dy, gint use_alpha)
{
	pixbuf_copy_point_real(src, sx, sy, dest, dx, dy, use_alpha, FALSE, 0);
}

void pixbuf_copy_point_at_alpha(GdkPixbuf *src, gint sx, gint sy,
				GdkPixbuf *dest, gint dx, gint dy, gint alpha)
{
	pixbuf_copy_point_real(src, sx, sy, dest, dx, dy, FALSE, TRUE, alpha);
}

void pixbuf_copy_line(GdkPixbuf *src, gint sx1, gint sy1, gint sx2, gint sy2,
		      GdkPixbuf *dest, gint dx, gint dy, gint use_alpha)
{
	guint8 r, g, b, a;
	guchar *s_pix, *d_pix;
	gint s_alpha, d_alpha;
	gint srs, drs;

	gint xd, yd;

	gfloat xstep, ystep;

	gfloat i, j;
	gint n, nt;

	if (sx1 == sx2 && sy1 == sy2);

	if (sx1 < 0 || sx1 >= gdk_pixbuf_get_width(src)) return;
	if (sy1 < 0 || sy1 >= gdk_pixbuf_get_height(src)) return;
	if (sx2 < 0 || sx2 >= gdk_pixbuf_get_width(src)) return;
	if (sy2 < 0 || sy2 >= gdk_pixbuf_get_height(src)) return;

	if (dx < 0 || dx >= gdk_pixbuf_get_width(dest)) return;
	if (dy < 0 || dy >= gdk_pixbuf_get_height(dest)) return;

	xd = sx2 - sx1;
	yd = sy2 - sy1;

	if (dx + xd < 0 || dx + xd >= gdk_pixbuf_get_width(dest)) return;
	if (dy + yd < 0 || dy + yd >= gdk_pixbuf_get_height(dest)) return;

	s_pix = gdk_pixbuf_get_pixels(src);
	srs = gdk_pixbuf_get_rowstride(src);
	s_alpha = gdk_pixbuf_get_has_alpha(src);

	d_pix = gdk_pixbuf_get_pixels(dest);
	drs = gdk_pixbuf_get_rowstride(dest);
	d_alpha = gdk_pixbuf_get_has_alpha(dest);

	nt = sqrt(xd * xd + yd * yd);
	xstep = (float)xd / nt;
	ystep = (float)yd / nt;

	i = j = 0.0;
	for (n = 0; n < nt; n++)
		{
		buf_pixel_get(s_pix, sx1 + (gint)j, sy1 + (gint)i, &r, &g, &b, &a, s_alpha, srs);
		buf_pixel_set(d_pix, dx + (gint)j, dy + (gint)i, r, g, b, use_alpha ? a : 255, d_alpha, drs);
		i += ystep;
		j += xstep;
		}
}
