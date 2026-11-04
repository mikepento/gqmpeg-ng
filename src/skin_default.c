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

#include "gqmpeg.h"
#include "skin_default.h"

#include "ui2_button.h"
#include "ui2_item.h"
#include "ui2_list.h"
#include "ui2_number.h"
#include "ui2_slider.h"
#include "ui2_skin.h"
#include "ui2_text.h"
#include "ui2_util.h"
#include "ui2_widget.h"


#define GQMPEG_SKIN_FONT "Sans Bold 10"


#include "default_skin/back.xpm"
#include "default_skin/back_mini.xpm"
#include "default_skin/btn_config.xpm"
#include "default_skin/btn_exit.xpm"
#include "default_skin/btn_iconify.xpm"
#include "default_skin/btn_minus.xpm"
#include "default_skin/btn_mixer.xpm"
#include "default_skin/btn_mlist.xpm"
#include "default_skin/btn_mnext.xpm"
#include "default_skin/btn_mode.xpm"
#include "default_skin/btn_mpause.xpm"
#include "default_skin/btn_mplay.xpm"
#include "default_skin/btn_mprev.xpm"
#include "default_skin/btn_mrepeat.xpm"
#include "default_skin/btn_mshuffle.xpm"
#include "default_skin/btn_mstop.xpm"
#include "default_skin/btn_next.xpm"
#include "default_skin/btn_p1.xpm"
#include "default_skin/btn_p10.xpm"
#include "default_skin/btn_p2.xpm"
#include "default_skin/btn_p3.xpm"
#include "default_skin/btn_p4.xpm"
#include "default_skin/btn_p5.xpm"
#include "default_skin/btn_p6.xpm"
#include "default_skin/btn_p7.xpm"
#include "default_skin/btn_p8.xpm"
#include "default_skin/btn_p9.xpm"
#include "default_skin/btn_pause.xpm"
#include "default_skin/btn_play.xpm"
#include "default_skin/btn_playlist.xpm"
#include "default_skin/btn_prev.xpm"
#include "default_skin/btn_repeat.xpm"
#include "default_skin/btn_repeat_1.xpm"
#include "default_skin/btn_shuffle.xpm"
#include "default_skin/btn_ab.xpm"
#include "default_skin/btn_stop.xpm"
#include "default_skin/btn_time.xpm"
#include "default_skin/dig_lrg.xpm"
#include "default_skin/dig_sml.xpm"
#include "default_skin/focus.xpm"
#include "default_skin/itm_live.xpm"
#include "default_skin/itm_load.xpm"
#include "default_skin/itm_mminus.xpm"
#include "default_skin/itm_stereo.xpm"
#include "default_skin/itm_time.xpm"
#include "default_skin/slider_pos.xpm"
#include "default_skin/slider_vol.xpm"

#include "default_skin/back_playlist.xpm"
#include "default_skin/btn_list.xpm"
#include "default_skin/btn_up.xpm"
#include "default_skin/btn_down.xpm"
#include "default_skin/btn_sizer.xpm"
#include "default_skin/btn_vert.xpm"
#include "default_skin/list_back.xpm"
#include "default_skin/list_row.xpm"
#include "default_skin/list_flags.xpm"

#include "default_skin/back_vert.xpm"
#include "default_skin/itm_vcolon.xpm"

#include "default_skin/flyby_back.xpm"
#include "default_skin/btn_snext.xpm"
#include "default_skin/btn_sprev.xpm"

#include "default_skin/menu_back.xpm"
#include "default_skin/menu_list_back.xpm"
#include "default_skin/menu_list_row.xpm"
#include "default_skin/menu_list_icons.xpm"


static WidgetData *add_item(const gchar *key, gchar **data, gint sections,
			    gint x, gint y, SkinData *skin, const gchar *text_id)
{
	ItemData *item;

	item = item_new_from_data(data, x, y, sections);
	return item_register(skin, item, key, text_id);
}

static DigitData *add_digit(const gchar *key, gchar **data, SkinData *skin, const gchar *text_id)
{
	DigitData *digit;

	digit = digit_new_from_data(data);
	digit_register(skin, digit, key, text_id);
	digit_unref(digit);

	return digit;
}

static WidgetData *add_number(const gchar *key, gint length, gint rotation, gint zeros, gint centered, gint x, gint y,
			      DigitData *digit, SkinData *skin, const gchar *text_id)
{
	NumberData *number;

	number = number_new(digit, x, y, length, zeros, centered, rotation);
	return number_register(skin, number, key, text_id);
}

static WidgetData *add_button(const gchar *key, gchar **data, gint prelight, gint light, gint x, gint y,
			      gchar **clip_data, SkinData *skin, const gchar *text_id)
{
	ButtonData *button;

	button = button_new_from_data(data, x, y, prelight, light, clip_data);
	return button_register(skin, button, key, text_id);
}

static WidgetData *add_slider(const gchar *key, gchar **data, gint has_press, gint prelight, gint vertical, gint reversed,
			      gint length, gint x, gint y, SkinData *skin, const gchar *text_id,
			      gint sizeable, gint real_length, gint border_1, gint border_2, gint stretch)
{
	SliderData *slider;

	slider = slider_new_from_data(data, x, y, length, vertical, reversed, has_press, prelight, NULL);
	if (sizeable) slider_set_attributes(slider, sizeable, real_length, border_1, border_2, stretch);
	return slider_register(skin, slider, key, text_id);
}

static WidgetData *add_text(const gchar *key, const gchar *font, gint width, gint sizeable, gint rotation,
			    guint8 r, guint8 g, guint8 b, guint8 a,
			    gint x, gint y, SkinData *skin, const gchar *text_id)
{
	TextData *text;

	text = text_new_from_x(font, x, y, width, sizeable, rotation);
	text_set_color(text, r, g, b, a);
	return text_register(skin, text, key, text_id);
}

/* double size utilities */
static void double_window(SkinData *skin)
{
	util_size(&skin->width);
	util_size(&skin->height);
	util_size(&skin->border_left);
	util_size(&skin->border_right);
	util_size(&skin->border_bottom);
	util_size(&skin->border_top);
}

static void double_focus(SkinData *skin)
{
	util_size(&skin->focus_border_left);
	util_size(&skin->focus_border_right);
	util_size(&skin->focus_border_bottom);
	util_size(&skin->focus_border_top);
}

static void add_focus(SkinData *skin)
{
	skin->focus_overlay = util_size_pixbuf(gdk_pixbuf_new_from_xpm_data((const char **)focus_xpm), TRUE);
	skin->focus_box_alpha = 150;
	skin->focus_has_border = TRUE;
	skin->focus_border_left = 5;
	skin->focus_border_right = 5;
	skin->focus_border_bottom = 5;
	skin->focus_border_top = 5;
}

static void finish_simple_window(SkinData *skin)
{
	skin->width = gdk_pixbuf_get_width(skin->real_overlay);
	skin->height = gdk_pixbuf_get_height(skin->real_overlay);

	/* this is merely to that the skin editor has sane values for these */
	skin->width_def = skin->width_min = skin->width_max = skin->width;
	skin->height_def = skin->height_min = skin->height_max = skin->height;
}

SkinData *skin_load_default_cb(UIData *ui, const gchar *key, gpointer data)
{
	SkinData *skin;
	DigitData *ds, *dl;
	WidgetData *wd;

	skin = skin_new();

	if (key && strcmp(key, "skindata_playlist") == 0)
		{
		ListData *ld;

		skin->real_overlay = util_size_pixbuf(gdk_pixbuf_new_from_xpm_data((const char **)back_playlist_xpm), TRUE);

		skin->width_def = skin->width = 337;
		skin->height_def = skin->height = 242;

		skin->sizeable = TRUE;
		skin->width_min = 337;
		skin->height_min = 146;
		skin->width_max = 449;
		skin->height_max = 402;

		skin->width_inc = 16;
		skin->height_inc = 16;

		skin->has_border = TRUE;
		skin->border_left = 126;
		skin->border_right = 208;
		skin->border_top = 60;
		skin->border_bottom = 66;

		add_text("playlist", GQMPEG_SKIN_FONT, 205, FALSE, 0, 255, 255, 255, 150, 91, 4, skin, NULL);

		wd = add_button("playlist", (gchar **)btn_playlist_xpm, TRUE, FALSE, 10, 202, NULL, skin, NULL);
		ui_widget_set_anchor(wd, FALSE, TRUE);
		wd = add_button("play", (gchar **)btn_play_xpm, TRUE, TRUE, 50, 202, NULL, skin, NULL);
		ui_widget_set_anchor(wd, FALSE, TRUE);
		wd = add_button("stop", (gchar **)btn_stop_xpm, TRUE, TRUE, 90, 202, NULL, skin, NULL);
		ui_widget_set_anchor(wd, FALSE, TRUE);
		wd = add_button("prev", (gchar **)btn_prev_xpm, TRUE, FALSE, 130, 212, NULL, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);
		wd = add_button("next", (gchar **)btn_next_xpm, TRUE, FALSE, 165, 212, NULL, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);

		ld = list_new(gdk_pixbuf_new_from_xpm_data((const char **)list_back_xpm),
			      16, 30, 284, 160, TRUE, 3,
			      0, 0, 0, 0, FALSE);
		list_image_row(ld, gdk_pixbuf_new_from_xpm_data((const char **)list_row_xpm),
			       TRUE, TRUE, 1, 1, FALSE, NULL);
		list_image_row_flag(ld, gdk_pixbuf_new_from_xpm_data((const char **)list_flags_xpm), 4, 0);
		list_set_font(ld, "Sans Bold 9", NULL, FALSE, 0, 0, 0, 200);

		list_set_column_attributes(ld, 0, 30, UI_LIST_COLUMN_SIZE_FIXED | UI_LIST_COLUMN_JUSTIFY_RIGHT, "number");
		list_set_column_attributes(ld, 1, 78, UI_LIST_COLUMN_SIZE_PROPORTIONAL, "title");
		list_set_column_attributes(ld, 2, 30, UI_LIST_COLUMN_SIZE_FIXED | UI_LIST_COLUMN_JUSTIFY_RIGHT, "time");

		wd = list_register(skin, ld, "playlist", NULL);

		wd = add_button("list_playlist_scroll_up", (gchar **)btn_up_xpm, TRUE, FALSE, 313, 24, NULL, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, FALSE);
		wd = add_slider("list_playlist_scroll", (gchar **)slider_vol_xpm,
			       	TRUE, TRUE, TRUE, FALSE, 40, 314, 41, skin, NULL,
				TRUE, 138, 2, 2, FALSE);
		ui_widget_set_anchor(wd, TRUE, FALSE);
		wd = add_button("list_playlist_scroll_down", (gchar **)btn_down_xpm, TRUE, FALSE, 313, 180, NULL, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);

		wd = add_button("skin_size", (gchar **)btn_sizer_xpm, TRUE, FALSE, 305, 210, NULL, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);

		wd = add_button("skin_iconify", (gchar **)btn_iconify_xpm, TRUE, FALSE, 301, 4, NULL, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, FALSE);
		wd = add_button("skin_close", (gchar **)btn_exit_xpm, TRUE, FALSE, 317, 4, NULL, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, FALSE);

		ds = add_digit("small",(gchar **)dig_sml_xpm, skin, NULL);
		dl = add_digit("large",(gchar **)dig_lrg_xpm, skin, NULL);

		wd = add_number("hour", 2, 0, FALSE, FALSE, 220, 192, dl, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);
		wd = add_number("minute", 2, 0, TRUE, FALSE, 248, 192, dl, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);
		wd = add_number("second", 2, 0, TRUE, FALSE, 276, 192, dl, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);

		wd = add_number("song", 3, 0, FALSE, TRUE, 240, 209, ds, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);
		wd = add_number("song_total", 3, 0, FALSE, TRUE, 269, 209, ds, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);

		wd = add_item("load", (gchar **)itm_load_xpm, 6, 178, 190, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);

		wd = add_button("time_remaining", (gchar **)btn_minus_xpm, TRUE, TRUE, 211, 199, NULL, skin, NULL);
		ui_widget_set_anchor(wd, TRUE, TRUE);

		skin_finalize(skin);
		}
	else if (key && (strcmp(key, "skindata_vert_r") == 0 || strcmp(key, "skindata_vert_l") == 0))
		{
		skin->real_overlay = util_size_pixbuf(gdk_pixbuf_new_from_xpm_data((const char **)back_vert_xpm), TRUE);

		ds = add_digit("small",(gchar **)dig_lrg_xpm, skin, NULL);

		if (strcmp(key, "skindata_vert_l") == 0)
			{
			add_text("title", GQMPEG_SKIN_FONT, 127, FALSE, 270, 0, 0, 0, 179, 2, 110, skin, NULL);
			add_number("second", 2, 270, TRUE, FALSE, 4, 57, ds, skin, NULL);
			add_number("minute", 2, 270, FALSE, FALSE, 4, 85, ds, skin, NULL);
			add_item("dummycolon", (gchar **)itm_vcolon_xpm, 1, 8, 82, skin, NULL);
			}
		else
			{
			add_text("title", GQMPEG_SKIN_FONT, 127, FALSE, 90, 0, 0, 0, 179, 4, 58, skin, NULL);
			add_number("minute", 2, 90, FALSE, FALSE, 5, 186, ds, skin, NULL);
			add_number("second", 2, 90, TRUE, FALSE, 5, 214, ds, skin, NULL);
			add_item("dummycolon", (gchar **)itm_vcolon_xpm, 1, 8, 210, skin, NULL);
			}

		add_button("play", (gchar **)btn_mplay_xpm, TRUE, TRUE, 4, 317, NULL, skin, NULL);
		add_button("stop", (gchar **)btn_mstop_xpm, TRUE, TRUE, 4, 301, NULL, skin, NULL);
		add_button("next", (gchar **)btn_mnext_xpm, TRUE, FALSE, 4, 280, NULL, skin, NULL);
		add_button("prev", (gchar **)btn_mprev_xpm, TRUE, FALSE, 4, 264, NULL, skin, NULL);

		add_button("playlist", (gchar **)btn_mlist_xpm, TRUE, FALSE, 4, 243, NULL, skin, NULL);

		add_button("skin_iconify", (gchar **)btn_iconify_xpm, TRUE, FALSE, 4, 36, NULL, skin, NULL);
		
		wd = add_button("skin_toggle", (gchar **)btn_vert_xpm, TRUE, FALSE, 4, 20, NULL, skin, NULL);
		if (strcmp(key, "skindata_vert_l") == 0)
			{
			ui_widget_set_data(wd, "data", "skindata_vert_r");
			}
		else
			{
			ui_widget_set_data(wd, "data", "skindata_vert_l");
			}

		add_button("skin_close", (gchar **)btn_exit_xpm, TRUE, FALSE, 4, 4, NULL, skin, NULL);

		finish_simple_window(skin);
		}
	else if (key && strcmp(key, "skindata_alt") == 0)
		{
		/* small mode */
		skin->real_overlay = util_size_pixbuf(gdk_pixbuf_new_from_xpm_data((const char **)back_mini_xpm), TRUE);

		add_text("title", GQMPEG_SKIN_FONT, 90, FALSE, 0, 0, 0, 0, 179, 6, 2, skin, NULL);

		add_button("playlist", (gchar **)btn_mlist_xpm, TRUE, FALSE, 174, 4, NULL, skin, NULL);

		add_button("pause", (gchar **)btn_mpause_xpm, TRUE, TRUE, 195, 4, NULL, skin, NULL);
		add_button("play", (gchar **)btn_mplay_xpm, TRUE, TRUE, 211, 4, NULL, skin, NULL);
		add_button("stop", (gchar **)btn_mstop_xpm, TRUE, TRUE, 227, 4, NULL, skin, NULL);
		add_button("prev", (gchar **)btn_mprev_xpm, TRUE, FALSE, 248, 4, NULL, skin, NULL);
		add_button("next", (gchar **)btn_mnext_xpm, TRUE, FALSE, 264, 4, NULL, skin, NULL);

		add_button("skin_toggle", (gchar **)btn_mode_xpm, TRUE, FALSE, 285, 4, NULL, skin, NULL);
		add_button("skin_iconify", (gchar **)btn_iconify_xpm, TRUE, FALSE, 301, 4, NULL, skin, NULL);
		add_button("exit", (gchar **)btn_exit_xpm, TRUE, FALSE, 317, 4, NULL, skin, NULL);

		add_button("repeat", (gchar **)btn_mrepeat_xpm, TRUE, TRUE, 160, 4, NULL, skin, NULL);
		add_button("shuffle", (gchar **)btn_mshuffle_xpm, TRUE, TRUE, 160, 12, NULL, skin, NULL);

		add_item("minus", (gchar **)itm_mminus_xpm, 2, 100, 9, skin, NULL);
		add_item("time_mode", (gchar **)itm_time_xpm, 3, 100, 14, skin, NULL);

		ds = add_digit("small",(gchar **)dig_lrg_xpm, skin, NULL);

		add_number("minute", 2, 0, FALSE, FALSE, 106, 5, ds, skin, NULL);
		add_number("second", 2, 0, TRUE, FALSE, 134, 5, ds, skin, NULL);

		finish_simple_window(skin);
		}
	else
		{
		/* main mode */
		skin->real_overlay = util_size_pixbuf(gdk_pixbuf_new_from_xpm_data((const char **)back_xpm), TRUE);

		add_text("title", GQMPEG_SKIN_FONT, 192, FALSE, 0, 0, 0, 0, 179, 104, 30, skin, NULL);

		add_button("playlist", (gchar **)btn_playlist_xpm, TRUE, FALSE, 10, 47, NULL, skin, NULL);

		wd = add_button("skin_open", (gchar **)btn_list_xpm, TRUE, FALSE, 11, 27, NULL, skin, NULL);
		ui_widget_set_data(wd, "data", "skindata_playlist");
		wd = add_button("skin_open", (gchar **)btn_vert_xpm, TRUE, FALSE, 27, 27, NULL, skin, NULL);
		ui_widget_set_data(wd, "data", "skindata_vert_r");

		add_button("shuffle", (gchar **)btn_shuffle_xpm, TRUE, TRUE, 59, 27, NULL, skin, NULL);
		add_button("repeat", (gchar **)btn_repeat_xpm, TRUE, TRUE, 59, 44, NULL, skin, NULL);
		add_button("loop_ab", (gchar **)btn_ab_xpm, TRUE, TRUE, 59, 61, NULL, skin, NULL);

		add_button("pause", (gchar **)btn_pause_xpm, TRUE, TRUE, 10, 87, NULL, skin, NULL);
		add_button("play", (gchar **)btn_play_xpm, TRUE, TRUE, 50, 87, NULL, skin, NULL);
		add_button("stop", (gchar **)btn_stop_xpm, TRUE, TRUE, 90, 87, NULL, skin, NULL);
		add_button("prev", (gchar **)btn_prev_xpm, TRUE, FALSE, 130, 97, NULL, skin, NULL);
		add_button("next", (gchar **)btn_next_xpm, TRUE, FALSE, 165, 97, NULL, skin, NULL);

		add_slider("position", (gchar **)slider_pos_xpm,
			    TRUE, TRUE, FALSE, FALSE, 317, 10, 117, skin, NULL,
			    FALSE, 317, 0, 0, FALSE);

		add_button("preset_1", (gchar **)btn_p1_xpm, TRUE, FALSE, 11, 130, NULL, skin, NULL);
		add_button("preset_2", (gchar **)btn_p2_xpm, TRUE, FALSE, 43, 130, NULL, skin, NULL);
		add_button("preset_3", (gchar **)btn_p3_xpm, TRUE, FALSE, 75, 130, NULL, skin, NULL);
		add_button("preset_4", (gchar **)btn_p4_xpm, TRUE, FALSE, 107, 130, NULL, skin, NULL);
		add_button("preset_5", (gchar **)btn_p5_xpm, TRUE, FALSE, 139, 130, NULL, skin, NULL);
		add_button("preset_6", (gchar **)btn_p6_xpm, TRUE, FALSE, 171, 130, NULL, skin, NULL);
		add_button("preset_7", (gchar **)btn_p7_xpm, TRUE, FALSE, 203, 130, NULL, skin, NULL);
		add_button("preset_8", (gchar **)btn_p8_xpm, TRUE, FALSE, 235, 130, NULL, skin, NULL);
		add_button("preset_9", (gchar **)btn_p9_xpm, TRUE, FALSE, 267, 130, NULL, skin, NULL);
		add_button("preset_10", (gchar **)btn_p10_xpm, TRUE, FALSE, 299, 130, NULL, skin, NULL);

		add_button("config", (gchar **)btn_config_xpm, TRUE, FALSE, 311, 97, NULL, skin, NULL);
		add_button("mixer", (gchar **)btn_mixer_xpm, TRUE, FALSE, 311, 75, NULL, skin, NULL);
		add_slider("volume", (gchar **)slider_vol_xpm,
			   TRUE, TRUE, TRUE, TRUE, 40, 315, 30, skin, NULL,
			   FALSE, 40, 0, 0, FALSE);

		add_button("time_total", (gchar **)btn_time_xpm, TRUE, TRUE, 103, 50, NULL, skin, NULL);
		add_button("time_remaining", (gchar **)btn_minus_xpm, TRUE, TRUE, 103, 62, NULL, skin, NULL);
		add_button("repeat_1", (gchar **)btn_repeat_1_xpm, TRUE, TRUE, 195, 48, NULL, skin, NULL);

		wd = add_button("skin_toggle", (gchar **)btn_mode_xpm, TRUE, FALSE, 285, 4, NULL, skin, NULL);
		ui_widget_set_data(wd, "data", "skindata_alt");
		add_button("skin_iconify", (gchar **)btn_iconify_xpm, TRUE, FALSE, 301, 4, NULL, skin, NULL);
		add_button("exit", (gchar **)btn_exit_xpm, TRUE, FALSE, 317, 4, NULL, skin, NULL);

		add_item("stereo", (gchar **)itm_stereo_xpm, 3, 233, 93, skin, NULL);

		ds = add_digit("small",(gchar **)dig_sml_xpm, skin, NULL);
		dl = add_digit("large",(gchar **)dig_lrg_xpm, skin, NULL);

		add_number("hour", 2, 0, FALSE, FALSE, 112, 55, dl, skin, NULL);
		add_number("minute", 2, 0, TRUE, FALSE, 140, 55, dl, skin, NULL);
		add_number("second", 2, 0, TRUE, FALSE, 169, 55, dl, skin, NULL);

		add_number("song", 4, 0, FALSE, FALSE, 206, 55, dl, skin, NULL);
		add_number("song_total", 3, 0, FALSE, FALSE, 260, 55, dl, skin, NULL);

		add_number("input_rate", 3, 0, FALSE, FALSE, 214, 75, ds, skin, NULL);
		add_number("input_hz", 2, 0, FALSE, FALSE, 263, 75, ds, skin, NULL);
		add_number("cpu", 2, 0, FALSE, FALSE, 263, 91, ds, skin, NULL);

		add_item("load", (gchar **)itm_load_xpm, 6, 252, 48, skin, NULL);
		add_item("time_mode", (gchar **)itm_live_xpm, 3, 172, 49, skin, NULL);

		finish_simple_window(skin);
		}
	
	/* focus */
	add_focus(skin);
	double_focus(skin);

	return skin;
}

SkinData *skin_load_default_flyby_cb(UIData *ui, const gchar *key, gpointer data)
{
	SkinData *skin;

	skin = skin_new();

	skin->real_overlay = util_size_pixbuf(gdk_pixbuf_new_from_xpm_data((const char **)flyby_back_xpm), TRUE);

	add_text("title", GQMPEG_SKIN_FONT, 236, FALSE, 0, 0, 0, 0, 179, 7, 5, skin, NULL);
	add_text("artist", GQMPEG_SKIN_FONT, 206, FALSE, 0, 0, 0, 0, 179, 7, 23, skin, NULL);

	add_button("next", (gchar **)btn_snext_xpm, TRUE, FALSE, 231, 30, NULL, skin, NULL);
	add_button("prev", (gchar **)btn_sprev_xpm, TRUE, FALSE, 218, 30, NULL, skin, NULL);

	skin->width = 250;
	skin->height = 48;

	skin->has_border = TRUE;
	skin->border_left = 8;
	skin->border_right = 8;
	skin->border_top = 8;
	skin->border_bottom = 8;

	/* this is merely to that the skin editor has sane values for these */
	skin->width_def = skin->width_min = skin->width_max = skin->width;
	skin->height_def = skin->height_min = skin->height_max = skin->height;

	return skin;
}

SkinData *skin_load_default_menu_cb(UIData *ui, const gchar *key, gpointer data)
{
	SkinData *skin;
	ListData *ld;
	WidgetData *wd;

	skin = skin_new();

	skin->real_overlay = util_size_pixbuf(gdk_pixbuf_new_from_xpm_data((const char **)menu_back_xpm), TRUE);
	skin->has_border = TRUE;
	skin->border_left = 6;
	skin->border_right = 6;
	skin->border_top = 6;
	skin->border_bottom = 6;
	skin->width = 25;
	skin->height = 24;

	ld = list_new(gdk_pixbuf_new_from_xpm_data((const char **)menu_list_back_xpm),
		      3, 3, 19, 18, TRUE, 2,
		      0, 0, 0, 0, FALSE);
	list_image_row(ld, gdk_pixbuf_new_from_xpm_data((const char **)menu_list_row_xpm),
		       TRUE, TRUE, 2, 2, FALSE, NULL);
	list_image_row_flag(ld, gdk_pixbuf_new_from_xpm_data((const char **)menu_list_icons_xpm), 4, 0);
	list_set_font(ld, GQMPEG_SKIN_FONT, NULL, FALSE, 0, 0, 0, 179);

	wd = list_register(skin, ld, "menu", NULL);
	
	if (key && strcmp(key, "skindata_submenu") == 0)
		{
		list_set_column_attributes(ld, 0, 9, UI_LIST_COLUMN_SIZE_FIXED, "icon");
		list_set_column_attributes(ld, 1, 16, 0, "text");
		}
	else
		{
		list_set_column_attributes(ld, 0, 16, 0, "text");
		list_set_column_attributes(ld, 1, 9, UI_LIST_COLUMN_SIZE_FIXED, "icon");
		ld->flag_column = 1;
		ld->row_border_left += 3;
		list_set_column_justify(ld, TRUE);

		ui_widget_set_data(wd, "data", "skindata_submenu");
		}

	double_window(skin);

	/* this is merely to that the skin editor has sane values for these */
	skin->width_def = skin->width_min = skin->width_max = skin->width;
	skin->height_def = skin->height_min = skin->height_max = skin->height;

	/* focus */
	skin->focus_overlay = util_size_pixbuf(gdk_pixbuf_new_from_xpm_data((const char **)focus_xpm), TRUE);
	skin->focus_box_alpha = 128;
	skin->focus_has_border = TRUE;
	skin->focus_border_left = 5;
	skin->focus_border_right = 5;
	skin->focus_border_bottom = 5;
	skin->focus_border_top = 5;

	double_focus(skin);

	return skin;
}

