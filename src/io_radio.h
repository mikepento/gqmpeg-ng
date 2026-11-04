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


#ifndef IO_RADIO_H
#define IO_RADIO_H


extern gint radio_enabled;
extern gchar *radio_device;


/*
 * =========================== io_radio.c
 */

gint radio_test(gint recheck);

void radio_init(void);

/*
 * =========================== io_radio_cfg.c
 */
void radio_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all);
void radio_config_save(FILE *f);
void radio_config_apply(void);
void radio_config_close(void);
GtkWidget *radio_config_init(void);


#endif


