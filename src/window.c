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
#include "window.h"

#include "btn_funcs.h"
#include "display.h"
#include "display_menu.h"
#include "menus.h"
#include "playlist.h"
#include "playlist-window.h"
#include "preferences.h"
#include "skin_default.h"
#include "ui2_display.h"
#include "ui2_main.h"

#include <gdk/gdkkeysyms.h> /* for key values */

#ifdef HAVE_XF86_KEYSYMS
  #include <X11/XF86keysym.h>
#endif

static gint key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	static gint last_key = 0;
	static gint last_state = 0;
	static guint32 last_time = 0;
	gint ret = FALSE;
	UIData *ui = data;

	/* cheap hacks/checks to avoid problems caused by holding down
	 * keys with fast repeat rates */
	if (event->keyval == last_key && event->state == last_state &&
			last_time + 500 >= event->time)
		{
		return FALSE;
		}
	else
		{
		last_key = event->keyval;
		last_state = event->state;
		last_time = event->time;
		}

	if (debug_mode) printf("key pressed: %d\n", event->keyval);

	if (event->state & GDK_CONTROL_MASK)
		{
		ret = TRUE;
		switch (event->keyval)
			{
			case 's': case 'S':
				btn_shuffle_pressed();
				break;
			case 'r': case 'R':
				btn_repeat_pressed();
				break;
			case 't': case 'T':
				time_toggle_pressed();
				break;
			case 'q': case 'Q':
				gqmpeg_exit();
				break;
			case 'a': case 'A':
				show_about_info();
				break;
			case 'o': case 'O':
				show_configuration_window();
				break;
			case 'i': case 'I':
				btn_iconify_pressed();
				break;
			case '-':
			case GDK_KP_Subtract:
				btn_balance_left_pressed();
				btn_balance_released();
				break;
			case '+': case '=':
			case GDK_KP_Add:
				btn_balance_right_pressed();
				btn_balance_released();
				break;
			case '1':
				btn_repeat_1_pressed();
				break;
			case 'w': case 'W':
				if (ui != main_window)
					{
					ui_close(ui);
					}
				break;
			default:
				ret = FALSE;
				break;
			}
		}
	else
		{
		ret = TRUE;
		switch (event->keyval)
			{
			case 'p': case 'P': case GDK_Return:
				btn_play_pressed();
				break;
			case 's': case 'S':
				btn_stop_pressed();
				break;
			case 'n': case 'N': case GDK_Page_Down: case GDK_KP_Page_Down:
				btn_next_down();
				btn_next_pressed();
				break;
			case 'b': case 'B': case GDK_Page_Up: case GDK_KP_Page_Up:
				btn_prev_down();
				btn_prev_pressed();
				break;
			case 'l': case 'L':
				playlist_window_show();
				break;
			case 'm': case 'M':
				btn_mixer_pressed();
				break;
			case 'w': case 'W':
				btn_mode_pressed();
				break;
			case 'i': case 'I':
				current_song_info_pressed();
				break;
			case '-':
			case GDK_KP_Subtract:
				btn_volume_down_pressed();
				btn_volume_released();
				break;
			case '+': case '=':
			case GDK_KP_Add:
				btn_volume_up_pressed();
				btn_volume_released();
				break;
			case '1':
				preset_1_pressed();
				break;
			case '2':
				preset_2_pressed();
				break;
			case '3':
				preset_3_pressed();
				break;
			case '4':
				preset_4_pressed();
				break;
			case '5':
				preset_5_pressed();
				break;
			case '6':
				preset_6_pressed();
				break;
			case '7':
				preset_7_pressed();
				break;
			case '8':
				preset_8_pressed();
				break;
			case '9':
				preset_9_pressed();
				break;
			case '0':
				preset_10_pressed();
				break;
			case GDK_Home: case GDK_KP_Home:
				current_song_set(playlist_get_first(), NULL);
				break;
			case GDK_End: case GDK_KP_End:
				current_song_set(playlist_get_last(), NULL);
				break;
			case 'a': case 'A':
				btn_loop_ab_pressed();
				break;
			case GDK_Escape:
				if (loop_ab_enabled)
					{
					loop_ab_enabled = FALSE;
					display_set_loop_ab(FALSE);
					}
				break;
			case '!':
				ui_debug_print_all_keys(main_window);
				break;
			case '~': case '`':
				btn_edit_skin_pressed(ui);
				break;
			case GDK_F1:
			case GDK_Help:
				help_window_show("documentation");
				break;

	/*
	 * Sorry about this #ifdef mess, but the XF86keysym.h file seems to change
	 * on almost every Xfree release - older versions do not have some of these.
	 */
#ifdef HAVE_XF86_KEYSYMS

#ifdef XF86XK_AudioLowerVolume
			case XF86XK_AudioLowerVolume:
				btn_volume_down_pressed();
				btn_volume_released();
				break;
#endif
#ifdef XF86XK_AudioMute
			case XF86XK_AudioMute:
				/* FIXME: gqmpeg has no mute facility */
				btn_mixer_pressed();
				break;
#endif
#ifdef XF86XK_AudioRaiseVolume
			case XF86XK_AudioRaiseVolume:
				btn_volume_up_pressed();
				btn_volume_released();
				break;
#endif
#ifdef XF86XK_AudioPlay
			case XF86XK_AudioPlay:
				btn_play_pressed();
				break;
#endif
#ifdef XF86XK_AudioStop
			case XF86XK_AudioStop:
				btn_stop_pressed();
				break;
#endif
#ifdef XF86XK_AudioPrev
			case XF86XK_AudioPrev:
				btn_prev_down();
				btn_prev_pressed();
				break;
#endif
#ifdef XF86XK_AudioNext
			case XF86XK_AudioNext:
				btn_next_down();
				btn_next_pressed();
				break;
#endif
#ifdef XF86XK_AudioPause
			case XF86XK_AudioPause:
				btn_pause_pressed();
				break;
#endif
#if defined(XF86XK_AudioMedia) || defined(XF86XK_Eject)
  #ifdef XF86XK_AudioMedia
			case XF86XK_AudioMedia:
  #endif
  #ifdef XF86XK_Eject
			case XF86XK_Eject:
  #endif
				playlist_window_toggle();
				break;
#endif
#ifdef XF86XK_OpenURL
			case XF86XK_OpenURL:
				btn_load_file_pressed();
				break;
#endif

#endif /* HAVE_XF86_KEYSYMS */

			default:
				ret = FALSE;
				break;
			}
		}

	return ret;
}

static gint destroy(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	gqmpeg_exit();

	return TRUE;
}

void main_window_set_title(const gchar *title)
{
	if (main_window)
		{
		gchar *buf;

		if (title && !dock_mode)
			{
			buf = g_strconcat(title, " - GQmpeg", NULL);
			}
		else
			{
			buf = g_strdup("GQmpeg");
			}

		ui_title_set(main_window, buf);
		g_free(buf);
		}
}

static void main_window_mouse_cb(UIData *ui, gint button, guint32 time, gpointer data)
{
	if (button == 3 || (dock_mode && button == 2))
		{
		if (skinned_menus_enable)
			{
			display_menu_popup(ui->skin_path, -1, -1, button, time, ui);
			}
		else
			{
			GtkWidget *menu;

			menu = menu_main(ui);
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, time);
			}
		return;
		}

	/* mouse wheel */
	else if (button == 4)
		{
		btn_volume_up_pressed();
		btn_volume_released();
		return;
		}
	else if (button == 5)
		{
		btn_volume_down_pressed();
		btn_volume_released();
		return;
		}

}

static gint subwindow_delete_cb(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	UIData *ui = data;

	ui_close(ui);
	return TRUE;
}

static void main_window_new_subwindow_cb(UIData *child, const gchar *key, gpointer data)
{
	const gchar *title;

	g_signal_connect(G_OBJECT(child->window), "delete_event",
			 G_CALLBACK(subwindow_delete_cb), child);
	g_signal_connect(G_OBJECT(child->display), "key_press_event",
			 G_CALLBACK(key_pressed), child);

	if (!key) return;

	ui_set_icon(child, (const char **)gqmpeg_icon_xpm, NULL, NULL);

	if (strstr(key, "list") || strstr(key, "playlist"))
		{
		title = _("Playlist - GQmpeg");
		}
	else if (strstr(key, "info"))
		{
		title = _("Song Information - GQmpeg");
		}
	else if (strstr(key, "extra"))
		{
		title = _("Extras - GQmpeg");
		}
	else if (strstr(key, "about"))
		{
		title = _("About - GQmpeg");
		}
	else
		{
		title = _("GQmpeg");
		}

	ui_title_set(child, title);
}

UIData *main_window_create(const gchar *skin_path)
{
	UIData *ui;

	ui = ui_new("GQmpeg", "gqmpeg", wm_decorations, "GQmpeg");
	if (dock_mode) ui_moveable_set(ui, FALSE);
	ui_set_icon(ui, (const char **)gqmpeg_icon_xpm, NULL, NULL);

	ui_set_skin_callback(ui, skin_load_default_cb, NULL);

	g_signal_connect(G_OBJECT(ui->window), "delete_event",
			 G_CALLBACK(destroy), ui);
	g_signal_connect(G_OBJECT(ui->display), "key_press_event",
			 G_CALLBACK(key_pressed), ui);

	ui_set_mouse_callback(ui, main_window_mouse_cb, NULL);
	ui_set_new_window_callback(ui, main_window_new_subwindow_cb, NULL);

	/* register the right stuff */
	display_register_widgets(ui);

/* ----	dnd_set_drop(display_area, display_area->window);
*/

	if (skin_path)
		{
		printf(_("loading skin..\n"));
		if (!ui_skin_load(ui, skin_path, NULL))
			{
			if (skin_default_path)
				{
				if (!ui_skin_load(ui, skin_default_path, skin_mode_key))
					{
					printf(_("Default skin not loaded, using built-in default\n"));
					ui_skin_load(ui, NULL, NULL);
					}
				}
			else
				{
				printf(_("Skin not loaded, using built-in default\n"));
				ui_skin_load(ui, NULL, NULL);
				}
			}
		}
	else if (skin_default_path)
		{
		if (!ui_skin_load(ui, skin_default_path, skin_mode_key))
			{
			printf(_("Skin in config file not loaded, using built-in default\n"));
			ui_skin_load(ui, NULL, NULL);
			}
		}
	else
		{
		ui_skin_load(ui, NULL, skin_mode_key);
		}

	playlist_window_update_titles();

	return ui;
}

