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
#include "players.h"

#include "display.h"
#include "playlist.h"
#include "types.h"
#include "ui_utildlg.h"

/* include each module header for init */
#include "io_mpg123.h"
#include "io_ogg123.h"
#include "io_xmp.h"
#include "io_radio.h"


static GList *io_list = NULL;
static IO_ModuleData *current_imd = NULL;

/*
 *-----------------------------------------------------------------------------
 * player module functions (public)
 *-----------------------------------------------------------------------------
 */

void player_modules_init(void)
{
	mpg123_init();
	ogg123_init();
	xmp_init();
	radio_init();

	if (io_list == NULL)
		{
		warning_dialog(_("GQmpeg warning"),
			       _("No player modules registered!\n"
			       "This is not necessarily a problem, but does make the program a little useless.\n"));
		}
}

void player_modules_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all)
{
	GList *work = io_list;
	while(work)
		{
		IO_ModuleData *imd = work->data;
		if (imd->config_load_func) imd->config_load_func(f, option, value, value_all);
		work = work->next;
		}
}

void player_modules_config_save(FILE *f)
{
	GList *work = io_list;
	while(work)
		{
		IO_ModuleData *imd = work->data;
		if (imd->config_save_func) imd->config_save_func(f);
		work = work->next;
		}
}

GList *player_modules_config_init(void)
{
	GList *list = NULL;
	GList *work = io_list;
	while(work)
		{
		GtkWidget *widget = NULL;
		IO_ModuleData *imd = work->data;
		if (imd->config_init_func)
			{
			widget = imd->config_init_func();
			}
		if (widget)
			{
			g_object_set_data(G_OBJECT(widget), "title", imd->title);
			list = g_list_append(list, widget);
			}
		work = work->next;
		}

	return list;
}

void player_modules_config_apply(void)
{
	GList *work = io_list;
	while(work)
		{
		IO_ModuleData *imd = work->data;
		if (imd->config_apply_func) imd->config_apply_func();
		work = work->next;
		}
}

void player_modules_config_close(void)
{
	GList *work = io_list;
	while(work)
		{
		IO_ModuleData *imd = work->data;
		if (imd->config_close_func) imd->config_close_func();
		work = work->next;
		}
}

/*
 *-----------------------------------------------------------------------------
 * player module functions (private)
 *-----------------------------------------------------------------------------
 */

static gint player_module_get_type_id(IO_ModuleData *imd)
{
	gint ret = 0;
	GList *work = io_list;

	while(work)
		{
		if (imd == work->data) return ret;
		ret++;
		work = work->next;
		}

	return -1;
}

static IO_ModuleData *player_module_by_type_id(gint type)
{
	gint c = 0;
	GList *work = io_list;

	while(work)
		{
		if (c == type) return work->data;
		c++;
		work = work->next;
		}

	return NULL;
}

static gint player_module_get_id_by_type(const gchar *path)
{
	gint ret;

	if (!path) return -1;
	
	ret = typelist_determine_module_id(path);

	if (ret == -1)
		{
		ret = filter_determine_module_id(path);
		}

	return ret;
}

static void player_module_get_types(const gchar *path, gint *type, gint *custom, gint *custom_type, gint *live)
{
	typelist_determine_ids(path, type, custom, custom_type, live);

	if (*type == -1)
		{
		*type = filter_determine_module_id(path);
		}
}

/*
 *-----------------------------------------------------------------------------
 * player module songdata functions (public)
 *-----------------------------------------------------------------------------
 */

SongData *player_module_songdata_init(const gchar *path)
{
	IO_ModuleData *imd = NULL;
	SongData *sd;

	sd = g_new0(SongData, 1);
	sd->path = g_strdup(path);
	sd->info_loaded = FALSE;
	sd->flags = 0;
	player_module_get_types(path, &sd->type, &sd->custom, &sd->custom_type, &sd->live);

	imd = player_module_by_type_id(sd->type);

	if (imd && imd->songdata_init_func)
		{
		imd->songdata_init_func(sd);
		}

	if (!imd)
		{
		printf(_("No registered player types match: %s\n"), path);
		sd->flags |= SONG_FLAG_UNKNOWN_TYPE;
		}

	return sd;
}

gint player_module_songdata_update(SongData *sd, gint read_info)
{
	IO_ModuleData *imd = NULL;

	if (!sd || !sd->path) return FALSE;

	imd = player_module_by_type_id(sd->type);

	if (imd && imd->songdata_info_func)
		{
		return imd->songdata_info_func(sd, read_info);
		}

	return FALSE;
}

GtkWidget *player_module_songdata_detail_info(const gchar *path)
{
	IO_ModuleData *imd;
	gint id = player_module_get_id_by_type(path);
	imd = player_module_by_type_id(id);

	if (imd && imd->info_func)
		{
		return imd->info_func(path);
		}

	return NULL;
}

/*
 *-----------------------------------------------------------------------------
 * player module start/stop/pause/seek functions (were public)
 *-----------------------------------------------------------------------------
 */

static gint playback_continue(SongData *sd);


static gint playback_start(SongData *sd, gint seconds)
{
	gint ret = FALSE;

	if (!sd) return FALSE;

	if (status == STATUS_PLAY) return FALSE;
	if (status == STATUS_STOP || status == STATUS_NEXT) current_imd = player_module_by_type_id(sd->type);

	if (!current_imd || !current_imd->start_func)
		{
		printf(_("No registered player types claim playback for: %s\n"), sd->path);
		return FALSE;
		}

	ret = current_imd->start_func(sd, seconds);

	if (ret)
		{
		status = STATUS_PLAY;
		}
	if (ret == -1)
		{
		current_imd = NULL;
		playlist_set_flags(playlist_get_index_by_data(sd), SONG_FLAG_PLAY_FAIL);
		}

	return ret;
}

static gint playback_stop(SongData *sd)
{
	gint ret = FALSE;

	if (!current_imd || !current_imd->stop_func || status == STATUS_STOP) return FALSE;

	ret = current_imd->stop_func(sd);

	if (ret == TRUE || ret == -1)
		{
		status = STATUS_STOP;
		current_imd = NULL;
		seconds = 0;
		seconds_remaining = 0;
		frames = 0;
		frames_remaining = 0;
		}

	return ret;
}

static gint playback_pause(SongData *sd)
{
	gint ret = FALSE;

	if (!current_imd || !current_imd->pause_func || status == STATUS_STOP) return FALSE;
	if (status == STATUS_PAUSE) return playback_continue(sd);

	ret = current_imd->pause_func(sd);

	if (ret)
		{
		status = STATUS_PAUSE;
		}
	if (ret == -1)
		{
		status = STATUS_STOP;
		current_imd = NULL;
		}

	return ret;
}

static gint playback_continue(SongData *sd)
{
	gint ret = FALSE;

	if (!current_imd || !current_imd->continue_func || status != STATUS_PAUSE) return FALSE;

	ret = current_imd->continue_func(sd);

	if (ret)
		{
		status = STATUS_PLAY;
		}
	if (ret == -1)
		{
		status = STATUS_STOP;
		current_imd = NULL;
		playlist_set_flags(playlist_get_index_by_data(sd), SONG_FLAG_PLAY_FAIL);
		}

	return ret;
}

static gint playback_seek(SongData *sd, gint seconds)
{
	gint ret = FALSE;

	if (!current_imd || !current_imd->seek_func) return FALSE;
	if (status != STATUS_PLAY && status != STATUS_PAUSE) return FALSE;

	ret = current_imd->seek_func(sd, seconds);

	if (ret == -1)
		{
		status = STATUS_STOP;
		current_imd = NULL;
		playlist_set_flags(playlist_get_index_by_data(sd), SONG_FLAG_PLAY_FAIL);
		}

	return ret;
}

/*
 *-----------------------------------------------------------------------------
 * player module start/stop/pause/seek functions (public) use these for control
 * the queueing is very sloppy right now (was half asleep during most of it :)
 *-----------------------------------------------------------------------------
 */

static gint exec_command(PlayerCommand cmd, SongData *sd, gint pos, gint queued);

typedef struct _QueueData QueueData;
struct _QueueData
{
	PlayerCommand cmd;
	SongData *sd;
	gint pos;
};

static QueueData *pb_queue = NULL;
static PlayerCommand cmd_pending = EXEC_NONE;
static SongData *sd_pending = NULL;

static SongData *queue_song = NULL;

static void playback_queue_free(void)
{
	if (!pb_queue) return;

	if (debug_mode) printf("cmd: unqueued: %d\n", pb_queue->cmd);

	g_free(pb_queue);
	pb_queue = NULL;
}

static gint playback_queue_command(PlayerCommand cmd, SongData *sd, gint pos)
{
	if (sd != sd_pending)
		{
		if (cmd == EXEC_PLAY) queue_song = sd;
		return FALSE;
		}

	if (pb_queue) return FALSE;

	playback_queue_free();
	
	pb_queue = g_new0(QueueData, 1);

	pb_queue->cmd = cmd;
	pb_queue->sd = sd;
	pb_queue->pos = pos;

	if (debug_mode) printf("cmd: queued: %d\n", cmd);

	return TRUE;
}

static void playback_queue_do(void)
{
	if (pb_queue)
		{
		if (pb_queue->sd == sd_pending || pb_queue->sd == current_song_get_data())
			{
			if (!exec_command(pb_queue->cmd, pb_queue->sd, pb_queue->pos, TRUE))
				{
				if (debug_mode) printf("cmd: queued command failed: %d\n", pb_queue->cmd);
				}
			}

		playback_queue_free();
		}
	if (queue_song)
		{
		if (queue_song != current_song_get_data())
			{
			queue_song = current_song_get_data();
			}
		if (status != STATUS_STOP && status != STATUS_NEXT)
			{
			SongData *sd = queue_song;
			queue_song = NULL;
			exec_command(EXEC_STOP, sd_pending, 0, TRUE);
			exec_command(EXEC_PLAY, sd, 0, FALSE);
			}
		else
			{
			exec_command(EXEC_PLAY, queue_song, 0, TRUE);
			}
		queue_song = NULL;
		}
}

static gint exec_success(gint success)
{
	if (success)
		{
		return TRUE;
		}

	if (debug_mode) printf("cmd: req for: %d FAILED!\n", cmd_pending);
	cmd_pending = EXEC_NONE;
	return FALSE;
}

static gint exec_command(PlayerCommand cmd, SongData *sd, gint pos, gint queued)
{
	gint ret = FALSE;

	if (!queued && cmd_pending != EXEC_NONE)
		{
		return playback_queue_command(cmd, sd, pos);
		}

	switch (cmd)
		{
		case EXEC_PLAY:
			if (status == STATUS_PLAY || status == STATUS_PAUSE)
				{
				ret = FALSE;
				}
			else if (status == STATUS_STOP || status == STATUS_NEXT)
				{
				cmd_pending = cmd;
				if (debug_mode) printf("cmd: req for: %d\n", cmd_pending);
				ret = exec_success(playback_start(sd, pos));
				}
			else 
				{
				ret = FALSE;
				}
			break;
		case EXEC_PAUSE:
			if (status != STATUS_PLAY)
				{
				ret = FALSE;
				}
			cmd_pending = cmd;
			if (debug_mode) printf("cmd: req for: %d\n", cmd_pending);
			ret = exec_success(playback_pause(sd));
			break;
		case EXEC_CONTINUE:
			if (status != STATUS_PAUSE)
				{
				ret = FALSE;
				}
			cmd_pending = cmd;
			if (debug_mode) printf("cmd: req for: %d\n", cmd_pending);
			ret = exec_success(playback_continue(sd));
			break;
		case EXEC_STOP:
			if (status == STATUS_PLAY || status == STATUS_PAUSE)
				{
				cmd_pending = cmd;
				if (debug_mode) printf("cmd: req for: %d\n", cmd_pending);
				ret = exec_success(playback_stop(sd));
				}
			else if (status == STATUS_NEXT)
				{
				/* this does not go through the pending stuff, BUG ? */
				status = STATUS_STOP;
				ret = TRUE;
				}
			else
				{
				ret = FALSE;
				}
			break;
		case EXEC_SEEK:
			if (status == STATUS_PAUSE || status == STATUS_PLAY)
				{
				cmd_pending = cmd;
				if (debug_mode) printf("cmd: req for: %d\n", cmd_pending);
				ret = exec_success(playback_seek(sd, pos));
				}
			else
				{
				ret = FALSE;
				}
			break;
		default:
			ret = FALSE;
			break;
		}

	if (ret) sd_pending = sd;

	return ret;
}

void playback_done_command(PlayerCommand cmd, gint err)
{
	if (cmd_pending == cmd)
		{
		cmd_pending = EXEC_NONE;
		if (debug_mode) printf("cmd: got ack for: %d\n", cmd);
		if (err)
			{
			if (debug_mode) printf("cmd: command ack with error: %d (queue emptied)\n", cmd);
			playback_queue_free();
			status = STATUS_STOP;
			sd_pending = NULL;
			}
		else switch (cmd)
			{
			case EXEC_PLAY:
				status = STATUS_PLAY;
				break;
			case EXEC_PAUSE:
				status = STATUS_PAUSE;
				break;
			case EXEC_CONTINUE:
				status = STATUS_PLAY;
				break;
			case EXEC_STOP:
				sd_pending = NULL;
				status = STATUS_STOP;
				break;
			case EXEC_SEEK:
#if 0
				/* seek should not change the status, unless it failed */
				status = STATUS_PLAY;
#endif
				break;
			default:
				break;
			}

		playback_queue_do();
		}
	else if (cmd == EXEC_NEXT && cmd_pending == EXEC_NONE)
		{
		cmd_pending = EXEC_NONE;
		status = STATUS_NEXT;
		sd_pending = NULL;
		}
	else if (cmd == EXEC_NEXT)
		{
		if (debug_mode) printf("cmd: ignore next ack, while expecting: %d\n", cmd_pending);
		}
	else
		{
		if (debug_mode) printf("cmd: unexpected ack: %d, expecting: %d\n", cmd, cmd_pending);
		}
}

gint playback_exec_command(PlayerCommand cmd, SongData *sd, gint pos)
{
	if (debug_mode) printf("cmd: exec cmd=%d stat=%d\n", cmd, status);
	return exec_command(cmd, sd, pos, FALSE);
}

/*
 *-----------------------------------------------------------------------------
 * player module interface functions (public)
 * modules can call these functions to register types, update status, etc. ??
 *-----------------------------------------------------------------------------
 */

gint player_module_register(IO_ModuleData *imd)
{
	io_list = g_list_append(io_list, imd);
	return player_module_get_type_id(imd);
}

void module_register_file_suffix_type(const gchar *extension, const gchar *description, gint module_id)
{
	add_to_filter(extension, description, module_id);
}

void module_register_misc_type(const gchar *description, const gchar *format, gint module_id, gint live,
			       gint (*is_type_func)(const gchar *),
			       GtkWidget *(*entry_setup_func)(const gchar *),
			       GtkWidget *(*edit_func)(SongData *),
			       gchar *(*get_path_func)(GtkWidget *))
{
	add_to_typelist(format, description, module_id, live,
			is_type_func, entry_setup_func, edit_func, get_path_func);
}

void module_playback_end(void)
{
	playback_done_command(EXEC_NEXT, FALSE);
	seconds = 0;
	seconds_remaining = 0;
	frames = 0;
	frames_remaining = 0;
}

void module_playback_error(SongData *sd)
{
	if (play_next_on_error && playlist_get_next(current_song_get_number()) >= 0)
		{
		playback_done_command(EXEC_NEXT, FALSE);
		}
	else
		{
		playback_done_command(EXEC_STOP, TRUE);
		status = STATUS_STOP;
		}
	if (sd) playlist_set_flags(playlist_get_index_by_data(sd), SONG_FLAG_PLAY_FAIL);
}

void module_playback_data_changed(void)
{
	display_set_io_information(FALSE);
}

gint player_module_id_exists(gint id)
{
	return (player_module_by_type_id(id) != NULL);
}

