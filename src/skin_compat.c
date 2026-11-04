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
#include "skin_compat.h"

#include "ui2_skin.h"
#include "ui2_button.h"
#include "ui2_dial.h"
#include "ui2_item.h"
#include "ui2_number.h"
#include "ui2_slider.h"
#include "ui2_text.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_fileops.h"


static gint parse_to_true_false(const gchar *text)
{
	if (!text) return FALSE;

	return (text[0] == 't' ||
		text[0] == 'T' ||
		strcasecmp(text, "true") == 0 ||
		text[0] == '1');
}

static gint parse_to_number(const gchar *text)
{
	if (!text) return 0;

	return strtol(text, NULL, 10);
}

static WidgetData *parse_button(gchar **vector, gint length, const gchar *dirpath,
				gchar *key, const gchar *id, gint can_have_light, SkinData *skin, gint edit)
{
	ButtonData *button;
	gint p;
	gchar *filename;
	gint prelight;
	gint light;
	gint x;
	gint y;
	gchar *clip_filename;
	WidgetData *wd = NULL;

	if (strcmp(key, vector[0]) != 0 || length < 5 + can_have_light) return NULL;
	
	filename = g_strconcat(dirpath, "/", vector[1], NULL);

	prelight = parse_to_true_false(vector[2]);

	p = 3;

	if (can_have_light)
		{
		light = parse_to_true_false(vector[p]);
		p++;
		}
	else
		{
		light = FALSE;
		}

	x = parse_to_number(vector[p]);
	p++;
	y = parse_to_number(vector[p]);
	p++;

	if (vector[p] != NULL)
		{
		clip_filename = g_strconcat(dirpath, "/", vector[p], NULL);
		}
	else
		{
		clip_filename = NULL;
		}

	button = button_new_from_file(filename, x, y, prelight, light, clip_filename);

	if (button)
		{
		wd = button_register(skin, button, id, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			ui_widget_set_data(wd, "clip_mask", clip_filename);
			}
		}
	else
		{
		printf(_("Unable to create button from: %s\n"), filename);
		}

	g_free(filename);
	g_free(clip_filename);

	return wd;
}

static void parse_digit(gchar **vector, gint length, const gchar *dirpath,
			gchar *key, const gchar *id, SkinData *skin, gint edit)
{
	DigitData *digit;
	gchar *filename;

	if (strcmp(key, vector[0]) != 0 || length < 2) return;

	filename = g_strconcat(dirpath, "/", vector[1], NULL);

	digit = digit_new_from_file(filename);

	if (digit)
		{
		WidgetData *wd;

		wd = digit_register(skin, digit, id, NULL);
		digit_unref(digit);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			}
		}

	g_free(filename);
}

static void parse_number(gchar **vector, gint length, const gchar *dirpath,
			 gchar *key, const gchar *id, gint default_length, gint zeros, SkinData *skin, gint edit)
{
	NumberData *number;
	DigitData *digit;

	gint p;
	gchar *name;
	gint num_length;
	gint center;
	gint x;
	gint y;

	gchar *filename = NULL;
	gchar *digit_id = NULL;

	if (strcmp(key, vector[0]) != 0 || length < 4) return;

	name = vector[1];

	p = 2;

	if (length > 4)
		{
		num_length = parse_to_number(vector[p]);
		p++;
		}
	else
		{
		num_length = default_length;
		}

	if (length > 5)
		{
		center = parse_to_true_false(vector[p]);
		p++;
		}
	else
		{
		center = FALSE;
		}

	x = parse_to_number(vector[p]);
	p++;
	y = parse_to_number(vector[p]);
	p++;
	
	if (strcmp(name, "Large") == 0)
		{
		WidgetData *wd;

		digit_id = "large";
		wd = skin_widget_get_by_key(skin, digit_id, digit_type_id());
		if (!wd)
			{
			printf(_("Large digit specified, but not defined\n"));
			return;
			}
		else
			{
			digit = wd->widget;
			digit_ref(digit);
			}
		}
	else if (strcmp(name, "Small") == 0)
		{
		WidgetData *wd;

		digit_id = "small";
		wd = skin_widget_get_by_key(skin, digit_id, digit_type_id());

		if (!wd)
			{
			printf(_("Small digit specified, but not defined\n"));
			return;
			}
		else
			{
			digit = wd->widget;
			digit_ref(digit);
			}
		
		}
	else
		{
		filename = g_strconcat(dirpath, "/", name, NULL);
		digit = digit_new_from_file(filename);
		if (!digit)
			{
			printf(_("Unable to load number file: %s"), filename);
			g_free(filename);
			return;
			}
		}

	number = number_new(digit, x, y, num_length, zeros, center, 0);
	digit_unref(digit);

	if (number)
		{
		WidgetData *wd;

		wd = number_register(skin, number, id, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			ui_widget_set_data(wd, "digit_id", digit_id);
			}
		}

	g_free(filename);
}

static void parse_item(gchar **vector, gint length, gchar const *dirpath,
		       gchar *key, const gchar *id, gint sections, SkinData *skin, gint edit)
{
	ItemData *item;
	gint p;
	gchar *filename;
	gint x;
	gint y;

	if (strcmp(key, vector[0]) != 0 || length < 4 + (sections == 0)) return;

	filename = g_strconcat(dirpath, "/", vector[1], NULL);

	p = 2;

	if (sections == 0)
		{
		sections = parse_to_number(vector[p]);
		p++;
		}

	x = parse_to_number(vector[p]);
	p++;
	y = parse_to_number(vector[p]);
	p++;

	item = item_new_from_file(filename, x, y, sections);

	if (item)
		{
		WidgetData *wd;

		wd = item_register(skin, item, id, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			}
		}
	else
		{
		printf(_("Unable to load item file: %s\n"), filename);
		}	

	g_free(filename);
}

static void parse_text(gchar **vector, gint length, const gchar *dirpath,
		       gchar *key, const gchar *id, SkinData *skin, gint edit)
{
	TextData *text;
	gint p;
	gchar *filename;
	gint text_length;
	gint x;
	gint y;
	gint extended = FALSE;

	if (strcmp(key, vector[0]) != 0 || length < 5) return;

	filename = g_strconcat(dirpath, "/", vector[1], NULL);

	text_length = parse_to_number(vector[2]);

	p = 3;

	if (length > 5)
		{
		extended = parse_to_true_false(vector[p]);
		p++;
		}

	x = parse_to_number(vector[p]);
	p++;
	y = parse_to_number(vector[p]);
	p++;


	text = text_new_from_file(filename, x, y, text_length, FALSE, 0, extended);

	if (text)
		{
		WidgetData *wd;

		/* text length is now specified in pixels instead of characters  */
		text->width = text_length * text->font->char_width;
		gdk_pixbuf_unref(text->pixbuf);
		text->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, text->width, text->height);

		wd = text_register(skin, text, id, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			}
		}
	else
		{
		printf(_("Unable to load font file: %s\n"), filename);
		}

	g_free(filename);
}

static void parse_slider(gchar **vector, gint length, const gchar *dirpath,
			 gchar *key, const gchar *id, const gchar *item_key, SkinData *skin, gint edit)
{
	SliderData *slider;
	gint p;
	gchar *filename;
	gint prelight;
	gint vertical = 0;
	gint reversed = 0;
	gint slider_length;
	gint x;
	gint y;
	
	if (strcmp(key, vector[0]) != 0 || length < 6) return;

	filename = g_strconcat(dirpath, "/", vector[1], NULL);

	prelight = parse_to_true_false(vector[2]);
	p = 3;

	if (length > 6)
		{
		vertical = parse_to_true_false(vector[p]);
		p++;
		}
	if (length > 7)
		{
		reversed = parse_to_true_false(vector[p]);
		p++;
		}

	slider_length = parse_to_number(vector[p]);
	p++;
	x = parse_to_number(vector[p]);
	p++;
	y = parse_to_number(vector[p]);
	p++;

	slider = slider_new_from_file(filename, x, y, slider_length,
				      vertical, reversed, TRUE, prelight, item_key);

	if (slider)
		{
		WidgetData *wd;

		wd = slider_register(skin, slider, id, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			}
		}
	else
		{
		printf(_("Unable to create slider from: %s\n"), filename);
		}

	g_free(filename);
}

static void parse_dial(gchar **vector, gint length, const gchar *dirpath,
		       gchar *key, const gchar *id, SkinData *skin, gint edit)
{
	DialData *dial;
	gchar *filename;
	gint has_press;
	gint has_prelight;
	gint reversed;
	gint angle_start;
	gint angle_end;
	gint offset_x;
	gint offset_y;
	gint center_x;
	gint center_y;
	gint x;
	gint y;
	gint w;
	gint h;
	gchar *clip_filename;
	
	if (strcmp(key, vector[0]) != 0 || length < 15) return;

	filename = g_strconcat(dirpath, "/", vector[1], NULL);

	has_press = parse_to_true_false(vector[2]);
	has_prelight = parse_to_true_false(vector[3]);

	reversed = parse_to_true_false(vector[4]);

	angle_start = parse_to_number(vector[5]);
	angle_end = parse_to_number(vector[6]);

	offset_x = parse_to_number(vector[7]);
	offset_y = parse_to_number(vector[8]);

	center_x = parse_to_number(vector[9]);
	center_y = parse_to_number(vector[10]);

	x = parse_to_number(vector[11]);
	y = parse_to_number(vector[12]);

	w = parse_to_number(vector[13]);
	h = parse_to_number(vector[14]);

	if (length > 15)
		{
		clip_filename = g_strconcat(dirpath, "/", vector[15], NULL);
		}
	else
		{
		clip_filename = NULL;
		}

	dial = dial_new_from_file(filename, x, y,
				  center_x, center_y,
				  angle_start, angle_end, offset_x, offset_y,
				  has_press, has_prelight, reversed,
				  w, h, clip_filename, NULL);

	if (dial)
		{
		WidgetData *wd;

		wd = dial_register(skin, dial, id, NULL);

		if (edit)
			{
			ui_widget_set_data(wd, "image", filename);
			ui_widget_set_data(wd, "clip_mask", clip_filename);
			}
		}
	else
		{
		printf(_("Unable to create dial from: %s\n"), filename);
		}

	g_free(filename);
	g_free(clip_filename);
}

static void compat_number_zeros(SkinData *s, const gchar *skey, const gchar *key)
{
	if (skin_widget_get_by_key(s, skey, number_type_id()) != NULL)
		{
		WidgetData *wd;

		wd = skin_widget_get_by_key(s, key, number_type_id());
		if (wd)
			{
			NumberData *number = wd->widget;
			number->zeros = TRUE;
			}
		}
}

static void compat_slider_with_item(SkinData *s, const gchar *slider_key, const gchar *item_key)
{
	WidgetData *swd;

	swd = skin_widget_get_by_key(s, slider_key, slider_type_id());
	if (swd)
		{
		SliderData *slider;
		WidgetData *wd;

		slider = swd->widget;
		wd = skin_widget_get_by_key(s, item_key, item_type_id());
		if (wd)
			{
			g_free(wd->key);
			wd->key = g_strdup(slider->item_key);
			}
		}
}

SkinData *skin_parse_v1(const gchar *skin_path, const gchar *datafile, gint edit)
{
	SkinData *s;
	FILE *f;
	char buf[1024];
	WidgetData *wd;
	gchar *pathl;

	pathl = path_from_utf8(datafile);
	f = fopen(pathl, "r");
	g_free(pathl);

	if (!f)
		{
		/* no file has been created yet */
		if (debug_mode) printf("Unable to open skin data file: %s\n", datafile);
		return NULL;
		}

	s = skin_new();

	while (fgets(buf,1024,f))
		{
		gchar **vector;
		gint length;
		gchar *p;

		if (buf[0]=='#') continue;
		if (buf[0]=='\n') continue;

		/* remove new line chars */
		p = buf;
		while (*p != '\0')
			{
			if (*p == '\n') *p = '\0';
			p++;
			}

		vector = g_strsplit(buf, " ", 16);

		if (!vector) continue;

		length = 0;
		while (vector[length] != NULL) length++;

		if (length < 2)
			{
			g_strfreev(vector);
			continue;
			}

		/* key - remove ':', added '=' now too! */
		p = vector[0];
		while (*p != '\0')
			{
			if (*p == ':' || *p == '=') *p = '\0';
			p++;
			}

		/* background */
		if (strcmp(vector[0], "Background") == 0 && length > 1 && !s->real_overlay)
			{
			gchar *filename;

			filename = g_strconcat(skin_path, "/", vector[1], NULL);

			s->real_overlay = util_size_pixbuf(util_pixbuf_new_from_file(filename), TRUE);
			if (s->real_overlay)
				{
				s->width = gdk_pixbuf_get_width(s->real_overlay);
				s->height = gdk_pixbuf_get_height(s->real_overlay);
				if (edit) s->background_filename = g_strdup(filename);
				}

			g_free(filename);
			}
		else if (strcmp(vector[0], "Mask") == 0 && length > 1 && !s->mask)
			{
			gchar *filename;

			filename = g_strconcat(skin_path, "/", vector[1], NULL);

			s->mask = util_size_pixbuf(util_pixbuf_new_from_file(filename), FALSE);
			if (s->mask && edit) s->background_mask_filename = g_strdup(filename);

			g_free(filename);
			}
		else if (strcmp(vector[0], "Transparent") == 0 && length > 1)
			{
			s->transparent = parse_to_true_false(vector[1]);
			}

		/* buttons */
		parse_button(vector, length, skin_path, "Play_Button", "play", TRUE, s, edit);
		parse_button(vector, length, skin_path, "Stop_Button", "stop", TRUE, s, edit);
		parse_button(vector, length, skin_path, "Pause_Button", "pause", TRUE, s, edit);
		parse_button(vector, length, skin_path, "Shuffle_Button", "shuffle", TRUE, s, edit);
		parse_button(vector, length, skin_path, "Repeat_Button", "repeat", TRUE, s, edit);

		parse_button(vector, length, skin_path, "Next_Button", "next", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Prev_Button", "prev", FALSE, s, edit);
		parse_button(vector, length, skin_path, "FF_Button", "ff", FALSE, s, edit);
		parse_button(vector, length, skin_path, "RW_Button", "rw", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Config_Button", "config", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Playlist_Button", "playlist", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Exit_Button", "exit", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Iconify_Button", "iconify", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Mixer_Button", "mixer", FALSE, s, edit);
		wd = parse_button(vector, length, skin_path, "Alt_Skin_Button", "skin_toggle", FALSE, s, edit);
		if (wd && filename_from_path((char *)datafile) && strcmp(filename_from_path((char *)datafile), "skindata") == 0)
			{
			ui_widget_set_data(wd, "data", "skindata_alt");
			}

		/* large / small digits */
		parse_digit(vector, length, skin_path, "Digit_Large_Default", "large", s, edit);
		parse_digit(vector, length, skin_path, "Digit_Small_Default", "small", s, edit);

		/* numbers */
		parse_number(vector, length, skin_path, "Hour_Number", "hour", 2, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Minute_Number", "minute", 2, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Second_Number", "second", 2, TRUE, s, edit);
		parse_number(vector, length, skin_path, "Hour_Total_Number", "playlist_hour", 2, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Minute_Total_Number", "playlist_minute", 2, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Second_Total_Number", "playlist_second", 2, TRUE, s, edit);
		parse_number(vector, length, skin_path, "Song_Number", "song", 3, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Total_Number", "song_total", 3, FALSE, s, edit);
		parse_number(vector, length, skin_path, "In_Rate_Number", "input_rate", 3, FALSE, s, edit);
		parse_number(vector, length, skin_path, "In_Hz_Number", "input_hz", 2, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Out_Bits_Number", "output_bits", 2, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Out_Hz_Number", "output_hz", 2, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Frame_Number", "frame", 5, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Frame_Total_Number", "frame_total", 5, FALSE, s, edit);
		parse_number(vector, length, skin_path, "CPU_Number", "cpu", 2, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Song_Minute_Number", "song_minute", 2, FALSE, s, edit);
		parse_number(vector, length, skin_path, "Song_Second_Number", "song_second", 2, TRUE, s, edit);

		/* items */
		parse_item(vector, length, skin_path, "Stereo_Item", "stereo", 3, s, edit);
		parse_item(vector, length, skin_path, "Shuffle_Item", "shuffle", 2, s, edit);
		parse_item(vector, length, skin_path, "Repeat_Item", "repeat", 2, s, edit);
		parse_item(vector, length, skin_path, "Minus_Item", "minus", 2, s, edit);
		parse_item(vector, length, skin_path, "Total_Item", "time_mode", 3, s, edit);
		parse_item(vector, length, skin_path, "Mpegversion_Item", "mpegversion", 3, s, edit);
		parse_item(vector, length, skin_path, "Mpeglayer_Item", "mpeglayer", 4, s, edit);
		parse_item(vector, length, skin_path, "Mpegmode_Item", "mpegmode", 5, s, edit);
		parse_item(vector, length, skin_path, "Status_Item", "status", 3, s, edit);

		/* font / text */
		parse_text(vector, length, skin_path, "Title", "title", s, edit);
		parse_text(vector, length, skin_path, "Artist", "artist", s, edit);
		parse_text(vector, length, skin_path, "Album", "album", s, edit);
		parse_text(vector, length, skin_path, "Genre", "genre", s, edit);

		/* position slider & item */
		parse_item(vector, length, skin_path, "Position_Item", "position", 0, s, edit);
		parse_slider(vector, length, skin_path, "Position_Slider", "position", "compat_position", s, edit);

		/* volume & balance */
		parse_button(vector, length, skin_path, "Volume_Up_Button", "volume_up", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Volume_Down_Button", "volume_down", FALSE, s, edit);
		parse_item(vector, length, skin_path, "Volume_Item", "volume", 0, s, edit);
		parse_slider(vector, length, skin_path, "Volume_Slider", "volume", "compat_volume", s, edit);

		parse_button(vector, length, skin_path, "Balance_Left_Button", "balance_left", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Balance_Right_Button", "balance_right", FALSE, s, edit);
		parse_item(vector, length, skin_path, "Balance_Item", "balance", 0, s, edit);
		parse_slider(vector, length, skin_path, "Balance_Slider", "balance", "compat_balance", s, edit);

		parse_button(vector, length, skin_path, "Preset_1_Button", "preset_1", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Preset_2_Button", "preset_2", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Preset_3_Button", "preset_3", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Preset_4_Button", "preset_4", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Preset_5_Button", "preset_5", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Preset_6_Button", "preset_6", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Preset_7_Button", "preset_7", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Preset_8_Button", "preset_8", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Preset_9_Button", "preset_9", FALSE, s, edit);
		parse_button(vector, length, skin_path, "Preset_10_Button", "preset_10", FALSE, s, edit);

		parse_dial(vector, length, skin_path, "Volume_Dial", "volume", s, edit);
		parse_dial(vector, length, skin_path, "Balance_Dial", "balance", s, edit);
		parse_dial(vector, length, skin_path, "Position_Dial", "position", s, edit);

		parse_item(vector, length, skin_path, "Load_Item", "load", 0, s, edit);
		parse_button(vector, length, skin_path, "Time_Remaining_Button", "minus", TRUE, s, edit);
		parse_button(vector, length, skin_path, "Time_Total_Button", "total_mode", TRUE, s, edit);

		g_strfreev(vector);
		}

	fclose(f);

	if (!s->real_overlay)
		{
		printf(_("No background image from: %s\n"), skin_path);
		skin_free(s);
		return NULL;
		}

	s->width_def = s->width_min = s->width_max = s->width;
	s->height_def = s->height_min = s->height_max = s->height;

	/* if hours are defined, change minutes to have leading 0's */
	compat_number_zeros(s, "hour", "minute");
	compat_number_zeros(s, "playlist_hour", "playlist_minute");

	compat_slider_with_item(s, "position", "position");
	compat_slider_with_item(s, "volume", "volume");
	compat_slider_with_item(s, "balance", "balance");

	return s;
}

