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
#include "ipc.h"

#include "btn_funcs.h"
#include "display.h"
#include "mixer.h"
#include "players.h"
#include "playlist.h"
#include "playlist-dlg.h"
#include "playlist-window.h"
#include "rcfile.h"
#include "ui2_main.h"
#include "ui_fileops.h"

#include <fcntl.h>
#include <signal.h>
#include <errno.h>

/* note:
 * this is really cheesy right now, basically to send a command to a running
 * gqmpeg, place a command in ~/.gqmpeg/command
 * the best way would be:
 *   echo "play" > ~/.gqmpeg/commands
 * the file only exists when gqmpeg is accepting commands.
 * the file is checked every 1/10th second.
 */

#define GQMPEG_FILE_IPC_COMMAND ".gqmpeg/command"
#define GQMPEG_FILE_IPC_LOCK ".gqmpeg/command.pid"

static gint ipc_fd_id = -1;
static FILE *ipc_file_id = NULL;
static gint ipc_cb_id = -1;
static gchar *ipc_file = NULL;

static void ipc_status_unregister_all(void);

/*
 *-----------------------------------------------------------------------------
 * The input side (take commands from here)
 *-----------------------------------------------------------------------------
 */

static gint pid_lock_file_create(void)
{
	FILE *f;
	gchar *lock_file;
	gchar *pathl;

	lock_file = g_strconcat(homedir(), "/", GQMPEG_FILE_IPC_LOCK, NULL);

	pathl = path_from_utf8(lock_file);
	f = fopen(pathl, "w");
	g_free(pathl);
	if (!f)
		{
		printf(_("error opening file for write %s\n"), lock_file);
		g_free(lock_file);
		return FALSE;
		}

	g_free(lock_file);

	fprintf(f, "%d\n", (int)getpid());
	fclose(f);

	return TRUE;
}

static void pid_lock_file_destroy(void)
{
	gchar *lock_file;

	lock_file = g_strconcat(homedir(), "/", GQMPEG_FILE_IPC_LOCK, NULL);
	if (isfile(lock_file))
		{
		gchar *pathl;

		pathl = path_from_utf8(lock_file);
		unlink(pathl);
		g_free(pathl);
		}

	g_free(lock_file);
}

static gint pid_lock_file_locked(void)
{
	FILE *f;
	gchar *lock_file;
	gchar *pathl;
	gchar buf[64];
        pid_t pid_v = -1;

	lock_file = g_strconcat(homedir(), "/", GQMPEG_FILE_IPC_LOCK, NULL);

	pathl = path_from_utf8(lock_file);
        f = fopen(pathl, "r");
	g_free(pathl);
	g_free(lock_file);
        if(!f) return FALSE;

	if (fgets(buf, sizeof(buf), f) == NULL)
		{
		fclose(f);
		return FALSE;
		}

        fclose(f);

	pid_v = (pid_t)strtol(buf, NULL, 10);
	if (pid_v < 1 || (kill (pid_v, 0) == -1 && errno != EPERM)) return FALSE;

	return TRUE;
}

void ipc_send(gchar *command)
{
	FILE *f;

	if (ipc_file)
		{
		gchar *pathl;

		pathl = path_from_utf8(ipc_file);
		f = fopen(pathl, "w");
		g_free(pathl);
		}
	else
		{
		gchar *path;
		gchar *pathl;

		path = g_strconcat(homedir(), "/.gqmpeg/command", NULL);
		pathl = path_from_utf8(path);
		f = fopen(pathl, "w"); /* blocking, huh ? */
		g_free(pathl);
		g_free(path);
		}

	if (!f)
		{
		printf(_("error opening command file for write %s\n"), ipc_file);
		return;
		}

	fprintf(f, "%s\n", command);

	fclose(f);
}

static gboolean ipc_read_data(GIOChannel *source, GIOCondition condition, gpointer data)
{
	gchar i_buf[1025];
	i_buf[1024] = '\0'; /* paranoid safety */

	while (fgets(i_buf, 1024, ipc_file_id))
		{
		if (debug_mode) printf("data_read_from_ipc\n");
		if (!strncmp(i_buf, "play_file", 9) && strlen(i_buf) > 10)
			{
			gchar *buf = quoted_value(i_buf + 10);
			if (is_playlist(buf))
				{
				playlist_load_from_file(buf, FALSE, TRUE, TRUE);
				}
			else
				{
				current_song_set_and_play(-1, buf);
				}
			g_free(buf);
			}
		else if (!strncmp(i_buf, "play", 4))
			{
			btn_play_pressed();
			}
		else if (!strncmp(i_buf, "stop", 4))
			{
			btn_stop_pressed();
			}
		else if (!strncmp(i_buf, "pause", 5))
			{
			btn_pause_pressed();
			}
		else if (!strncmp(i_buf, "next", 4))
			{
			btn_next_down();
			btn_next_pressed();
			}
		else if (!strncmp(i_buf, "prev", 4))
			{
			btn_prev_down();
			btn_prev_pressed();
			}
		else if (!strncmp(i_buf, "seek", 4) && strlen(i_buf) > 5)
			{
			if (status == STATUS_PLAY || status == STATUS_PAUSE)
				{
				gint n;
				n = strtol(i_buf + 5 ,NULL, 10);
				if (i_buf[5] == '-' || i_buf[5] == '+')
					{
					n += seconds;
					}
				if (n < 0) n = 0;
				if (n > seconds + seconds_remaining) n = seconds + seconds_remaining;
				playback_exec_command(EXEC_SEEK, current_song_get_data(), n);
				}
			}
		else if (!strncmp(i_buf, "pladd_play", 10) && strlen(i_buf) > 11)
			{
			gchar *buf = quoted_value(i_buf + 11);
			playlist_add(buf, TRUE);
			g_free(buf);

			current_song_set_and_play(playlist_get_count() - 1, NULL);
			}
		else if (!strncmp(i_buf, "pladd", 5) && strlen(i_buf) > 6)
			{
			gchar *buf = quoted_value(i_buf + 6);
			playlist_add(buf, TRUE);
			g_free(buf);
			}
		else if (!strncmp(i_buf, "plrm", 4) && strlen(i_buf) > 5)
			{
			gchar *buf = quoted_value(i_buf + 5);
			playlist_remove(buf, -1, TRUE);
			g_free(buf);
			}
		else if (!strncmp(i_buf, "plclr", 4))
			{
			playlist_clear();
			}
		else if (!strncmp(i_buf, "plload", 6) && strlen(i_buf) > 7)
			{
			gchar *buf = quoted_value(i_buf + 7);
			playlist_load_from_file(buf, FALSE, FALSE, TRUE);
			g_free(buf);
			}
		else if (!strncmp(i_buf, "plappend", 8) && strlen(i_buf) > 9)
			{
			gchar *buf = quoted_value(i_buf + 9);
			playlist_load_from_file(buf, TRUE, FALSE, TRUE);
			g_free(buf);
			}
		else if (!strncmp(i_buf, "status_add", 10) && strlen(i_buf) > 11)
			{
			gchar *buf1;
			gchar *buf2;

			buf1 = quoted_value(i_buf + 11);
			if (buf1 && strlen(i_buf) > 13 + strlen(buf1))
				{
				buf2 = quoted_value(i_buf + 13 + strlen(buf1));
				ipc_status_register(buf1, buf2);
				g_free(buf2);
				}
			g_free(buf1);
			}
		else if (!strncmp(i_buf, "status_rm", 9) && strlen(i_buf) > 10)
			{
			gchar *buf;
			buf = quoted_value(i_buf + 10);
			ipc_status_unregister(buf);
			g_free(buf);
			}
		else if (!strncmp(i_buf, "show_playlist", 13))
			{
			playlist_window_show();
			}
		else if (!strncmp(i_buf, "show_open_file", 14))
			{
			playlist_dialog_load_file(NULL, FALSE);
			}
		else if (!strncmp(i_buf, "show_open_custom", 16))
			{
			playlist_dialog_add_custom_type(FALSE);
			}
		else if (!strncmp(i_buf, "show_open_playlist", 18))
			{
			playlist_dialog_load(NULL);
			}
		else if (!strncmp(i_buf, "skin", 4) && strlen(i_buf) > 5)
			{
			gchar *buf = quoted_value(i_buf + 5);
			ui_skin_load(main_window, buf, NULL);
			g_free(buf);
			}
		else if (!strncmp(i_buf, "volume", 6) && strlen(i_buf) > 7)
			{
			gint p = 7;
			gint n;

			if (i_buf[p] == '"') p++;	/* ipc_send() quotes everything */
			n = strtol(i_buf + p ,NULL, 10);
			if (i_buf[p] == '-' || i_buf[p] == '+')
				{
				n += get_volume();
				}
			if (n < 0) n = 0;
			if (n > 100) n = 100;
			set_volume(n);
			display_set_volume();
			}
		else if (!strncmp(i_buf, "quit", 4))
			{
			gqmpeg_schedule_exit();
			}
		else if (!strncmp(i_buf, "help", 4))
			{
			print_term(_("available ipc commands:\n"));
			print_term(_("play, pause, stop, next, prev, quit\n"));
			print_term(_("seek [+|-]n                seek n seconds into file, or +/-n\n"));
			print_term(_("pladd {\"file\"|file}        add file to playlist\n"));
			print_term(_("pladd_play {\"file\"|file}   add file to playlist and play it\n"));
			print_term(_("plremove {\"file\"|file}     remove file from playlist\n"));
			print_term(_("plclr                      clear playlist\n"));
			print_term(_("plload {playlist}          load playlist\n"));
			print_term(_("plappend {playlist}        append playlist\n"));
			print_term(_("play_file {\"file\"|file}    play file, do not add to playlist\n"));
			print_term(_("                           or, if playlist, load playlist\n"));
			print_term(_("   file is a path or http://server:port\n"));
			print_term(_("skin \"file\"                set skin\n"));
			print_term(_("volume [+|-]n              set volume to n (0 - 100) or +/-n\n"));
			print_term(_("getting a status printed to a file is also possible:\n"));
			print_term(_("status_add \"key\" \"file\"    start status printing to file\n"));
			print_term(_("status_rm \"key\"|\"file\"     stop printing to file or key\n"));
			print_term(_("these commands open file requestors, windows, etc.\n"));
			print_term(_("show_playlist\n"));
			print_term(_("show_open_file, show_open_custom, show_open_playlist\n"));
			}
		else
			{
			printf(_("unknown ipc command: [%s]\n"), i_buf);
			}
		}
#if 0
	/* this appears to be needed for NetBSD, removed really should no longer be after move to gdk_input_add */
	if (feof(ipc_file_id) != 0)
		rewind (ipc_file_id);
#endif

	return TRUE;
}

void ipc_on(void)
{
	GIOChannel *channel;
	gchar *pathl;

	if (ipc_file_id || ipc_cb_id > -1) return;

	if (pid_lock_file_locked())
		{
		printf(_("ipc command file already locked, ipc disabled\n"));
		return;
		}

	if (!ipc_file)
		{
		ipc_file = g_strconcat(homedir(), "/", GQMPEG_FILE_IPC_COMMAND, NULL);
		}

	pathl = path_from_utf8(ipc_file);
	unlink(pathl);
	if (mkfifo(pathl, S_IRUSR | S_IWUSR) != 0)
		{
		printf(_("Failed to mkfifo for %s\n"), ipc_file);
		g_free(pathl);
		return;
		}

	ipc_fd_id = open(pathl, O_RDWR | O_NONBLOCK);
	g_free(pathl);

	if (ipc_fd_id == -1)
		{
		printf(_("unable to open %s\n"), ipc_file);
		return;
		}
	ipc_file_id = fdopen (ipc_fd_id, "r");

	pid_lock_file_create();

	channel = g_io_channel_unix_new(ipc_fd_id);
	ipc_cb_id = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, G_IO_IN,
					ipc_read_data, NULL, NULL);
	g_io_channel_unref(channel);

#if 0
	ipc_cb_id = gdk_input_add(ipc_fd_id,
                           GDK_INPUT_READ,
                           (GdkInputFunction) ipc_read_data,
                           NULL);
#endif

	if (debug_mode) printf("ipc_on\n");
}

void ipc_off(void)
{
	ipc_status_unregister_all();

	if (ipc_cb_id > -1)
		{
		g_source_remove(ipc_cb_id);
#if 0
		gdk_input_remove(ipc_cb_id);
#endif
		ipc_cb_id = -1;
		}

	if (ipc_file_id != NULL) fclose(ipc_file_id);

	ipc_file_id = NULL;

	if (ipc_fd_id != -1) close (ipc_fd_id);

	ipc_fd_id = -1;

	if (ipc_file)
		{
		gchar *pathl;

		pathl = path_from_utf8(ipc_file);
		unlink(pathl);
		g_free(pathl);

		g_free(ipc_file);
		ipc_file = NULL;
		pid_lock_file_destroy();
		}

	if (debug_mode) printf("ipc_off\n");
}

gint ipc_another_process_active(void)
{
	return pid_lock_file_locked();
}

/*
 *-----------------------------------------------------------------------------
 * The output side (get status here)
 *-----------------------------------------------------------------------------
 */

typedef struct _IpcStatData IpcStatData;
struct _IpcStatData
{
	gchar *key;
	gchar *path;
	int fd;
};

static GList *subscribed_apps = NULL;

static IpcStatData* ipc_status_find_by_key(gchar *key)
{
	GList *work = subscribed_apps;
	while(work)
		{
		IpcStatData *isd = work->data;
		if (isd->key && strcmp(key, isd->key) == 0) return isd;
		work = work->next;
		}

	return NULL;
}

static IpcStatData* ipc_status_find_by_path(gchar *path)
{
	GList *work = subscribed_apps;
	while(work)
		{
		IpcStatData *isd = work->data;
		if (isd->path && strcmp(path, isd->path) == 0) return isd;
		work = work->next;
		}

	return NULL;
}

static void ipc_status_remove_by_data(IpcStatData *isd)
{
	if (!isd) return;

	if (debug_mode) printf("Removing for ipc status key=%s path=%s\n", isd->key, isd->path);

	subscribed_apps = g_list_remove(subscribed_apps, isd);
	if (isd->fd != -1) close(isd->fd);
	g_free(isd->key);
	g_free(isd->path);
	g_free(isd);
}

static sig_atomic_t sigpipe_occured = FALSE;

static void sighandler_sigpipe(int sig)
{
	sigpipe_occured = TRUE;
}

static gint ipc_status_write(IpcStatData *isd, const gchar *text)
{
	struct sigaction new_action, old_action;
	int w;

	if (isd->fd < 0) return -1;

	sigpipe_occured = FALSE;

	new_action.sa_handler = sighandler_sigpipe;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;

	/* setup our signal handler */
	sigaction (SIGPIPE, &new_action, &old_action);

	w = write(isd->fd, text, strlen(text));

	if (sigpipe_occured || w < 0)
		{
		if (sigpipe_occured)
			printf("SIGPIPE writing to %s, file closed.\n", isd->path);
		else
			printf("ERROR writing to %s, file closed.\n", isd->path);
		close (isd->fd);
		isd->fd = -1;
		w = -1;
		}

	/* restore the original signal handler */
	sigaction (SIGPIPE, &old_action, NULL);

	return w;
}

static gint ipc_status_write_status(IpcStatData *isd, gint write_song, gint write_time)
{
	gchar *buf = NULL;
	int w;

	if (!isd || isd->fd == -1) return FALSE;

	if (write_song)
		{
		const gchar *path = current_song_get_path();
		const gchar *title = current_song_get_title();
		const gchar *artist = current_song_get_artist();
		const gchar *album = current_song_get_album();

		buf = g_strdup_printf("track: %d of %d\n", current_song_get_number() + 1, playlist_get_count());
		w = ipc_status_write(isd, buf);
		g_free(buf);
		if (w == -1) return FALSE;

		if (path)
			{
			buf = g_strdup_printf("file:\"%s\"\n", path);
			}
		else
			{
			buf = g_strdup("song:\"\"\n");
			}
		w = ipc_status_write(isd, buf);
		g_free(buf);
		if (w == -1) return FALSE;

		if (title)
			{
			buf = g_strdup_printf("name:\"%s\"\n", title);
			}
		else
			{
			buf = g_strdup("name:\"\"\n");
			}
		w = ipc_status_write(isd, buf);
		g_free(buf);
		if (w == -1) return FALSE;

		if (artist)
			{
			buf = g_strdup_printf("artist:\"%s\"\n", artist);
			}
		else
			{
			buf = g_strdup("artist:\"\"\n");
			}
		w = ipc_status_write(isd, buf);
		g_free(buf);
		if (w == -1) return FALSE;

		if (album)
			{
			buf = g_strdup_printf("album:\"%s\"\n", album);
			}
		else
			{
			buf = g_strdup("album:\"\"\n");
			}
		w = ipc_status_write(isd, buf);
		g_free(buf);
		if (w == -1) return FALSE;
		}

	if (write_time)
 		{
		if (status == STATUS_PLAY)
			{
			buf = g_strdup_printf("time: play %d %d\n", seconds, seconds_remaining);
			}
		else if (status == STATUS_PAUSE)
			{
			buf = g_strdup_printf("time: pause %d %d\n", seconds, seconds_remaining);
			}
		else
			{
			buf = g_strdup_printf("time: stop %d %d\n", seconds, seconds_remaining);
			}
		w = ipc_status_write(isd, buf);
		g_free(buf);
		if (w == -1) return FALSE;
		}

	return TRUE;
}

void ipc_status_update(void)
{
	static gint previous_ipc_status = -1;
	static SongData *previous_ipc_sd = NULL;
	static gint previous_ipc_total = 0;
	static gint previous_ipc_time = 0;

	GList *work;
	SongData *sd;
	gint total;
	gint write_song = FALSE;
	gint write_time = FALSE;

	if (!subscribed_apps) return;

	sd = current_song_get_data();
	total = playlist_get_count();

	if (previous_ipc_status == status &&
	    previous_ipc_sd == sd &&
	    previous_ipc_total == total &&
	    previous_ipc_time == seconds) return;

	if (previous_ipc_sd != sd || previous_ipc_total != total) write_song = TRUE;
	if (previous_ipc_status != status || previous_ipc_time != seconds) write_time = TRUE;

	work = subscribed_apps;
	while(work)
		{
		IpcStatData *isd = work->data;
		work = work->next;

		if (!ipc_status_write_status(isd, write_song, write_time))
			{
			printf("Failed to update status for %s\n", isd->key);
			ipc_status_remove_by_data(isd);
			}
		}

	previous_ipc_status = status;
	previous_ipc_sd = sd;
	previous_ipc_total = total;
	previous_ipc_time = seconds;
}

void ipc_status_send(const gchar *text)
{
	gchar *cmd;
	GList *work;

	if (!subscribed_apps || !text) return;

	cmd = g_strconcat(text, "\n", NULL);

	work = subscribed_apps;
	while(work)
		{
		IpcStatData *isd = work->data;
		work = work->next;

		if (ipc_status_write(isd, cmd) == -1)
			{
			printf("Failed to update status for %s\n", isd->key);
			ipc_status_remove_by_data(isd);
			}
		}

	g_free(cmd);
}

#define MAX_IPC_SUBSCRIBERS 16

gint ipc_status_register(gchar *key, gchar *path)
{
	IpcStatData *isd;

	if (debug_mode) printf("Registering for ipc status key=%s path=%s\n", key, path);

	if (!key || !path) return FALSE;

	if (ipc_status_find_by_key(key) != NULL)
		{
		printf("Attempt to register duplicate key for status (must be unique) [%s]\n", key);
		return FALSE;
		}
	if (ipc_status_find_by_path(path) != NULL)
		{
		printf("Attempt to register duplicate path for status (must be unique) [%s]\n", path);
		return FALSE;
		}

	if (g_list_length(subscribed_apps) >= MAX_IPC_SUBSCRIBERS)
		{
		printf("Maximum ipc subscribers (%d) reached!\nkey = \"%s\"\npath = \"%s\"\n",
		       MAX_IPC_SUBSCRIBERS, key, path);
		return FALSE;
		}


	isd = g_new(IpcStatData, 1);
	isd->fd = open(path, O_WRONLY | O_APPEND | O_NONBLOCK);
	if (isd->fd == -1)
		{
		g_free(isd);
		printf("Unable to open %s for status writing\n", path);
		return FALSE;
		}

	isd->key = g_strdup(key);
	isd->path = g_strdup(path);

	subscribed_apps = g_list_append(subscribed_apps, isd);
	ipc_status_write_status(isd, TRUE, TRUE);

	return TRUE;
}

void ipc_status_unregister(gchar *key)
{
	IpcStatData *isd;

	if (!key) return;

	if (key)
		{
		isd = ipc_status_find_by_key(key);
		}
	if (!isd) /* also accept path */
		{
		isd = ipc_status_find_by_path(key);
		}

	if (isd)
		{
		if (isd->fd != -1)
			{
			ipc_status_write(isd, "close\n");
			}
		ipc_status_remove_by_data(isd);
		}
}

static void ipc_status_unregister_all(void)
{
	GList *work = subscribed_apps;
	while(work)
		{
		IpcStatData *isd = work->data;
		work = work->next;
		if (isd->fd != -1)
			{
			ipc_status_write(isd, "exit\n");
			}
		ipc_status_remove_by_data(isd);
		}
}

