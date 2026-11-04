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

#ifndef PLAYERS_H
#define PLAYERS_H

typedef struct _IO_ModuleData IO_ModuleData;
struct _IO_ModuleData
{
	gchar *title;
	gchar *description;

	void (* songdata_init_func)(SongData *sd);
	gint (* songdata_info_func)(SongData *sd, gint read_info);

	gint (* start_func)(SongData *sd, gint position);
	gint (* stop_func)(SongData *sd);

	gint (* pause_func)(SongData *sd);
	gint (* continue_func)(SongData *sd);

	gint (* seek_func)(SongData *sd, gint position);

	void (* config_load_func)(FILE *f, const gchar *option, const gchar *value, const gchar *value_all);
	void (* config_save_func)(FILE *f);

	GtkWidget *(* config_init_func)(void);
	void (* config_apply_func)(void);
	void (* config_close_func)(void);

	GtkWidget *(* info_func)(const gchar *name);
};

/*
 *----------------------------------------------------------------------------
 * misc module interface functions
 *----------------------------------------------------------------------------
 */

gint player_module_register(IO_ModuleData *imd);
void module_register_file_suffix_type(const gchar *extension, const gchar *description, gint module_id);
void module_register_misc_type(const gchar *description, const gchar *format, gint module_id, gint live,
			       gint (*is_type_func)(const gchar *),
			       GtkWidget *(*entry_setup_func)(const gchar *),
			       GtkWidget *(*edit_func)(SongData *),
			       gchar *(*get_path_func)(GtkWidget *));

gint player_module_id_exists(gint id);

void playback_done_command(PlayerCommand cmd, gint err);

void module_playback_end(void);
void module_playback_error(SongData *sd);

/* call this when the input or output rates/channels/et.c change */
void module_playback_data_changed(void);

/*
 *----------------------------------------------------------------------------
 * misc stuff
 *----------------------------------------------------------------------------
 */

void player_modules_init(void);
void player_modules_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all);
void player_modules_config_save(FILE *f);
GList *player_modules_config_init(void);
void player_modules_config_apply(void);
void player_modules_config_close(void);

SongData *player_module_songdata_init(const gchar *path);
gint player_module_songdata_update(SongData *sd, gint read_info);
GtkWidget *player_module_songdata_detail_info(const gchar *path);

gint playback_exec_command(PlayerCommand cmd, SongData *sd, gint pos);


#endif


