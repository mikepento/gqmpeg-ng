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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "intl.h"

/*
 *-------------------------------------
 * Standard library includes
 *-------------------------------------
 */

#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

/*
 *-------------------------------------
 * includes for glib / gtk / etc.
 *-------------------------------------
 */

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/*
 *----------------------------------------------------------------------------
 * defines
 *----------------------------------------------------------------------------
 */

#define		GQMPEG_RC_DIR		".gqmpeg"
#define		GQMPEG_RC_DIR_SKIN	".gqmpeg/skins"
#define		GQMPEG_RC_DIR_PLAYLIST	".gqmpeg/playlists"
#define		GQMPEG_RC_FILE_SESSION	".gqmpeg/session-playlist.gqmpeg"
#define		GQMPEG_SKIN_HELP_PATH	GQMPEG_SKINDIR"/SKIN-SPECS"
#define		GQMPEG_RC_FILE		"gqmpegrc"

#define		list_width 180
#define		list_height 180
#define		playlist_window_width 500
#define		playlist_window_height 400

#define		STATUS_STOP 0
#define		STATUS_PAUSE 1
#define		STATUS_PLAY 2
#define		STATUS_NEXT 3

#define		PLAYLIST_MODE_EMPTY 0
#define		PLAYLIST_MODE_SESSION 1
#define		PLAYLIST_MODE_FILE 2

#include "typedefs.h"
#include "ui2_typedefs.h"

/*
 *----------------------------------------------------------------------------
 * globals
 *----------------------------------------------------------------------------
 */

	/* main window widgets */
extern UIData *main_window;
extern GtkWidget *playlist_window;

	/* file lists */
extern GList *filelist;
extern GList *dirlist;

	/* filtering */
extern GList *filter;
extern gint disable_filtering;
extern gint show_dot_files;

	/* playlist info */
extern gchar *playlist_pathname;
extern gchar *playlist_filename;

	/* song time info */
extern gint seconds;
extern gint seconds_remaining;
extern gint frames;
extern gint frames_remaining;

	/* input info */
extern gint input_channels;
extern gint input_hz;
extern gint input_bitrate;

	/* output info */
extern gint output_channels;
extern gint output_hz;
extern gint output_bits;

	/* soon to be removed, but not until necessary (more players added)
	 * because this will effect the skin format by deprecating
	 * MP3 specific widgets
	 * possible problem with backwards skin compatibility !!!
	 */
extern gint mpeg_version;
extern gint mpeg_layer;
extern gint mpeg_mode;

	/* options */
extern gint initial_playlist_mode;
extern gchar *initial_playlist_pathname;
extern gint initial_playlist_position;
extern gint initial_directory_enable;
extern gchar *initial_directory_path;
extern gint show_playlist_on_startup;
extern gint start_playing_on_startup;
extern gint read_file_information;
extern gint window_pos_x;
extern gint window_pos_y;
extern gint window_list_x;
extern gint window_list_y;
extern gint window_list_w;
extern gint window_list_h;
extern gint window_list_div_dir;
extern gint window_list_div_mid;
extern gchar *window_list_columns;
extern gint wm_decorations;
extern gint prev_includes_current;

	/* miscellaneous */
extern gchar *current_path;
extern gchar *mixer_command;
extern gint mixer_device_id;

extern gint status;
extern gint repeat_mode;
extern gint shuffle_mode;
extern gint save_mode_in_playlist;
extern gint obey_mode_in_playlist;

extern gchar *preset_name[];
extern gchar *preset_file[];
extern gint play_presets;
extern gint show_remaining_time;
extern gint show_total_time;

extern int pid;

extern gchar *skin_default_path;
extern gchar *skin_mode_key;

extern gint pos_slider_in_drag;
extern gint new_song;

extern gint show_extra_winamp_info;

extern gint title_scrolls_always;
extern gint title_show_extension;
extern gint title_convert_underscores;

extern gchar **gqmpeg_icon_xpm;

extern gint allow_ipc;

extern gint dock_mode;

extern gint drop_single_to_playlist;
extern gint play_next_on_error;

extern gint loop_ab_enabled;
extern gint loop_ab_start;
extern gint loop_ab_end;

extern gint repeat_1_enabled;
extern gint change_delay;

extern gint flyby_enabled;
extern gint flyby_location;

extern gint skinned_menus_enable;

/*
 * =========================== globals.c
 */
void init_vars(void);

/*
 * =========================== main.c
 */
gint convert_chars(gchar *text, gchar s, gchar d);
guint32 elapsed_run_time(void);

void load_preset(gint n);
gint preset_is_active(gint n);

SongData *current_song_get_data(void);
gint current_song_get_number(void);
void current_song_set(gint n, gchar *path);
gint current_song_is_in_playlist(void);
void current_song_set_to_next(void);
void current_song_set_to_prev(void);
void current_song_set_and_play(gint n, gchar *path);

const gchar *current_song_get_path(void);
const gchar *current_song_get_title(void);
const gchar *current_song_get_artist(void);
const gchar *current_song_get_album(void);
const gchar *current_song_get_genre(void);
const gchar *current_song_get_year(void);
const gchar *current_song_get_comment(void);
gint current_song_get_track(void);

void help_window_show(const gchar *key);

void gqmpeg_exit(void);
/*
 * Some situations do not handle exiting
 * immediately too well (like ipc_read)
 */
void gqmpeg_schedule_exit(void);

