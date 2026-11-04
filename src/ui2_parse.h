/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#ifndef UI2_PARSE_H
#define UI2_PARSE_H


gint key_list_read_bool(GList *list, const gchar *key);
gint key_list_read_int(GList *list, const gchar *key, gint *value);
const gchar *key_list_read_chars(GList *list, const gchar *key, gchar **text);
gchar *key_list_read_path(GList *list, const gchar *key, const gchar *base);


SkinData *skin_parse(const gchar *skin_dir, const gchar *datafile, gint edit);


#endif


