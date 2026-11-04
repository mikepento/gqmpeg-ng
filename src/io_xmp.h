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


#ifndef IO_XMP_H
#define IO_XMP_H


extern gint xmp_enabled;

/* software mixer options */
extern gint xmp_frequency;
extern gint xmp_8bit;
extern gint xmp_interpolate;

/* sound options */
extern gint xmp_mono;
extern gint xmp_pan_percent;

/* output to wav */
extern gint xmp_output_to_wav;
extern gint xmp_output_to_wav_to_path;
extern gchar *xmp_output_to_wav_path;


/*
 * =========================== io_xmp.c
 */

gint xmp_extra_data_get_positions(SongData *sd);
gint xmp_extra_data_get_patterns(SongData *sd);
gint xmp_extra_data_get_instruments(SongData *sd);
gint xmp_extra_data_get_channels(SongData *sd);
const gchar *xmp_extra_data_get_type(SongData *sd);
const gchar *xmp_extra_data_get_tracker(SongData *sd);

void xmp_init(void);


/*
 * =========================== io_xmp_cfg.c
 */
void xmp_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all);
void xmp_config_save(FILE *f);
void xmp_config_apply(void);
void xmp_config_close(void);
GtkWidget *xmp_config_init(void);

/*
 * =========================== mod_info.c
 */
GtkWidget *mod_create_info_window(const gchar *file);


#endif

