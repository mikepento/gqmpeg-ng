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


#ifndef IO_OGG123_H
#define IO_OGG123_H


typedef struct _OGGInfo OGGInfo;
struct _OGGInfo
{
	gint pass_integrity;
	gint channels;
	gint bitrate_lower;
	gint bitrate_upper;
	gint bitrate_average;
	gint bitrate_nominal;
	gint length;		/* seconds */
	gint bytes;		/* file size */
	gchar *vendor;
};


extern gint ogg123_enabled;

extern gint ogg123_device;
extern gchar *ogg123_device_options;
extern gchar *ogg123_extra_options;


/*
 * =========================== io_ogg123.c
 */
const gchar *ogg123_get_device(gint device);
GList *ogg123_get_device_list(void);

OGGInfo *ogg123_info_get(const gchar *path);
void ogg123_info_free(OGGInfo *info);
GList *ogg123_comment_get(const gchar *path);
const gchar *ogg123_comment_value(GList *list, const gchar *key);
gint ogg123_comment_set(const gchar *path, GList *comments);

void ogg123_init(void);

/*
 * =========================== io_ogg123_cfg.c
 */
void ogg123_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all);
void ogg123_config_save(FILE *f);
void ogg123_config_apply(void);
void ogg123_config_close(void);
GtkWidget *ogg123_config_init(void);

/*
 * =========================== ogg_info.c
 */
GtkWidget *ogg123_info_window(const gchar *file);


#endif


