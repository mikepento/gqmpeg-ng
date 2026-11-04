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
#include "io_mpg123.h"

#include "players.h"
#include "ui_fileops.h"
#include "ui_utildlg.h"

#include <fcntl.h>
#include <errno.h>


/* ----------------------------------------------------------
   input / output interface to mpg123
   ----------------------------------------------------------*/

#define MPG123_BINARY "mpg123"

#define MPG123_VER_0_59O 0	/* versions 0.59o and 0.59p */
#define MPG123_VER_0_59Q 1	/* version 0.59q */
#define MPG123_VER_UNKNOWN 9	/* unknown, treat as latest in case newer ver */

#define MPG123_VER_STRING "0.59q"	/* text representation of latest version */

enum {
	MPG123_TYPE_FILE,
	MPG123_TYPE_HTTP
};

typedef struct {
        gint read_id3;
        gint is_http;
} Mpg123ExtraData;


gint mpg123_enabled = FALSE;

	/* mpg123 options */
gint mpg123_buffer_enable = FALSE;
gint mpg123_buffer = 512;
gint mpg123_downsample = MPG123_DOWNSAMPLE_AUTO;
gint mpg123_custom_sample_size = 44100;
gint mpg123_mono = FALSE;
gint mpg123_8bit = FALSE;
gint mpg123_device_enable = FALSE;
gchar *mpg123_device = NULL;
gint mpg123_aggressive = FALSE;
gint mpg123_disable_resync = FALSE;
gint mpg123_to_wav = FALSE;
gint mpg123_to_raw = FALSE;
gint mpg123_to_wav_path_enable = FALSE;
gchar *mpg123_to_wav_path = NULL;
gint mpg123_read_id3_tags = TRUE;
gchar *mpg123_extra_options = NULL;

/* internal stuff */
static int mpg123_pipe[2];
static gint mpg123_pipe_read_id = -1;
static gint mpg123_data_check_id = -1;
static gint mpg123_version = MPG123_VER_0_59O;

static SongData *current_sd = NULL;


static void stop_playing_file(void);
static gint mpg123_extra_is_http(SongData *sd);
static gchar *http_path_ensure_root(const gchar *path);

/*
 *-----------------------------------------------------------------------------
 * input parsers
 *-----------------------------------------------------------------------------
 */

static gint parse_till_char(gchar **ptr, gchar c)
{
	gchar *start;
	gchar *end;
	gchar tmp;
	gint val;

	start = *ptr;
	while (*start == ' ') start++;
	end = start;
	while (*end != c && *end != '\0') end++;

	tmp = *end;
	*end = '\0';
	val = (gint)strtol(start, NULL, 10);
	*end = tmp;

	*ptr = end;

	return val;
}

static gint parse_header_info(gchar *text)
{
	gchar buf[128];
	gchar *ptr;
	gchar *ptr2;
	gchar *max;
	gchar *end;

	max = buf + sizeof(buf) - 1;
	end = text + strlen(text);

	if (debug_mode > 2) printf("mpg123 header: ");

	ptr = text;

	/* Version line */
	if (strncmp(text, "Version", 7) == 0 && text[7] != '\0')
		{
		if (debug_mode > 2) printf("Version\n");

		ptr += 8;
		ptr2 = buf;
		while (*ptr != ' ' && *ptr != '\0')
			{
			*ptr2 = *ptr;
			if (ptr2 < max) ptr2++;
			ptr++;
			}
		*ptr2 = '\0';
		if (strcmp(buf, "0.59o") == 0)
			{
			mpg123_version = MPG123_VER_0_59O;
			}
		else if (strcmp(buf, "0.59p") == 0)
			{
			mpg123_version = MPG123_VER_0_59O;
			}
		else if (strcmp(buf, "0.59q") == 0)
			{
			mpg123_version = MPG123_VER_0_59Q;
			}
		else if (strcmp(buf, "0.59r") == 0)
			{
			mpg123_version = MPG123_VER_0_59Q;
			}
		else if (strncmp(buf, "0.59o", 5) < 1)
			{
			static gint warning_done = FALSE;
			mpg123_version = MPG123_VER_0_59O;
			if (debug_mode || !warning_done)
				{
				printf(_("mpg123 version prior to 0.59o found!\n"));
				printf(_("assuming 0.59o compatible\n"));
				printf(_("The version line reported was:\n%s\n"), text);
				}
			warning_done = TRUE;
			}
		else
			{
			static gint warning_done = FALSE;
			mpg123_version = MPG123_VER_UNKNOWN;
			if (debug_mode || !warning_done)
				{
				printf(_("unknown version of mpg123, assuming %s compatible\n"), MPG123_VER_STRING);
				printf(_("(GQmpeg requires mpg123 0.59o or later)\n"));
				printf(_("The version line reported was:\n%s\n"), text);
				}
			warning_done = TRUE;
			}
		if (debug_mode) printf(_("mpg123 version detected: %d\n"), mpg123_version);

		return TRUE;
		}

	/* MPEG, layer, freq, mode line */
	if (strncmp(text, "MPEG", 4) == 0)
		{
		if (debug_mode > 2) printf("MPEG\n");

		ptr += 4;
		ptr2 = buf;
		while (*ptr != 'L' && *ptr != '\0')
			{
			if (*ptr != ' ' && *ptr != ',')
				{
				*ptr2 = *ptr;
				if (ptr2 < max) ptr2++;
				}
			ptr++;
			}
		*ptr2 = '\0';
		mpeg_version = strtol(buf, NULL, 10);

		if (ptr + 6 > end) return TRUE;

		ptr += 6;
		ptr2 = buf;
		while (*ptr != 'F' && *ptr != '\0')
			{
			if (*ptr != ' ' && *ptr != ',')
				{
				*ptr2 = *ptr;
				if (ptr2 < max) ptr2++;
				}
			ptr++;
			}
		*ptr2 = '\0';
		mpeg_layer = strlen(buf);

		if (ptr + 5 > end) return TRUE;

		ptr += 5;
		ptr2 = buf;
		while (*ptr != 'm' && *ptr != '\0')
			{
			if (*ptr != ' ' && *ptr != ',')
				{
				*ptr2 = *ptr;
				if (ptr2 < max) ptr2++;
				}
			ptr++;
			}
		*ptr2 = '\0';
		input_hz = strtol(buf, NULL, 10);

		if (ptr + 5 > end) return TRUE;

		ptr += 5;
		ptr2 = buf;
		while (*ptr != ',' && *ptr != '\0')
			{
			if (*ptr != ' ')
				{
				*ptr2 = *ptr;
				if (ptr2 < max) ptr2++;
				}
			ptr++;
			}
		*ptr2 = '\0';
		if (strcmp(buf, "Stereo") == 0) mpeg_mode = 1;
		else if (strcmp(buf, "Joint-Stereo") == 0) mpeg_mode = 2;
		else if (strcmp(buf, "Dual-Channel") == 0) mpeg_mode = 3;
		else if (strcmp(buf, "Single-Channel") == 0) mpeg_mode = 4;

		return TRUE;
		}

	/* Channels  line */
	if (strncmp(text, "Channels:", 9) == 0)
		{
		if (debug_mode > 2) printf ("Channels\n");

		ptr += 9;
		input_channels = parse_till_char(&ptr, ',');

		return TRUE;
		}

	/* Bitrate line */
	if (strncmp(text, "Bitrate:", 8) == 0)
		{
		if (debug_mode > 2) printf("Bitrate\n");

		ptr += 8;
		input_bitrate = parse_till_char(&ptr, 'K');

		return TRUE;
		}

	/* Audio output line */
	if (strncmp(text, "Audio:", 6) == 0)
		{
		if (debug_mode > 2) printf("Audio\n");

		ptr += 6;
		/* output_conversion = (ignored) */
		parse_till_char(&ptr, ',');

		if (ptr + 7 > end) return TRUE;

		ptr += 7;
		ptr2 = buf;
		while (*ptr != 'e' && *ptr != '\0')
			{
			if (*ptr != ' ' && *ptr != ',')
				{
				*ptr2 = *ptr;
				if (ptr2 < max) ptr2++;
				}
			ptr++;
			}
		*ptr2 = '\0';
		output_hz = strtol(buf, NULL, 10);

		if (ptr + 9 > end) return TRUE;

		ptr += 9;
		ptr2 = buf;
		while (*ptr != ',' && *ptr != '\0')
			{
			if (*ptr >= '0' && *ptr <= '9')
				{
				*ptr2 = *ptr;
				if (ptr2 < max) ptr2++;
				}
			ptr++;
			}
		*ptr2 = '\0';
		output_bits = strtol(buf, NULL, 10);

		if (ptr + 11 > end) return TRUE;

		ptr += 11;
		output_channels = parse_till_char(&ptr, '\n');

		new_song = TRUE;
		return TRUE;
		}

	if (debug_mode > 2) printf("not parsed\n");

	return FALSE;
}

/* parse the output to get the current song position
 * including seconds and frames (played and remaining)
 */
static gint parse_frame_info(gchar *text)
{
	gchar *ptr;

	gint frame_cnt;
	gint frame_cnt_rem;
	gint sec_cnt;
	gint sec_cnt_rem;
	gint min_cnt;
	gint min_cnt_rem;

	if (strncmp(text, "Frame", 5) != 0 || text[6] == '\0') return FALSE;

	if (debug_mode > 2) printf("Frame\n");

	ptr = text + 6;

	frame_cnt = parse_till_char(&ptr, '[');
	if (*ptr == '\0') return TRUE;
	ptr++;

	frame_cnt_rem = parse_till_char(&ptr, ']');
	while (*ptr != ':' && *ptr != '\0') ptr++;
	if (*ptr == '\0') return TRUE;
	ptr++;

	if (mpg123_version == MPG123_VER_0_59O) /* 0.00 [000.00] */
		{
		sec_cnt = parse_till_char(&ptr, '[');
		if (*ptr == '\0') return TRUE;
		ptr++;

		sec_cnt_rem = parse_till_char(&ptr, ']');

		min_cnt = 0;
		min_cnt_rem = 0;
		}
	else /* 00:00.00 [00:00.00] */
		{
		min_cnt = parse_till_char(&ptr, ':');
		if (*ptr == '\0') return TRUE;
		ptr++;

		sec_cnt = parse_till_char(&ptr, '[');
		if (*ptr == '\0') return TRUE;
		ptr++;

		min_cnt_rem = parse_till_char(&ptr, ':');
		if (*ptr == '\0') return TRUE;
		ptr++;

		sec_cnt_rem = parse_till_char(&ptr, ']');
		}

	frames = frame_cnt;
	frames_remaining = frame_cnt_rem;
	seconds = min_cnt * 60 + sec_cnt;
	seconds_remaining = min_cnt_rem * 60 + sec_cnt_rem;
	
	/* printf("min = [%d] sec = [%d] (%d)\n", min_cnt, sec_cnt, seconds); */

	return TRUE;
}

static void handle_warnings(gchar *text)
{
	if (!text || strcmp(text, "") == 0) return;
	/* normal message output */
	if (strncmp(text, "High Perf", 9) == 0) return;
	if (strncmp(text, "Uses code", 9) == 0) return;
	if (strncmp(text, "THIS SOFT", 9) == 0) return;
	if (strncmp(text, "Playing M", 9) == 0) return;
	/* ID3 tags */
	if (strncmp(text, "Title  :", 8) == 0) return;
	if (strncmp(text, "Album  :", 8) == 0) return;
	if (strncmp(text, "Comment:", 8) == 0) return;
	/* misc output */
	if (strncmp(text, "Junk at t", 9) == 0) return;
	if (strncmp(text, "mpg123: Can't rewind stream", 27) == 0) return;
	if (strncmp(text, "Directory:", 10) == 0) return;
	if (strncmp(text, "Getting real-time priority", 26) == 0) return;

	/* mpg123 0.59s-pre */
	if (strncmp(text, "Found new ID3 Header", 20) == 0) return;

	/* for freeamp in mpg123 compat mode */
	if (strncmp(text, "Copyright", 9) == 0) return;
	if (strncmp(text, "This soft", 9) == 0) return;

	/* Tru64's mpg123 version reports the device it uses on start */
	if (strncmp(text, "Using device", 12) == 0) return;

	/* ignore lines that begin with an escape sequence (0.59r) */
	if (text[0] == 27 && text[1] == ']') return;

	printf(_("Warning, unrecognized mpg123 output:\"%s\"\n"), text);

	if (debug_mode)
		{
		gchar *ptr = text;
		while (*ptr != '\0')
			{
			printf("[%d]", *ptr);
			ptr++;
			}
		printf("\n");
		}
}

static void pipe_reader_shutdown(void)
{
	if (mpg123_data_check_id != -1) gtk_timeout_remove(mpg123_data_check_id);
	mpg123_data_check_id = -1;

	if (mpg123_pipe_read_id != -1) g_source_remove(mpg123_pipe_read_id);
	mpg123_pipe_read_id = -1;

	close(mpg123_pipe[0]);
	mpg123_pipe[0] = -1;
}

static void stop_data(void)
{
	pipe_reader_shutdown();
	pid = 0;
	if (debug_mode) printf("mpg123 closed\n");
}

static void song_finished(void)
{
	stop_data();
	module_playback_end();
	current_sd = NULL;
}

/* this routine parses the output from mpg123 to check it's current status
   when the end is detected, 1 is returned; any error returns -1;
   a detected frame (second & frame info) return 1; 2 is returned
   on anything else (including header info) */
static gint parse_pipe_buffer(gchar *text)
{
	if (debug_mode > 1) printf("mpg123 output:\"%s\"\n", text);

	if (parse_frame_info(text)) return 1;

	/* check for end of song (mpg123 reports a line starting
		with a '[') */
	if (strncmp(text, "[", 1) == 0)
		{
		waitpid(pid, NULL, 0);
		song_finished();
		return 0;
		}

	/* start looking for mpg123 */

	/* mpg123 errors */
	if (strncmp(text, "Can't open", 10) == 0 ||
	    strncmp(text, "connect", 7) == 0 ||
	    strstr(text, "file or directory") ||
	    strncmp(text, "HTTP", 4) == 0 ||
	    strstr(text, "Device busy") )
		{
		gchar *buf;

		printf(_("Error, mpg123 reported:\"%s\"\n"), text);
		buf = g_strdup_printf("mpg123: %s", text);
		warning_dialog(_("GQmpeg: mpg123 error"), buf);
		g_free(buf);

		stop_playing_file();
		module_playback_error(current_sd);
		current_sd = NULL;
		return -1;
		}

	if (!parse_header_info(text))
		{
		/* Unknown output, print it, it may be relevent */
		handle_warnings(text);
		}

	return 2;
}

static void handle_end_of_file(void)
{
	printf(_("mpg123 disappeared! (unexpected EOF)\n"));
	stop_playing_file();
	module_playback_error(current_sd);
	current_sd = NULL;
}

static void handle_read_error(void)
{
	printf(_("error reading from mpg123!\n"));
	stop_playing_file();
	module_playback_error(current_sd);
	current_sd = NULL;
}

static time_t mpg123_data_check_last_time = 0;

static gint mpg123_data_check_update_cb(gpointer data)
{
	time_t secs;
	time_t new_time;

	secs = (time_t)GPOINTER_TO_INT(data);
	new_time = time(NULL);

	if (new_time > mpg123_data_check_last_time + secs)
		{
		printf(_("Error: mpg123 stopped sending data! (timeout)\n"));
		stop_playing_file();
		module_playback_error(current_sd);
		current_sd = NULL;

		mpg123_data_check_id = -1;
		return FALSE;
		}

	return TRUE;
}

static void mpg123_data_check_update(gint secs)
{
	if (mpg123_data_check_id == -1)
		{
		mpg123_data_check_id = gtk_timeout_add(1000, mpg123_data_check_update_cb,
						       GINT_TO_POINTER(secs));
		}
	mpg123_data_check_last_time = time(NULL);
}

static char mpg123_read_buffer[1024];
static gint mpg123_read_buffer_pos = 0;

static gboolean mpg123_pipe_read_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	gint is_http = GPOINTER_TO_INT(data);
	gchar buf[1024];
	gint r;
	gint p;
	gint l;
	gchar *ptr;

	if (status != STATUS_PLAY)
		{
		pipe_reader_shutdown();
		return TRUE;
		}

	r = read(mpg123_pipe[0], buf, sizeof(buf));

	if (debug_mode > 2) printf("mpg123 read input %d bytes\n", r);

	if (r == -1)
		{
		handle_read_error();
		return TRUE;
		}

	if (r == 0)
		{
		handle_end_of_file();
		return TRUE;
                }

	ptr = mpg123_read_buffer + mpg123_read_buffer_pos;
	l = sizeof(mpg123_read_buffer) - 1;
	p = 0;
	while(p < r)
		{
		gchar c;
		c = buf[p];
		p++;

		if (c == '\r' || c == '\n' || c == 7) /* the 7 is for 0.59r and it's escape sequences */
			{
			if (mpg123_read_buffer_pos > 0)
				{
				mpg123_read_buffer[mpg123_read_buffer_pos] = '\0';
				if (parse_pipe_buffer(mpg123_read_buffer) == -1) return TRUE;
				if (status == STATUS_NEXT)
					{
					pipe_reader_shutdown();
					return TRUE;
					}
				mpg123_read_buffer_pos = 0;
				}
			}
		else if (mpg123_read_buffer_pos < l)
			{
			mpg123_read_buffer[mpg123_read_buffer_pos] = c;
			mpg123_read_buffer_pos++;
			}
		}

	/* Check to see if mpg123 sent data within the last
	 * 10 seconds. No data means mpg123 encountered an error we
	 * did not catch or it crashed.
	 * NOTE: 20 seconds (132 times) for http.
	 */
	mpg123_data_check_update((is_http) ? 20 : 10);

	return TRUE;
}

static void mpg123_input_read_reset(void)
{
	mpg123_read_buffer_pos = 0;
}

#define MPG123_MAX_COMMANDS 32

static gint start_playing_file(SongData *sd, gint position)
{
	pid_t frk_pid;
	char cmd_arguments[MPG123_MAX_COMMANDS][512];
	char *cmd_ptr[MPG123_MAX_COMMANDS];
	size_t cmd_max;
	int cmd_cnt = 0;
	gint start_frame;
	gchar *exec_bin = MPG123_BINARY;
	GIOChannel *channel;

	cmd_max = sizeof(cmd_arguments) / MPG123_MAX_COMMANDS;

	if (position == 0)
		{
		start_frame = 0;
		}
	else
		{
		if (seconds_remaining + seconds > 0 && frames_remaining + frames > 0)
			{
			start_frame = (gfloat)position / (seconds_remaining + seconds) * (frames_remaining + frames);
			}
		else
			{
			start_frame = 0;
			}
		}

	/* create all command line arguments */

	strncpy(cmd_arguments[cmd_cnt], MPG123_BINARY, cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	strncpy(cmd_arguments[cmd_cnt], "-v", cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	strncpy(cmd_arguments[cmd_cnt], "-k", cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;
	snprintf(cmd_arguments[cmd_cnt], cmd_max, "%d", start_frame);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	if (mpg123_buffer_enable && !mpg123_to_wav)
		{
		strncpy(cmd_arguments[cmd_cnt], "-b", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		snprintf(cmd_arguments[cmd_cnt], cmd_max, "%d", mpg123_buffer);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (mpg123_downsample != MPG123_DOWNSAMPLE_AUTO)
		{
		if (mpg123_downsample == MPG123_DOWNSAMPLE_22)
			{
			strncpy(cmd_arguments[cmd_cnt], "-2", cmd_max);
			cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
			cmd_cnt++;
			}
		if (mpg123_downsample == MPG123_DOWNSAMPLE_11)
			{
			strncpy(cmd_arguments[cmd_cnt], "-4", cmd_max);
			cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
			cmd_cnt++;
			}
		if (mpg123_downsample == MPG123_DOWNSAMPLE_CUSTOM)
			{
			snprintf(cmd_arguments[cmd_cnt], cmd_max, "-r %d", mpg123_custom_sample_size);
			cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
			cmd_cnt++;
			}
		}

	if (mpg123_mono)
		{
		strncpy(cmd_arguments[cmd_cnt], "-m", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (mpg123_8bit)
		{
		strncpy(cmd_arguments[cmd_cnt], "--8bit", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (mpg123_aggressive)
		{
		strncpy(cmd_arguments[cmd_cnt], "--aggressive", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (mpg123_disable_resync)
		{
		strncpy(cmd_arguments[cmd_cnt], "--resync", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (mpg123_device_enable && mpg123_device && !mpg123_to_wav)
		{
		strncpy(cmd_arguments[cmd_cnt], "-a", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		strncpy(cmd_arguments[cmd_cnt], mpg123_device, cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (mpg123_to_wav)
		{
		const gchar *ext;
		gchar *buf;

		if (mpg123_to_raw)
			{
			strncpy(cmd_arguments[cmd_cnt], "--cdr", cmd_max);
			cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
			cmd_cnt++;
			ext = ".raw";
			}
		else
			{
			strncpy(cmd_arguments[cmd_cnt], "-w", cmd_max);
			cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
			cmd_cnt++;
			ext = ".wav";
			}

		if (mpg123_to_wav_path_enable && mpg123_to_wav_path)
			{
			gchar *tmp = remove_extension_from_path(filename_from_path(sd->path));
			buf = g_strconcat(mpg123_to_wav_path, "/", tmp, ext, NULL);
			g_free(tmp);
			}
		else
			{
			gchar *tmp = remove_extension_from_path(sd->path);
			buf = g_strconcat(tmp, ".wav", NULL);
			g_free(tmp);
			}
		strncpy(cmd_arguments[cmd_cnt], buf, cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;

		g_free(buf);
		}

	if (mpg123_extra_options &&
	    strlen(mpg123_extra_options) > 0 &&
	    cmd_cnt + 2 < MPG123_MAX_COMMANDS)
		{
		gchar **vector;
		gint i;

		vector = g_strsplit(mpg123_extra_options, " ", MPG123_MAX_COMMANDS - 2 - cmd_cnt);
		i = 0;

		while(vector && vector[i] != NULL)
			{
			strncpy(cmd_arguments[cmd_cnt], vector[i], cmd_max);
			cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
			cmd_cnt++;
			i++;
			}
		g_strfreev(vector);
		}

	if (mpg123_extra_is_http(sd))
		{
		gchar *buf;
		buf = http_path_ensure_root(sd->path);
		strncpy(cmd_arguments[cmd_cnt], buf, cmd_max);
		g_free(buf);
		}
	else
		{
		strncpy(cmd_arguments[cmd_cnt], sd->path, cmd_max);
		}
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	strncpy(cmd_arguments[cmd_cnt], "", cmd_max);
	cmd_ptr[cmd_cnt] = NULL;
	cmd_cnt++;


	/* Create the pipe. */
	if (pipe (mpg123_pipe))
		{
		fprintf (stderr, _("Pipe failed.\n"));
		return FALSE;
		}

	if (debug_mode) printf("opening: %s\n", sd->path);

	/* Create the child process. */
	frk_pid = fork ();
	if (frk_pid == (pid_t) 0)
		{
		/* This is the child process. */
		dup2(mpg123_pipe[1],2);
		close(mpg123_pipe[0]);

		/* set the group (session) id to this process for future killing */
		setsid();

		execvp(exec_bin, cmd_ptr);
		printf(_("unable to run %s (in the path?)\n"), exec_bin);
		_exit(1);
		}
	else if (frk_pid < (pid_t) 0)
		{
		/* The fork failed. */
		fprintf (stderr, _("Fork failed.\n"));
		pid = 0;
		return FALSE;
		}
	else
		{
		/* This is the parent process. */
		pid = (int) frk_pid;
		fcntl(mpg123_pipe[0], F_SETFL, O_NONBLOCK);
		close(mpg123_pipe[1]);
		mpg123_pipe[1] = -1;
		}

	if (debug_mode) printf("mpg123 pid = %d\n", pid);

	mpg123_input_read_reset();
	channel = g_io_channel_unix_new(mpg123_pipe[0]);
	mpg123_pipe_read_id = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, G_IO_IN,
			      mpg123_pipe_read_cb, GINT_TO_POINTER(mpg123_extra_is_http(sd)), NULL);
	g_io_channel_unref(channel);
#if 0
	mpg123_pipe_read_id = gdk_input_add(mpg123_pipe[0], GDK_INPUT_READ,
					    mpg123_pipe_read_cb, GINT_TO_POINTER(mpg123_extra_is_http(sd)));
#endif

	return TRUE;
}

static void stop_playing_file(void)
{
	pid_t t;

	if (pid <= 0) return;

	if (debug_mode) printf("sending SIGTERM to pid = -%d\n", pid);

	/* kill the entire mpg123 group to work around mpg123 buffer bug */
	if (kill(-pid, SIGINT) ==-1)
		{
		/* eek!, maybe mpg123 is still in startup, so wait, try again */
		if (debug_mode) printf("waiting 1 sec to try again: pid = -%d\n", pid);
		sleep(1);
		if (kill(-pid, SIGINT) == -1)
			{
			printf(_("Failed to successfully send signal to pid = %d\n"), (int)pid);
			}
		}

	if (debug_mode) printf("first waitpid = %d\n", (int)pid);
	t = waitpid (pid, NULL, WNOHANG);
	if (t != pid && t != -1)
		{
		if (debug_mode) printf("second waitpid, 1st returned: %d\n", t);
		/* this one WILL HANG, so if someone sent a SIGSTOP to mpg123... */
		waitpid (pid, NULL, 0);
		if (debug_mode) printf("second waitpid done.\n");
		}

	stop_data();
}

/*
 *----------------------------------------------------------------------------
 * mpg123 module extra data
 *----------------------------------------------------------------------------
 */

static void mpg123_extra_data_free(gpointer data)
{
	Mpg123ExtraData *xd = data;

	if (!xd) return;

	g_free(xd);
}

static Mpg123ExtraData *mpg123_extra_data_new(SongData *sd)
{
	Mpg123ExtraData *xd;

	if (!sd) return NULL;

	if (sd->data) return sd->data;

	xd = g_new0(Mpg123ExtraData, 1);
	xd->read_id3 = FALSE;
	xd->is_http = type_is_http(sd->path);

	sd->data = xd;
	sd->free_data_func = mpg123_extra_data_free;

	return xd;
}

static gint mpg123_extra_is_http(SongData *sd)
{
	Mpg123ExtraData *xd;

	if (!sd || !sd->data) return FALSE;

	xd = sd->data;
	return xd->is_http;
}

static gchar *http_path_ensure_root(const gchar *path)
{
	const gchar *p;

	if (!path) return NULL;

	if (strncmp(path, "http:/", 6) != 0) return g_strdup(path);
	p = path + 6;

	while (*p == '/') p++;

	while (*p != '/' && *p != ':' && *p != '\0') p++;
	if (*p == '/') return g_strdup(path);
	if (*p == '\0') return g_strconcat (path, "/", NULL);

	p++;
	while (*p != '/' && *p != '\0') p++;
	if (*p == '/') return g_strdup(path);

	return g_strconcat (path, "/", NULL);
}

/*
 *----------------------------------------------------------------------------
 * mpg123 module callback funcs
 *----------------------------------------------------------------------------
 */

gint type_is_http(const gchar *path)
{
	if (!path || path[0] != 'h') return FALSE;
	if (strncmp(path, "http://", 7) == 0) return TRUE;

	return FALSE;
}

static void mpg123_data_init(SongData *sd)
{
	sd->type_description = _("MPEG audio file");
	mpg123_extra_data_new(sd);
}

static gint mpg123_data_set(SongData *sd, gint read_info)
{
	if (mpg123_extra_is_http(sd))
		{
		if (read_info && !sd->info_loaded)
			{
			gchar *buf;
			gchar *p;

			sd->info_loaded = TRUE;

			buf = g_strdup(sd->path + 7);

			if (strchr(buf, '/') == NULL)
				{
				sd->title = buf;
				}
			else
				{
				/* well, this won't change when the option changes after initial load */
				if (!title_show_extension)
					{
					sd->title = remove_extension_from_path(filename_from_path(buf));
					}
				else
					{
					sd->title = g_strdup(filename_from_path(buf));
					}
				if (title_convert_underscores) convert_chars(sd->title, '_', ' ');
				if (!sd->title || strlen(sd->title) < 1)
					{
					g_free(sd->title);
					sd->title = g_strdup(buf);
					}
				g_free(buf);
				}

			/* set the comment to contain base url */
			if (!sd->comment)
				{
				buf = g_strdup(sd->path);
				p = buf + 7;
				while (*p != '\0')
					{
					if (*p == ':' || *p == '/') *p = '\0';
					p++;
					}
				sd->comment = g_strdup(buf);
				g_free(buf);
				}
			}
		return TRUE;
		}

	if (read_info)
		{
		Mpg123ExtraData *xd = mpg123_extra_data_new(sd);

		if (mpg123_read_id3_tags && !xd->read_id3)
			{
			ID3_Data *idd;

			idd = get_id3_tag(sd->path);
			if (idd)
				{
				sd->title = idd->title;
				sd->artist = idd->artist;
				sd->album = idd->album;
				sd->year = idd->year;
				sd->comment = idd->comment;
				sd->genre = idd->genre_description;
				sd->track = idd->track;

				g_free(idd); /* don't free members */
				}
			xd->read_id3 = TRUE;
			}

		if (!sd->info_loaded)
			{
			Mpg_Data *md;

			md = get_mpg_header_info(sd->path);
			if (md)
				{
				sd->length = md->length;
				sd->live = FALSE;
				sd->bit_rate = md->bit_rate;
				sd->bit_depth = 16;
				sd->khz_rate = md->sample_rate;
				if (md->stereo)
					{
					sd->channels = 2;
					}
				else
					{
					sd->channels = 1;
					}

				g_free(md);
				}
			else
				{
				sd->flags |= SONG_FLAG_NOT_FOUND;
				}
			}
		sd->info_loaded = TRUE;
		}

	return TRUE;
}

/*
 *----------------------------------------------------------------------------
 * mpg123 module http edit funcs
 *----------------------------------------------------------------------------
 */

static GtkWidget *mpg123_http_entry_setup(const gchar *path)
{
	GtkWidget *vbox;
	GtkWidget *entry;

	vbox = gtk_vbox_new(FALSE, 0);

	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), "http://");
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show(entry);

	g_object_set_data(G_OBJECT(vbox), "http_URL", entry);

	return vbox;
}

static GtkWidget *mpg123_http_edit(SongData *sd)
{
	GtkWidget *vbox;
	GtkWidget *entry;

	vbox = gtk_vbox_new(FALSE, 0);

	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), sd->path);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_widget_show(entry);

	g_object_set_data(G_OBJECT(vbox), "http_URL", entry);

	return vbox;
}

static gchar *mpg123_http_get_path(GtkWidget *widget)
{
	GtkWidget *entry = g_object_get_data(G_OBJECT(widget), "http_URL");

	if (!entry) return NULL;

	return g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

/*
 *----------------------------------------------------------------------------
 * mpg123 module play funcs
 *----------------------------------------------------------------------------
 */

static gint mpg123_start(SongData *sd, gint position)
{
	gint ret;

	if (debug_mode) printf("play started at %d\n", position);
	current_sd = sd;

	if (mpg123_extra_is_http(sd))
		{
		ret = start_playing_file(sd, 0);
		}
	else
		{
		ret = start_playing_file(sd, position);
		}

	playback_done_command(EXEC_PLAY, !ret);

	return ret;
}

static gint mpg123_stop(SongData *sd)
{
	if (debug_mode) printf("play stopped\n");
	if (sd != current_sd)
		{
		printf(_("io_mpg123.c warning: attempt to stop playback of non matching file\n"));
		}
	stop_playing_file();
	current_sd = NULL;
	playback_done_command(EXEC_STOP, FALSE);
	return TRUE;
}

static gint mpg123_pause(SongData *sd)
{
	if (debug_mode) printf("play paused at %d\n", seconds);
	if (sd != current_sd)
		{
		printf(_("io_mpg123.c warning: attempt to pause playback of non matching file\n"));
		}
	stop_playing_file();
	if (mpg123_extra_is_http(sd))
		{
		seconds = 0;
		frames = 0;
		}
	playback_done_command(EXEC_PAUSE, FALSE);
	return TRUE;
}

static gint mpg123_continue(SongData *sd)
{
	gint ret;

	if (debug_mode) printf("play restarted at %d\n", seconds);
	if (sd != current_sd)
		{
		printf(_("io_mpg123.c warning: attempt to continue playback of non matching file\n"));
		}
	current_sd = sd;
	if (mpg123_extra_is_http(sd))
		{
		start_playing_file(sd, 0);
		}
	ret = start_playing_file(sd, seconds);
	playback_done_command(EXEC_CONTINUE, !ret);
	return ret;
}

static gint mpg123_seek(SongData *sd, gint position)
{
	if (debug_mode) printf("play seeking to %d\n", position);
	if (sd != current_sd)
		{
		printf(_("io_mpg123.c warning: attempt to seek in non matching file\n"));
		}
	if (mpg123_extra_is_http(sd))
		{
		/* live streams don't seek */
		playback_done_command(EXEC_SEEK, FALSE);
		return TRUE;
		}

	seconds_remaining += seconds - position;
	seconds = position;

	if (status == STATUS_PLAY)
		{
		gint ret;
		stop_playing_file();
		current_sd = sd;
		ret = start_playing_file(sd, position);
		playback_done_command(EXEC_SEEK, !ret);
		return ret;
		}
	else
		{
		playback_done_command(EXEC_SEEK, FALSE);
		return TRUE;
		}
}

/*
 *----------------------------------------------------------------------------
 * player module interface routines
 *----------------------------------------------------------------------------
 */

void mpg123_init(void)
{
	IO_ModuleData *imd;
	gchar *mp3_desc = "MP3 file";
	gint id;

	mpg123_enabled = file_in_path(MPG123_BINARY);
	if (!mpg123_enabled)
		{
		printf(_("Failed to find mpg123 in your path!\n"));
		printf(_("mpg123 player module disabled.\n"));
		}

	imd = g_new0(IO_ModuleData, 1);

	imd->title = "mpg123";
	imd->description = _("mp3 player");

	if (mpg123_enabled)
		{
		imd->songdata_init_func = mpg123_data_init;
		imd->songdata_info_func = mpg123_data_set;

		imd->start_func = mpg123_start;
		imd->stop_func = mpg123_stop;
		imd->pause_func = mpg123_pause;
		imd->continue_func = mpg123_continue;
		imd->seek_func = mpg123_seek;

		imd->info_func = mpg_create_info_window;
		}

	imd->config_load_func = mpg123_config_load;
	imd->config_save_func = mpg123_config_save;
	imd->config_init_func = mpg123_config_init;
	imd->config_apply_func = mpg123_config_apply;
	imd->config_close_func = mpg123_config_close;


	id = player_module_register(imd);

	module_register_file_suffix_type(".mp3", mp3_desc, id);
	module_register_file_suffix_type(".mp2", mp3_desc, id);
	module_register_file_suffix_type(".mpeg", mp3_desc, id);
	module_register_file_suffix_type(".mpg", mp3_desc, id);
	module_register_file_suffix_type(".m2a", mp3_desc, id);
	module_register_misc_type(_("MP3/Shoutcast stream"), "http://server:port", id, TRUE,
			type_is_http, mpg123_http_entry_setup, mpg123_http_edit, mpg123_http_get_path);

}

