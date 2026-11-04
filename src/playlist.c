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
#include "playlist.h"

#include "display.h"
#include "ipc.h"
#include "players.h"
#include "playlist-window.h"
#include "types.h"
#include "ui_fileops.h"
#include "ui_utildlg.h"

/*
 *-----------------------------------------------------------------------------
 * static vars
 *-----------------------------------------------------------------------------
 */

static GList *shuffle_list;
static GList *playlist;
static gint playlist_count = 0;
static gint playlist_length = 0;

/*
 *-----------------------------------------------------------------------------
 * static funcs
 *-----------------------------------------------------------------------------
 */

static GList *shuffle_list_find_node(gint p);
static void shuffle_list_add (gint p);
static void shuffle_list_remove (gpointer data);
static gint shuffle_list_get_first(void);
static gint shuffle_list_get_next(gint p);
static gint shuffle_list_get_prev(gint p);

static void real_playlist_init(void);
static void real_playlist_clear(void);
static SongData *real_playlist_add_item(const gchar *path, gint read_info);
static SongData *real_playlist_insert_item(const gchar *path, gint n, gint read_info);
static gint real_playlist_remove_item(gint n);
static gint real_playlist_move_item(gint s, gint t);

static void playlist_background_reset(gint start);


/*
 *-----------------------------------------------------------------------------
 * misc utils
 *-----------------------------------------------------------------------------
 */

static gint path_list_sort_cb(gconstpointer a, gconstpointer b)
{
	return strcmp((gchar *)a, (gchar *)b);
}

GList *path_list_sort(GList *list)
{
	return g_list_sort(list, path_list_sort_cb);
}

GList *path_list_filter(GList *list, gint is_dir_list)
{
	GList *work;

	if (disable_filtering && show_dot_files) return list;

	work = list;
	while (work)
		{
		gchar *name = work->data;
		const gchar *base;

		base = filename_from_path(name);

		if ((!show_dot_files && file_is_hidden(base)) ||
		    (!is_dir_list && !disable_filtering && !file_is_in_filter(base)) )
			{
			GList *link = work;
			work = work->next;
			list = g_list_remove_link(list, link);
			g_free(name);
			g_list_free(link);
			}
		else
			{
			work = work->next;
			}
		}

	return list;
}


/*
 *-----------------------------------------------------------------------------
 * playlist shuffle functions (private)
 *-----------------------------------------------------------------------------
 */

static GList *shuffle_list_find_node(gint p)
{
	GList *work = g_list_nth(playlist, p);
	if (!work) return NULL;
	return g_list_find(shuffle_list, work->data);
}

static void shuffle_list_add (gint p)
{
	gint shuffle_p = (float)rand() / RAND_MAX * ( g_list_length(shuffle_list) + 1);
	GList *node = g_list_nth(playlist, p);
	shuffle_list = g_list_insert(shuffle_list, node->data, shuffle_p);
}

static void shuffle_list_remove (gpointer data)
{
	shuffle_list = g_list_remove(shuffle_list, data);
}

static gint shuffle_list_get_next(gint p)
{
	GList *node;

	if (!shuffle_list) return -1;

	node = shuffle_list_find_node(p);

	if (node)
		{
		node = node->next;
		}
	else
		{
		node = shuffle_list;
		}

	if (!node) return -1;

	return g_list_index(playlist, node->data);
}

static gint shuffle_list_get_prev(gint p)
{
	GList *node;

	if (!shuffle_list) return -1;

	node = shuffle_list_find_node(p);

	if (node)
		{
		node = node->prev;
		}
	else
		{
		node = g_list_last(shuffle_list);
		}

	if (!node) return -1;

	return g_list_index(playlist, node->data);
}

static gint shuffle_list_get_first(void)
{
	if (!playlist) return -1;
	if (!shuffle_list) return 0;
	return g_list_index(playlist, shuffle_list->data);
}

static gint shuffle_list_get_last(void)
{
	if (!playlist) return -1;
	if (!shuffle_list) return playlist_count - 1;
	return g_list_index(playlist, g_list_last(shuffle_list)->data);
}

/*
 *-----------------------------------------------------------------------------
 * playlist shuffle functions (public)
 *-----------------------------------------------------------------------------
 */

void shuffle_list_create(gint start_with_current)
{
	GList *work = NULL;
	GList *node = NULL;
	int i;

	if (shuffle_list)
		{
		shuffle_list_destroy();
		}

	if (debug_mode) printf("Shuffling:");

	if (playlist_count == 0) return;
		
	for (i=0; i < playlist_count; i++)
		{
		if (i != current_song_get_number() || !start_with_current)
			work = g_list_prepend(work, playlist_get_data(i));
		}
	work = g_list_reverse(work);

	if (start_with_current && current_song_get_number() >= 0 && current_song_get_number() < playlist_get_count())
		{
		node = g_list_nth(playlist, current_song_get_number());
		shuffle_list = g_list_prepend(shuffle_list, node->data);
		if (debug_mode) printf("%d ", current_song_get_number());
		}
	else if (playlist_get_count() > 1 && current_song_get_number() >= 0 && current_song_get_number() < playlist_get_count())
		{
		gint p;

		/* remove current song so we can not pick it */
		node = g_list_nth(work, current_song_get_number());
		work = g_list_remove(work, node->data);

		p = (float)rand() / RAND_MAX * g_list_length(work);
		node = g_list_nth(work, p);
		if (debug_mode) printf("%d ", g_list_index(playlist, node->data));
		shuffle_list = g_list_prepend(shuffle_list, node->data);

		/* and put current song back in*/
		work = g_list_insert(work, playlist_get_data(current_song_get_number()), current_song_get_number());

		work = g_list_remove(work, node->data);
		}

	while (work)
		{
		gint p =  (float)rand() / RAND_MAX * g_list_length(work);
		node = g_list_nth(work, p);
		if (debug_mode) printf("%d ",g_list_index(playlist, node->data));
		shuffle_list = g_list_prepend(shuffle_list, node->data);
		work = g_list_remove(work, node->data);
		}
	shuffle_list = g_list_reverse(shuffle_list);

	if (debug_mode) printf ("\n");
}

void shuffle_list_destroy(void)
{
	if (debug_mode) printf("Clearing shuffle list\n");
	if (!shuffle_list) return;
	g_list_free(shuffle_list);
	shuffle_list = NULL;
}

/*
 *-----------------------------------------------------------------------------
 * misc playlist format functions
 *-----------------------------------------------------------------------------
 */

static gchar *line_title(const gchar *text)
{
	const gchar *ptr;
	gint c = 0;

	if (!text || *text == '\0') return NULL;

	ptr = text;
	while (*ptr != '\0' && *ptr != '\n' && *ptr != '\r')
		{
		ptr++;
		c++;
		}

	return g_strndup(text, c);
}

static gchar *m3u_title(const gchar *text)
{
	const gchar *ptr;

	if (!text || strncmp(text, "#EXTINF:", 8) != 0) return NULL;

	ptr = text + 8;
	while (*ptr != ',' && *ptr != '\0') ptr++;
	if (*ptr != ',') return NULL;
	ptr++;

	return line_title(ptr);
}

/*
 *-----------------------------------------------------------------------------
 * playlist loading functions (public)
 *-----------------------------------------------------------------------------
 */

static gint  playlist_load_idle_id = -1;
static FILE  *playlist_load_fp = NULL;
static gint  playlist_load_append = FALSE;
static gint  playlist_load_strict = TRUE;
static gchar *playlist_load_path = NULL;
static guint32 playlist_load_timer = 0;
static GList *playlist_load_list = NULL;
static gint playlist_load_m3u = FALSE;
static gchar *playlist_load_next_title = NULL;
static gchar *playlist_load_next_comment = NULL;

static void playlist_load_set_next_title(const gchar *text)
{
	g_free(playlist_load_next_title);
	playlist_load_next_title = g_strdup(text);
}

static void playlist_load_set_next_comment(const gchar *text)
{
	g_free(playlist_load_next_comment);
	playlist_load_next_comment = g_strdup(text);
}

static void playlist_load_line_path(const gchar *path)
{
	playlist_load_list = g_list_prepend(playlist_load_list, playlist_add(path, FALSE));

	if (playlist_load_next_title || playlist_load_next_comment)
		{
		SongData *sd = playlist_load_list->data;

		/* oh well, if customized, we need original data NOW!
		 * this will hurt background loading a bit :(
		 * additional note: this should all be moved to a playlist_data_set_custom?? funcs
		 */
		playlist_update_by_data(sd);

		if (playlist_load_next_title)
			{
			playlist_data_customize_title(sd, playlist_load_next_title);
			playlist_load_set_next_title(NULL);
			}
		if (playlist_load_next_comment)
			{
			playlist_data_customize_comment(sd, playlist_load_next_comment);
			playlist_load_set_next_comment(NULL);
			}
		}
}

static gint playlist_load_list_iterate(void)
{
	SongData *sd;

	if (!playlist_load_list) return FALSE;

	sd = playlist_load_list->data;
	playlist_load_list = g_list_remove(playlist_load_list, sd);

	playlist_update_by_data(sd);

	return (playlist_load_list != NULL);
}		

/* return 0 on eof, 1 on line parsed, 2 on song added */
static gint playlist_load_line(FILE *f)
{
	gchar s_buf[1024];
	gchar *s_buf_ptr;

	if (debug_mode) putchar('.');

	if (f && fgets(s_buf, 1024, f))
		{
		if (*s_buf=='#')
			{
			if (!playlist_load_append && obey_mode_in_playlist && strncmp(s_buf,"# Shuffl",8) == 0)
				{
				gint old_shuffle = shuffle_mode;
				gint old_repeat = repeat_mode;
				if (sscanf(s_buf, "# Shuffle: %d Repeat: %d",
						&shuffle_mode, &repeat_mode) > 0)
					{
					if (old_shuffle != shuffle_mode)
						{
						if (shuffle_mode)
							shuffle_list_create(FALSE);
						else
							shuffle_list_destroy();
						display_set_shuffle();
						}
					if (old_repeat != repeat_mode)
						display_set_repeat();
					}
				}
			else if (strncmp(s_buf, "#TITLE:", 7) == 0)
				{
				playlist_load_set_next_title(line_title(s_buf + 7));
				}
			else if (strncmp(s_buf, "#COMMENT:", 9) == 0)
				{
				playlist_load_set_next_comment(line_title(s_buf + 9));
				}
			else if (playlist_load_m3u && strncmp(s_buf, "#EXTINF:", 8) == 0)
				{
				/* m3u song comment */
				playlist_load_set_next_title(m3u_title(s_buf));
				}
			else if (strncmp(s_buf, "#EXTM3U", 7) == 0)
				{
				playlist_load_m3u = TRUE;
				}
			return 1;
			}
		if (*s_buf=='\n') return 1;
		s_buf_ptr = s_buf;
		while (*s_buf_ptr != '\n' && *s_buf_ptr != '\0') s_buf_ptr++;
		*s_buf_ptr = '\0';
		if (!playlist_load_strict)
			{
			/* if not strict gqmpeg playlist, we are probably importing an
			   unknown file, so only add lines that pass the filters */
			if (typelist_determine_type_id(s_buf) != -1 ||
			    file_is_in_filter(s_buf) )
				{
				playlist_load_line_path(s_buf);
				return 2;
				}
			else
				{
				/* try to determine relative paths */
				if (!strncmp(s_buf, "../", 3) || !strncmp(s_buf, "./", 2) ||
				    (*s_buf != '/' && strchr(s_buf, ':') == NULL) )
					{
					gchar *buf;
					gchar *base;
					base = remove_level_from_path(playlist_load_path);
					buf = g_strconcat(base, "/", s_buf, NULL);
					g_free(base);
					parse_out_relatives(buf);
						if (typelist_determine_type_id(buf) != -1 ||
						    file_is_in_filter(buf) )
						{
						playlist_load_line_path(buf);
						return 2;
						}
					else
						{
						printf(_("undetermined type:%s\n"), s_buf);
						}
						g_free(buf);
					}
				else
					{
					printf(_("undetermined type:%s\n"), s_buf);
					}
				}
			}
		else
			{
			playlist_load_line_path(s_buf);
			return 2;
			}
		return 1;
		}

	return 0;
}

static void playlist_load_finish(gint background)
{
	if (debug_mode) printf("playlist finish\n");

	if (!playlist_load_append)
		{
		gint playlist_song;

		if (!background)
			{
			if (playlist_count > 0)
				{
				if (shuffle_mode)
					{
					playlist_song = shuffle_list_get_first();
					}
				else
					{
					playlist_song = 0;
					}
				}
			else
				{
				playlist_song = -1;
				}
			current_song_set(playlist_song, NULL);
			}
		}
}

static void playlist_load_end(void)
{
	if (debug_mode) printf("playlist end\n");

	display_thaw();
	display_set_loading(FALSE);
	display_set_song_count(playlist_count);
	display_set_song_number(current_song_get_number());
	display_playlist_refresh();

	if (playlist_load_fp)
		{
		fclose (playlist_load_fp);
		playlist_load_fp = NULL;

		ipc_status_send("playlist:done");
		}
	if (playlist_load_idle_id != -1)
		{
		playlist_load_idle_id = -1;
		}
	if (playlist_load_list)
		{
		g_list_free(playlist_load_list);
		playlist_load_list = NULL;
		}
}

static gint playlist_load_idle_cb(gpointer data)
{
	guint32 new_time;

	if (playlist_load_idle_id == -1 || (!playlist_load_fp && !playlist_load_list) || !playlist_load_path)
		{
		playlist_load_end();
		return FALSE;
		}

	new_time = elapsed_run_time();

	if (new_time - playlist_load_timer > DISPLAY_LOADING_ANIMATE_TIME)
		{
		playlist_load_timer = new_time;
		display_set_loading(TRUE);

		display_thaw();
		display_set_song_count(playlist_count);
		display_set_song_number(current_song_get_number());
		display_freeze();
		}

	if (playlist_load_fp)
		{
		if (playlist_load_line(playlist_load_fp) == 0)
			{
			fclose (playlist_load_fp);
			playlist_load_fp = NULL;
			}
		}
	else if (!playlist_load_list_iterate())
		{
		playlist_load_end();
		playlist_load_finish(TRUE);
		}

	return TRUE;
}

gint playlist_load_start(const gchar *fn, gint append, gint strict, gint parse_background, gint info_background)
{
	gchar s_buf[1025];
	gchar *pathl;

	if (playlist_load_idle_id || playlist_load_fp)
		{
		if (playlist_load_idle_id != -1) gtk_idle_remove(playlist_load_idle_id);
		playlist_load_end();		
		}

	playlist_load_append = append;
	playlist_load_strict = strict;
	g_free(playlist_load_path);
	playlist_load_path = g_strdup(fn);

	playlist_load_m3u = FALSE;
	playlist_load_set_next_title(NULL);
	playlist_load_set_next_comment(NULL);

	pathl = path_from_utf8(playlist_load_path);
	playlist_load_fp = fopen(pathl, "r");
	g_free(pathl);
	if (!playlist_load_fp)
		{
		/* file open failed */
		printf(_("failed to open \"%s\"\n"), playlist_load_path);
		return FALSE;
		}
	else
		{
		gint c;
		gchar *buf;

		if (strict)
			{
			/* check first line for valid playlist */
			fgets(s_buf, 1024, playlist_load_fp);
			if (strncmp(s_buf,"# GQ", 4) != 0)
				{
				/* we reach here, file is not a valid playlist */
				fclose (playlist_load_fp);
				playlist_load_fp = NULL;
				return -1;
				}
			}

		buf = g_strdup_printf("playlist:read %s", playlist_load_path);
		ipc_status_send(buf);
		g_free(buf);

		/* load playlist */
		if (!append)
			{
			playlist_clear();
			if (playlist_load_list)
				{
				g_list_free(playlist_load_list);
				playlist_load_list = NULL;
				}
			}
		display_freeze();
		display_set_loading(TRUE);

		c = 1;
		while (c == 1)
			{
			c = playlist_load_line(playlist_load_fp);
			}
		if (c == 0)
			{
			playlist_load_end();
			return FALSE;
			}
		if (info_background)
			{
			/* if random mode, load it all first,
			 * this is the only way to avoid the first song always being #1
			 */
			if (!parse_background)
				{
				while (c != 0)
					{
					c = playlist_load_line(playlist_load_fp);
					}
				fclose(playlist_load_fp);
				playlist_load_fp = NULL;
				current_song_set(shuffle_list_get_first(), NULL);

				ipc_status_send("playlist:done");
				}
			else
				{
				current_song_set(0, NULL);
				}

			if (playlist_load_idle_id == -1)
				{
				playlist_load_idle_id = gtk_idle_add(playlist_load_idle_cb, NULL);
				}
			if (debug_mode) printf("playlist start\n");

			display_set_song_count(playlist_count);

			display_set_loading(TRUE);

			playlist_load_timer = elapsed_run_time();
			}
		else
			{
			display_set_loading(TRUE);

			while (c != 0)
				{
				c = playlist_load_line(playlist_load_fp);
				}
			while(playlist_load_list_iterate());

			playlist_load_end();
			playlist_load_finish(FALSE);
			}
		g_free(playlist_pathname);
		playlist_pathname = g_strdup(playlist_load_path);
		g_free(playlist_filename);
		playlist_filename = g_strdup(filename_from_path(playlist_pathname));

		display_presets_refresh();
		}

	return TRUE;
}

/* returns FALSE if can't access file, TRUE if a playlist,
	(-1) otherwise (invalid playlist)*/
gint playlist_load(const gchar *fn, gint append, gint strict)
{
	return playlist_load_start(fn, append, strict, !shuffle_mode, TRUE);
}

gint playlist_load_from_file(const gchar *path, gint append, gint strict, gint show_warnings)
{
	gint load_val;
	gint last_status;

	last_status = status;

	load_val = playlist_load(path, append, strict);
	if (show_warnings)
		{
		if (load_val == FALSE)
			{
			warning_dialog(_("Open failed"), _("The specified file could\nnot be opened."));
			}
		if (load_val == -1)
			{
			warning_dialog(_("Invalid file"), _("The specified file is\nnot a valid playlist."));
			}
		}

	playlist_window_update_titles();

	if (load_val == TRUE && !append && last_status == STATUS_PLAY)
		{
		playback_exec_command(EXEC_PLAY, current_song_get_data(), 0);
		}

	return load_val;
}

/*
 *-----------------------------------------------------------------------------
 * playlist saving functions (public)
 *-----------------------------------------------------------------------------
 */

gint playlist_save(const gchar *fn)
{
	gchar *pathl;
	int i;
	FILE *f;

	pathl = path_from_utf8(fn);
	f = fopen (pathl, "w");
	g_free(pathl);
	if (!f)
		{
		/* file open failed */
		printf(_("failed to open %s\n"),fn);
		return FALSE;
		}

	fprintf(f,"# GQmpeg song playlist\n");
	fprintf(f,"# created by version %s\n", VERSION);

	if (save_mode_in_playlist)
		{
		fprintf(f,"# Shuffle:%d Repeat:%d (please only use 0 or 1)\n", shuffle_mode, repeat_mode);
		}

	for (i=0; i < playlist_count; i++)
		{
		SongData *sd;

		sd = playlist_get_data(i);
		if (sd)
			{
			if (sd->customized)
				{
				if (sd->custom_title && sd->title && strlen(sd->title) > 0)
					{
					fprintf(f, "#TITLE:%s\n", sd->title);
					}
				if (sd->custom_comment && sd->comment && strlen(sd->comment) > 0)
					{
					fprintf(f, "#COMMENT:%s\n", sd->comment);
					}
				}
			fprintf(f, "%s\n", playlist_data_get_path(sd));
			}
		}

	fprintf(f,"# end of list #\n");
	fclose (f);

	if (strcmp(fn, playlist_pathname) != 0)
		{
		g_free(playlist_pathname);
		playlist_pathname = g_strdup(fn);
		g_free(playlist_filename);
		playlist_filename = g_strdup(filename_from_path(playlist_pathname));
		playlist_window_update_titles();

		display_presets_refresh();
		}

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * playlist load directory functions
 *-----------------------------------------------------------------------------
 */

static gint playlist_append_dir_func(const gchar *d, gint recursive, gint row)
{
	GList *files;
	GList *dirs;
	GList *work;

	if (!path_list(d, &files, &dirs)) return row;

	files = path_list_filter(files, FALSE);
	files = path_list_sort(files);

	work = files;
	while (work)
		{
		gchar *path = work->data;
		work = work->next;
		if (row < 0)
			{
			playlist_add(path, TRUE);
			}
		else
			{
			playlist_insert(path, row, TRUE);
			row ++;
			}
		}
	path_list_free(files);

	if (recursive)
		{
		dirs = path_list_filter(dirs, TRUE);
		dirs = path_list_sort(dirs);
		work = dirs;
		while (work)
			{
			gchar *path = work->data;
			work = work->next;
			row = playlist_append_dir_func(path, row, TRUE);
			}
		}
	path_list_free(dirs);

	return row;
}

void playlist_append_from_dir(const gchar *path, gint recursive)
{
	playlist_append_dir_func(path, recursive, -1);
	display_set_song_count(playlist_count);
}

gint playlist_insert_from_dir(const gchar *path, gint row, gint recursive)
{
	row = playlist_append_dir_func(path, recursive, row);
	display_set_song_count(playlist_count);

	return row;
}

/*
 *-----------------------------------------------------------------------------
 * playlist data functions (public)
 *-----------------------------------------------------------------------------
 */

SongData *playlist_data_new(const gchar *path, gint read_info)
{
	SongData *sd;

	if (!path) return NULL;

	sd = player_module_songdata_init(path);
	if (read_info) player_module_songdata_update(sd, read_file_information);

	return sd;
}

void playlist_data_free(SongData *sd)
{
	g_free(sd->path);
	g_free(sd->title);
	g_free(sd->artist);
	g_free(sd->album);
	/* g_free(sd->genre); not freed, points to static */
	g_free(sd->comment);
	g_free(sd->year);

	if (sd->free_data_func) sd->free_data_func(sd->data);

	g_free(sd);
}

const gchar *playlist_data_get_path(SongData *sd)
{
	if (!sd) return NULL;
	return sd->path;
}

const gchar *playlist_data_get_title(SongData *sd)
{
	if (!sd) return NULL;

	if (sd->title) return sd->title;
	if (!sd->path || sd->path[0] != '/')
		{
		return sd->path;
		}

	if (!title_show_extension || title_convert_underscores)
		{
		static gchar *title_text = NULL;

		if (title_text) g_free(title_text);

		if (title_show_extension)
			{
			title_text = g_strdup(filename_from_path(sd->path));
			}
		else
			{
			title_text = remove_extension_from_path(filename_from_path(sd->path));
			}

		if (title_convert_underscores) convert_chars(title_text, '_', ' ');

		return title_text;
		}

	return filename_from_path(sd->path);
}

const gchar *playlist_data_get_artist(SongData *sd)
{
	if (!sd) return NULL;
	return sd->artist;
}

const gchar *playlist_data_get_album(SongData *sd)
{
	if (!sd) return NULL;
	return sd->album;
}

const gchar *playlist_data_get_genre(SongData *sd)
{
	if (!sd) return NULL;
	return sd->genre;
}

const gchar *playlist_data_get_year(SongData *sd)
{
	if (!sd) return NULL;
	return sd->year;
}

const gchar *playlist_data_get_comment(SongData *sd)
{
	if (!sd) return NULL;
	return sd->comment;
}

gint playlist_data_get_track(SongData *sd)
{
	if (!sd) return 0;
	return (gint)sd->track;
}

gchar *playlist_data_get_time(SongData *sd)
{
	if (!sd) return NULL;

	if (sd->live) return g_strdup(_("live"));

	return g_strdup_printf("%0d:%02d", sd->length / 60, sd->length % 60);
}

/*
 *-----------------------------------------------------------------------------
 * playlist basic functions (private)
 *-----------------------------------------------------------------------------
 */

static void real_playlist_init(void)
{
	playlist = NULL;
	playlist_count = 0;
	playlist_length = 0;
}

static void real_playlist_clear(void)
{
	playlist_background_reset(FALSE);

        if (playlist)
                {
                GList *list;
                list = playlist;
                while (list)
                        {
                        playlist_data_free(list->data);
                        list = list->next;
                        }
                g_list_free(playlist);
                playlist = NULL;
                }
	real_playlist_init();

	if (shuffle_mode) shuffle_list_destroy();
}

static SongData *real_playlist_add_item(const gchar *path, gint read_info)
{
	SongData *sd = playlist_data_new(path, read_info);
	playlist = g_list_append(playlist, sd);
	playlist_count++;
	if (sd->length) playlist_length += sd->length;

	if (shuffle_mode) shuffle_list_add (g_list_index(playlist, sd));

	return sd;
}

static SongData *real_playlist_insert_item(const gchar *path, gint n, gint read_info)
{
	SongData *sd = playlist_data_new(path, read_info);

	if (n >= playlist_count)
		{
		playlist = g_list_append(playlist, sd);
		}
	else
		{
		playlist = g_list_insert(playlist, sd, n);
		}

	playlist_count++;
	if (sd->length) playlist_length += sd->length;

	if (shuffle_mode) shuffle_list_add (g_list_index(playlist, sd));

	return sd;
}

static gint real_playlist_remove_item(gint n)
{
	SongData *sd;
	GList *list;
	if (n >= playlist_count) return FALSE;
	list = g_list_nth(playlist, n);

	if (!list) return FALSE;

	sd = list->data;

	if (shuffle_mode) shuffle_list_remove (sd);

	playlist = g_list_remove(playlist, sd);
	playlist_count--;
	if (sd->length) playlist_length -= sd->length;

	playlist_data_free(sd);

	return TRUE;
}

static gint real_playlist_move_item(gint s, gint t)
{
	SongData *sd;
	GList *list = NULL;

	if (s >= playlist_count || t >= playlist_count || s == t || s < 0 || t < 0) return FALSE;
	list = g_list_nth(playlist, s);

	if (!list) return FALSE;

	sd = list->data;

	playlist = g_list_remove(playlist, sd);
	playlist = g_list_insert(playlist, sd, t);

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * playlist info retrieval functions (public)
 *-----------------------------------------------------------------------------
 */

SongData *playlist_get_data(gint n)
{
	GList *list;
	if (n >= playlist_count || n < 0) return NULL;
	list = g_list_nth(playlist, n);
	if (!list) return NULL;
	return list->data;
}

const gchar *playlist_get_item(gint n)
{
	SongData *sd;
	sd = playlist_get_data(n);
	return (playlist_data_get_path(sd));
}

const gchar *playlist_get_title(gint n)
{
	SongData *sd = playlist_get_data(n);
	return (playlist_data_get_title(sd));
}

const gchar *playlist_get_artist(gint n)
{
	SongData *sd = playlist_get_data(n);
	return (playlist_data_get_artist(sd));
}

const gchar *playlist_get_album(gint n)
{
	SongData *sd = playlist_get_data(n);
	return (playlist_data_get_album(sd));
}

const gchar *playlist_get_genre(gint n)
{
	SongData *sd = playlist_get_data(n);
	return (playlist_data_get_genre(sd));
}

const gchar *playlist_get_year(gint n)
{
	SongData *sd = playlist_get_data(n);
	return (playlist_data_get_year(sd));
}

const gchar *playlist_get_comment(gint n)
{
	SongData *sd = playlist_get_data(n);
	return (playlist_data_get_comment(sd));
}

gint playlist_get_track(gint n)
{
	SongData *sd = playlist_get_data(n);
	return (playlist_data_get_track(sd));
}

gchar *playlist_get_time(gint n)
{
	SongData *sd = playlist_get_data(n);
	return (playlist_data_get_time(sd));
}

gint playlist_item_is_live(gint n)
{
	SongData *sd = playlist_get_data(n);
	if (!sd) return FALSE;

	return sd->live;
}

gint playlist_item_is_custom(gint n)
{
	SongData *sd = playlist_get_data(n);
	if (!sd) return FALSE;

	return sd->custom;
}

gint playlist_get_index(const gchar *path)
{
	SongData *sd;
	GList *list = playlist;
	gint c = 0;

	while (list)
		{
		sd = list->data;
		if (strcmp(sd->path, path) == 0) return c;
		c++;
		list = list->next;
		}

	return -1;
}

/* this is much faster than the above, but needs more specific data */
gint playlist_get_index_by_data(SongData *sd)
{
	return g_list_index(playlist, sd);
}

gint playlist_get_count(void)
{
	return playlist_count;
}

gint playlist_get_length(void)
{
	return playlist_length;
}

/* the get accumulated does not include the current song */
gint playlist_get_length_accumulated(gint n)
{
	gint r;
	gint total;

	if (n < 0 || n >= playlist_get_count()) return playlist_get_length();

	total = 0;
	r = playlist_get_first();
	while (r != n)
		{
		SongData *sd = playlist_get_data(r);
		if (sd) total += sd->length;
		r = playlist_get_next(r);
		}

	return total;
}

/* the get remaing does include the current song */
gint playlist_get_length_remaining(gint n)
{
	gint r;
	gint total;

	if (n < 0 || n >= playlist_get_count()) return playlist_get_length();

	total = 0;
	r = n;
	while (r != -1)
		{
		SongData *sd = playlist_get_data(r);
		if (sd) total += sd->length;
		r = playlist_get_next(r);
		}

	return total;
}

gint playlist_get_next(gint n)
{
	if (playlist_count == 0) return -1;

	if (shuffle_mode)
		{
		return shuffle_list_get_next(n);
		}
	else
		{
		if (n == -1)
			{
			return 0;
			}
		else if (n + 1 < playlist_count)
			{
			return n + 1;
			}
		}

	return -1;
}

gint playlist_get_prev(gint n)
{
	if (playlist_count == 0) return -1;

	if (shuffle_mode)
		{
		return shuffle_list_get_prev(n);
		}

	if (n == -1)
		{
		return playlist_count - 1;
		}
	else if (n - 1 >= 0)
		{
		return n - 1;
		}

	return -1;
}

gint playlist_get_first(void)
{
	if (playlist_count == 0) return -1;
	if (shuffle_mode)
		{
		return shuffle_list_get_first();
		}

	return 0;
}

gint playlist_get_last(void)
{
	if (playlist_count == 0) return -1;
	if (shuffle_mode)
		{
		return shuffle_list_get_last();
		}

	return (playlist_count - 1);
}

/*
 *-----------------------------------------------------------------------------
 * playlist management functions (public)
 *-----------------------------------------------------------------------------
 */

static void playlist_recalc_length(void)
{
	GList *work;

	playlist_length = 0;

	work = playlist;
	while (work)
		{
		SongData *sd = work->data;
		if (sd->length) playlist_length += sd->length;
		work = work->next;
		}
}

void playlist_update_all_info(void)
{
	GList *list;

	if (!playlist) return;

	list = playlist;
	while (list)
		{
		SongData *sd = list->data;
		player_module_songdata_update(sd, read_file_information);
		list = list->next;
		}

	if (read_file_information)
		{
		playlist_recalc_length();
		}

	playlist_window_populate();
}

void playlist_sort_by_func(GCompareFunc sort_func)
{
	if (!playlist) return;
	playlist = g_list_sort(playlist, sort_func);
}

/*
 *-----------------------------------------------------------------------------
 * playlist manipulation functions (public)
 *-----------------------------------------------------------------------------
 */

SongData *playlist_add(const gchar *path, gint read_info)
{
	SongData *sd;

	sd = real_playlist_add_item(path, read_info);

	playlist_window_append(playlist_count - 1);

	if (playlist_count == 1 && current_song_get_path() == NULL)
		{
		current_song_set(0, NULL);
		}
	display_set_song_count(playlist_count);
	display_total_time_changed();

	return sd;
}
 
SongData *playlist_insert(const gchar *path, gint n, gint read_info)
{
	SongData *sd;

	sd = real_playlist_insert_item(path, n, read_info);

	playlist_window_insert(n);

	if (n <= current_song_get_number())
		{
		display_set_song_number(current_song_get_number());
		}
	display_set_song_count(playlist_count);
	display_total_time_changed();

	return sd;
}

gint playlist_move(gint s, gint t)
{
	if (!real_playlist_move_item(s, t)) return FALSE;

	display_set_song_number(current_song_get_number());
	display_total_time_changed();

	playlist_window_move(s, t);

	return TRUE;
}

gint playlist_remove(const gchar *path, gint n, gint all)
{
	gint ret = FALSE;

	if (path)
		{
		n = playlist_get_index(path);
		if (n < 0) return FALSE;
		}

	while (n >= 0)
		{
		gint p = current_song_get_number();
		gint removed = real_playlist_remove_item(n);

		if (removed)
			{
			playlist_window_remove(n);

			if (p == n)
				{
				if (p < playlist_count)
					{
					current_song_set(p, NULL);
					}
				else
					{
					current_song_set(playlist_count - 1, NULL);
					}
				}
			else if (n < p)
				{
				display_set_song_number(current_song_get_number());
				}
			display_set_song_count(playlist_count);
			}

		if (path && all && removed)
			{
			n = playlist_get_index(path);
			}
		else
			{
			n = -1;
			}

		ret = ret | removed;
		}

	display_total_time_changed();

	return ret;
}

void playlist_update_by_data_full(SongData *sd, gint force)
{
	gint n;

	n = g_list_index(playlist, sd);
	if (n < 0) return;

	/* do not do it more than once when forcing */
	if (force && sd->info_loaded) return;

	playlist_length -= sd->length;

	player_module_songdata_update(sd, (read_file_information || force));

	if (current_song_get_data() == sd) display_set_song_text_info(-1, TRUE);

	playlist_window_update_item(n);

	playlist_length += sd->length;
	display_total_time_changed();
}

void playlist_update_by_data(SongData *sd)
{
	playlist_update_by_data_full(sd, FALSE);
}

void playlist_update(gint n, const gchar *path)
{
	gint p = current_song_get_number();

	if (!real_playlist_remove_item(n)) return;

	if (path)
		{
		real_playlist_insert_item(path, n, TRUE);
		playlist_window_update_item(n);
		if (n == p)
			{
			current_song_set(n, NULL);
			}
		}
	else
		{
		playlist_window_remove(n);
		if (n == p)
			{
			current_song_set_to_next();
			}
		}
	display_total_time_changed();
}

void playlist_update_by_path(const gchar *path)
{
	gint i;
	if (!path) return;

	for (i = 0; i < playlist_get_count(); i++)
		{
		if (strcmp(path, playlist_get_item(i)) == 0)
		playlist_update(i, path);
		}
}

void playlist_replace(const gchar *old_path, const gchar *new_path)
{
	gint n = playlist_get_index(old_path);
	while (n >= 0)
		{
		playlist_update(n, new_path);
		n = playlist_get_index(old_path);
		}
}

void playlist_clear(void)
{
	if (current_song_is_in_playlist()) current_song_set(-1, NULL);
	display_set_song_count(0);
	real_playlist_clear();
	display_total_time_changed();
	playlist_window_clear();
}

/*
 *-----------------------------------------------------------------------------
 * playlist data customization functions (public)
 *-----------------------------------------------------------------------------
 */

static void playlist_data_customize_chk(SongData *sd)
{
	sd->customized = (sd->custom_title || sd->custom_artist || sd->custom_album ||
			  sd->custom_comment || sd->custom_length);
	if (sd->customized)
		{
		sd->flags |= SONG_FLAG_CUSTOMIZED;
		}
	else
		{
		sd->flags &= ~SONG_FLAG_CUSTOMIZED;
		}
}

void playlist_data_customize_title(SongData *sd, const gchar *title)
{
	gint n;

	if (!sd) return;

	if (title && strlen(title) == 0) title = NULL;

	g_free(sd->title);
	sd->title = g_strdup(title);

	sd->custom_title = (title != NULL);
	playlist_data_customize_chk(sd);

	if (current_song_get_data() == sd) display_set_song_text_info(-1, TRUE);

	n = playlist_get_index_by_data(sd);
	playlist_window_update_item(n);
}

void playlist_data_customize_comment(SongData *sd, const gchar *comment)
{
	gint n;

	if (!sd) return;

	if (comment && strlen(comment) == 0) comment = NULL;

	g_free(sd->comment);
	sd->comment = g_strdup(comment);

	sd->custom_comment = (comment != NULL);
	playlist_data_customize_chk(sd);

	if (current_song_get_data() == sd) display_set_song_text_info(-1, TRUE);

	n = playlist_get_index_by_data(sd);
	playlist_window_update_item(n);
}

/*
 *-----------------------------------------------------------------------------
 * song flag functions (public)
 *-----------------------------------------------------------------------------
 */

SongFlags playlist_get_flags(gint n)
{
	SongData *sd = playlist_get_data(n);

	if (!sd) return 0;
	return sd->flags;
}

void playlist_set_flags(gint n, SongFlags flags)
{
	SongData *sd = playlist_get_data(n);

	if (!sd) return;
	sd->flags |= flags;

	playlist_window_update_song_icon_by_flags(n, sd->flags);
}

void playlist_unset_flags(gint n, SongFlags flags)
{
	SongData *sd = playlist_get_data(n);

	if (!sd) return;
	sd->flags &= ~flags;

	playlist_window_update_song_icon_by_flags(n, sd->flags);
}

/*
 *-----------------------------------------------------------------------------
 * playlist utility functions (public)
 *-----------------------------------------------------------------------------
 */

gint is_playlist(const gchar *path)
{
	if (strlen(path) > 7 && strncasecmp(path + (strlen(path) - 7), ".gqmpeg", 7) == 0) return TRUE;

	return FALSE;
}

void playlist_randomize(void)
{
	GList *list;

	list = playlist;
	playlist = NULL;

	while(list)
		{
		GList *node;
		gint p;

		p =  (float)rand() / RAND_MAX * g_list_length(list);
		node = g_list_nth(list, p);
		playlist = g_list_prepend(playlist, node->data);
		list = g_list_remove(list, node->data);
		}
	playlist_window_populate();
	display_total_time_changed();
	display_set_song_number(current_song_get_number());
}

/*
 *-----------------------------------------------------------------------------
 * playlist on demand info background loading
 *-----------------------------------------------------------------------------
 */

static GList *playlist_background_list = NULL;
static gint playlist_background_id = -1;

static gint playlist_background_cb(gpointer data)
{
	static guint32 old_time = 0;

	SongData *sd;
	guint32 new_time;

	if (playlist_background_list == NULL)
		{
		playlist_background_id = -1;
		display_set_loading(FALSE);
		return FALSE;
		}

	new_time = elapsed_run_time();
	if (new_time - old_time > DISPLAY_LOADING_ANIMATE_TIME)
		{
		old_time = new_time;
		display_set_loading(TRUE);
		}

	sd = playlist_background_list->data;
	playlist_background_list = g_list_remove(playlist_background_list, sd);

	/* this call does the pointer validation for us */
	playlist_update_by_data_full(sd, TRUE);

	return TRUE;
}

static void playlist_background_reset(gint start)
{
	if (!start && playlist_background_id != -1)
		{
		gtk_idle_remove(playlist_background_id);
		playlist_background_id = -1;

		g_list_free(playlist_background_list);
		playlist_background_list = NULL;

		display_set_loading(FALSE);
		return;
		}

	if (start && playlist_background_id == -1)
		{
		playlist_background_id = gtk_idle_add(playlist_background_cb, NULL);
		}
}

void playlist_update_background(SongData *sd)
{
	if (!sd || sd->info_loaded) return;

	playlist_background_list = g_list_append(playlist_background_list, sd);
	playlist_background_reset(TRUE);
}

