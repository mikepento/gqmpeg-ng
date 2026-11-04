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


#ifndef MIXER_H
#define MIXER_H


void mixer_init(gint init_device_id);
void set_volume(gint vol);
gint get_volume(void);
void set_balance(gint bal);
gint get_balance(void);

GList *get_mixer_device_list(void);
gint get_mixer_device_id(gchar *device_name);
void set_mixer_device(gint device_id);


#endif
