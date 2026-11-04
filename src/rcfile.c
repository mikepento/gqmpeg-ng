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
#include "rcfile.h"

#include "players.h"
#include "ui_fileops.h"

gchar *quoted_value(const gchar *text)
{
	const gchar *ptr;
	gint c = 0;
	gint l = strlen(text);

	if (l == 0) return NULL;

	while (c < l && text[c] !='"') c++;
	if (text[c] == '"')
		{
		gint e;
		c++;
		ptr = text + c;
		e = c;
		while (e < l && text[e] !='"') e++;
		if (text[e] == '"')
			{
			if (e - c > 0)
				{
				return g_strndup(ptr, e - c);
				}
			}
		}
	else
		/* for compatibility with older formats (<0.3.7)
		 * read a line without quotes too */
		{
		c = 0;
		while (c < l && text[c] !=' ' && text[c] !=8 && text[c] != '\n') c++;
		if (c != 0)
			{
			return g_strndup(text, c);
			}
		}

	return NULL;
}

void write_char_option(FILE *f, const gchar *label, const gchar *text)
{
	if (text)
		fprintf(f,"%s: \"%s\"\n", label, text);
	else
		fprintf(f,"%s: \n", label);
}

gchar *read_char_option(FILE *f, const gchar *option, const gchar *label, const gchar *value, gchar *text)
{
	if (!strcasecmp(option, label))
		{
		g_free(text);
		text = quoted_value(value);
		}
	return text;
}

void write_int_option(FILE *f, const gchar *label, gint n)
{
	fprintf(f,"%s: %d\n", label, n);
}

gint read_int_option(FILE *f, const gchar *option, const gchar *label, const gchar *value, gint n)
{
	if (!strcasecmp(option, label))
		{
		n = strtol(value,NULL,10);
		}
	return n;
}

void write_bool_option(FILE *f, const gchar *label, gint n)
{
	fprintf(f,"%s: ", label);
	if (n) fprintf(f,"true\n"); else fprintf(f,"false\n");
}

gint read_bool_option(FILE *f, const gchar *option, const gchar *label, const gchar *value, gint n)
{
	if (!strcasecmp(option, label))
		{
		if (!strcasecmp(value, "true"))
			n = TRUE;
		else
			n = FALSE;
		}
	return n;
}

void save_options(void)
{
	FILE *f;
	gchar *rc_path;
	gchar *rc_pathl;
	gchar *rcdir;
	gint i;

	rcdir = g_strconcat(homedir(), "/", GQMPEG_RC_DIR, NULL);
	if (!isdir(rcdir))
		{
		printf(_("Creating gqmpeg dir:%s\n"), rcdir);
		if (!mkdir_utf8(rcdir, 0755))
			{
			printf(_("Could not create dir:%s\n"), rcdir);
			printf(_("Config not saved!\n"));
			g_free(rcdir);
			return;
			}
		}

	rc_path = g_strconcat(rcdir, "/", GQMPEG_RC_FILE, NULL);
	g_free(rcdir);

	rc_pathl = path_from_utf8(rc_path);
	f = fopen(rc_pathl, "w");
	g_free(rc_pathl);
	if (!f)
		{
		printf(_("error saving config file: %s\n"), rc_path);
		g_free(rc_path);
		return;
		}

	fprintf(f,"######################################################################\n");
	fprintf(f,"#                         GQmpeg config file         version %7s #\n", VERSION);
	fprintf(f,"#                                                                    #\n");
	fprintf(f,"#  Everything in this file can be changed in the option dialogs.     #\n");
	fprintf(f,"#      (so there should be no need to edit this file by hand)        #\n");
	fprintf(f,"#                                                                    #\n");
	fprintf(f,"######################################################################\n");

	fprintf(f,"\n##### General Options #####\n\n");

	write_bool_option(f, "read_file_information", read_file_information);

	fprintf(f,"\n");
	write_bool_option(f, "smart_window_placement", slik_smart_placement);
	write_bool_option(f, "remember_window_position", slik_remember_position);
	write_bool_option(f, "wm_decorations", wm_decorations);
	write_bool_option(f, "double_size", slik_double_size);
	fprintf(f,"\n");
	write_char_option(f, "mixer_command", mixer_command);
	write_int_option(f, "mixer_device", mixer_device_id);
	fprintf(f,"\n");
	write_char_option(f, "skin_path", skin_default_path);
	write_char_option(f, "skin_mode", skin_mode_key);
	write_bool_option(f, "show_extra_winamp_info", show_extra_winamp_info);
	fprintf(f,"\n");
	write_bool_option(f, "prev_includes_current_song", prev_includes_current);
	fprintf(f,"\n");
	write_bool_option(f, "title_display_scrolls_always", title_scrolls_always);
	write_bool_option(f, "title_display_show_extension", title_show_extension);
	write_bool_option(f, "title_display_convert_underscores", title_convert_underscores);
	fprintf(f,"\n");
	write_bool_option(f, "allow_ipc", allow_ipc);
	fprintf(f,"\n");
	write_int_option(f, "delay_between_songs", change_delay);
	fprintf(f,"\n");
	write_bool_option(f, "flyby_enabled", flyby_enabled);
	write_int_option(f, "flyby_location", flyby_location);
	fprintf(f,"\n");
	write_bool_option(f, "keyboard_focus_enabled", slik_focus_enable);
	write_bool_option(f, "skinned_menus_enabled", skinned_menus_enable);

	fprintf(f,"\n##### Startup Options #####\n\n");

	fprintf(f,"initial_playlist_mode: ");
	if (initial_playlist_mode == PLAYLIST_MODE_EMPTY) fprintf(f,"empty\n");
	if (initial_playlist_mode == PLAYLIST_MODE_SESSION) fprintf(f,"save_session\n");
	if (initial_playlist_mode == PLAYLIST_MODE_FILE) fprintf(f,"open_file\n");

	write_char_option(f, "initial_playlist", initial_playlist_pathname);
	fprintf(f,"\n");
	write_bool_option(f, "play_on_startup" , start_playing_on_startup);
	write_bool_option(f, "show_playlist", show_playlist_on_startup);
	fprintf(f,"\n");
	write_bool_option(f, "enable_startup_directory", initial_directory_enable);
	write_char_option(f, "startup_directory", initial_directory_path);

	/* save player module data */
	player_modules_config_save(f);

	fprintf(f,"\n##### Playlist options #####\n\n");

	write_bool_option(f, "obey_mode_in_playlist", obey_mode_in_playlist);
	write_bool_option(f, "save_mode_in_playlist", save_mode_in_playlist);
	write_bool_option(f, "show_hidden_files", show_dot_files);
	write_bool_option(f, "show_all_files", disable_filtering);
	write_bool_option(f, "add_single_drops_to_playlist", drop_single_to_playlist);
	write_bool_option(f, "play_next_on_error", play_next_on_error);

	fprintf(f,"\n##### Presets #####\n\n");

	write_bool_option(f, "play_presets", play_presets);

	for (i=0; i<10 ; i++)
		{
		fprintf(f,"preset_%d_name: ", i + 1);
		if (preset_name[i]) fprintf(f,"\"%s\"", preset_name[i]);
		fprintf(f, "\n");
		fprintf(f,"preset_%d_file: ", i + 1);
		if (preset_file[i]) fprintf(f,"\"%s\"", preset_file[i]);
		fprintf(f,"\n\n");
		}

	fprintf(f,"\n##### Current state #####\n\n");

	write_int_option(f, "window_position_x", window_pos_x);
	write_int_option(f, "window_position_y", window_pos_y);
	fprintf(f,"\n");
	write_int_option(f, "window_list_x", window_list_x);
	write_int_option(f, "window_list_y", window_list_y);
	write_int_option(f, "window_list_width", window_list_w);
	write_int_option(f, "window_list_height", window_list_h);
	write_int_option(f, "window_list_divider_dir", window_list_div_dir);
	write_int_option(f, "window_list_divider_mid", window_list_div_mid);
	write_char_option(f, "window_list_columns", window_list_columns);
	fprintf(f,"\n");
	write_bool_option(f, "repeat", repeat_mode);
	write_bool_option(f, "shuffle", shuffle_mode);
	fprintf(f,"\n");
	write_bool_option(f, "show_time_remaining", show_remaining_time);
	write_bool_option(f, "show_time_total", show_total_time);
	write_int_option(f, "initial_playlist_position", initial_playlist_position);

	fprintf(f,"\n##### Transparency Options #####\n\n");

	write_bool_option(f, "colorshift_enable", slik_colorshift_on);
	write_int_option(f, "colorshift_red", slik_colorshift_r);
	write_int_option(f, "colorshift_green", slik_colorshift_g);
	write_int_option(f, "colorshift_blue", slik_colorshift_b);
	write_int_option(f, "colorshift_alpha", slik_colorshift_a);
	fprintf(f,"\n");
	write_bool_option(f, "transparency_force", slik_transparency_force);
	write_int_option(f, "transparency_force_alpha", slik_transparency_force_a);

	fprintf(f,"\n");
	fprintf(f,"######################################################################\n");
	fprintf(f,"#                      end of GQmpeg config file                     #\n");
	fprintf(f,"######################################################################\n");

	fclose(f);

	g_free(rc_path);
}

void load_options(void)
{
	FILE *f;
	gchar *rc_path;
	gchar *rc_pathl;
	gchar s_buf[1024];
	gchar *s_buf_ptr;
	gchar option[1024];
	gchar value[1024];
	gchar value_all[1024];
	gint c,l,i;

	rc_path = g_strconcat(homedir(), "/", GQMPEG_RC_DIR, "/", GQMPEG_RC_FILE, NULL);

	rc_pathl = path_from_utf8(rc_path);
	f = fopen(rc_pathl, "r");
	g_free(rc_pathl);
	if (!f)
		{
		g_free(rc_path);
		return;
		}

	while (fgets(s_buf,1024,f))
		{
		if (s_buf[0]=='#') continue;
		if (s_buf[0]=='\n') continue;
		c = 0;
		l = strlen(s_buf);
		while (s_buf[c] != ':' && c < l) c++;
		if (c >= l) continue;
		s_buf[c] = '\0';
		c++;
		while (s_buf[c] == ' ' && c < l) c++;
		while (s_buf[c] == 8 && c < l) c++;
		while (s_buf[c] == ' ' && c < l) c++;
		s_buf_ptr = s_buf + c;
		strncpy(value_all, s_buf_ptr, sizeof(value_all));
		while (s_buf[c] != 8 && s_buf[c] != ' ' && s_buf[c] != '\n' && c < l) c++;
		s_buf[c] = '\0';
		strncpy(option, s_buf, sizeof(option));
		strncpy(value, s_buf_ptr, sizeof(value));

	/* general options */

		read_file_information = read_bool_option(f, option,
			"read_file_information", value, read_file_information);
		/* for backwards compatibility */
		read_file_information = read_bool_option(f, option,
			"playlist_accounting", value, read_file_information);

		slik_smart_placement = read_bool_option(f, option,
			"smart_window_placement", value, slik_smart_placement);
		slik_remember_position = read_bool_option(f, option,
			"remember_window_position", value, slik_remember_position);
		wm_decorations = read_bool_option(f, option,
			"wm_decorations", value, wm_decorations);
		slik_double_size = read_bool_option(f, option,
			"double_size", value, slik_double_size);

		mixer_command = read_char_option(f, option,
			"mixer_command", value_all, mixer_command);
		mixer_device_id = read_int_option(f, option,
			"mixer_device", value, mixer_device_id);

		/* compat loading */
		skin_default_path = read_char_option(f, option,
			"default_skin", value_all, skin_default_path);
		skin_default_path = read_char_option(f, option,
			"skin_path", value_all, skin_default_path);

		skin_mode_key = read_char_option(f, option,
			"skin_mode", value_all, skin_mode_key);

		show_extra_winamp_info = read_bool_option(f, option,
			"show_extra_winamp_info", value, show_extra_winamp_info);

		prev_includes_current = read_bool_option(f, option,
			"prev_includes_current_song", value, prev_includes_current);

		title_scrolls_always = read_bool_option(f, option,
			"title_display_scrolls_always", value, title_scrolls_always);
		title_show_extension = read_bool_option(f, option,
			"title_display_show_extension", value, title_show_extension);
		title_convert_underscores = read_bool_option(f, option,
			"title_display_convert_underscores", value, title_convert_underscores);

		allow_ipc = read_bool_option(f, option,
			"allow_ipc", value, allow_ipc);

		change_delay = read_int_option(f, option,
			"delay_between_songs", value, change_delay);

		flyby_enabled = read_bool_option(f, option,
			"flyby_enabled", value, flyby_enabled);
		flyby_location = read_int_option(f, option,
			"flyby_location", value, flyby_location);

		slik_focus_enable = read_bool_option(f, option,
			"keyboard_focus_enabled", value, slik_focus_enable);
		skinned_menus_enable = read_bool_option(f, option,
			"skinned_menus_enabled", value, skinned_menus_enable);

	/* startup options */

		if (!strcasecmp(option,"initial_playlist_mode"))
			{
			if (!strcasecmp(value,"empty")) initial_playlist_mode = PLAYLIST_MODE_EMPTY;
			if (!strcasecmp(value,"save_session")) initial_playlist_mode = PLAYLIST_MODE_SESSION;
			if (!strcasecmp(value,"open_file")) initial_playlist_mode = PLAYLIST_MODE_FILE;
			}

		initial_playlist_pathname = read_char_option(f, option,
			"initial_playlist", value_all, initial_playlist_pathname);

		start_playing_on_startup = read_bool_option(f, option,
			"play_on_startup", value, start_playing_on_startup);
		show_playlist_on_startup = read_bool_option(f, option,
			"show_playlist", value, show_playlist_on_startup);
		initial_directory_enable = read_bool_option(f, option,
			"enable_startup_directory", value, initial_directory_enable);
		initial_directory_path = read_char_option(f, option,
			"startup_directory", value_all, initial_directory_path);

	/* load player module data */
	player_modules_config_load(f, option, value, value_all);

	/* playlist */

		obey_mode_in_playlist = read_bool_option(f, option,
			"obey_mode_in_playlist", value, obey_mode_in_playlist);
		save_mode_in_playlist = read_bool_option(f, option,
			"save_mode_in_playlist", value, save_mode_in_playlist);
		show_dot_files = read_bool_option(f, option,
			"show_hidden_files", value, show_dot_files);
		disable_filtering = read_bool_option(f, option,
			"show_all_files", value, disable_filtering);
		drop_single_to_playlist = read_bool_option(f, option,
			"add_single_drops_to_playlist", value, drop_single_to_playlist);
		play_next_on_error = read_bool_option(f, option,
			"play_next_on_error", value, play_next_on_error);

	/* presets */

		play_presets = read_bool_option(f, option,
			"play_presets", value, play_presets);
		if (!strncasecmp(option, "preset_", 7))
			{
			for (i=0; i<10; i++)
				{
				gchar s[32];
				snprintf(s, sizeof(s), "preset_%d_name", i+1);
				if (!strcasecmp(option, s))
					{
					g_free (preset_name[i]);
					preset_name[i] = quoted_value(value_all);
					}
				snprintf(s, sizeof(s), "preset_%d_file", i+1);
				if (!strcasecmp(option, s))
					{
					g_free (preset_file[i]);
					preset_file[i] = quoted_value(value_all);
					}
				}
			}

	/* current state */

		window_pos_x = read_int_option(f, option,
			"window_position_x", value, window_pos_x);
		window_pos_y = read_int_option(f, option,
			"window_position_y", value, window_pos_y);

		window_list_x = read_int_option(f, option,
			"window_list_x", value, window_list_x);
		window_list_y = read_int_option(f, option,
			"window_list_y", value, window_list_y);
		window_list_w = read_int_option(f, option,
			"window_list_width", value, window_list_w);
		window_list_h = read_int_option(f, option,
			"window_list_height", value, window_list_h);
		window_list_div_dir = read_int_option(f, option,
			"window_list_divider_dir", value, window_list_div_dir);
		window_list_div_mid = read_int_option(f, option,
			"window_list_divider_mid", value, window_list_div_mid);
		window_list_columns = read_char_option(f, option,
			"window_list_columns", value_all, window_list_columns);

		repeat_mode = read_bool_option(f, option,
			"repeat", value, repeat_mode);
		shuffle_mode = read_bool_option(f, option,
			"shuffle", value, shuffle_mode);

		show_remaining_time = read_bool_option(f, option,
			"show_time_remaining", value, show_remaining_time);
		show_total_time = read_bool_option(f, option,
			"show_time_total", value, show_total_time);

		initial_playlist_position = read_int_option(f, option,
			"initial_playlist_position", value, initial_playlist_position);

	/* transparency */

		slik_colorshift_on = read_bool_option(f, option,
			"colorshift_enable", value, slik_colorshift_on);
		slik_colorshift_r = read_int_option(f, option,
			"colorshift_red", value, slik_colorshift_r);
		slik_colorshift_g = read_int_option(f, option,
			"colorshift_green", value, slik_colorshift_g);
		slik_colorshift_b = read_int_option(f, option,
			"colorshift_blue", value, slik_colorshift_b);
		slik_colorshift_a = read_int_option(f, option,
			"colorshift_alpha", value, slik_colorshift_a);

		slik_transparency_force = read_bool_option(f, option,
			"transparency_force", value, slik_transparency_force);
		slik_transparency_force_a = read_int_option(f, option,
			"transparency_force_alpha", value, slik_transparency_force_a);
		}

	fclose(f);
	g_free(rc_path);
}



