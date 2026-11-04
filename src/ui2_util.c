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

#include "ui2_includes.h"
#include "ui2_typedefs.h"
#include "ui2_util.h"

#include "ui2_main.h"
#include "ui_fileops.h"
#include "ui_pixbuf_ops.h"

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>


/*
 * try 0.5 or 3 ;)
 * for non-whole numbers change GDK_INTERP_NEAREST to GDK_INTERP_BILINEAR below
 */
#define DOUBLE_MULTIPLIER 2


void util_size(gint *n)
{
	if (slik_scale && slik_scale_value > 0.1)
		{
		if (*n > 0)
			{
			*n = (gint)((gfloat)*n * slik_scale_value);
			if (*n < 1) *n = 1;
			}
		}
	else if (slik_double_size)
		{
		*n *= DOUBLE_MULTIPLIER;
		}
}

void util_color(guint8 *r, guint8 *g, guint8 *b, guint8 *a)
{
	if (slik_colorshift_on && r && g && b)
		{
		gint saturation = (*r + *g + *b) / 3;

		*r = pixel_shift(*r, saturation, slik_colorshift_r, slik_colorshift_a);
		*g = pixel_shift(*g, saturation, slik_colorshift_g, slik_colorshift_a);
		*b = pixel_shift(*b, saturation, slik_colorshift_b, slik_colorshift_a);
		}
	if (slik_transparency_force && a)
		{
		gint t;

		t = (gint)*a;
		t = (t * 3 + t * slik_transparency_force_a / 255) / 4;
		*a = t;
		}
}

GdkPixbuf *util_pixbuf_new_from_file(const gchar *path)
{
	GdkPixbuf *pixbuf;
	gchar *pathl;

	pathl = path_from_utf8(path);
	pixbuf = gdk_pixbuf_new_from_file(pathl, NULL);
	g_free(pathl);

	return pixbuf;
}

GdkPixbuf *util_size_pixbuf(GdkPixbuf *pb, gint apply_adjustments)
{
	if (slik_scale && slik_scale_value > 0.1)
		{
		gint w, h;
		GdkPixbuf *tmp;

		tmp = pb;
		w = gdk_pixbuf_get_width(tmp);
		h = gdk_pixbuf_get_height(tmp);
		w = (gint)((gfloat)w * slik_scale_value);
		if (w < 1) w = 1;
		h = (gint)((gfloat)h * slik_scale_value);
		if (h < 1) h = 1;

		pb = gdk_pixbuf_scale_simple(tmp, w, h, GDK_INTERP_NEAREST);

		gdk_pixbuf_unref(tmp);
		}
	else if (pb && slik_double_size)
		{
		GdkPixbuf *tmp = pb;
		pb = gdk_pixbuf_scale_simple(tmp, gdk_pixbuf_get_width(tmp) * DOUBLE_MULTIPLIER,
					     gdk_pixbuf_get_height(tmp) * DOUBLE_MULTIPLIER,
					     GDK_INTERP_NEAREST);
		gdk_pixbuf_unref(tmp);
		}

	if (pb && apply_adjustments && slik_transparency_force)
		{
		gint t;
		if (!gdk_pixbuf_get_has_alpha(pb))
			{
			GdkPixbuf *tmp = pb;
			pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
					    gdk_pixbuf_get_width(pb), gdk_pixbuf_get_height(pb));
			pixbuf_copy_area(tmp, 0, 0, pb, 0, 0,
					 gdk_pixbuf_get_width(pb), gdk_pixbuf_get_height(pb), FALSE);
			pixbuf_alpha_adjust(pb, 0, 0, gdk_pixbuf_get_width(pb), gdk_pixbuf_get_height(pb), 255);
			gdk_pixbuf_unref(tmp);
			}
		t = (gint)((float)-64.0 * (255 - slik_transparency_force_a) / 255);
		pixbuf_alpha_adjust(pb, 0, 0, gdk_pixbuf_get_width(pb), gdk_pixbuf_get_height(pb), t);
		}

	if (pb && apply_adjustments && slik_colorshift_on)
		{
		pixbuf_draw_rect_shift(pb, 0, 0, gdk_pixbuf_get_width(pb), gdk_pixbuf_get_height(pb),
				       slik_colorshift_r, slik_colorshift_g, slik_colorshift_b, slik_colorshift_a);
		}

	return pb;
}

gint util_clip_region(gint x, gint y, gint w, gint h,
		      gint clip_x, gint clip_y, gint clip_w, gint clip_h,
		      gint *rx, gint *ry, gint *rw, gint *rh)
{
	if (clip_x + clip_w < x ||
	    clip_x > x + w ||
	    clip_y + clip_h < y ||
	    clip_y > y + h)
		{
		return FALSE;
		}

	*rx = MAX(x, clip_x);
	*rw = MIN((x + w), (clip_x + clip_w)) - *rx;

	*ry = MAX(y, clip_y);
	*rh = MIN((y + h), (clip_y + clip_h)) - *ry;

	return TRUE;
}

void util_pixbuf_copy_border_clipped(GdkPixbuf *src,
				     gint border_left, gint border_right,
				     gint border_top, gint border_bottom,
				     GdkPixbuf *dest, gint x, gint y, gint w, gint h,
				     gint clip_x, gint clip_y, gint clip_w, gint clip_h,
				     gint alpha)
{
	gint bx, by, bw, bh;
	gint sw, sh;
	gint rx, ry;

	sw = gdk_pixbuf_get_width(src);
	sh = gdk_pixbuf_get_height(src);

	border_left = MIN(border_left, w);
	border_right = MIN(border_right, w);
	border_top = MIN(border_top, h);
	border_bottom = MIN(border_bottom, h);

	/* top left */
	rx = x;
	ry = y;
	if (util_clip_region(rx, ry, border_left, border_top,
			     clip_x, clip_y, clip_w, clip_h,
			     &bx, &by, &bw, &bh))
		{
		pixbuf_copy_area_alpha(src, bx - rx, by - ry,
				       dest, bx, by, bw, bh, alpha);
		}
	/* top */
	rx = x + border_left;
	ry = y;
	if (util_clip_region(rx, ry, w - border_left - border_right, border_top,
			     clip_x, clip_y, clip_w, clip_h,
			     &bx, &by, &bw, &bh))
		{
		pixbuf_copy_fill_alpha_offset(src, border_left, 0, sw - border_left - border_right, border_top,
					      dest, bx, by, bw, bh, FALSE, alpha,
					      bx - rx, by - ry);
		}
	/* top right */
	rx = x + w - border_right;
	ry = y;
	if (util_clip_region(rx, ry, border_right, border_top,
			     clip_x, clip_y, clip_w, clip_h,
			     &bx, &by, &bw, &bh))
		{
		pixbuf_copy_area_alpha(src, sw - border_right + (bx - rx), by - ry,
				       dest, bx, by, bw, bh, alpha);
		}
	/* left */
	rx = x;
	ry = y + border_top;
	if (util_clip_region(rx, ry, border_left, h - border_top - border_bottom,
			     clip_x, clip_y, clip_w, clip_h,
			     &bx, &by, &bw, &bh))
		{
		pixbuf_copy_fill_alpha_offset(src, 0, border_top, border_left, sh - border_top - border_bottom,
					      dest, bx, by, bw, bh, FALSE, alpha,
					      bx - rx, by - ry);
		}
	/* right */
	rx = x + w - border_right;
	ry = y + border_top;
	if (util_clip_region(rx, ry, border_right, h - border_top - border_bottom,
			     clip_x, clip_y, clip_w, clip_h,
			     &bx, &by, &bw, &bh))
		{
		pixbuf_copy_fill_alpha_offset(src, sw - border_right, border_top, border_right, sh - border_top - border_bottom,
					      dest, bx, by, bw, bh, FALSE, alpha,
					      bx - rx, by - ry);
		}
	/* bottom left */
	rx = x;
	ry = y + h - border_bottom;
	if (util_clip_region(rx, ry, border_left, border_bottom,
			     clip_x, clip_y, clip_w, clip_h,
			     &bx, &by, &bw, &bh))
		{
		pixbuf_copy_area_alpha(src, bx - rx, sh - border_bottom + (by - ry),
				       dest, bx, by, bw, bh, alpha);
		}
	/* bottom */
	rx = x + border_left;
	ry = y + h - border_bottom;
	if (util_clip_region(rx, ry, w - border_left - border_right, border_bottom,
			     clip_x, clip_y, clip_w, clip_h,
			     &bx, &by, &bw, &bh))
		{
		pixbuf_copy_fill_alpha_offset(src, border_left, sh - border_bottom, sw - border_left - border_right, border_bottom,
					      dest, bx, by, bw, bh, FALSE, alpha,
					      bx - rx, by - ry);
		}
	/* bottom right */
	rx = x + w - border_right;
	ry = y + h - border_bottom;
	if (util_clip_region(rx, ry, border_right, border_bottom,
			     clip_x, clip_y, clip_w, clip_h,
			     &bx, &by, &bw, &bh))
		{
		pixbuf_copy_area_alpha(src, sw - border_right + (bx - rx), sh - border_bottom + (by - ry),
				       dest, bx, by, bw, bh, alpha);
		}
}


/* 
 * returns a portion of the root window in a gdk_pixbuf
 * any part that is off the screen is set to black.
 */

gint util_pixbuf_fill_from_root_window(GdkPixbuf *pb, gint x, gint y, gint get_all)
{
	GdkVisual *gdkvisual;
	GdkWindow *rootwindow;
	gint screen_w;
	gint screen_h;
	gint screen_x;
	gint screen_y;
	gint clear_window = TRUE;
	gint w;
	gint h;

	if (!pb) return FALSE;

	w = gdk_pixbuf_get_width(pb);
	h = gdk_pixbuf_get_height(pb);

	if (w < 1 || h < 1 ) return FALSE;

	screen_w = gdk_screen_width();
	screen_h = gdk_screen_height();

	if (x >= screen_w || y >= screen_h || x + w < 0 || y + h < 0)
		{
		pixbuf_draw_rect_fill(pb, 0, 0, w, h, 0, 0, 0, 255);
		return TRUE;
		}

	screen_x = x;
	if (screen_x < 0)
		{
		pixbuf_draw_rect_fill(pb, 0, 0, -screen_x, h, 0, 0, 0, 255);
		w += screen_x;
		screen_x = 0;
		}
	if (screen_x + w > screen_w)
		{
		pixbuf_draw_rect_fill(pb, w - (screen_x + w - screen_w), 0, screen_x + w - screen_w, h, 0, 0, 0, 255);
		w -= screen_x + w - screen_w;
		}

	screen_y = y;
	if (screen_y < 0)
		{
		pixbuf_draw_rect_fill(pb, 0, 0, w, -screen_y, 0, 0, 0, 255);
		h += screen_y;
		screen_y = 0;
		}
	if (screen_y + h > screen_h)
		{
		pixbuf_draw_rect_fill(pb, 0, h - (screen_y + h - screen_h), w, screen_y + h - screen_h, 0, 0, 0, 255);
		h -= screen_y + h - screen_h;
		}

	if (w < 1 || h < 1) return TRUE;

	rootwindow = gdk_get_default_root_window();
	if (!rootwindow) return TRUE;

	gdkvisual = gdk_drawable_get_visual(rootwindow);
	if (gdkvisual != gdk_visual_get_system()) return TRUE;

	if (debug_mode) printf("root window capture: %d, %d (%d x %d) to %d, %d\n", screen_x, screen_y, w, h, x, y);

	if (!get_all)
		{
		/*
		 * get the backing pixmap
		 * Don't even ask how this works, I slapped it together
		 * from various references from other apps that do the same.
		 */
		Atom prop, type;
		int format;
		unsigned long length, after;
		unsigned char *data = NULL;
		Window desktop_window;

		gdk_error_trap_push();

		desktop_window = GDK_ROOT_WINDOW();

		prop = XInternAtom(GDK_DISPLAY(), "_XROOTPMAP_ID", True);
  
		if (prop != None)
			{
			XGetWindowProperty(GDK_DISPLAY(), desktop_window, prop, 0L, 1L, False,
					   AnyPropertyType, &type, &format, &length, &after,
					   &data);

			if (type == XA_PIXMAP)
				{
				Pixmap p;
				p = *((Pixmap *)data);

				if (p != None)
					{
					static GdkPixmap *pp = NULL;
					GdkColormap *cmap;
					gint p_w, p_h;

					clear_window = FALSE;

					if (!pp) pp = gdk_pixmap_foreign_new(p);
					cmap = gdk_drawable_get_colormap(rootwindow);

					gdk_drawable_get_size(pp, &p_w, &p_h);

					if (p_w < screen_x + w || p_h < screen_y + h)
						{
						/* tiled */
						gint i, j;
						
						for (j = (screen_y / p_h) * p_h; j < screen_y + h; j += p_h)
						    for (i = (screen_x / p_w) * p_w; i < screen_x + w; i += p_w)
							{
							gint offset_x, offset_y;
							gint offset_w, offset_h;

							if (j < screen_y)
								{
								offset_y = screen_y - j;
								}
							else
								{
								offset_y = 0;
								}

							offset_h = p_h - offset_y;
							if (j + offset_y + offset_h >= screen_y + h) offset_h = (screen_y + h) - (j + offset_y);

							if (i < screen_x)
								{
								offset_x = screen_x - i;
								}
							else
								{
								offset_x = 0;
								}

							offset_w = p_w - offset_x;
							if (i + offset_x + offset_w >= screen_x + w) offset_w = (screen_x + w) - (i + offset_x);

							gdk_pixbuf_get_from_drawable(pb, pp, cmap,
										     offset_x, offset_y,
									     	     (i + offset_x) - screen_x, (j + offset_y) - screen_y, offset_w, offset_h);
							}
						}
					else
						{
						gdk_pixbuf_get_from_drawable(pb, pp, cmap,
									     screen_x, screen_y, 
								     	     screen_x - x, screen_y - y, w, h);
						}
					}
				}
			if (data) XFree(data);
			}
		gdk_error_trap_pop();
		}
	else
		{
		/* merely get whatever is on the screen in this area, including other windows */
		gdk_pixbuf_get_from_drawable(pb, rootwindow, NULL,
					     screen_x, screen_y, 
					     screen_x - x, screen_y - y, w, h);
		clear_window = FALSE;
		}

	if (clear_window) pixbuf_draw_rect_fill(pb, screen_x - x, screen_y - y, w, h, 0, 0, 0, 255);

	return TRUE;
}

/* returns a newly allocated pixbuf */

GdkPixbuf *util_pixbuf_from_root_window(gint x, gint y, gint w, gint h, gint get_all)
{
	GdkPixbuf *pb;

	if (w < 1 || h < 1 ) return NULL;

	pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);
	util_pixbuf_fill_from_root_window(pb, x, y, get_all);

	return pb;
}

/*
 *---------------------------------------------------------
 * state saving / restoring stuff
 *---------------------------------------------------------
 */

typedef struct _StateData StateData;
struct _StateData
{
	gchar *skin_name;
	GList *window_list;	/* list of active WindowStateData *'s */
};

typedef struct _WindowStateData WindowStateData;
struct _WindowStateData
{
	gchar *id;
	gint active;	/* window is currently open */
	gint x;		/* root window coords */
	gint y;
	gint width;	/* window size */
	gint height;
};


static GList *slik_state_list = NULL;


gint ui_states_save(const gchar *path)
{
	FILE *f;
	gchar *pathl;
	GList *work;

	ui_sync_states();

	pathl = path_from_utf8(path);
	f = fopen(pathl, "w");
	g_free(pathl);
	if (!f)
		{
		printf(_("Unable to save skin state information to: %s\n"), path);
		return FALSE;
		}

	fprintf(f, "#SLIK skin states\n");
	fprintf(f, "\n");

	work = slik_state_list;
	while (work)
		{
		StateData *sd;
		GList *w;

		sd = work->data;
		work = work->next;

		fprintf(f, "[%s]\n", sd->skin_name);

		w = sd->window_list;
		while (w)
			{
			WindowStateData *ws = w->data;
			w = w->next;

			fprintf(f, "%s = [ %d ] %d, %d ( %d x %d )\n", ws->id,
				ws->active, ws->x, ws->y, ws->width, ws->height);
			}

		fprintf(f, "\n");
		}

	fprintf(f, "#end\n");

	fclose(f);

	return TRUE;
}

gint ui_states_load(const gchar *path)
{
	FILE *f;
	gchar *key = NULL;
	gchar s_buf[1024];
	gchar *pathl;

	pathl = path_from_utf8(path);
	f = fopen(pathl, "r");
	g_free(pathl);
	if (!f) return FALSE;

	/* first line must start with correct comment to id file */
	if (!fgets(s_buf,1024,f) ||
	    strncmp(s_buf, "#SLIK skin state", 15) != 0)
		{
		fclose(f);
		return FALSE;
		}

	while (fgets(s_buf, 1024, f))
		{
		if (s_buf[0]=='#') continue;
		if (s_buf[0]=='[')
			{
			gint c;
			gchar *ptr;

			ptr = s_buf + 1;
			c = 0;
			while(ptr[c] != ']' && ptr[c] != '\n' && ptr[c] != '\0') c++;

			g_free(key);
			key = g_strndup(ptr, c);
			}
		else if (key)
			{
			gchar window_id[256];
			gint active;
			gint x;
			gint y;
			gint w;
			gint h;
			if (sscanf(s_buf, "%255s = [ %d ] %d, %d ( %d x %d )", window_id, &active, &x, &y, &w, &h) == 6)
				{
				ui_state_set(key, window_id, x, y, w, h);
				ui_state_set_active(key, window_id, active);
				}
			}
		}

	fclose(f);

	g_free(key);

	return TRUE;
}

static StateData *ui_state_find_skin(const gchar *skin_name)
{
	GList *work;

	if (!skin_name) skin_name = "default";

	work = slik_state_list;
	while (work)
		{
		StateData *sd = work->data;
		work = work->next;

		if (strcmp(skin_name, sd->skin_name) == 0) return sd;
		}

	return NULL;
}

static WindowStateData *ui_state_find_window(const gchar *window_id, StateData *sd)
{
	GList *work;

	if (!sd || !window_id) return NULL;

	work = sd->window_list;
	while (work)
		{
		WindowStateData *ws = work->data;
		work = work->next;

		if (strcmp(window_id, ws->id) == 0) return ws;
		}

	return NULL;
}

/* this creates things if they do not already exist */
static WindowStateData *ui_state_get_window(const gchar *skin_name, const gchar *window_id)
{
	StateData *sd;
	WindowStateData *ws;

	if (!window_id) return NULL;
	if (!skin_name) skin_name = "default";

	sd = ui_state_find_skin(skin_name);

	if (!sd)
		{
		sd = g_new0(StateData, 1);
		sd->skin_name = g_strdup(skin_name);
		slik_state_list = g_list_append(slik_state_list, sd);
		}

	ws = ui_state_find_window(window_id, sd);

	if (!ws)
		{
		ws = g_new0(WindowStateData, 1);
		ws->id = g_strdup(window_id);
		sd->window_list = g_list_append(sd->window_list, ws);
		}

	return ws;
}

void ui_state_set(const gchar *skin_name, const gchar *window_id, gint x, gint y, gint w, gint h)
{
	WindowStateData *ws;

	if (debug_mode) printf("set: %s %s\n", skin_name, window_id);

	ws = ui_state_get_window(skin_name, window_id);
	if (!ws) return;

	ws->x = x;
	ws->y = y;
	ws->width = w;
	ws->height = h;
}

gint ui_state_get(const gchar *skin_name, const gchar *window_id, gint *x, gint *y, gint *w, gint *h)
{
	StateData *sd;
	WindowStateData *ws;

	if (debug_mode) printf("get: %s %s\n", skin_name, window_id);

	sd = ui_state_find_skin(skin_name);
	if (!sd) return FALSE;

	ws = ui_state_find_window(window_id, sd);
	if (!ws) return FALSE;

	if (x) *x = ws->x;
	if (y) *y = ws->y;
	if (w) *w = ws->width;
	if (h) *h = ws->height;

	return TRUE;
}

void ui_state_set_active(const gchar *skin_name, const gchar *window_id, gint active)
{
	WindowStateData *ws;

	ws = ui_state_get_window(skin_name, window_id);
	if (!ws) return;

	ws->active = active;
}

gint ui_state_get_active(const gchar *skin_name, const gchar *window_id)
{
	StateData *sd;
	WindowStateData *ws;

	sd = ui_state_find_skin(skin_name);
	if (!sd) return FALSE;

	ws = ui_state_find_window(window_id, sd);
	if (!ws) return FALSE;

	return ws->active;
}



