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

#ifndef UI2_LIST_H
#define UI2_LIST_H


#include "ui2_text.h"


/*
 * This widget is only a view, the data is requested only when display is necessary
 */

typedef enum
{
	UI_LIST_COLUMN_SIZE_FIXED        = 1 << 0,
	UI_LIST_COLUMN_SIZE_PROPORTIONAL = 1 << 1,
	UI_LIST_COLUMN_JUSTIFY_RIGHT     = 1 << 2,
} ListColumnFlags;

typedef struct _ListRowData ListRowData;
struct _ListRowData
{
	gchar **text;		/* Null terminated string array (cell text) */
	gint cells;		/* count of strings (excluding NULL, above) */
	gint selected;
	gint sensitive;
	gint flag_mask;		/* mask of flags for additional info */
};

typedef struct _ListData ListData;
struct _ListData
{
	GdkPixbuf *overlay;
	GdkPixbuf *pixbuf;
	GdkPixbuf *clip_mask;
	gint border_left;
	gint border_right;
	gint border_top;
	gint border_bottom;
	gint border_stretch;
	gint stretch;

	GdkPixbuf *row_overlay;
	GdkPixbuf *row_overlay_center;
	gint row_height;
	gint row_width;
	gint row_border_left;
	gint row_border_right;
	gint row_stretch;
	gint row_has_press;
	gint row_has_prelight;

	GdkPixbuf *divider_overlay;
	gint divider_width;
	gint divider_height;

	FontData *font;
	guint8 text_r;
	guint8 text_g;
	guint8 text_b;
	guint8 text_a;

	GdkPixbuf *flag_overlay;
	gint flag_width;
	gint flag_height;
	gint flag_sections;
	gint flag_column;

	gint width;
	gint height;
	gint x;
	gint y;

	gint sizeable;

	SkinData *skin;

	gint column_count;
	gint *column_widths;		/* array of column widths */
	gint *column_real_widths;	/* array of actual column widths */
	ListColumnFlags *column_flags;	/* column attributes (justify, etc. */
	gchar **column_keys;		/* column text keys */

	gint columns_right_justify;	/* column calculation gor right to left */

	gint row_count;
	gint row_start;		/* first visible row */
	gint row_end;		/* last visible row */
	GList *row_list;	/* this is only a list of _visible_ rows */

	gint row_press;
	gint row_prelight;

	gint region_width;
	gint region_height;

	gint text_voff;		/* vertical offsets for centering */
	gint divider_voff;
	gint flag_voff;

	UIData *ui;
	WidgetData *wd;

	gint timeout_id;	/* timeout_id -- used for scroll delays */
	guint32 click_time;	/* used to check for a double click ? */
	gint scroll_count;	/* for delayed scroll speedup after 8 rows */

	/* delayed scrolling (from slider) */
	gint scroll_idle_id;
	gfloat scroll_idle_value;

	/* mouse stuff */
	gint pressed;
	gint press_x;
	gint press_y;
	gint press_row;
	gint in_drag;

	/* focus */
	gint focus_row;

	gint menu_style_select;

	gint force_sync;	/* bit of a hack to force updates from the editor */
};


/* ceate a new list, please call this and all list_image_* once */
ListData *list_new(GdkPixbuf *pb, gint x, gint y, gint w, gint h, gint sizeable, gint columns,
                   gint border_top, gint border_right, gint border_bottom, gint border_left, gint stretch);
void list_image_row(ListData *ld, GdkPixbuf *pb, gint has_press, gint has_prelight,
                    gint border_left, gint border_right, gint stretch, GdkPixbuf *divider);
void list_image_row_flag(ListData *ld, GdkPixbuf *pb, gint sections, gint column);
void list_set_font(ListData *ld, const gchar *description, GdkPixbuf *pb, gint extended,
		   guint8 r, guint8 g, guint8 b, guint8 a);

void list_set_column_attributes(ListData *ld, gint column, gint length, ListColumnFlags flags, const gchar *key);
void list_set_column_justify(ListData *ld, gint justify_right);

void list_free(ListData *ld);


void list_row_column_set_text(ListData *ld, ListRowData *rd, const gchar *column_key, const gchar *text);
void list_row_set_flag(ListRowData *rd, gint flag);
void list_row_set_sensitive(ListRowData *rd, gint sensitive);

WidgetData *list_register(SkinData *skin, ListData *ld, const gchar *key, const gchar *text_id);
RegisterData *list_register_key(const gchar *key, UIData *ui,
				gint (*length_request_func)(ListData *list, const gchar *key, gpointer data), gpointer length_request_data,
				gint (*row_info_func)(ListData *list, const gchar *key, gint row, ListRowData *rd, gpointer data), gpointer row_info_data,
				void (*row_click_func)(ListData *list, const gchar *key, gint row, ListRowData *rd, gint button, gpointer data), gpointer row_click_data,
				void (*row_select_func)(ListData *list, const gchar *key, gint row, ListRowData *rd, gpointer data), gpointer row_select_data,
				gint (*row_move_func)(ListData *list, const gchar *key, gint source_row, gint dest_row, gpointer data), gpointer row_move_data);

gint list_row_insert(const gchar *key, UIData *ui, gint row);
gint list_row_remove(const gchar *key, UIData *ui, gint row);
gint list_row_update(const gchar *key, UIData *ui, gint row);
gint list_refresh(const gchar *key, UIData *ui);

void list_set_menu_style(ListData *ld, gint menu_style);

gint list_scroll_row(const gchar *key, UIData *ui, gint rows);
gint list_scroll_to(const gchar *key, UIData *ui, gint row);

WidgetType list_type_id(void);
void list_type_init(void);

/* for use by menus */
void list_register_menu_funcs(const gchar *key, UIData *ui,
			      gint (*func)(ListData *list, const gchar *key, gint row, gint activated, gint up, gpointer data),
			      gpointer data);

/* private, for use by editor */
void list_row_text_set(ListRowData *rd, gint column, const gchar *text);


#endif

