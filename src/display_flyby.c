/*
 * GQmpeg
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "gqmpeg.h"
#include "display_flyby.h"

#include "display_menu.h"
#include "menus.h"
#include "playlist.h"
#include "skin_default.h"
#include "ui2_button.h"
#include "ui2_display.h"
#include "ui2_main.h"
#include "ui2_skin.h"
#include "ui2_text.h"
#include "ui2_util.h"
#include "ui_pixbuf_ops.h"

#include <math.h>

/* define key:
 * time		length to display the window
 * delay	length to wait before starting display (delay from song start)
 * slide_time	delay between window movement updates
 * slide_total	total time to slide from OFF to ON (hidden to fully visible)
 *
 * The total time for 1 uninterrupted cycle is:
 *
 *      delay + slide_total(showing) + time + slide_total(hiding)
 *
 * x/y_offset	offset to place window from edge of screen
 */

#define DISPLAY_FLYBY_TIME 5000
#define DISPLAY_FLYBY_DELAY 4000
#define DISPLAY_FLYBY_SLIDE_TIME 33
#define DISPLAY_FLYBY_SLIDE_TOTAL 500

#define DISPLAY_FLYBY_X_OFFSET 16
#define DISPLAY_FLYBY_Y_OFFSET 16

typedef enum
{
	FLYBY_STATE_OFF = 0,
	FLYBY_STATE_WAITING = 1,
	FLYBY_STATE_SLIDE_IN = 2,
	FLYBY_STATE_ON = 3,
	FLYBY_STATE_SLIDE_OUT = 4
} FlyByState;

typedef struct _FlyByData FlyByData;
struct _FlyByData
{
	UIData *ui;
	gint timeout_id;
	gint song_index;

	gint x;
	gint y;

	gint pos_x;
	gint pos_y;
	gint off_x;
	gint off_y;

	gint back_x;
	gint back_y;
	GdkPixbuf *backing;

	FlyByState state;

	gint slide_pos;
};

static FlyByData *flyby = NULL;


static void display_flyby_set_state(FlyByData *fb, FlyByState state, gint cancel_timer, gint extend);


const gchar *display_flyby_location_to_text(gint location)
{
	switch (location)
		{
		case 0:
		default:
			return _("Upper left");
			break;
		case 1:
			return _("Upper right");
			break;
		case 2:
			return _("Lower right");
			break;
		case 3:
			return _("Lower left");
			break;
		}

	return "";
}

static gint display_flyby_slide(FlyByData *fb, gint slide_in)
{
	double offset;
	gint x;

	fb->slide_pos++;
	offset = (double)fb->slide_pos / (DISPLAY_FLYBY_SLIDE_TOTAL / DISPLAY_FLYBY_SLIDE_TIME) * 9.0;

	if (!slide_in) offset = 9.0 - offset;
	offset = log10(offset + 1.0);

	x = abs(fb->pos_x - fb->off_x);
	x = (gint)((double)x * offset);

	if (slide_in || TRUE)
		{
		if (fb->off_x < fb->pos_x)
			{
			fb->x = MIN(fb->pos_x, (fb->off_x + x));
			}
		else
			{
			fb->x = MAX(fb->pos_x, (fb->off_x - x));
			}
		}
	else
		{
		if (fb->pos_x < fb->off_x)
			{
			fb->x = MIN(fb->off_x, (fb->pos_x + x));
			}
		else
			{
			fb->x = MAX(fb->off_x, (fb->pos_x - x));
			}
		}

	if (fb->backing)
		{
		/* request a backing sync ourselves, as gdk_window_move does not spawn move event */
		skin_sync_back(fb->ui->skin, fb->ui);
		ui_display_sync(fb->ui, FALSE);
		}

	gdk_window_move(fb->ui->window->window, fb->x, fb->y);

	if (slide_in)
		{
		return (fb->x == fb->pos_x && fb->y == fb->pos_y);
		}

	return (fb->x == fb->off_x && fb->y == fb->off_y);
}

static gint display_flyby_back_cb(UIData *ui, GdkPixbuf *pb, gpointer data)
{
	FlyByData *fb = data;
	gint x, y, w, h;
	gint sw, sh;
	gint bw, bh;

	if (!fb || !fb->backing) return FALSE;

	sw = fb->ui->skin->width;
	sh = fb->ui->skin->height;
	bw = gdk_pixbuf_get_width(fb->backing);
	bh = gdk_pixbuf_get_height(fb->backing);

	if (util_clip_region(fb->x, fb->y, sw, sh,
			     fb->back_x, fb->back_y, bw, bh,
			     &x, &y, &w, &h))
		{
		pixbuf_copy_area(fb->backing, x - fb->back_x, y - fb->back_y,
				 pb, fb->x < 0 ? 0 - fb->x : 0, fb->y < 0 ? 0 - fb->y : 0,
				 w, h, FALSE);
		}

	return TRUE;
}

static void display_flyby_destroy(GtkWidget *widget, gpointer data)
{
	if (flyby && flyby == data)
		{
		if (flyby->backing) gdk_pixbuf_unref(flyby->backing);
		g_free(flyby);
		flyby = NULL;
		}
}

static gint display_flyby_delete(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	return TRUE;
}

static void display_flyby_update(FlyByData *fb)
{
	SongData *sd;

	if (!fb->ui->skin) return;

	text_set_text("now_title", fb->ui, current_song_get_title());
	text_set_text("now_artist", fb->ui, current_song_get_artist());

	sd = playlist_get_data(playlist_get_next(current_song_get_number()));

	text_set_text("next_title", fb->ui, playlist_data_get_title(sd));
	text_set_text("next_artist", fb->ui, playlist_data_get_artist(sd));
}

static void display_flyby_show_cb(GtkWidget *widget, gpointer data)
{
	FlyByData *fb = data;

	display_flyby_update(fb);
}

static const gchar *display_flyby_text_now(TextData *text, const gchar *key, gpointer data)
{
	return _("now playing:");
}

static const gchar *display_flyby_text_now_title(TextData *text, const gchar *key, gpointer data)
{
	return current_song_get_title();
}

static const gchar *display_flyby_text_now_artist(TextData *text, const gchar *key, gpointer data)
{
	return current_song_get_artist();
}

static const gchar *display_flyby_text_next(TextData *text, const gchar *key, gpointer data)
{
	return _("up next:");
}

static const gchar *display_flyby_text_next_title(TextData *text, const gchar *key, gpointer data)
{
	gint n;

	n = playlist_get_next(current_song_get_number());
	if (n == -1) return NULL;

	return playlist_get_title(n);
}

static const gchar *display_flyby_text_next_artist(TextData *text, const gchar *key, gpointer data)
{
	gint n;

	n = playlist_get_next(current_song_get_number());
	if (n == -1) return NULL;

	return playlist_get_artist(n);
}

static void display_flyby_button_hide(ButtonData *button, const gchar *key, gpointer data)
{
	FlyByData *fb = data;

	if (fb->state == FLYBY_STATE_ON || fb->state == FLYBY_STATE_SLIDE_IN)
		{
		display_flyby_set_state(fb, FLYBY_STATE_SLIDE_OUT, TRUE, FALSE);
		}
}

static void display_flyby_extend(FlyByData *fb)
{
	if (fb->state != FLYBY_STATE_ON) return;

	display_flyby_set_state(fb, fb->state, TRUE, TRUE);
}

static gint display_flyby_motion(GtkWidget *w, GdkEventMotion *event, gpointer data)
{
	FlyByData *fb = data;

	display_flyby_extend(fb);
	return FALSE;
}

static gint display_flyby_pressed(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	FlyByData *fb = data;

	display_flyby_extend(fb);

	if (event->button == 3)
		{
		/* copied from window.c */
		if (skinned_menus_enable)
			{
			display_menu_popup(main_window->skin_path, -1, -1,
					   event->button, event->time, main_window);
			}
		else
			{
			GtkWidget *menu;

			menu = menu_main(main_window);
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
				       event->button, event->time);
			}
		return TRUE;
		}

	return FALSE;
}

static gint display_flyby_released(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	FlyByData *fb = data;

	display_flyby_extend(fb);
	return FALSE;
}

static FlyByData *display_flyby_init(void)
{
	FlyByData *fb;
	GtkWidget *window;

	fb = g_new0(FlyByData , 1);

	fb->timeout_id = -1;
	fb->song_index = -1;
	fb->state = FLYBY_STATE_OFF;
	fb->backing = NULL;

	window = gtk_window_new(GTK_WINDOW_POPUP);
	g_signal_connect(G_OBJECT(window), "destroy",
			 G_CALLBACK(display_flyby_destroy), fb);
	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(display_flyby_delete), fb);
	g_signal_connect(G_OBJECT(window), "hide",
			 G_CALLBACK(display_flyby_show_cb), fb);

	fb->ui = ui_new_into_container("GQmpeg", "flyby", window);
	fb->ui->window = window;

	ui_group_set_child(main_window, fb->ui);

	/* register widgets */
	text_register_key("now", fb->ui, display_flyby_text_now, fb);
	text_register_key("now_title", fb->ui, display_flyby_text_now_title, fb);
	text_register_key("now_artist", fb->ui, display_flyby_text_now_artist, fb);
	text_register_key("next", fb->ui, display_flyby_text_next, fb);
	text_register_key("next_title", fb->ui, display_flyby_text_next_title, fb);
	text_register_key("next_artist", fb->ui, display_flyby_text_next_artist, fb);

	button_register_key("hide_flyby", fb->ui,
			    NULL, NULL,
			    display_flyby_button_hide, fb,
			    NULL, NULL,
			    NULL, NULL);

	ui_set_skin_callback(fb->ui, skin_load_default_flyby_cb, fb);
	ui_set_back_callback(fb->ui, display_flyby_back_cb, fb);

	g_signal_connect(G_OBJECT(fb->ui->display),"motion_notify_event",
			 G_CALLBACK(display_flyby_motion), fb);
        g_signal_connect(G_OBJECT(fb->ui->display),"button_press_event",
			 G_CALLBACK(display_flyby_pressed), fb);
        g_signal_connect(G_OBJECT(fb->ui->display),"button_release_event",
			 G_CALLBACK(display_flyby_released), fb);

	return fb;
}

static void display_flyby_setup(FlyByData *fb)
{
	gint fw, fh;
	gint sw, sh;
	gint ox, oy;
	gint px, py;
	gint bx, by;
	gint back_w, back_h;

	fw = fb->ui->skin->width;
	fh = fb->ui->skin->height;
	sw = gdk_screen_width();
	sh = gdk_screen_height();

	switch (flyby_location)
		{
		case 0:
			px = DISPLAY_FLYBY_X_OFFSET;
			py = DISPLAY_FLYBY_Y_OFFSET;
			ox = 0 - fw;
			oy = py;
			bx = 0;
			by = py;
			break;
		case 1:
			px = sw - fw - DISPLAY_FLYBY_X_OFFSET;
			py = DISPLAY_FLYBY_Y_OFFSET;
			ox = sw;
			oy = py;
			bx = px;
			by = py;
			break;
		case 2:
			px = sw - fw - DISPLAY_FLYBY_X_OFFSET;
			py = sh - fh - DISPLAY_FLYBY_Y_OFFSET;
			ox = sw;
			oy = py;
			bx = px;
			by = py;
			break;
		case 3:
		default:
			px = DISPLAY_FLYBY_X_OFFSET;
			py = sh - fh - DISPLAY_FLYBY_Y_OFFSET;
			ox = 0 - fw;
			oy = py;
			bx = 0;
			by = py;
			break;
		}

	fb->pos_x = px;
	fb->pos_y = py;
	fb->off_x = ox;
	fb->off_y = oy;

	fb->x = ox;
	fb->y = oy;

	fb->back_x = bx;
	fb->back_y = by;

	back_w = fw + DISPLAY_FLYBY_X_OFFSET;
	back_h = fh;

	if (fb->backing) gdk_pixbuf_unref(fb->backing);
	if (fb->ui->skin->transparent)
		{
		fb->backing = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, back_w, back_h);
		util_pixbuf_fill_from_root_window(fb->backing, bx, by, TRUE);
		}
	else
		{
		fb->backing = NULL;
		}
}

static void display_flyby_show(void)
{
	const gchar *skin_path;

	if (!flyby)
		{
		flyby = display_flyby_init();
		}

	skin_path = main_window->skin_path;
	if (!flyby->ui->skin ||
	    (flyby->ui->skin_path) != (skin_path) ||
	    (flyby->ui->skin_path && !skin_path) ||
	    (skin_path && strcmp(skin_path, flyby->ui->skin_path) != 0) )
		{
		if (!ui_skin_load(flyby->ui, skin_path, "skindata_flyby"))
			{
			ui_skin_load(flyby->ui, NULL, "skindata_flyby");
			}
		}

	display_flyby_setup(flyby);

	if (!GTK_WIDGET_REALIZED(flyby->ui->window)) gtk_widget_realize(flyby->ui->window);
	gdk_window_move(flyby->ui->window->window, flyby->x, flyby->y);
	gtk_widget_show(flyby->ui->window);
}

static gint display_flyby_timer_cb(gpointer data)
{
	FlyByData *fb = data;

	if (fb->state != FLYBY_STATE_SLIDE_OUT &&
	    (status != STATUS_PLAY || flyby->song_index != current_song_get_number()))
		{
		display_flyby_set_state(fb, FLYBY_STATE_OFF, FALSE, FALSE);
		return FALSE;
		}

	switch (fb->state)
		{
		case FLYBY_STATE_WAITING:
			display_flyby_set_state(fb, fb->state + 1, FALSE, FALSE);
			return FALSE;
			break;
		case FLYBY_STATE_ON:
			if (fb->ui && fb->ui->active_widget)
				{
				/* user interacting with it, do not hide */
				return TRUE;
				}

			display_flyby_set_state(fb, fb->state + 1, FALSE, FALSE);
			return FALSE;
			break;
		case FLYBY_STATE_SLIDE_IN:
			if (display_flyby_slide(fb, TRUE))
				{
				display_flyby_set_state(fb, fb->state + 1, FALSE, FALSE);
				return FALSE;
				}
			return TRUE;
			break;
		case FLYBY_STATE_SLIDE_OUT:
			if (display_flyby_slide(fb, FALSE))
				{
				display_flyby_set_state(fb, FLYBY_STATE_OFF, FALSE, FALSE);
				return FALSE;
				}
			return TRUE;
			break;
		default:
			printf("Error: unknown flyby state\n");
			break;
		}
	fb->timeout_id = -1;
	return FALSE;
}

static void display_flyby_set_state(FlyByData *fb, FlyByState state, gint cancel_timer, gint extend)
{
	gint delay = -1;

	if (fb->state == state && !extend) return;

	if (fb->timeout_id != -1)
		{
		if (cancel_timer) gtk_timeout_remove(fb->timeout_id);
		fb->timeout_id = -1;
		}

	switch (state)
		{
		case FLYBY_STATE_WAITING:
			delay = DISPLAY_FLYBY_DELAY;
			break;
		case FLYBY_STATE_ON:
			delay = DISPLAY_FLYBY_TIME;
			break;
		case FLYBY_STATE_SLIDE_IN:
			display_flyby_show();
			fb->slide_pos = 0;
			delay = DISPLAY_FLYBY_SLIDE_TIME;
			break;
		case FLYBY_STATE_SLIDE_OUT:
			fb->slide_pos = 0;
			delay = DISPLAY_FLYBY_SLIDE_TIME;
			break;
		case FLYBY_STATE_OFF:
		default:
			if (fb->state == FLYBY_STATE_SLIDE_OUT &&
			    (fb->x != fb->off_x || fb->y != fb->off_y) )
				{
				state = FLYBY_STATE_SLIDE_OUT;
				delay = DISPLAY_FLYBY_SLIDE_TIME;
				}
			else if (fb->state == FLYBY_STATE_SLIDE_IN)
				{
				/* invert counter, to maintain position */
				fb->slide_pos = (DISPLAY_FLYBY_SLIDE_TOTAL / DISPLAY_FLYBY_SLIDE_TIME) - fb->slide_pos;
				state = FLYBY_STATE_SLIDE_OUT;
				delay = DISPLAY_FLYBY_SLIDE_TIME;
				}
			else if (fb->state == FLYBY_STATE_ON)
				{
				fb->slide_pos = 0;
				state = FLYBY_STATE_SLIDE_OUT;
				delay = DISPLAY_FLYBY_SLIDE_TIME;
				}
			else if (GTK_WIDGET_VISIBLE(fb->ui->window))
				{
				gtk_widget_hide(fb->ui->window);
				}
			break;
		}

	if (debug_mode) printf("flyby state change: %d from %d, delay %d\n", state, fb->state, delay);

	fb->state = state;

	if (delay > 0)
		{
		fb->timeout_id = gtk_timeout_add(delay, display_flyby_timer_cb, fb);
		}
}

void display_flyby_start(void)
{
	if (!flyby_enabled) return;

	if (!flyby)
		{
		flyby = display_flyby_init();
		}

	display_flyby_update(flyby);

	if (flyby->state != FLYBY_STATE_OFF) return;

	flyby->song_index = current_song_get_number();
	display_flyby_set_state(flyby, FLYBY_STATE_WAITING, TRUE, FALSE);
}

void display_flyby_reset(void)
{
	if (flyby && flyby->state != FLYBY_STATE_OFF)
		{
		display_flyby_update(flyby);
		if (!flyby->ui || !flyby->ui->active_widget)
			{
			display_flyby_set_state(flyby, FLYBY_STATE_OFF, TRUE, FALSE);
			}
		}
}

