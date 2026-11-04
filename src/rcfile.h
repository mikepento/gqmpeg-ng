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


#ifndef RCFILE_H
#define RCFILE_H


gchar *quoted_value(const gchar *text);
void write_char_option(FILE *f, const gchar *label, const gchar *text);
gchar *read_char_option(FILE *f, const gchar *option, const gchar *label, const gchar *value, gchar *text);
void write_int_option(FILE *f, const gchar *label, gint n);
gint read_int_option(FILE *f, const gchar *option, const gchar *label, const gchar *value, gint n);
void write_bool_option(FILE *f, const gchar *label, gint n);
gint read_bool_option(FILE *f, const gchar *option, const gchar *label, const gchar *value, gint n);

void save_options(void);
void load_options(void);


#endif

