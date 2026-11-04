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
 * the TODO for this module (using ogg123)
 * > All :)
 */

#include "gqmpeg.h"
#include "io_ogg123.h"

#include "players.h"
#include "ui_fileops.h"
#include "ui_utildlg.h"

#include <fcntl.h>
#include <time.h>

#define OGG123_BINARY "ogg123"
#define OGG123_BINARY_INFO "ogginfo"
#define OGG123_BINARY_COMMENT "vorbiscomment"

/* ----------------------------------------------------------
   input / output interface to ogg123
   ----------------------------------------------------------*/

gint ogg123_enabled = FALSE;
gint ogg123_info_found = FALSE;
gint ogg123_comment_found = FALSE;

gchar *ogg123_device_options = NULL;
gchar *ogg123_extra_options = NULL;

/* internal stuff */
static SongData *ogg123_current_sd = NULL;
static gint ogg123_control_read_id = -1;
static gint ogg123_child_pid = -1;
static int ogg123_pipe[2];


/*
 *-----------------------------------------------------------------------------
 * pipe/fork util
 *-----------------------------------------------------------------------------
 */

static gint ogg123_pipe_open(char *ogg_argv[], gint *ogg_child, int ogg_pipe[2])
{
	pid_t frk_pid;

	/* Create the pipe. */
	if (pipe(ogg_pipe))
		{
		fprintf (stderr, _("Pipe failed.\n"));
		return FALSE;
		}

	if (debug_mode) printf("ogg123_pipe_open: %s\n", ogg_argv[0]);

	/* Create the child process. */
	frk_pid = fork ();
	if (frk_pid == (pid_t) 0)
		{
		/* This is the child process. */
		dup2(ogg_pipe[1], 1);
		dup2(ogg_pipe[1], 2);
		close(ogg_pipe[0]);

		execvp(ogg_argv[0], ogg_argv);
		printf(_("unable to run %s (in the path?)\n"), ogg_argv[0]);
		_exit(1);
		}
	else if (frk_pid < (pid_t) 0)
		{
		/* The fork failed. */
		fprintf (stderr, _("Fork failed.\n"));
		close(ogg_pipe[0]);
		close(ogg_pipe[1]);
		*ogg_child = -1;
		return FALSE;
		}
	else
		{
		/* This is the parent process. */
		*ogg_child = (int) frk_pid;

		close(ogg_pipe[1]);
		ogg_pipe[1] = -1;
		}

	if (debug_mode) printf("ogg123_pipe_open pid = %d\n", *ogg_child);

	return TRUE;
}

static FILE *ogg123_pipe_open_fd(char *ogg_argv[], gint *ogg_child, int ogg_pipe[2])
{
	FILE *f;

	if (!ogg123_pipe_open(ogg_argv, ogg_child, ogg_pipe))
		{
		printf("Failed to run \"%s\n\"", ogg_argv[0]);
		return NULL;
		}
	f = fdopen(ogg_pipe[0], "r");
	if (!f)
		{
		close(ogg_pipe[0]);
		return NULL;
		}

	return f;
}

/*
 *----------------------------------------------------------------------------
 * title/description utils
 *----------------------------------------------------------------------------
 */

static const gchar *info_line_value(gchar *text)
{
	gchar *ptr, *ptr2;

	ptr = text;
	while (*ptr != '=' && *ptr != ':' && *ptr != '\0') ptr++;
	if (*ptr == '\0') return NULL;
	ptr++;

	while (*ptr == ' ') ptr++;
	
	/* remove new line char */
	ptr2 = ptr;
	while (*ptr2 != '\0')
		{
		if (*ptr2 == '\n' || *ptr2 == '\r') *ptr2 = '\0';
		ptr2++;
		}

	return ptr;
}

static gint info_line_key_test(gchar *text, const gchar *key)
{
	if (!text || !key) return FALSE;

	while (*text == ' ' || *text == 9) text++;

	return (strncasecmp(text, key, strlen(key)) == 0);
}

static gchar *info_line_key(gchar *text)
{
	gchar *ptr;
	gchar *ret;

	if (!text) return NULL;

	ptr = text;
	while (*ptr != '=' && *ptr != '\0') ptr++;
	if (*ptr == '\0') return NULL;

	*ptr = '\0';
	ret = g_strdup(text);
	*ptr = '=';

	return ret;
}

static gint info_time_string_to_val(const gchar *text)
{
	const gchar *ptr;
	gint n;
	gint value = 0;

	ptr = text;
	while (*ptr != '\0') ptr++;

	while (ptr > text)
		{
		gint mp;
		gchar *tmp;

		switch (*ptr)
			{
			case 's':
				mp = 1;
				break;
			case 'm':
				mp = 60;
				break;
			case 'h':
				mp = 3600;
				break;
			default:
				ptr--;
				continue;
				break;
			}
		ptr--;
		n = 1;
		while (ptr > text && *ptr != ':')
			{
			n++;
			ptr--;
			}

		if (*ptr == ':')
			{
			tmp = g_strndup(ptr + 1, n - 1);
			}
		else
			{
			tmp = g_strndup(ptr, n);
			}

		value += mp * (gint)strtol(tmp, NULL, 10);
		g_free(tmp);

		if (ptr > text) ptr--;
		}

	return value;
}

OGGInfo *ogg123_info_get(const gchar *path)
{
	OGGInfo *info = NULL;
	FILE *f;
	char *command[3];
	gint child_pid;
	int child_pipe[2];
	char buf[128];

	if (!ogg123_info_found) return NULL;

	command[0] = OGG123_BINARY_INFO;
	command[1] = (char *)path;
	command[2] = NULL;

	f = ogg123_pipe_open_fd(command, &child_pid, child_pipe);
	if (!f) return NULL;

	info = g_new0(OGGInfo, 1);
	info->pass_integrity = TRUE;
	info->bitrate_lower = -1;
	info->bitrate_upper = -1;
	info->vendor = NULL;

	while (fgets(buf, sizeof(buf), f) != NULL)
		{
		const gchar *value;
		value = info_line_value(buf);

		if (debug_mode) printf("value is [%s]\n", value);

		if (info_line_key_test(buf, "Warning") || info_line_key_test(buf, "Error"))
			{
			info->pass_integrity = FALSE;
			}
		if (info_line_key_test(buf, "Vendor"))
			{
			if (!info->vendor) info->vendor = g_strdup(value);
			}
		else if (!value)
			{
			continue;
			}
		else if (info_line_key_test(buf, "Playback length"))
			{
			info->length = info_time_string_to_val(value);
			}
		else if (info_line_key_test(buf, "Nominal bitrate"))
			{
			info->bitrate_nominal = (gint)(strtol(value, NULL, 10) * 1000.0);
			}
		else if (info_line_key_test(buf, "Average bitrate"))
			{
			info->bitrate_average = (gint)(strtol(value, NULL, 10) * 1000.0);
			}
		else if (info_line_key_test(buf, "Upper bitrate"))
			{
			info->bitrate_upper = (gint)(strtol(value, NULL, 10) * 1000.0);
			}
		else if (info_line_key_test(buf, "Lower bitrate"))
			{
			info->bitrate_lower = (gint)(strtol(value, NULL, 10) * 1000.0);
			}
		else if (info_line_key_test(buf, "Channels"))
			{
			info->channels = (gint)strtol(value, NULL, 10);
			}
		}

	fclose(f);	/* also closes child_pipe[0] */
	waitpid(child_pid, NULL, 0);	/* eat zombie */

	info->bytes = filesize(path);

	return info;
}

void ogg123_info_free(OGGInfo *info)
{
	if (!info) return;

	g_free(info->vendor);
	g_free(info);
}

/*
 * List is ordered key, value, key, value ...
 * use path_list_free() to free the returned list
 */
GList *ogg123_comment_get(const gchar *path)
{
	GList *list = NULL;
	FILE *f;
	char *command[4];
	gint child_pid;
	int child_pipe[2];
	char buf[128];

	if (!ogg123_comment_found) return NULL;

	command[0] = OGG123_BINARY_COMMENT;
	command[1] = "-l";
	command[2] = (char *)path;
	command[3] = NULL;

	f = ogg123_pipe_open_fd(command, &child_pid, child_pipe);
	if (!f) return NULL;

	while (fgets(buf, sizeof(buf), f) != NULL)
		{
		gchar *key;

		key = info_line_key(buf);

		if (key)
			{
			const gchar *value;
			value = info_line_value(buf);

			if (value)
				{
				list = g_list_prepend(list, key);
				list = g_list_prepend(list, g_strdup(value));
				key = NULL;
				}
			}
		g_free(key);
		}

	fclose(f);	/* also closes child_pipe[0] */
	waitpid(child_pid, NULL, 0);	/* eat zombie */

	return g_list_reverse(list);
}

const gchar *ogg123_comment_value(GList *list, const gchar *key)
{
	GList *work;

	if (!key) return NULL;

	work = list;
	while(work)
		{
		const gchar *poss = work->data;

		if (strcasecmp(poss, key) == 0)
			{
			if (work->next)
				{
				work = work->next;
				if (work) return (const gchar *)work->data;
				}
			return NULL;
			}

		work = work->next;
		if (work) work = work->next;
		}

	return NULL;
}

gint ogg123_comment_set(const gchar *path, GList *comments)
{
	FILE *f;
	GList *errlist;
	GList *comment_list;
	GList *work;
	char buf[128];
	int argc;
	char **argv;
	gint child_pid;
	int child_pipe[2];
	
	if (!ogg123_comment_found) return TRUE;
	if (!path) return FALSE;

	comment_list = NULL;
	errlist = NULL;

	/* get the size needed for argv */
	work = comments;
	argc = 4;
	while (work && work->next)
		{
		if (work->data) argc+=2;
		work = work->next;
		work = work->next;
		}

	argv = g_malloc0(argc * sizeof(argv[0]));
	argc = 0;
	argv[argc] = OGG123_BINARY_COMMENT;
	argc++;
	argv[argc] = "-w";
	argc++;
	argv[argc] = (char *)path;
	argc++;

	/* fill comment arguments */
	work = comments;
	while (work && work->next)
		{
		const gchar *key;
		const gchar *value;

		key = work->data;
		value = work->next->data;

		if (key && strlen(key) > 0 && value && strlen(value) > 0)
			{
			argv[argc] = "-t";
			argc++;
			argv[argc] = g_strdup_printf("%s=%s", key, value);
			/* remember allocated strings for later free'ing */
			comment_list = g_list_prepend(comment_list, argv[argc]);
			argc++;
			}

		work = work->next;
		work = work->next;
		}

	argv[argc] = NULL;

	f = ogg123_pipe_open_fd(argv, &child_pid, child_pipe);
	if (f)
		{
		while (fgets(buf, sizeof(buf), f) != NULL)
			{
			if (strlen(buf) > 0)
				{
				errlist = g_list_append(errlist, g_strdup(buf));
				}
			}
		fclose(f);
		waitpid(child_pid, NULL, 0);
		}

	if (errlist)
		{
		GenericDialog *gd;
		GtkWidget *tw;
		GtkTextIter iter;
		GtkTextBuffer *buffer;
		gchar *msg;

		msg = g_strdup_printf(_("Failed to set comments for file:\n%s"), path);
		gd = generic_dialog_new(_("Comment set failed - GQmpeg"), msg,
					"GQmpeg", "warning", TRUE, NULL, NULL);
		g_free(msg);

	        generic_dialog_add_stock(gd, NULL, GTK_STOCK_OK, NULL, TRUE);

		tw = gtk_text_view_new();
		gtk_widget_set_size_request(tw, 400, 200);
		gtk_text_view_set_editable(GTK_TEXT_VIEW(tw), FALSE);
		gtk_box_pack_start(GTK_BOX(gd->vbox), tw, TRUE, TRUE, 5);
		gtk_widget_show(tw);

		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tw));
		gtk_text_buffer_create_tag(buffer, "monospace",
					   "family", "monospace", NULL);
		gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

		work = errlist;
		while (work)
			{
			gchar *txt = work->data;

			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, txt, -1,
								 "monospace", NULL);
			work = work->next;
			}

		gtk_widget_show(gd->dialog);
		}

	path_list_free(errlist);
	path_list_free(comment_list);
	return (errlist == NULL);
}

/*
 *-----------------------------------------------------------------------------
 * output device utils
 *-----------------------------------------------------------------------------
 */

/* FIXME! this should be seriously fixed to only list available options */

enum {
	OGG123_DEV_OSS = 0,
	OGG123_DEV_ESD = 1,
	OGG123_DEV_IRIX = 2,
	OGG123_DEV_SUN = 3,
	OGG123_DEV_WAV = 4,
	OGG123_DEV_DEF = 5
};

static const gchar *ogg123_output_devices[] =
{
	"oss",
	"esd",
	"irix",
	"sun",
	"wav",
	"default",
	NULL
};
static gint ogg123_output_devices_count = 6;

/* sorry - it got ugly */
#if defined(linux) || defined(__FreeBSD__) || defined (__NetBSD__) || defined(__OpenBSD__)
  gint ogg123_device = OGG123_DEV_OSS;
#elif defined(sun)
  gint ogg123_device = OGG123_DEV_SUN;
#elif defined(SGI)
  gint ogg123_device = OGG123_DEV_IRIX;
#else
  gint ogg123_device = OGG123_DEV_DEF; /* use ogg123's default (from ogg123rc) */
#endif


const gchar *ogg123_get_device(gint device)
{
	if (device >= ogg123_output_devices_count || device < 0) device = 0;

	return ogg123_output_devices[device];
}

/* don't free the data in the list */
GList *ogg123_get_device_list(void)
{
	GList *list = NULL;
	gint i;

	for (i = 0; i < ogg123_output_devices_count; i++)
		{
		list = g_list_append(list, (gchar *)ogg123_get_device(i));
		}

	return list;
}


/*
 *-----------------------------------------------------------------------------
 * misc
 *-----------------------------------------------------------------------------
 */

/*
 *-----------------------------------------------------------------------------
 * child control
 *-----------------------------------------------------------------------------
 */

static void ogg123_child_shutdown(void)
{

	if (ogg123_child_pid != -1)
		{
		pid_t t;

		if (kill(ogg123_child_pid, SIGINT) == -1)
			{
			printf(_("Failed to successfully send signal to pid = %d\n"), ogg123_child_pid);
			}
		
		t = waitpid(ogg123_child_pid, NULL, WNOHANG);
		/* maybe it just took a little bit */
		if (t != ogg123_child_pid && t != -1) waitpid(ogg123_child_pid, NULL, 0);
		}

	if (ogg123_control_read_id != -1) g_source_remove(ogg123_control_read_id);
	ogg123_control_read_id = -1;
	close(ogg123_pipe[0]);
	ogg123_pipe[0] = -1;

	ogg123_child_pid = -1;
	pid = 0;
}

static void ogg123_error(void)
{
	ogg123_child_shutdown();
	module_playback_error(ogg123_current_sd);
}

static void ogg123_input_error(void)
{
	ogg123_error();
}

static void ogg123_input_end(void)
{
	waitpid(ogg123_child_pid, NULL, 0);
	ogg123_child_pid = -1;
}

/*
 *-----------------------------------------------------------------------------
 * input parsing
 *-----------------------------------------------------------------------------
 */

static gint parse_time(const gchar *text)
{
	gint m, s;

	if (sscanf(text, "%d:%d", &m, &s) < 2) return 0;

	return (m * 60 + s);
}

static void parse_bitrate(const gchar *text)
{
	static time_t old_t = 0;
	time_t new_t;
	const gchar *ptr;
	gint force = FALSE;

	ptr = strstr(text, "Bitrate: ");
	if (ptr)
		{
		/* 1.0 rc1 and rc2 */
		ptr += 9;
		}
	else
		{
		/* 1.0 rc3 (and newer?) */
		ptr = strstr(text, "kbps");
		if (ptr)
			{
			while (ptr > text && *ptr != '(') ptr--;
			if (*ptr == '(')
				{
				ptr++;
				}
			else
				{
				ptr = NULL;
				}
			}
		}

	/* ptr should now be pointing directly at the number now (or NULL) */
	if (ptr)
		{
		gint new_rate;

		new_rate = (gint)strtol(ptr, NULL, 10);
		if (input_bitrate != 0)
			{
			/* show a trend, ugly but it makes the rate change more smoothly */
			input_bitrate = ((input_bitrate * 4) + new_rate) / 5;
			}
		else
			{
			input_bitrate = new_rate;
			force = TRUE;
			}
		}

	/* we only update once per second */
	new_t = time(NULL);
	if (new_t > old_t || force)
		{
		module_playback_data_changed();
		old_t = new_t;
		}
}

static gint ogg123_input_parse(const gchar *buffer)
{
	if (debug_mode > 1) printf("ogg123 output:\"%s\"\n", buffer);

/* Note: different releases have different output formats...
 *
 * 1.0 rc1 and rc2:
 *			Time: 00:01.50 [04:32.13] of 04:33.63, Bitrate: 122.3
 * 1.0 rc3:
 *			Time: 00:03.92 [04:48.17] of 04:52.09  (140.9 kbps)  Output Buffer 75.0%
 *
 * as of right now this only effects parse_bitrate()
 */

	if (strncmp(buffer, "Time: ", 6) == 0)
		{
		const gchar *ptr;

		/* time */
		ptr = buffer + 6;
		if (strlen(ptr) < 8) return FALSE;
		seconds = parse_time(ptr);
		frames = seconds;
		ptr += 8;
		ptr = strchr(ptr, '[');
		if (!ptr) return FALSE;
		ptr++;
		if (strlen(ptr) < 8) return FALSE;
		seconds_remaining = parse_time(ptr);
		frames_remaining = seconds_remaining;
		parse_bitrate(ptr);
		}
	else if (strncmp(buffer, "Bitstream is", 12) == 0)
		{
		sscanf(buffer, "Bitstream is %d channel, %d", &output_channels, &output_hz);
		/* FIXME these are broken, really. */
		input_channels = output_channels;
		input_hz = 44100;
		module_playback_data_changed();
		}
	/* look for end */
	else if (strncmp(buffer, "Done.", 5) == 0)
		{
		ogg123_input_end();
		ogg123_child_shutdown();
		module_playback_end();
		return FALSE;
		}
	else if (strncmp(buffer, "No such device", 14) == 0)
		{
		/* device error */
		gchar *buf;

		printf(_("Error, ogg123 reported:\"%s\")\n"), buffer);
		buf = g_strdup_printf("ogg123: %s", buffer);
		warning_dialog(_("GQmpeg: ogg123 error"), buf);
		g_free(buf);

		ogg123_error();
		return FALSE;
		}

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * input buffer
 *-----------------------------------------------------------------------------
 */

static char ogg123_read_buffer[1024];
static gint ogg123_read_buffer_pos = 0;

static gboolean ogg123_input_read_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	gchar buf[1024];
	gint r;
	gint p;
	gint l;
	gchar *ptr;

	r = read(ogg123_pipe[0], buf, sizeof(buf));

	if (r == -1)
		{
		ogg123_input_error();
		return TRUE;
		}
	if (r == 0)
		{
		printf(_("ogg123 disappeared! (unexpected EOF)\n"));
		ogg123_input_end();
		ogg123_error();
		return TRUE;
		}

	ptr = ogg123_read_buffer + ogg123_read_buffer_pos;
	l = sizeof(ogg123_read_buffer) - 1;
	p = 0;
	while(p < r)
		{
		gchar c;
		c = buf[p];
		p++;

		if (c == '\b' && ogg123_read_buffer_pos > 0)
			{
			ogg123_read_buffer_pos--;
			}
		else if (c == '\r' || c == '\n')
			{
 			if (ogg123_read_buffer_pos > 0)
				{
				ogg123_read_buffer[ogg123_read_buffer_pos] = '\0';
				if (!ogg123_input_parse(ogg123_read_buffer)) return TRUE;
				ogg123_read_buffer_pos = 0;
				}
			}
		else if (ogg123_read_buffer_pos < l)
			{
			ogg123_read_buffer[ogg123_read_buffer_pos] = c;
			ogg123_read_buffer_pos++;
			}
		}

#if 0
	/* this is only needed if the input is not newline/return based */
	if (ogg123_read_buffer_pos > 0)
		{
		ogg123_read_buffer[ogg123_read_buffer_pos] = '\0';
		ogg123_input_parse(ogg123_read_buffer);
		}
#endif

	return TRUE;
}

static void ogg123_input_read_reset(void)
{
	ogg123_read_buffer_pos = 0;
}

/*
 *-----------------------------------------------------------------------------
 * child setup (fork)
 *-----------------------------------------------------------------------------
 */

#define OGG123_MAX_COMMANDS 32

static gint ogg123_child_run(SongData *sd, gint position)
{
	char cmd_arguments[OGG123_MAX_COMMANDS][512];
	char *cmd_ptr[OGG123_MAX_COMMANDS];
	int cmd_cnt = 0;
	size_t cmd_max;
	GIOChannel *channel;

	cmd_max = sizeof(cmd_arguments) / OGG123_MAX_COMMANDS;

	if (!sd || ogg123_child_pid != -1) return FALSE;

	strncpy(cmd_arguments[cmd_cnt], OGG123_BINARY, cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	strncpy(cmd_arguments[cmd_cnt], "-v", cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	if (position > 0)
		{
		strncpy(cmd_arguments[cmd_cnt], "-k", cmd_max);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;

		snprintf(cmd_arguments[cmd_cnt], cmd_max, "%d", position);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (strcmp(ogg123_get_device(ogg123_device), "default") != 0)
		{
		snprintf(cmd_arguments[cmd_cnt], cmd_max, "--device=%s", ogg123_get_device(ogg123_device));
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (ogg123_device_options && strlen(ogg123_device_options) > 0)
		{
		snprintf(cmd_arguments[cmd_cnt], cmd_max, "--device-option=%s", ogg123_device_options);
		cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
		cmd_cnt++;
		}

	if (ogg123_extra_options &&
	    strlen(ogg123_extra_options) > 0 &&
	    cmd_cnt + 2 < OGG123_MAX_COMMANDS)
		{
		gchar **vector;
		gint i;

		vector = g_strsplit(ogg123_extra_options, " ", OGG123_MAX_COMMANDS - 2 - cmd_cnt);
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

	strncpy(cmd_arguments[cmd_cnt], sd->path, cmd_max);
	cmd_ptr[cmd_cnt] = cmd_arguments[cmd_cnt];
	cmd_cnt++;

	strncpy(cmd_arguments[cmd_cnt], "", cmd_max);
	cmd_ptr[cmd_cnt] = NULL;
	cmd_cnt++;

	pid = 0;
	if (!ogg123_pipe_open(cmd_ptr, &ogg123_child_pid, ogg123_pipe))
		{
		return FALSE;
		}
	pid = ogg123_child_pid;

	ogg123_input_read_reset();

	channel = g_io_channel_unix_new(ogg123_pipe[0]);
	ogg123_control_read_id = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, G_IO_IN,
						     ogg123_input_read_cb, NULL, NULL);
	g_io_channel_unref(channel);

#if 0
	ogg123_control_read_id = gdk_input_add (ogg123_pipe[0], GDK_INPUT_READ, ogg123_input_read_cb, NULL);
#endif

	return TRUE;
}

/*
 *----------------------------------------------------------------------------
 * module callback funcs
 *----------------------------------------------------------------------------
 */

static void ogg123_data_init(SongData *sd)
{
	sd->type_description = _("Ogg bitstream audio file");
}

static gint ogg123_data_set(SongData *sd, gint read_info)
{
	if (read_info && !sd->info_loaded)
		{
		OGGInfo *info;
		GList *comments;
		const gchar *track_text;

		sd->info_loaded = TRUE;

		info = ogg123_info_get(sd->path);
		if (info)
			{
			sd->length = info->length;
			sd->bit_rate = info->bitrate_nominal / 1000;
			sd->channels = info->channels;
			sd->bit_depth = 16;	/* these two correct ? */
			sd->khz_rate = 44;
			if (!info->pass_integrity) sd->flags |= SONG_FLAG_PLAY_FAIL;
			ogg123_info_free(info);
			}

		comments = ogg123_comment_get(sd->path);

		g_free(sd->title);
		sd->title = g_strdup(ogg123_comment_value(comments, "TITLE"));
		g_free(sd->artist);
		sd->artist = g_strdup(ogg123_comment_value(comments, "ARTIST"));
		g_free(sd->album);
		sd->album = g_strdup(ogg123_comment_value(comments, "ALBUM"));
		g_free(sd->genre);
		sd->genre = g_strdup(ogg123_comment_value(comments, "GENRE"));

		g_free(sd->comment);
		sd->comment = g_strdup(ogg123_comment_value(comments, "DESCRIPTION"));
		if (!sd->comment)
			{
			/* support old COMMENT created with older GQmpegs */
			sd->comment = g_strdup(ogg123_comment_value(comments, "COMMENT"));
			}

		g_free(sd->year);
		sd->year = g_strdup(ogg123_comment_value(comments, "DATE"));
		if (!sd->year)
			{
			/* same */
			sd->year = g_strdup(ogg123_comment_value(comments, "YEAR"));
			}

		track_text = ogg123_comment_value(comments, "TRACKNUMBER");
		if (track_text)
			{
			sd->track = (guint8)strtol(track_text, NULL, 10);
			}

		path_list_free(comments);
		}

	return TRUE;
}

/*
 *----------------------------------------------------------------------------
 * module play funcs
 *----------------------------------------------------------------------------
 */

static gint ogg123_start(SongData *sd, gint position)
{
	gint ret;

	if (debug_mode) printf("io_ogg123.c: play started at %d\n", position);
	ogg123_current_sd = sd;

	ret = ogg123_child_run(sd, position);

	playback_done_command(EXEC_PLAY, !ret);

	return ret;
}

static gint ogg123_stop(SongData *sd)
{
	if (debug_mode) printf("io_ogg123.c: play stopped\n");
	if (sd != ogg123_current_sd)
		{
		printf("io_ogg123.c warning: attempt to stop playback of non matching file\n");
		}
	ogg123_child_shutdown();
	ogg123_current_sd = NULL;

	playback_done_command(EXEC_STOP, FALSE);
	return TRUE;
}

static gint ogg123_pause(SongData *sd)
{
	if (debug_mode) printf("io_ogg123.c: play paused at %d\n", seconds);
	if (sd != ogg123_current_sd)
		{
		printf("io_ogg123.c warning: attempt to pause playback of non matching file\n");
		}

	ogg123_child_shutdown();

	playback_done_command(EXEC_PAUSE, FALSE);
        return TRUE;
}

static gint ogg123_continue(SongData *sd)
{
	gint ret;

	if (debug_mode) printf("io_ogg123.c: play restarted at %d\n", seconds);
	if (sd != ogg123_current_sd)
		{
		printf("io_ogg123.c warning: attempt to continue playback of non matching file\n");
		}
	ogg123_current_sd = sd;
	ret = ogg123_child_run(sd, seconds);

        playback_done_command(EXEC_CONTINUE, !ret);
        return ret;
}

static gint ogg123_seek(SongData *sd, gint position)
{
	if (debug_mode) printf("io_ogg123.c: play seeking to %d\n", position);
	if (sd != ogg123_current_sd)
		{
		printf("io_ogg123.c warning: attempt to seek in non matching file\n");
		}

	if (status == STATUS_PLAY)
		{
		gint ret;

		ogg123_child_shutdown();

		seconds_remaining += seconds - position;
		seconds = position;

		ret = ogg123_child_run(sd, position);

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

void ogg123_init(void)
{
	IO_ModuleData *imd;
	gchar *mod_desc = "OGG file";
	gint id;

	if (debug_mode) printf("ogg123 module initing...\n");

	ogg123_enabled = file_in_path(OGG123_BINARY);
	if (!ogg123_enabled)
		{
		printf(_("Failed to find %s in your path!\n"), OGG123_BINARY);
		printf(_("ogg123 player module disabled.\n"));
		}

	ogg123_info_found = file_in_path(OGG123_BINARY_INFO);
	if (!ogg123_info_found)
		{
		printf(_("\"%s\" not found.\n"), OGG123_BINARY_INFO);
		}

	ogg123_comment_found = file_in_path(OGG123_BINARY_COMMENT);
	if (!ogg123_comment_found)
		{
		printf(_("\"%s\" not found, comment support disabled.\n"), OGG123_BINARY_COMMENT);
		}

	imd = g_new0(IO_ModuleData, 1);

	imd->title = "ogg123";
	imd->description = _("ogg player");

	if (ogg123_enabled)
		{
		imd->songdata_init_func = ogg123_data_init;
		imd->songdata_info_func = ogg123_data_set;

		imd->start_func = ogg123_start;
		imd->stop_func = ogg123_stop;
		imd->pause_func = ogg123_pause;
		imd->continue_func = ogg123_continue;
		imd->seek_func = ogg123_seek;

		imd->info_func = ogg123_info_window;
		}

	imd->config_load_func = ogg123_config_load;
	imd->config_save_func = ogg123_config_save;
	imd->config_init_func = ogg123_config_init;
	imd->config_apply_func = ogg123_config_apply;
	imd->config_close_func = ogg123_config_close;

	id = player_module_register(imd);

	module_register_file_suffix_type(".ogg", mod_desc, id);
}

