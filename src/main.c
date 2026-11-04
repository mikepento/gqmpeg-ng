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

#include "btn_funcs.h"
#include "cpu_perc.h"
#include "display.h"
#include "display_flyby.h"
#include "dndutil.h"
#include "dock.h"
#include "filelist.h"
#include "ipc.h"
#include "mixer.h"
#include "players.h"
#include "playlist.h"
#include "playlist-window.h"
#include "rcfile.h"
#include "ui2_editor.h"
#include "ui2_main.h"
#include "ui2_util.h"
#include "ui_bookmark.h"
#include "ui_fileops.h"
#include "ui_help.h"
#include "ui_tabcomp.h"
#include "types.h"
#include "window.h"

#include <fcntl.h>
#include <signal.h>

static gint update_timeout_id = -1;

static gchar *startup_skin_file = NULL;
static gint startup_dockwm = FALSE;

/*
 *-----------------------------------------------------------------------------
 * misc utils (public)
 *-----------------------------------------------------------------------------
 */

gint convert_chars(gchar *text, gchar s, gchar d)
{
	gchar *p;
	gint c;

	if (!text) return 0;

	p = text;
	c = 0;
	while(*p != '\0')
		{
		if (*p == s)
			{
			*p = d;
			c++;
			}
		p++;
		}

	return c;
}

/*
 * Returns time in thousandths (1/1000) of a second since very first call
 * it is also called once in startup in main to initialize the zero point,
 * so this will return time since app start as well.
 */
guint32 elapsed_run_time(void)
{
	static GTimeVal *start_tv = NULL;
	GTimeVal tv;

	g_get_current_time(&tv);

	if (!start_tv)
		{
		start_tv = g_new0(GTimeVal, 1);
		start_tv->tv_sec = tv.tv_sec;
		start_tv->tv_usec = tv.tv_usec;
		}

	if (start_tv->tv_usec > tv.tv_usec)
		{
		tv.tv_usec += G_USEC_PER_SEC;
		tv.tv_sec--;
		}

	return (guint32)((tv.tv_sec - start_tv->tv_sec) * 1000 +
			 ((tv.tv_usec - start_tv->tv_usec) * 1000 / G_USEC_PER_SEC) );
}

/*
 *-----------------------------------------------------------------------------
 * load a preset, smartly (public)
 *-----------------------------------------------------------------------------
 */

void load_preset(gint n)
{
	gchar *path;

	if (n < 0 || n > 9) return;

	path = preset_file[n];

	if (!path) return;

	if (playlist_load_from_file(preset_file[n], FALSE, TRUE, FALSE) == TRUE)
                {
		if (play_presets) playback_exec_command(EXEC_PLAY, current_song_get_data(), 0);
                }
	else
		{
		current_song_set_and_play(-1, path);
		}
}

gint preset_is_active(gint n)
{
	const gchar *match_path;

	if (n < 0 || n > 9 ||
	    !preset_name[n] || !preset_file[n]) return FALSE;

	if (current_song_get_number() == -1)
		{
		match_path = current_song_get_path();
		}
	else
		{
		match_path = NULL;
		}

	return ((!match_path && playlist_pathname && strcmp(playlist_pathname, preset_file[n]) == 0) ||
		(match_path && strcmp(match_path, preset_file[n]) == 0) );
}

/*
 *-----------------------------------------------------------------------------
 * current song routines (public)
 *-----------------------------------------------------------------------------
 */

static SongData *current_songdata = NULL;
static gint current_song_in_playlist = FALSE;

static void clear_current_song(void)
{
	if (!current_songdata) return;

	if (status != STATUS_STOP && status != STATUS_NEXT)
		{
		playback_exec_command(EXEC_STOP, current_songdata, 0);
		}

	if (!current_song_in_playlist)
		{
		playlist_data_free(current_songdata);
		}

	current_songdata = NULL;
	current_song_in_playlist = FALSE;

	if (loop_ab_enabled)
		{
		loop_ab_enabled = FALSE;
		display_set_loop_ab(FALSE);
		}
}

SongData *current_song_get_data(void)
{
	return current_songdata;
}

gint current_song_get_number(void)
{
	if (current_song_in_playlist && current_songdata)
		{
		return playlist_get_index_by_data(current_songdata);
		}
	else
		{
		return -1;
		}
}

void current_song_set(gint n, gchar *path)
{
	gint old_status = status;
	gint old_song = current_song_get_number();

	if (path)
		{
		clear_current_song();
		current_songdata = playlist_data_new(path, TRUE);
		current_song_in_playlist = FALSE;

		display_presets_refresh();
		}
	else if (n >= 0)
		{
		SongData *sd = playlist_get_data(n);
		if (sd)
			{
			if (current_song_in_playlist)
				{
				if (current_songdata == sd && sd->live) return;
				}
			clear_current_song();
			current_songdata = sd;
			current_song_in_playlist = TRUE;
			}
		else
			{
			clear_current_song();
			current_song_in_playlist = FALSE;
			}
		}
	else
		{
		clear_current_song();
		}

	display_set_song_info();
	main_window_set_title(current_song_get_title());

	if ((old_status == STATUS_PLAY || old_status == STATUS_NEXT) && current_songdata)
		{
		playback_exec_command(EXEC_PLAY, current_songdata, 0);
		}

	playlist_unset_flags(old_song, SONG_FLAG_PLAYING);
	playlist_set_flags(current_song_get_number(), SONG_FLAG_PLAYING);
	display_total_time_changed();
	display_flyby_reset();
}

gint current_song_is_in_playlist(void)
{
	return current_song_in_playlist;
}

void current_song_set_to_next(void)
{
	gint n = current_song_get_number();
	if (n == -1)
		{
		gint p = playlist_get_first();
		if (p >= 0)
			{
			current_song_set(p, NULL);
			}
		else
			{
			if (status == STATUS_NEXT)
				{
				if (repeat_mode && current_songdata)
					{
					playback_exec_command(EXEC_PLAY, current_songdata, 0);
					}
				else
					{
					status = STATUS_STOP;
					}
				}
			}
		}
	else
		{
		gint p = playlist_get_next(n);
		if (p >= 0)
			{
			current_song_set(p, NULL);
			}
		else if (repeat_mode)
			{
			if (shuffle_mode)
				{
				shuffle_list_create(FALSE);
				}
			current_song_set(playlist_get_first(), NULL);
			}
		else
			{
			if (status == STATUS_NEXT) status = STATUS_STOP;
			}
		}
}

void current_song_set_to_prev(void)
{
	gint p = playlist_get_prev(current_song_get_number());
	
	if (p >= 0) current_song_set(p, NULL);
}

void current_song_set_and_play(gint n, gchar *path)
{
	current_song_set(n, path);
	if (status == STATUS_STOP)
		{
		playback_exec_command(EXEC_PLAY, current_songdata, 0);
		}
}

const gchar *current_song_get_path(void)
{
	return playlist_data_get_path(current_songdata);
}

const gchar *current_song_get_title(void)
{
	return playlist_data_get_title(current_songdata);
}

const gchar *current_song_get_artist(void)
{
	return playlist_data_get_artist(current_songdata);
}

const gchar *current_song_get_album(void)
{
	return playlist_data_get_album(current_songdata);
}

const gchar *current_song_get_genre(void)
{
	return playlist_data_get_genre(current_songdata);
}

const gchar *current_song_get_year(void)
{
	return playlist_data_get_year(current_songdata);
}

const gchar *current_song_get_comment(void)
{
	return playlist_data_get_comment(current_songdata);
}

gint current_song_get_track(void)
{
	return playlist_data_get_track(current_songdata);
}

/*
 *-----------------------------------------------------------------------------
 * the main loop! called 8 times per second (125 ms)
 *-----------------------------------------------------------------------------
 */

static gint update_condition(gpointer data)
{
	static gint status_chk = -1;
	static gint update_counter = 0;
	static gint mixer_counter = 0;

	/* update mixer (volume/balance controls) every 4 seconds */
	mixer_counter++;
	if (mixer_counter > 32)
		{
		mixer_counter = 0;
		display_set_volume();
		display_set_balance();
		}

	ipc_status_update();

	if (main_window_widgets_update()) return TRUE;

	/* this is true once the song's header info is set by the player */
	if (new_song && (frames != 0 || seconds != 0))
		{
		display_set_io_information(FALSE);
		display_set_song_text_info(-1, TRUE);
		display_set_title(current_song_get_title());
		new_song = FALSE;
		}

	if (display_total_time_has_changed() && status != STATUS_NEXT)
		{
		if (status == STATUS_STOP)
			display_set_time(0, 0, FALSE);
		else
			display_set_time(seconds, seconds_remaining, FALSE);
		}

	update_counter++;

	if (loop_ab_enabled)
		{
		if (status != STATUS_PLAY && status != STATUS_PAUSE)
			{
			loop_ab_enabled = FALSE;
			display_set_loop_ab(FALSE);
			}
		else if (loop_ab_end == 0)
			{
			static gint loop_count = 0;
			/* blink */
			loop_count++;
			if (loop_count < 4)
				{
				display_set_loop_ab(TRUE);
				}
			else
				{
				display_set_loop_ab(FALSE);
				if (loop_count > 5) loop_count = 0;
				}
			}
		else if (seconds >= loop_ab_end)
			{
			playback_exec_command(EXEC_SEEK, current_song_get_data(), loop_ab_start);
			}
		}

	if (status == STATUS_PLAY)
		{
		static int old_seconds;

		if (debug_mode) putchar('u');

		/* display */
		if (update_counter > 15)
			{
			update_counter = 0;
			display_set_cpu(check_child_cpu_usage(pid));
			}
		if (seconds != old_seconds && !pos_slider_in_drag)
			{
			display_set_time(seconds, seconds_remaining, FALSE);
			display_set_frame_count(frames);
			display_set_position(-1.0);
			}
		old_seconds = seconds;
		}

	if (status == STATUS_NEXT)
		{
		static gint change_count = 0;
		gint use_change_delay = TRUE;

		if (change_delay > 0)
			{
			if (status_chk != STATUS_NEXT)
				{
				if (repeat_1_enabled ||
				    repeat_mode ||
				    playlist_get_next(current_song_get_number()) >= 0)
					{
					change_count = change_delay * 8;
					if (!repeat_1_enabled)
						{
						status = STATUS_STOP;
						current_song_set_to_next();
						status = STATUS_NEXT;
						}
					status_chk = STATUS_NEXT;
					display_set_cpu(0);
					display_set_frame_count(0);
					display_set_io_information(TRUE);
					display_flyby_reset();
					}
				else
					{
					change_count = 0;
					use_change_delay = FALSE;
					}
				}
			}
		else
			{
			change_count = 0;
			}

		if (change_count < 1)
			{
			if (repeat_1_enabled || (use_change_delay && change_delay > 0))
				{
				status = STATUS_STOP;
				playback_exec_command(EXEC_PLAY, current_song_get_data(), 0);
				}
			else
				{
				current_song_set_to_next();
				}
			display_flyby_start();
			}
		else
			{
			static gint change_blink = 0;

			change_count--;
			display_set_time(change_count / 8 + 1, 0, TRUE);
			if (debug_mode) printf("delay %d\n", change_count);
			change_blink++;
			if (change_blink < 4)
				{
				display_set_status(TRUE, TRUE);
				}
			else
				{
				display_set_status(TRUE, FALSE);
				if (change_blink > 5) change_blink = 0;
				}
			}
		}

	if (status == status_chk)
		{
		if (debug_mode) putchar('c');
		return TRUE;
		}
	if (status == STATUS_PLAY)
		{
		display_set_status(FALSE, FALSE);
		}
	if (status == STATUS_STOP)
		{
		display_set_cpu(0);
		display_set_time(0, 0, FALSE);
		display_set_frame_count(0);
		display_set_io_information(TRUE);
		display_set_position(-1.0);
		display_set_status(FALSE, FALSE);
		display_flyby_reset();
		}
	if (status == STATUS_PAUSE)
		{
		display_set_cpu(0);
		display_set_status(FALSE, FALSE);
		display_flyby_reset();
		}
	status_chk = status;
	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * ipc command line parsing
 *-----------------------------------------------------------------------------
 */

static gchar *prepend_path_if_match(const gchar *text)
{
	gchar *buf;

	if (!text) return NULL;

	if (text[0] == '/') return g_strdup(text);

	buf = concat_dir_and_file(current_path, text);
	if (isname(buf))
		{
		return buf;
		}

	g_free(buf);
	return g_strdup(text);
}

static void command_add_option(gchar **command , gchar *text, gint value)
{
	gchar *buf;

	if (!value) return;

	buf = g_strconcat(*command, " ", text, NULL);
	g_free(*command);
	*command = buf;
}

static gint send_if_ipc_match(gchar *short_str, gchar *long_str, gchar *command,
			      gint *i, int argc, char *argv[], gint extra, gint multiple, gint required)
{
	gchar *text = argv[*i];

	if (!strcmp(text, short_str) || (long_str && !strcmp(text, long_str)))
		{
		if (!ipc_another_process_active())
			{
			static gint tried_once = FALSE;
			gint c = 0;
			gint found = FALSE;
			gchar *exec_cmd;

			if (required)
				{
				printf("GQmpeg process not active, \"%s\" ignored.\n", text);
				return TRUE;
				}

			if (tried_once) return TRUE;
			tried_once = TRUE;
			exec_cmd = g_strdup("gqmpeg");
			command_add_option(&exec_cmd , "--debug", debug_mode);
			command_add_option(&exec_cmd , "--dock", (dock_mode && !startup_dockwm) );
			command_add_option(&exec_cmd , "--dockwm", startup_dockwm);
			if (startup_skin_file)
				{
				gchar *buf = g_strconcat("--skin:", startup_skin_file, NULL);
				command_add_option(&exec_cmd , buf, TRUE);
				g_free(buf);
				}
			command_add_option(&exec_cmd , "--blank &", TRUE);
			if (debug_mode) printf("[%s]\n", exec_cmd);
			if (system(exec_cmd) == -1)
				{
				printf(_("execing gqmpeg failed\n"));
				g_free(exec_cmd);
				return TRUE;
				}
			g_free(exec_cmd);
			while (c < 10 && !found)
				{
				sleep(1);
				found = ipc_another_process_active();
				c++;
				}
			if (!found)
				{
				printf(_("Timeout waiting for ipc of spawned gqmpeg process\n"));
				return TRUE;
				}
			}
		if (extra)
			{
			if (*i < argc - 1)
				{
				gchar *path;
				gchar *buf;
				*i = *i + 1;
				path = prepend_path_if_match(argv[*i]);
				if (*i < argc - 1 && strncmp(argv[*i], "-", 1) != 0 && strcmp(command, "play_file") == 0)
					{
					/* add to playlist for multiple files with -f */
					buf = g_strconcat("pladd_play \"", path, "\"",NULL);
					}
				else
					{
					buf = g_strconcat(command, " \"", path, "\"",NULL);
					}
				g_free(path);
				ipc_send(buf);
				g_free(buf);
				if (multiple)
					{
					/* special case, only play first one for -f */
					if (strcmp(command, "play_file") == 0)
						{
						command = "pladd";
						}
					while (*i < argc - 1 && strncmp(argv[*i], "-", 1) != 0)
						{
						*i = *i + 1;
						path = prepend_path_if_match(argv[*i]);
						buf = g_strconcat(command, " \"", path, "\"",NULL);
						g_free(path);
						ipc_send(buf);
						g_free(buf);
						}
					}
				}
			else
				{
				printf(_("ipc %s command requires data\n"), text);
				}
			}
		else
			{
			ipc_send(command);
			}
		return TRUE;
		}

	return FALSE;
}

static gint check_for_ipc_command(gint *i, int argc, char *argv[])
{
	if (send_if_ipc_match("-p", "--play", "play", i, argc, argv, FALSE, FALSE, FALSE)) return TRUE;
	if (send_if_ipc_match("-s", "--stop", "stop", i, argc, argv, FALSE, FALSE, FALSE)) return TRUE;
	if (send_if_ipc_match("-n", "--next", "next", i, argc, argv, FALSE, FALSE, FALSE)) return TRUE;
	if (send_if_ipc_match("-b", "--back", "prev", i, argc, argv, FALSE, FALSE, FALSE)) return TRUE;
	if (send_if_ipc_match("-ps", "--pause", "pause", i, argc, argv, FALSE, FALSE, FALSE)) return TRUE;
	if (send_if_ipc_match("-pladd", "--playlist_add", "pladd", i, argc, argv, TRUE, TRUE, FALSE)) return TRUE;
	if (send_if_ipc_match("-plrm", "--playlist_remove", "plrm", i, argc, argv, TRUE, TRUE, FALSE)) return TRUE;
	if (send_if_ipc_match("-plclr", "--playlist_clear", "plclr", i, argc, argv, FALSE, FALSE, FALSE)) return TRUE;
	if (send_if_ipc_match("-pladdplay", "--playlist_add_play", "pladd_play", i, argc, argv, TRUE, FALSE, FALSE)) return TRUE;
	if (send_if_ipc_match("-pl", "--playlist", "plload", i, argc, argv, TRUE, FALSE, FALSE)) return TRUE;
	if (send_if_ipc_match("-plappend", "--playlist_append", "plappend", i, argc, argv, TRUE, TRUE, FALSE)) return TRUE;
	if (send_if_ipc_match("-f", "--file", "play_file", i, argc, argv, TRUE, TRUE, FALSE)) return TRUE;
	if (send_if_ipc_match("-q", "--quit", "quit", i, argc, argv, FALSE, FALSE, TRUE)) return TRUE;

	if (send_if_ipc_match("--skin_set", NULL, "skin", i, argc, argv, TRUE, FALSE, TRUE)) return TRUE;
	if (send_if_ipc_match("--volume", NULL, "volume", i, argc, argv, TRUE, FALSE, TRUE)) return TRUE;

	return FALSE;
}

static void ipc_status(void)
{
	gchar s_buf[1024];
	gchar *command;
	gchar *tmp_file;
	int fd;
	gint s;
	gchar *buf;
	gint success = FALSE;
	
	if (!ipc_another_process_active())
		{
		printf("GQmpeg process not active, \"%s\" ignored.\n", "--status");
		return;
		}

	buf = g_strconcat(homedir(), "/", GQMPEG_RC_DIR, "/~status_tmp.", NULL);
	tmp_file = unique_filename(buf, NULL, NULL, FALSE);
	g_free(buf);

	buf = tmp_file;
	tmp_file = path_from_utf8(buf);
	g_free(buf);

	if (!tmp_file || mkfifo(tmp_file, S_IRUSR | S_IWUSR) != 0)
		{
		printf("unable to create fifo: %s\n", tmp_file);
		g_free(tmp_file);
		return;
		}

	fd = open(tmp_file, O_RDONLY | O_NONBLOCK);
	if (fd == -1)
		{
		printf("unable to open fifo: %s\n", tmp_file);
		unlink(tmp_file);
		g_free(tmp_file);
		return;
		}

	command = g_strdup_printf("status_add \"status\" \"%s\"", tmp_file);
	ipc_send(command);
	g_free(command);

	while ((s = read(fd, s_buf, sizeof(s_buf))) > 0)
		{
		gchar *sb = g_strndup(s_buf, s);
		printf("%s", sb);
		g_free(sb);
		success = TRUE;
		}

	ipc_send("status_rm \"status\"");
	close(fd);

	unlink(tmp_file);
	g_free(tmp_file);

	if (!success)
		{
		printf("Status retrieval failed.\n");
		}
}
/*
 *-----------------------------------------------------------------------------
 * misc start-up utils
 *-----------------------------------------------------------------------------
 */

#define RC_HISTORY_NAME "history"
#define RC_STATES_NAME "window_states"

static void keys_load(void)
{
	gchar *path;

	path = g_strconcat(homedir(), "/", GQMPEG_RC_DIR, "/", RC_HISTORY_NAME, NULL);
	history_list_load(path);
	g_free(path);
}

static void keys_save(void)
{
	gchar *path;

	path = g_strconcat(homedir(), "/", GQMPEG_RC_DIR, "/", RC_HISTORY_NAME, NULL);
	history_list_save(path);
	g_free(path);
}

static void states_load(void)
{
	gchar *path;

	path = g_strconcat(homedir(), "/", GQMPEG_RC_DIR, "/", RC_STATES_NAME, NULL);
	ui_states_load(path);
	g_free(path);
}

static void states_save(void)
{
	gchar *path;

	path = g_strconcat(homedir(), "/", GQMPEG_RC_DIR, "/", RC_STATES_NAME, NULL);
	ui_states_save(path);
	g_free(path);
}

static void check_for_home_path(gchar *path)
{
	gchar *buf;
	buf = g_strconcat(homedir(), "/", path, NULL);
	if (!isdir(buf))
		{
		printf(_("Creating GQmpeg dir:%s\n"), buf);
		if (!mkdir_utf8(buf, 0755))
			{
			printf(_("Could not create dir:%s\n"), buf);
			}
		}
	g_free(buf);
}

static gchar *determine_skin_path(gchar *file)
{
	gchar *path = NULL;
	gchar *buf;

	if (isdir(file))
		{
		path = g_strdup(file);
		}
	else
		{
		buf = g_strconcat(current_path, "/", file, NULL);
		if (isdir(buf))
			{
			path = buf;
			}
		else
			{
			g_free(buf);
			buf = g_strconcat(homedir(), "/", 
					GQMPEG_RC_DIR_SKIN, "/", file, NULL);
			if (isdir(buf))
				{
				path = buf;
				}
			else
				{
				g_free(buf);
				buf = g_strconcat(GQMPEG_SKINDIR, "/", file, NULL);
				if (isdir(buf))
					{
					path = buf;
					}
				else
					{
					g_free(buf);
					}
				}
			}
		}

	if (path)
		{
		if (debug_mode) printf (_("Using skin: %s\n"), path);
		}
	else
		{
		printf (_("Skin not found: %s\n"), file);
		}

	return path;
}

/*
 *-----------------------------------------------------------------------------
 * help function
 *-----------------------------------------------------------------------------
 */

static GtkWidget *help_window = NULL;

static void help_window_destroy_cb(GtkWidget *window, gpointer data)
{
	help_window = NULL;
}

void help_window_show(const gchar *key)
{
	if (help_window)
		{
		gdk_window_raise(help_window->window);
		if (key) help_window_set_key(help_window, key);
		return;
		}

	help_window = help_window_new(_("Help - GQmpeg"), "GQmpeg", "help",
				      GQMPEG_SKINDIR "/README", key);
	g_signal_connect(G_OBJECT(help_window), "destroy",
			 G_CALLBACK(help_window_destroy_cb), NULL);
}

static void show_help(void)
{
	printf("GQmpeg %s\n", VERSION);
	print_term(_("usage: gqmpeg [option(s)] [file(s) | ipc_command(s)]\n"));
	print_term(_(" standard options:\n"));
	print_term(_("  --skin:<skindir>             load specified skin\n"));
	print_term(_("  --dock                       enable docking behavior\n"));
	print_term(_("  --dockwm                     enable Window Maker docking\n"));
	print_term(_("  --skinhelp                   print mouse event coordinates\n"));
	print_term(_("  --debug                      enable debug output\n"));
	print_term(_("  -h,  --help                  displays this message\n"));
	print_term(_("  -v,  --version               shows version info\n"));
	print_term(_("  --blank                      start with empty playlist\n"));
	print_term(_("  --geometry [+|-]xxx[+|-]yyy  specify window position\n"));
	print_term(_("  --crack                      reserved\n"));
	print_term(_(" available ipc_commands:\n"));
	print_term(_("  -p          --play\n"));
	print_term(_("  -s          --stop\n"));
	print_term(_("  -n          --next\n"));
	print_term(_("  -b          --back\n"));
	print_term(_("  -ps         --pause\n"));
	print_term(_("  -pladd      --playlist_add <f>[..]    add file to playlist\n"));
	print_term(_("  -pladdplay  --playlist_add_play <f>   add file to playlist and play it\n"));
	print_term(_("  -plrm       --playlist_remove <f>[..] remove file from playlist\n"));
	print_term(_("  -plclr      --playlist_clear          clear playlist\n"));
	print_term(_("  -pl         --playlist <f>            load playlist\n"));
	print_term(_("  -plappend   --playlist_append <f>[..] append playlist\n"));
	print_term(_("  -f          --file <f>[..]            play file, do not add to playlist\n"));
	print_term(_("                                        or, if playlist, load playlist\n"));
	print_term(_("  -q          --quit                    tell GQmpeg to quit\n"));
	print_term(_("              --skin_set <f>            set skin\n"));
	print_term(_("              --status                  print current status, song, time\n"));
	print_term(_("              --volume [+|-]#           set, or increment volume (0-100)\n"));
	print_term(_("              * files can also be of the format http://server:port\n"));
}

/*
 *-----------------------------------------------------------------------------
 * exit function
 *-----------------------------------------------------------------------------
 */

void gqmpeg_exit(void)
{
	gchar *session_file;

	g_free(skin_default_path);
	skin_default_path = g_strdup(main_window->skin_path);
	g_free(skin_mode_key);
	skin_mode_key = g_strdup(main_window->skin_mode_key);

	playlist_window_hide();	/* to save the current pos/size */

	if (status != STATUS_STOP) playback_exec_command(EXEC_STOP, current_song_get_data(), 0);

	if (!dock_mode)
		{
		gint x = 0;
		gint y = 0;
		ui_geometry_get(main_window, &x, &y, NULL, NULL);
		window_pos_x = x;
		window_pos_y = y;
		}

	initial_playlist_position = current_song_get_number();

	save_options();
	keys_save();
	states_save();

	/* save playlist between sessions (even if option not enabled) */
	session_file = g_strconcat(homedir(), "/", GQMPEG_RC_FILE_SESSION, NULL);
	playlist_save(session_file);
	g_free(session_file);

	/* remove the ~/.gqmpeg/command file, so that existance of this file
	   usually indicates gqmpeg is already running */
	ipc_off();

	gtk_main_quit();
}

static gint gqmpeg_schedule_exit_cb(gpointer data)
{
	gqmpeg_exit();
	
	return FALSE;
}

void gqmpeg_schedule_exit(void)
{
	gtk_idle_add(gqmpeg_schedule_exit_cb, NULL);
}

/*
 *-----------------------------------------------------------------------------
 * main routine
 *-----------------------------------------------------------------------------
 */

int main (int argc, char *argv[])
{
	const gchar *cbuf;
	gchar *buf;
	gchar *default_current_path = NULL;
	gint i;
	gint blank_startup = FALSE;
	GList *list_buf = NULL;
	GList *playlist_buf = NULL;
	gint tc = 0;
	gint geometry_set = FALSE;
	gint geometry_allow = FALSE;
	gint geometry_x = 0;
	gint geometry_y = 0;

	/* setup locale, i18n */
	gtk_set_locale();
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	init_vars();

	/* setup random seed for shuffle mode */
	srand(time(NULL));

	/* check that we can find a valid home directory, if not exit semi-gracefully */
	cbuf = homedir();
	if (!cbuf)
		{
		printf(_("Unable to determine the home directory\n"));
		return 0;
		}
	else if (!isdir(cbuf))
		{
		printf(_("Could not find home directory: %s\n"), cbuf);
		return 0;
		}

	/* create gqmpeg dirs */
	check_for_home_path(GQMPEG_RC_DIR);
	check_for_home_path(GQMPEG_RC_DIR_SKIN);
	check_for_home_path(GQMPEG_RC_DIR_PLAYLIST);

	playlist_clear();

	if (argc > 1)
		{
		gint have_ipc_command = FALSE;
		i = 1;
		while (i < argc)
			{
			gchar *command_line_path = prepend_path_if_match(argv[i]);

			if (strncmp(argv[i], "-", 1) != 0)
				{
				if (isdir(command_line_path))
					{
					if (!default_current_path)
						{
						default_current_path = g_strdup(argv[i]);
						}
					}
				else
					{
					if (is_playlist(command_line_path))
						{
						playlist_buf = g_list_prepend(playlist_buf, g_strdup(command_line_path));
						}
					else
						{
						list_buf = g_list_prepend(list_buf, g_strdup(command_line_path));
						}
					}
				}
			else if (strcmp(argv[i],"--debug") == 0)
				{
				debug_mode++;
				printf("debugging enabled (level %d)\n", debug_mode);
				}
			else if (strcmp(argv[i],"--blank") == 0)
				{
				blank_startup = TRUE;
				i = argc;
				}
			else if (strcmp(argv[i],"--geometry") == 0)
				{
				if (i + 1 < argc)
					{
					const gchar *geom = argv[i + 1];
					const gchar *ptr;
	
					geometry_set = FALSE;
	
					if (*geom == '+' || *geom == '-')
						{
						ptr = geom + 1;
						while (*ptr != '+' && *ptr != '-' && *ptr != '\n') ptr++;
						if (*ptr != '\n')
							{
							buf = g_strndup(geom, ptr - geom);
							geometry_x = strtol(buf, NULL, 10);
							g_free(buf);
							geometry_y = strtol(ptr, NULL, 10);
							geometry_set = TRUE;
							}
						}
					i++;
					}
				else
					{
					printf(_("--geometry missing argument\n"));
					}
				if (!geometry_set) printf(_("Invalid geometry format ([+|-]xxx[+|-]yyy)\n"));
				}
			else if (check_for_ipc_command(&i, argc, argv))
				{
				have_ipc_command = TRUE;
				}
			else if (strcmp(argv[i],"--skinhelp") == 0)
				{
				debug_skin = TRUE;
				printf("skin debugging enabled\n");
				}
			else if (strcmp(argv[i],"--dock") == 0)
				{
				dock_mode = TRUE;
				}
			else if (strcmp(argv[i],"--dockwm") == 0)
				{
				dock_mode = TRUE;
				startup_dockwm = TRUE;
				}
			else if (strncmp(argv[i],"--skin:", 7) == 0)
				{
				if (startup_skin_file)
					{
					printf(_("Ignoring multiple skins!\n"));
					}
				else
					{
					startup_skin_file = determine_skin_path(argv[i] + 7);
					}
				}
			else if (strcmp(argv[i],"--status") == 0)
				{
				ipc_status();
				return 0;
				}
			else if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"--help") == 0)
				{
				show_help();
				return 0;
				}
			else if (strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"--version") == 0)
				{
				printf("GQmpeg %s\n", VERSION);
				return 0;
				}
			else if (strcmp(argv[i],"--crack") == 0)
				{
				tc++;
				}
			else
				{
				printf(_("unknown option: %s\nuse --help for options\n"),argv[i]);
				}
			g_free(command_line_path);
			i++;
			}
		if (have_ipc_command)
			{
			return 0;
			}
		}

	if (playlist_buf) playlist_buf = g_list_reverse(playlist_buf);
	if (list_buf) list_buf = g_list_reverse(list_buf);

/* */
	/*
	 * the gtk inits are moved after the command line parser
	 * (for speed with ipc commands)
	 */
	gtk_init (&argc, &argv);

	buf = g_strconcat(homedir(), "/.gqmpeg/gtkrc", NULL);
	if (isfile(buf)) gtk_rc_parse(buf);
	g_free(buf);

	/* call this before loading options, so that modules load options too */
	player_modules_init();

	load_options();
	keys_load();
	states_load();

	/* now load/append playlists */

	if (playlist_buf)
		{
		GList *work = playlist_buf;
		while(work)
			{
			if (!work->prev)
				{
				printf(_("Loading playlist:%s\n"), (gchar *)work->data);
				playlist_load(work->data, FALSE, TRUE);
				}
			else
				{
				printf(_("Appending playlist:%s\n"), (gchar *)work->data);
				playlist_load(work->data, TRUE, TRUE);
				}
			g_free(work->data);
			work = work->next;
			}
		g_list_free(playlist_buf);
		}
	if (list_buf)
		{
		GList *work = list_buf;
		while(work)
			{
			if (typelist_determine_type_id(work->data) != -1 || isfile(work->data))
				{
				printf(_("Adding song:%s\n"), (gchar *)work->data);
				playlist_add(work->data, TRUE);
				if (current_song_get_number() == -1)
					{
					current_song_set(playlist_get_count() - 1, NULL);
					}
				}
			else
				{
				printf(_("Unable to determine type:%s\n"), (gchar *)work->data);
				}
			g_free(work->data);
			work = work->next;
			}
		g_list_free(list_buf);
		}

	if (initial_playlist_mode == PLAYLIST_MODE_EMPTY)
		{
		}
	if (initial_playlist_mode == PLAYLIST_MODE_SESSION && playlist_get_count() == 0 && !blank_startup)
		{
		gchar *session_file;
		session_file = g_strconcat(homedir(), "/", GQMPEG_RC_FILE_SESSION, NULL);
		if (isfile(session_file))
			{
			if (start_playing_on_startup)
				{
				if (playlist_load_start(session_file, FALSE, TRUE, FALSE, TRUE) &&
				    initial_playlist_position > -1 &&
				    initial_playlist_position < playlist_get_count() )
					{
					if (debug_mode) printf("Restoring song number %d\n", initial_playlist_position);
					current_song_set(initial_playlist_position, NULL);
					}
				}
			else
				{
				playlist_load(session_file, FALSE, TRUE);
				}
			}
		else
			{
			printf(_("Could not load session-playlist file\n"));
			}
		g_free(session_file);
		g_free(playlist_pathname);
		playlist_pathname = g_strconcat(homedir(), "/", GQMPEG_RC_DIR_PLAYLIST, "/Untitled.gqmpeg", NULL);
		g_free(playlist_filename);
		playlist_filename = g_strdup(_("Untitled.gqmpeg"));
		}
	if (initial_playlist_mode == PLAYLIST_MODE_FILE && playlist_get_count() == 0 && !blank_startup)
		{
		playlist_load(initial_playlist_pathname, FALSE, TRUE);
		}

	if (default_current_path)
		{
		change_to_path(default_current_path);
		g_free(default_current_path);
		}
	else if (initial_directory_enable)
		{
		if (isdir(initial_directory_path))
			{
			change_to_path(initial_directory_path);
			}
		else
			{
			printf(_("config file contains invalid startup path:\n%s\n"), initial_directory_path);
			filelist_refresh();
			}
		}
	else
		{
		filelist_refresh();
		}

	/* start our timer */
	elapsed_run_time();

	/* set up mixer */

	if (!mixer_command) mixer_command = g_strdup("xmixer");
	mixer_init(mixer_device_id);

	/* set up main window */
	main_window = main_window_create(startup_skin_file);
	g_free(startup_skin_file);
	startup_skin_file = NULL;

	if (geometry_set)
		{
		/* remember state */
		geometry_allow = main_window->allow_move;
		/* turn off positioning */
		main_window->allow_move = FALSE;
		}

	main_window_set_title(current_song_get_title());

	if (startup_dockwm) windowmaker_setup_dock();

	gtk_widget_show(main_window->window);

	if (geometry_set)
		{
		/* restore positioning state */
		main_window->allow_move = geometry_allow;
		gdk_window_move(main_window->window->window, geometry_x, geometry_y);
		}

	dnd_init();

	if (show_playlist_on_startup && !blank_startup) playlist_window_show();

	update_timeout_id = gtk_timeout_add(125, (GtkFunction)update_condition, NULL);

	if (start_playing_on_startup && !blank_startup) btn_play_pressed();

	if (allow_ipc) ipc_on();

	while (tc > 0)
		{
		tc--;
		ui_distort(main_window);
		}

	/* register base dirs to editor */
	buf = g_strconcat(homedir(), "/", GQMPEG_RC_DIR_SKIN, NULL);
	ui_edit_add_skin_resource(buf);
	g_free(buf);
	ui_edit_add_skin_resource(GQMPEG_SKINDIR);

	gtk_main ();
	return 0;
}

