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

#include "ui2_includes.h"
#include "ui2_typedefs.h"
#include "ui2_parse.h"

#include <ctype.h>

/* GQmpeg compatibility parsers includes */

#include "skin_compat.h"
#include "skin_wamp.h"

/* GQmpeg compatibility end */

#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_fileops.h"


static gint vector_length(gchar **vector)
{
	gint length = 0;

	if (!vector) return 0;

	while (vector[length] != NULL) length++;
	return length;
}

static gint break_section(const gchar *section, gchar **type, gchar **item)
{
	const gchar *p;
	gint l;

	if (!section) return FALSE;

	p = section;
	l = 0;

	while(*p != '_' && *p != '\0')
		{
		p++;
		l++;
		}

	if (*p == '_')
		{
		*type = g_strndup(section, l);
		p++;
		if (*p != '\0')
			{
			*item = g_strdup(p);
			}
		else
			{
			*item = NULL;
			}
		}
	else
		{
		*type = g_strdup(section);
		*item = NULL;
		}

	return TRUE;
}

static gchar **list_find_vector(GList *list, const gchar *key)
{
	GList *work;

	work = list;
	while(work)
		{
		gchar **vector = work->data;
		work = work->next;

		if (strcasecmp(vector[0], key) == 0)
			{
			return vector;
			}
		}
	return NULL;
}

gint key_list_read_bool(GList *list, const gchar *key)
{
	gchar **vector;
	vector = list_find_vector(list, key);

	if (vector &&
	    (strcasecmp(vector[1], "true") == 0 || strcasecmp(vector[1], "t") == 0 || *vector[1] == '1') )
		{
		return TRUE;
		}

	return FALSE;
}

gint key_list_read_int(GList *list, const gchar *key, gint *value)
{
	gchar **vector;
	vector = list_find_vector(list, key);

	if (vector)
		{
		*value = strtol(vector[1], NULL, 10);
		return TRUE;
		}

	return FALSE;
}

static gint key_list_read_uint8(GList *list, const gchar *key, guint8 *value)
{
	gint new_val;

	if (key_list_read_int(list, key, &new_val))
		{
		*value = (guint8)new_val;
		return TRUE;
		}
	return FALSE;
}

/* return pointer into vector if found, NULL otherwise */
const gchar *key_list_read_chars(GList *list, const gchar *key, gchar **text)
{
	gchar **vector;
	vector = list_find_vector(list, key);

	if (vector)
		{
		if (text) *text = g_strdup(vector[1]);
		return vector[1];
		}

	return NULL;
}

gchar *key_list_read_path(GList *list, const gchar *key, const gchar *base)
{
	const gchar *name;

	name = key_list_read_chars(list, key, NULL);
	if (!name) return NULL;

	return g_strconcat(base, "/", name, NULL);
}

static gint skin_parse_main(SkinData *s, GList *list, const gchar *skin_dir, gint edit)
{
	gchar *filename;
	gchar *mask;
	
	if (s->real_overlay)
		{
		printf("warning: duplicate section [main] encountered, ignored.\n");
		return FALSE;
		}

	filename = key_list_read_path(list, "image", skin_dir);
	if (!filename) return FALSE;
	mask = key_list_read_path(list, "mask", skin_dir);
	s->transparent = key_list_read_bool(list, "transparent");

	s->real_overlay = util_size_pixbuf(util_pixbuf_new_from_file(filename), TRUE);

	if (s->real_overlay)
		{
		s->width = gdk_pixbuf_get_width(s->real_overlay);
		s->height = gdk_pixbuf_get_height(s->real_overlay);
		s->width_def = s->width;
		s->height_def = s->height;
		if (edit) s->background_filename = g_strdup(filename);
		}
	else
		{
		printf("failed to load file \"%s\"\n", filename);
		}

	if (mask)
		{
		s->mask = util_size_pixbuf(util_pixbuf_new_from_file(mask), FALSE);
		if (s->mask && edit) s->background_mask_filename = g_strdup(mask);
		g_free(mask);
		}

	s->has_border = key_list_read_bool(list, "border");
	if (s->has_border)
		{
		if (!key_list_read_int(list, "border_left", &s->border_left)) s->border_left = 1;
		if (!key_list_read_int(list, "border_right", &s->border_right)) s->border_right = 1;
		if (!key_list_read_int(list, "border_top", &s->border_top)) s->border_top = 1;
		if (!key_list_read_int(list, "border_bottom", &s->border_bottom)) s->border_bottom = 1;
		util_size(&s->border_left);
		util_size(&s->border_right);
		util_size(&s->border_top);
		util_size(&s->border_bottom);

		s->border_left_stretch = key_list_read_bool(list, "border_left_stretch");
		s->border_right_stretch = key_list_read_bool(list, "border_right_stretch");
		s->border_top_stretch = key_list_read_bool(list, "border_top_stretch");
		s->border_bottom_stretch = key_list_read_bool(list, "border_bottom_stretch");
		}

	s->sizeable = key_list_read_bool(list, "sizeable");
	s->stretch = key_list_read_bool(list, "stretch");

	if (!key_list_read_int(list, "width", &s->width_def) ||
	    !key_list_read_int(list, "height", &s->height_def))
		{
		s->width_def = s->width;
		s->height_def = s->height;
		}
	else
		{
		util_size(&s->width_def);
		util_size(&s->height_def);
		}

	if (s->sizeable)
		{
		if (key_list_read_int(list, "width_min", &s->width_min))
			util_size(&s->width_min);
		else
			s->width_min = s->width_def;

		if (key_list_read_int(list, "width_max", &s->width_max))
			util_size(&s->width_max);
		else
			s->width_max = s->width_def;

		if (key_list_read_int(list, "height_min", &s->height_min))
			util_size(&s->height_min);
		else
			s->height_min = s->height_def;

		if (key_list_read_int(list, "height_max", &s->height_max))
			util_size(&s->height_max);
		else
			s->height_max = s->height_def;

		if (!key_list_read_int(list, "width_increment", &s->width_inc)) s->width_inc = 1;
		if (!key_list_read_int(list, "height_increment", &s->height_inc)) s->height_inc = 1;
		}
	else
		{
		s->width_min = s->width_max = s->width_def;
		s->height_min = s->height_max = s->height_def;
		s->width_inc = s->height_inc = 1;
		}

	util_size(&s->width_inc);
	util_size(&s->height_inc);

	s->width = s->width_def;
	s->height = s->height_def;

	g_free(filename);

	/* now the focus stuff */

	filename = key_list_read_path(list, "focus", skin_dir);
	if (filename && !s->focus_overlay)
		{
		s->focus_overlay = util_size_pixbuf(util_pixbuf_new_from_file(filename), TRUE);

		if (!s->focus_overlay) printf("failed to load file \"%s\"\n", filename);
		if (edit) s->focus_filename = g_strdup(filename);
		}
	g_free(filename);

	s->focus_stretch = key_list_read_bool(list, "focus_stretch");
	s->focus_has_border = key_list_read_bool(list, "focus_has_border");
	if (s->focus_has_border)
		{
		if (key_list_read_int(list, "focus_border_left", &s->focus_border_left))
			util_size(&s->focus_border_left);
		if (key_list_read_int(list, "focus_border_right", &s->focus_border_right))
			util_size(&s->focus_border_right);
		if (key_list_read_int(list, "focus_border_top", &s->focus_border_top))
			util_size(&s->focus_border_top);
		if (key_list_read_int(list, "focus_border_bottom", &s->focus_border_bottom))
			util_size(&s->focus_border_bottom);
		}

	s->focus_anchor_right = key_list_read_bool(list, "focus_anchor_right");
	s->focus_anchor_bottom = key_list_read_bool(list, "focus_anchor_bottom");

	s->focus_box_filled = key_list_read_bool(list, "focus_filled");
	key_list_read_uint8(list, "focus_red", &s->focus_box_r);
	key_list_read_uint8(list, "focus_green", &s->focus_box_g);
	key_list_read_uint8(list, "focus_blue", &s->focus_box_b);
	key_list_read_uint8(list, "focus_alpha", &s->focus_box_alpha);

	return (s->real_overlay != NULL);
}

static void skin_parse_standard_data(WidgetData *wd, SkinData *skin, GList *list, gint edit)
{
	ui_widget_set_data(wd, "data", key_list_read_chars(list, "data", NULL));
	g_free(wd->text_id);
	wd->text_id = g_strdup(key_list_read_chars(list, "id", NULL));

	wd->anchor_right = key_list_read_bool(list, "anchor_right");
	wd->anchor_bottom = key_list_read_bool(list, "anchor_bottom");
}

static gint skin_parse_section(SkinData *s, const gchar *section, GList *list, const gchar *skin_dir, gint edit)
{
	gchar *type;
	gchar *key;
	gint success;

	if (!s || !section || !list || !skin_dir) return FALSE;

	if (!break_section(section, &type, &key))
		{
		printf("Unable to determine subsection \"%s\" in: %s\n", section, skin_dir);
		return FALSE;
		}

	if (strcmp(type, "main") == 0)
		{
		success = skin_parse_main(s, list, skin_dir, edit);
		}
	else
		{
		WidgetObjectData *od;

		od = ui_widget_object_by_text(type);
		if (od && od->func_parse)
			{
			WidgetData *wd;
			wd = od->func_parse(s, list, skin_dir, key, edit);

			success = (wd != NULL);
			if (wd) skin_parse_standard_data(wd, s, list, edit);
			}
		else
			{
			printf("unknown section type \"%s\" for: %s\n", section, skin_dir);
			success = FALSE;
			}
		}

	if (!success)
		{
		printf("failed to initialize widget from section \"%s\" for: %s\n", section, skin_dir);
		}

	g_free(type);
	g_free(key);

	return success;
}

SkinData *skin_parse(const gchar *skin_dir, const gchar *datafile, gint edit)
{
	SkinData *s;
	FILE *f;
	char buf[1024];
	GList *list;
	gchar *section;
	gint fail = 0;
	gchar *pathl;

	pathl = path_from_utf8(datafile);
	f = fopen(pathl, "r");
	g_free(pathl);
	if (!f)
		{

/* GQmpeg change winamp compatibility*/

		/* try wamp */
		if (!datafile ||
		    (filename_from_path((char *)datafile) && strcmp(filename_from_path((char *)datafile), "skindata") == 0) )
			{
			s = skin_load_wamp_normal(skin_dir);
			}
		else if (datafile &&
			 filename_from_path((char *)datafile) && strcmp(filename_from_path((char *)datafile), "skindata_alt") == 0)
			{
			s = skin_load_wamp_small(skin_dir);
			}
		else
			{
			s = NULL;
			}

		if (s) return s;

/* GQmpeg change end */

		if (debug_mode) printf("Unable to open skin data file: %s\n", datafile);
		return NULL;
		}

	s = skin_new();

	section = NULL;
	list = NULL;
	while (fgets(buf, sizeof(buf), f))
		{
		gchar *p;

		if (buf[0]=='#') continue;
		if (buf[0]=='\n') continue;

		/* remove new line chars */
		p = buf;
		while (*p != '\0')
			{
			if (*p == '\n') *p = '\0';
			p++;
			}

/* GQmpeg change for V1 compatibility */

		if (!section && strncmp(buf, "Background:", 11) == 0)
			{
			if (debug_mode) printf("Skin \"%s\" is a version 1 skin\n", skin_dir);
			skin_free(s);
			fclose(f);
			return skin_parse_v1(skin_dir, datafile, edit);
			}

/* GQmpeg change end */

		if (buf[0] == '[')
			{
			/* new section */
			if (section && list)
				{
				/* parse section */
				skin_parse_section(s, section, list, skin_dir, edit);
				}
			g_free(section);
			section = g_strdup(buf + 1);
			p = section;
			while (*p != '\0')
				{
				if (*p == ']')
					{
					*p = '\0';
					}	
				else
					{
					*p = tolower(*p);
					}
				p++;
				}
			g_list_foreach(list, (GFunc)g_strfreev, NULL);
			g_list_free(list);
			list = NULL;
			}
		else if (section)
			{
			gchar **vector;

			vector = g_strsplit(buf, " = ", 64);
			if (vector)
				{
				if (vector_length(vector) >= 2)
					{
					list = g_list_prepend(list, vector);
					}
				else
					{
					g_strfreev(vector);
					}
				}
			}
		else
			{
			if (debug_mode) printf("unknown data in %s file: \"%s\"\n", datafile, buf);
			fail++;
			}
		if (fail > 8)
			{
			printf("too many junk lines (>9) in: \"%s\", giving up\n", datafile);
			break;
			}
		}

	fclose(f);

	/* parse remaining data, then free it */
	if (section && list)
		{
		/* parse section */
		skin_parse_section(s, section, list, skin_dir, edit);
		}
	g_free(section);
	g_list_foreach(list, (GFunc)g_strfreev, NULL);
	g_list_free(list);

	if (!s->overlay && !s->real_overlay)
		{
		printf(_("No background image from: %s\n"), skin_dir);
		skin_free(s);
		return NULL;
		}

	skin_finalize(s);

	return s;
}

