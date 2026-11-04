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


#ifndef IO_MPG123_H
#define IO_MPG123_H


#define		MPG123_DOWNSAMPLE_AUTO 0
#define		MPG123_DOWNSAMPLE_22 1
#define		MPG123_DOWNSAMPLE_11 2
#define		MPG123_DOWNSAMPLE_CUSTOM 3

typedef struct _Mpg_Data Mpg_Data;
struct _Mpg_Data
{
	gint version;
	gint layer;
	gint error_protection;
	gint bit_rate;
	gint sample_rate;
	gint padding;
	gint extension;
	gint channel_mode;
	gint mode_extension;
	gint copyright;
	gint original;
	gint emphasis;

	gchar *version_text;
	gchar *version_number_text;
	gchar *layer_text;
	gchar *mode_text;

	gint stereo;			/* if true, 2 or more channels */
	gint file_length;		/* length of file in bytes */
	gint length;			/* length in seconds */
};

typedef struct _ID3_Data ID3_Data;
struct _ID3_Data
{
	gchar *title;
	gchar *artist;
	gchar *album;
	gchar *year;
	gchar *comment;
	guint8 genre;
	gchar *genre_description;
	guint8 track;	/* id3 v1.1 spec */
};

extern gint mpg123_enabled;

	/* options */
extern gint mpg123_buffer_enable;
extern gint mpg123_buffer;
extern gint mpg123_downsample;
extern gint mpg123_custom_sample_size;
extern gint mpg123_mono;
extern gint mpg123_8bit;
extern gint mpg123_device_enable;
extern gchar *mpg123_device;
extern gint mpg123_aggressive;
extern gint mpg123_disable_resync;
extern gint mpg123_to_wav;
extern gint mpg123_to_raw;
extern gint mpg123_to_wav_path_enable;
extern gchar *mpg123_to_wav_path;
extern gint mpg123_read_id3_tags;
extern gchar *mpg123_extra_options;

/*
 * =========================== io_mpg123.c
 */
gint type_is_http(const gchar *path);
void mpg123_init(void);

/*
 * =========================== io_mpg123_cfg.c
 */
void mpg123_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all);
void mpg123_config_save(FILE *f);
void mpg123_config_apply(void);
void mpg123_config_close(void);
GtkWidget *mpg123_config_init(void);

/*
 * =========================== mpg_hdrutil.c
 */
Mpg_Data *get_mpg_header_info(const gchar *path);

/*
 * =========================== mpg_tagutil.c
 */
gchar* get_id3_genre(gint g);
ID3_Data *get_id3_tag(const gchar *path);
gint set_id3_tag(const gchar *path, const gchar *title, const gchar *artist, const gchar *album,
		 const gchar *year, const gchar *comment, guint8 genre, guint8 track);
void free_id3_tag(ID3_Data *d);

/*
 * =========================== mpg_info.c
 */
GtkWidget *mpg_create_info_window(const gchar *file);


#endif

