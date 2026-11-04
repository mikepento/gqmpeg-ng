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


/*
 * the TODO for this module (using xmp)
 * > All :)
 */

#include "gqmpeg.h"
#include "io_xmp.h"

#include "players.h"
#include "ui_fileops.h"
#include "ui_utildlg.h"

#include <fcntl.h>

#define XMP_BINARY "xmp"

typedef struct {
	gint positions;
	gint patterns;
	gint instruments;
	gint channels;
	gchar *type;
	gchar *tracker;
} XmpExtraData;


static XmpExtraData *xmp_extra_data_new(SongData *sd);


/* ----------------------------------------------------------
   input / output interface to xmp
   ----------------------------------------------------------*/

gint xmp_enabled = FALSE;

/* software mixer options */
gint xmp_frequency = 44100;
gint xmp_8bit = FALSE;
gint xmp_interpolate = TRUE;

/* sound options */
gint xmp_mono = FALSE;
gint xmp_pan_percent = 80;

/* output to wav */
gint xmp_output_to_wav = FALSE;
gint xmp_output_to_wav_to_path = FALSE;
gchar *xmp_output_to_wav_path = NULL;


/* internal stuff */
static SongData *xmp_current_sd = NULL;
static gint xmp_control_read_id = -1;
static gint xmp_child_pid = -1;
static int xmp_pipe_i[2];
static int xmp_pipe_o[2];

static gint xmp_position = 0;
static gint xmp_position_total = 0;

static guint32 xmp_start_time = 0;
static guint32 xmp_seconds_offset = 0;
static gint xmp_paused = FALSE;

/*
 *-----------------------------------------------------------------------------
 * common utils
 *-----------------------------------------------------------------------------
 */

static sig_atomic_t sigpipe_occured = FALSE;

static void sighandler_sigpipe(int sig)
{
	sigpipe_occured = TRUE;
}

static gint pipe_write(int fd, const gchar *text)
{
	struct sigaction new_action, old_action;
	int w;

	if (fd < 0) return -1;

	sigpipe_occured = FALSE;

	new_action.sa_handler = sighandler_sigpipe;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;

	/* setup our signal handler */
	sigaction (SIGPIPE, &new_action, &old_action);

	w = write(fd, text, strlen(text));

	if (sigpipe_occured || w < 0)
		{
		if (sigpipe_occured)
			{
			printf("io_xmp.c: SIGPIPE writing to pipe\n");
			w = -2;
			}
		else
			{
			printf("io_xmp.c: ERROR writing to pipe\n");
			}
		}

	/* restore the original signal handler */
	sigaction (SIGPIPE, &old_action, NULL);

	return w;
}

/* takes seconds and return estimated seek pattern */
static gint xmp_real_position(SongData *sd, gint pattern)
{
	gint p;
	gint c;

	if (sd->length < 1) return 0;
	c = xmp_extra_data_get_positions(sd);
	if (c == 0) return c;

	p = (float)pattern / sd->length * c;

	return p;
}

static gint xmp_real_seconds(SongData *sd, gint position)
{
	gint c;

	if (sd->length < 1) return 0;
	c = xmp_extra_data_get_positions(sd);

	return (float)(position + 1) / c * sd->length;
}

static void xmp_time_restart(void)
{
	xmp_seconds_offset = seconds;
	xmp_start_time = elapsed_run_time();;
}

static void xmp_time_reset(SongData *sd, gint position)
{
	gint p;
	gint c;

	p = xmp_real_position(sd, position);
	c = xmp_extra_data_get_positions(sd);

	if (c < 1)
		{
		seconds = 0;
		seconds_remaining = 0;
		}
	else
		{
		seconds = (float)p / c * sd->length;
		seconds_remaining = sd->length - seconds;
		}
	frames = p;
	frames_remaining = c - frames;

	xmp_time_restart();
}

static void xmp_time_do(void)
{
	gint passed;

	if (!xmp_current_sd) return;

	passed = elapsed_run_time() - xmp_start_time;
	seconds = xmp_seconds_offset + passed;
	if (xmp_current_sd->length > 0) seconds_remaining = xmp_current_sd->length - seconds;

	frames = xmp_position;
	frames_remaining = xmp_position_total - xmp_position;
}

/*
 *-----------------------------------------------------------------------------
 * child control
 *-----------------------------------------------------------------------------
 */

static void xmp_child_shutdown(void)
{
	if (xmp_child_pid != -1)
		{
		gint w;
		w = pipe_write(xmp_pipe_o[1], "q"); /* it probably never gets here though */
		if (w < 1)
			{
			if (kill(xmp_child_pid, SIGINT) ==-1)
				{
				printf(_("Failed to successfully send signal to pid = %d\n"), xmp_child_pid);
				}
			}
		waitpid (xmp_child_pid, NULL, 0); /* should be WNOHANG */
		}
	if (xmp_control_read_id != -1) g_source_remove(xmp_control_read_id);
	xmp_control_read_id = -1;
	close(xmp_pipe_i[0]);
	close(xmp_pipe_o[1]);

	xmp_child_pid = -1;
	pid = 0;
}

static void xmp_error(void)
{
	xmp_child_shutdown();
	module_playback_error(NULL);
}

static void xmp_input_error(void)
{
	xmp_error();
}

static void xmp_input_end(void)
{
	xmp_child_shutdown();
	module_playback_end();
}

static void xmp_child_gone(void)
{
	waitpid (xmp_child_pid, NULL, 0);
	xmp_child_pid = -1;
	pid = 0;
}

/*
 *-----------------------------------------------------------------------------
 * input parsing
 *-----------------------------------------------------------------------------
 */

static gint parse_two_hex_chars(gchar *text)
{
	gchar buf[3];

	buf[0] = *text;
	text++;
	buf[1] = *text;
	buf[2] = '\0';

	return (gint)strtol(buf, NULL, 8);
}

static gint xmp_input_parse(gchar *buffer, gint length)
{
	if (length < 1) return TRUE;

	/* make it look like a real string, so things are easier */
	buffer[length] = '\0';

	if (debug_mode > 1) printf("xmp output:\"%s\"\n", buffer);

	if (strncmp(buffer, "Tempo[", 6) == 0)
		{
		gchar *ptr;
		ptr = strstr(buffer, "Pos[");
		if (ptr && strlen(ptr) > 9)
			{
			ptr+=4;
			xmp_position = parse_two_hex_chars(ptr);
			ptr+=3;
			xmp_position_total = parse_two_hex_chars(ptr);
			}
#if 0
		ptr = strstr(buffer, "Row[");
		if (ptr && strlen(ptr) > 9)
			{
			ptr+=4;
			xmp_row = parse_two_hex_chars(ptr);
			ptr+=3;
			xmp_row_total = parse_two_hex_chars(ptr);
			}
#endif
		if (strstr(buffer, "PAUSED"))
			{
			xmp_paused = TRUE;
			return TRUE;
			}

		xmp_paused = FALSE;
		xmp_time_do();
		return TRUE;
		}
	else if (strncmp(buffer, "Estimated time", 14) == 0 && length > 17 && xmp_current_sd)
		{
		gchar *ptr;
		gchar *pb;
		gint l;

		ptr = buffer + 17;
		l = 0;
		while (*ptr != 'm' && *ptr !='\0')
			{
			ptr++;
			l++;
			}
		if (l > 0)
			{
			pb = g_strndup(buffer + 17, l);
			xmp_current_sd->length = 60 * (gint)strtol(pb, NULL, 10);
			g_free(pb);
			}
		if (strncmp(ptr, "min", 3) == 0)
			{
			ptr += 3;
			xmp_current_sd->length += (gint)strtol(ptr, NULL, 10);
			}
		seconds_remaining = xmp_current_sd->length - seconds;
		}
	else if (strncmp(buffer, "Module length", 13) == 0 && length > 17 && xmp_current_sd)
		{
		XmpExtraData *xd = xmp_extra_data_new(xmp_current_sd);
		xd->positions = (gint)strtol(buffer + 17, NULL, 10);
		xmp_position_total = xd->positions;
		}

	/* look for end */
	if (strncmp(buffer, "Elapsed time", 12) == 0)
		{
		xmp_input_end();
		return FALSE;
		}

	/* look for errors */
	if (strncmp(buffer, "xmp:", 4) == 0)
		{
		printf("xmp error: \"%s\"\n", buffer);
		warning_dialog(_("GQmpeg: xmp error"), buffer);
		xmp_error();
		return FALSE;
		}

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * input buffer
 *-----------------------------------------------------------------------------
 */

static char xmp_read_buffer[2048];
static gint xmp_read_buffer_pos = 0;
static gint xmp_read_buffer_len = 0;

static gboolean xmp_input_read_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	gchar buf[2048];
	gint r;
	gint p;
	gint l;
	gchar *ptr;

	r = read(xmp_pipe_i[0], buf, sizeof(buf));

	if (r == -1)
		{
		xmp_input_error();
		return TRUE;
		}
	if (r == 0)
		{
		printf(_("xmp disappeared! (unexpected EOF)\n"));
		xmp_child_gone();
		xmp_input_error();
		return TRUE;
		}

	ptr = xmp_read_buffer + xmp_read_buffer_pos;
	l = sizeof(xmp_read_buffer) - 1;
	p = 0;
	while(p < r)
		{
		gchar c;
		c = buf[p];
		p++;

		if (c == '\b' && xmp_read_buffer_pos > 0)
			{
			xmp_read_buffer_pos--;
			}
		else if (c == '\r')
			{
			xmp_read_buffer_pos = 0;
			}
		else if (c == '\n')
			{
			if (!xmp_input_parse(xmp_read_buffer, xmp_read_buffer_len)) return TRUE;
			xmp_read_buffer_len = xmp_read_buffer_pos = 0;
			}
		else if (xmp_read_buffer_pos < l)
			{
			xmp_read_buffer[xmp_read_buffer_pos] = c;
			xmp_read_buffer_pos++;
			if (xmp_read_buffer_pos > xmp_read_buffer_len) xmp_read_buffer_len = xmp_read_buffer_pos;
			}
		}

	xmp_input_parse(xmp_read_buffer, xmp_read_buffer_len);

	return TRUE;
}

static void xmp_input_read_reset(void)
{
	xmp_read_buffer_pos = 0;
	xmp_read_buffer_len = 0;
}

/*
 *-----------------------------------------------------------------------------
 * child setup (fork)
 *-----------------------------------------------------------------------------
 */

static gint xmp_child_run(SongData *sd, gint position)
{
	pid_t frk_pid;
	char cmd_arguments[16][512];
	char *cmd_ptr[16];
	size_t cmd_max;
	int cmd_cnt = 0;
        gint start_pattern;
	gchar *exec_bin = XMP_BINARY;
	GIOChannel *channel;

	cmd_max = sizeof(cmd_arguments) / 16;

	if (!sd) return FALSE;

	start_pattern = xmp_real_position(sd, position);

	strncpy(cmd_arguments[cmd_cnt], XMP_BINARY, cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	strncpy(cmd_arguments[cmd_cnt], "--norc", cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	strncpy(cmd_arguments[cmd_cnt], "-v", cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	if (start_pattern > 0)
		{
		strncpy(cmd_arguments[cmd_cnt], "--start", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;

		snprintf(cmd_arguments[cmd_cnt], cmd_max, "%dX", start_pattern);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	strncpy(cmd_arguments[cmd_cnt], "--frequency", cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	snprintf(cmd_arguments[cmd_cnt], cmd_max, "%d", xmp_frequency);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	if (xmp_8bit)
		{
		strncpy(cmd_arguments[cmd_cnt], "--bits", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;

		strncpy(cmd_arguments[cmd_cnt], "8", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}
	if (xmp_interpolate)
		{
		strncpy(cmd_arguments[cmd_cnt], "--interpolate", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (xmp_mono)
		{
		strncpy(cmd_arguments[cmd_cnt], "--mono", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}
	else
		{
		strncpy(cmd_arguments[cmd_cnt], "--stereo", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	strncpy(cmd_arguments[cmd_cnt], "--pan", cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	snprintf(cmd_arguments[cmd_cnt], cmd_max, "%d", xmp_pan_percent);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	strncpy(cmd_arguments[cmd_cnt], sd->path, cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	strncpy(cmd_arguments[cmd_cnt], "", cmd_max);
	cmd_ptr[cmd_cnt] = NULL;
	cmd_cnt++;

	/* Create the pipe. */
	if (pipe(xmp_pipe_i))
                {
                fprintf (stderr, _("Pipe failed.\n"));
                return FALSE;
                }

	if (pipe(xmp_pipe_o))
                {
                fprintf (stderr, _("Pipe failed.\n"));
		close(xmp_pipe_i[0]);
		close(xmp_pipe_i[1]);
                return FALSE;
                }

        if (debug_mode) printf("opening: %s\n", sd->path);

	/* Create the child process. */
	frk_pid = fork ();
	if (frk_pid == (pid_t) 0)
		{
		/* This is the child process. */
		dup2(xmp_pipe_i[1], 2);
		dup2(xmp_pipe_o[0], 0);
		fcntl(xmp_pipe_o[0], F_SETFL, O_NONBLOCK);

		close(xmp_pipe_i[0]);
		close(xmp_pipe_o[1]);

		execvp(exec_bin, cmd_ptr);
		printf(_("unable to run %s (in the path?)\n"), exec_bin);
		_exit(1);
		}
	else if (frk_pid < (pid_t) 0)
		{
		/* The fork failed. */
		fprintf (stderr, _("Fork failed.\n"));
		close(xmp_pipe_i[0]);
		close(xmp_pipe_i[1]);
		close(xmp_pipe_o[0]);
		close(xmp_pipe_o[1]);
		pid = 0;
		xmp_child_pid = -1;
		return FALSE;
		}
	else
		{
		/* This is the parent process. */
		xmp_child_pid = (int) frk_pid;
		pid = xmp_child_pid;

		close(xmp_pipe_i[1]);
		close(xmp_pipe_o[0]);

		fcntl(xmp_pipe_i[1], F_SETFL, O_NONBLOCK);
		}

	if (debug_mode) printf("xmp pid = %d\n", xmp_child_pid);

	xmp_input_read_reset();

	channel = g_io_channel_unix_new(xmp_pipe_i[0]);
	xmp_control_read_id = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, G_IO_IN,
						  xmp_input_read_cb, NULL, NULL);
	g_io_channel_unref(channel);
#if 0
	xmp_control_read_id = gdk_input_add (xmp_pipe_i[0], GDK_INPUT_READ, xmp_input_read_cb, NULL);
#endif

	xmp_time_reset(sd, position);

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * real funcs
 *-----------------------------------------------------------------------------
 */

static gint xmp_action_start(SongData *sd, gint position)
{
	if (xmp_child_run(sd, position))
		{
		return TRUE;
		}
	return FALSE;
}

static gint xmp_action_stop(SongData *sd)
{
	xmp_child_shutdown();
	return TRUE;
}

static gint xmp_action_pause(SongData *sd)
{
	if (xmp_paused) return FALSE;

	return (pipe_write(xmp_pipe_o[1], " ") > 0);
}

static gint xmp_action_continue(SongData *sd)
{
	if (!xmp_paused) return FALSE;

	xmp_time_restart();
	return (pipe_write(xmp_pipe_o[1], " ") > 0);
}

static gint xmp_action_seek(gint position)
{
	gchar *seek_char;
	gint p;
	gint l;
	SongData *sd = xmp_current_sd;

	/* this is the best that can be done for the moment */
	if (!sd) return FALSE;

	p = xmp_real_position(sd, position);

	if (p == xmp_position)
		{
		return TRUE;
		}
	if (p == 0)
		{
		/* xmp will exit if this is attempted by sending b's */
		xmp_child_shutdown();
		return xmp_child_run(sd, position);
		}

	if (p < xmp_position)
		{
		seek_char = "b";
		l = xmp_position - p;
		}
	else
		{
		seek_char = "f";
		l = p - xmp_position;
		}

	while (l > 0)
		{
		if (pipe_write(xmp_pipe_o[1], seek_char) < 1) return FALSE;
		l--;
		}

	/* update clock */
	seconds = xmp_real_seconds(sd, p);
	seconds_remaining = sd->length - seconds;

	xmp_time_restart();

	return TRUE;
}

/*
 *----------------------------------------------------------------------------
 * read file info
 *----------------------------------------------------------------------------
 */

static void xmp_extra_data_free(gpointer data)
{
	XmpExtraData *xd = data;

	if (!xd) return;

	g_free(xd->type);	
	g_free(xd->tracker);
	g_free(xd);
}

static XmpExtraData *xmp_extra_data_new(SongData *sd)
{
	XmpExtraData *xd;

	if (!sd) return NULL;

	if (sd->data) return sd->data;

	xd = g_new0(XmpExtraData, 1);
	xd->positions = 0;
	xd->patterns = 0;
	xd->instruments = 0;
	xd->channels = 0;
	xd->type = NULL;
	xd->tracker = NULL;

	sd->data = xd;

	sd->free_data_func = xmp_extra_data_free;

	return xd;
}

gint xmp_extra_data_get_positions(SongData *sd)
{
	XmpExtraData *xd;

	if (!sd || !sd->data) return 0;

	xd = sd->data;
	return xd->positions;
}

gint xmp_extra_data_get_patterns(SongData *sd)
{
	XmpExtraData *xd;

	if (!sd || !sd->data) return 0;

	xd = sd->data;
	return xd->patterns;
}

gint xmp_extra_data_get_instruments(SongData *sd)
{
	XmpExtraData *xd;

	if (!sd || !sd->data) return 0;

	xd = sd->data;
	return xd->instruments;
}

gint xmp_extra_data_get_channels(SongData *sd)
{
	XmpExtraData *xd;

	if (!sd || !sd->data) return 0;

	xd = sd->data;
	return xd->channels;
}

const gchar *xmp_extra_data_get_type(SongData *sd)
{
	XmpExtraData *xd;

	if (!sd || !sd->data) return NULL;

	xd = sd->data;
	return xd->type;
}

const gchar *xmp_extra_data_get_tracker(SongData *sd)
{
	XmpExtraData *xd;

	if (!sd || !sd->data) return NULL;

	xd = sd->data;
	return xd->tracker;
}

static gint xmp_data_load(SongData *sd)
{
	FILE *f;
	gchar *command_line;
        gchar buf[1024];
	gint ret = FALSE;

	if (!sd->path || !isfile(sd->path)) return FALSE;

	sd->live = FALSE;
	sd->length = -1;

	command_line = g_strdup_printf("%s -o /dev/null --load-only \"%s\" 2>&1", XMP_BINARY, sd->path);

	f = popen(command_line, "r");

	g_free(command_line);

	if (!f) return FALSE;

	while (fgets(buf, sizeof(buf), f) != NULL)
                {
		gint length;
		gchar *ptr;
		ptr = buf;
		while (*ptr != '\0')
			{
			if (*ptr == '\n') *ptr = '\0';
			ptr++;
			}

		length = strlen (buf);
		if (strncmp(buf, "Module title", 12) == 0 && length > 17)
			{
			if (sd->title) g_free(sd->title);
			sd->title = g_strdup(buf + 17);
			ret = TRUE;
			}
		else if (strncmp(buf, "Estimated time", 14) == 0 && length > 17)
			{
			gchar *ptr;
			gchar *pb;
			gint l;

			ptr = buf + 17;
			l = 0;
			while (*ptr != 'm' && *ptr !='\0')
				{
				ptr++;
				l++;
				}
			if (l > 0)
				{
				pb = g_strndup(buf + 17, l);
				sd->length = 60 * (gint)strtol(pb, NULL, 10);
				g_free(pb);
				}
			if (strncmp(ptr, "min", 3) == 0)
				{
				ptr += 3;
				sd->length += (gint)strtol(ptr, NULL, 10);
				}
			}
		else if (strncmp(buf, "Module length", 13) == 0 && length > 17)
			{
			XmpExtraData *xd = xmp_extra_data_new(sd);
			xd->positions = (gint)strtol(buf + 17, NULL, 10);
			}
		else if (strncmp(buf, "Stored patterns", 15) == 0 && length > 17)
			{
			XmpExtraData *xd = xmp_extra_data_new(sd);
			xd->patterns = (gint)strtol(buf + 17, NULL, 10);
			}
		else if (strncmp(buf, "Instruments", 11) == 0 && length > 17)
			{
			XmpExtraData *xd = xmp_extra_data_new(sd);
			xd->instruments = (gint)strtol(buf + 17, NULL, 10);
			}
		else if (strncmp(buf, "Channels", 8) == 0 && length > 17)
			{
			XmpExtraData *xd = xmp_extra_data_new(sd);
			xd->channels = (gint)strtol(buf + 17, NULL, 10);
			}
		else if (strncmp(buf, "Module type", 11) == 0 && length > 17)
			{
			XmpExtraData *xd = xmp_extra_data_new(sd);
			g_free(xd->type);
			xd->type = g_strdup(buf + 17);
			}
		else if (strncmp(buf, "Tracker name", 12) == 0 && length > 17)
			{
			XmpExtraData *xd = xmp_extra_data_new(sd);
			g_free(xd->tracker);
			xd->tracker = g_strdup(buf + 17);
			}
		}

	pclose(f);

	return ret;
}

/*
 *----------------------------------------------------------------------------
 * module callback funcs
 *----------------------------------------------------------------------------
 */

static void xmp_data_init(SongData *sd)
{
	sd->type_description = _("MOD format audio file");
}

static gint xmp_data_set(SongData *sd, gint read_info)
{
	if (read_info && !sd->info_loaded)
		{
		sd->info_loaded = TRUE;

		if (!xmp_data_load(sd))
			{
			sd->flags |= SONG_FLAG_NOT_FOUND;
			}
		}

	return TRUE;
}

/*
 *----------------------------------------------------------------------------
 * module play funcs
 *----------------------------------------------------------------------------
 */

static gint xmp_start(SongData *sd, gint position)
{
	gint ret;

	if (debug_mode) printf("io_xmp.c: play started at %d\n", position);
	xmp_current_sd = sd;

	ret = xmp_action_start(sd, position);

	playback_done_command(EXEC_PLAY, !ret);

	return ret;
}

static gint xmp_stop(SongData *sd)
{
	if (debug_mode) printf("play stopped\n");
	if (sd != xmp_current_sd)
		{
		printf("io_xmp.c warning: attempt to stop playback of non matching file\n");
		}
	xmp_action_stop(sd);
	xmp_current_sd = NULL;

	playback_done_command(EXEC_STOP, FALSE);
	return TRUE;
}

static gint xmp_pause(SongData *sd)
{
	if (debug_mode) printf("io_xmp.c: play paused at %d\n", seconds);
	if (sd != xmp_current_sd)
		{
		printf("io_xmp.c warning: attempt to pause playback of non matching file\n");
		}

	xmp_action_pause(sd);

	playback_done_command(EXEC_PAUSE, FALSE);
        return TRUE;
}

static gint xmp_continue(SongData *sd)
{
	gint ret;

	if (debug_mode) printf("io_xmp.c: play restarted at %d\n", seconds);
	if (sd != xmp_current_sd)
		{
		printf("io_xmp.c warning: attempt to continue playback of non matching file\n");
		}
	xmp_current_sd = sd;
	ret = xmp_action_continue(sd);

        playback_done_command(EXEC_CONTINUE, !ret);
        return ret;
}

static gint xmp_seek(SongData *sd, gint position)
{
	gint ret;

	if (debug_mode) printf("io_xmp.c: play seeking to %d\n", position);
	if (sd != xmp_current_sd)
		{
		printf("io_xmp.c warning: attempt to seek in non matching file\n");
		}

	seconds_remaining += seconds - position;
	seconds = position;

	ret = xmp_action_seek(position);

	playback_done_command(EXEC_SEEK, !ret);
	return ret;
}

/*
 *----------------------------------------------------------------------------
 * player module interface routines
 *----------------------------------------------------------------------------
 */

void xmp_init(void)
{
	IO_ModuleData *imd;
	gchar *mod_desc = "MOD file";
	gint id;

	if (debug_mode) printf("xmp module initing...\n");

	xmp_enabled = file_in_path(XMP_BINARY);
	if (!xmp_enabled)
		{
		printf(_("Failed to find %s in your path!\n"), XMP_BINARY);
		printf(_("xmp player module disabled.\n"));
		}

	imd = g_new0(IO_ModuleData, 1);

	imd->title = "xmp";
	imd->description = _("mod player");

	if (xmp_enabled)
		{
		imd->songdata_init_func = xmp_data_init;
		imd->songdata_info_func = xmp_data_set;

		imd->start_func = xmp_start;
		imd->stop_func = xmp_stop;
		imd->pause_func = xmp_pause;
		imd->continue_func = xmp_continue;
		imd->seek_func = xmp_seek;

		imd->info_func = mod_create_info_window;
		}

	imd->config_load_func = xmp_config_load;
	imd->config_save_func = xmp_config_save;
	imd->config_init_func = xmp_config_init;
	imd->config_apply_func = xmp_config_apply;
	imd->config_close_func = xmp_config_close;

	id = player_module_register(imd);

	module_register_file_suffix_type(".mod", mod_desc, id);

	/* this is what happens when everyone expands on the standard in their own way ;) */
	module_register_file_suffix_type(".669", mod_desc, id);
	module_register_file_suffix_type(".amf", mod_desc, id);
	module_register_file_suffix_type(".dsm", mod_desc, id);
	module_register_file_suffix_type(".far", mod_desc, id);
	module_register_file_suffix_type(".gdm", mod_desc, id);
	module_register_file_suffix_type(".it" , mod_desc, id);
	module_register_file_suffix_type(".imf", mod_desc, id);
	module_register_file_suffix_type(".med", mod_desc, id);
	module_register_file_suffix_type(".m15", mod_desc, id);
	module_register_file_suffix_type(".mtm", mod_desc, id);
	module_register_file_suffix_type(".okt", mod_desc, id);
	module_register_file_suffix_type(".s3m", mod_desc, id);
	module_register_file_suffix_type(".stm", mod_desc, id);
	module_register_file_suffix_type(".stx", mod_desc, id);
	module_register_file_suffix_type(".ult", mod_desc, id);
	module_register_file_suffix_type(".uni", mod_desc, id);
	module_register_file_suffix_type(".xm" , mod_desc, id);
}



