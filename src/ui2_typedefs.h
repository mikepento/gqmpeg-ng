/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#ifndef UI2_TYPEDEFS_H
#define UI2_TYPEDEFS_H


/*
 *-------------------------------------
 * SLIK defines
 *-------------------------------------
 */

#define SLIK_VERSION "0.91.1"

#define SKIN_SIZE_MIN 4
#define SKIN_SIZE_MAX 2000

/*
 *-------------------------------------
 * SLIK data structures
 *-------------------------------------
 */

typedef gint WidgetType;

typedef struct _WidgetObjectData WidgetObjectData;
typedef struct _WidgetData WidgetData;

typedef struct _UIData UIData;
typedef struct _RegisterData RegisterData;
typedef struct _SkinData SkinData;

typedef struct _EditData EditData;


struct _WidgetObjectData
{
	WidgetType type;
	gchar *description;

	gint is_visible;

	gint priority;

	/* general ui */
	void (*func_draw)(gpointer widget, const gchar *key, gint update, gint force, GdkPixbuf *pb, UIData *ui);
	void (*func_reset)(gpointer widget, const gchar *key, GdkPixbuf *pb, UIData *ui);
	gint (*func_press)(gpointer widget, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui);
	void (*func_release)(gpointer widget, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui);
	void (*func_motion)(gpointer widget, const gchar *key, gint x, gint y, GdkPixbuf *pb, UIData *ui);
	void (*func_back)(gpointer widget, GdkPixbuf *pb);
	void (*func_free)(gpointer widget);

	gint (*func_get_geometry)(gpointer widget, gint *x, gint *y, gint *w, gint *h);
	void (*func_set_coord)(gpointer widget, gint x, gint y);
	void (*func_set_size)(gpointer widget, gint dev_w, gint dev_h);

	/* focus stuff */
	gint (*func_focus_draw)(gpointer widget, const gchar *key,
				gint x, gint y, gint w, gint h, GdkPixbuf *pb, UIData *ui);
	gint (*func_focus_key_event)(gpointer widget, const gchar *key, GdkEventKey *event, GdkPixbuf *pb, UIData *ui);

	/* loading */
	WidgetData *(*func_parse)(SkinData *skin, GList *list, const gchar *skin_dir, const gchar *key, gint edit);

	/* internal widget init stuff */
	void (*func_init)(gpointer widget, const gchar *key, UIData *ui);

	/* editor */
	GdkPixbuf *(*func_get_pixbuf)(gpointer widget);

	void (*func_edit_write)(FILE *f, WidgetData *wd, SkinData *skin, const gchar *dir);
	gpointer (*func_edit_read)(UIData *ui, WidgetData *wd, GList *list);
	void (*func_edit_free)(gpointer data);

	gpointer (*func_edit_props)(UIData *ui, WidgetData *wd, GtkWidget *vbox, gpointer detail);
	GtkWidget *(*func_edit_page_new)(EditData *ed);
	void (*func_edit_page_add)(GtkWidget *widget, gpointer data);

	gint (*func_edit_removable)(gpointer widget);
};


struct _WidgetData
{
	WidgetType type;
	gchar *key;
	gchar *text_id;
	gpointer widget;
	GList *data_list;

	const WidgetObjectData *od;

	gint hidden;
	gint in_bounds;

	gint anchor_right;
	gint anchor_bottom;
};


struct _RegisterData
{
	WidgetType type;
	gchar *key;
	gpointer callbacks;	/* struct containing update functions */
	guint callbacks_l;	/* byte size of callback data (used for copying) */
	gint private;		/* signal is internal to skin widgets ? (cleared on every skin change) */
	gpointer private_widget;	/* pointer to widget that holds this key (only used on unreg) */
};


struct _SkinData
{
	GdkPixbuf *overlay;	/* background */
	GdkPixbuf *pixbuf;	/* pixbuf */
	GdkPixmap *buffer;	/* offscreen buffer */
	GdkPixbuf *mask;	/* shape mask */
	GdkBitmap *mask_buffer;	/* bitmap version of mask */
	gint width;
	gint height;
	gint transparent;

	GdkPixbuf *real_overlay;	/* used to hold original image for borders / stretch / resize */

	gint stretch;		/* stretch image to a size? (width, height) */

	/* borders */
	gint has_border;
	gint border_left;
	gint border_right;
	gint border_top;
	gint border_bottom;
	gint border_left_stretch;
	gint border_right_stretch;
	gint border_top_stretch;
	gint border_bottom_stretch;

	/* resizing */
	gint sizeable;
	gint width_def;
	gint width_min;
	gint width_max;
	gint width_inc;
	gint height_def;
	gint height_min;
	gint height_max;
	gint height_inc;

	/* focus */
	GdkPixbuf *focus_overlay;
	gint focus_stretch;
	gint focus_has_border;
	gint focus_border_left;
	gint focus_border_right;
	gint focus_border_top;
	gint focus_border_bottom;
	gint focus_anchor_right;
	gint focus_anchor_bottom;

	gint focus_box_filled;
	guint8 focus_box_alpha;
	guint8 focus_box_r;
	guint8 focus_box_g;
	guint8 focus_box_b;

	GList *widget_list;	/* widget list of WidgetData structs */

	UIData *ui;		/* ui attached to */

	gchar *background_filename;		/* used by editor */
	gchar *background_mask_filename;	/* " */
	gchar *focus_filename;

	/* used by widgets that need extra offscreen scratch areas to work in */
	GdkPixbuf *scratch_pixbuf;
	GdkPixmap *scratch_pixmap;

	/* delayed idle resize */
	gint resize_idle_id;
	gint resize_idle_width;
	gint resize_idle_height;
};


struct _UIData
{
	GtkWidget *window;
	GtkWidget *display;

	gchar *key;		/* key to reference ui */
	gchar *class;		/* window class */

	GList *register_list;	/* list of registered functions */
	SkinData *skin;
	gchar *skin_path;	/* location of skin, NULL for default? */
	gchar *skin_mode_key;	/* filename of skindata equivelent, NULL for default */

	gint decorations;	/* show decorations */
	gint allow_move;	/* move by dragging mouse */
	gint focus_enable;	/* force keyboard focus on, usually only for menus */

	gint sticky;		/* sticky */

	/* called for click on a button greater than 2 (3, 4, 5, etc.) */
	void (*click_func)(UIData *ui, gint button, guint32 time, gpointer data);
	gpointer click_data;

	/* default skin handler (used as last resort backup) */
	SkinData *(*skin_func)(UIData *ui, const gchar *key, gpointer data);
	gpointer skin_data;

	/* called to request a background update, return TRUE if you set it */
	gint (*back_func)(UIData *ui, GdkPixbuf *pixbuf, gpointer data);
	gpointer back_data;

	/* called when a new child window has opened */
	void (*new_window_func)(UIData *ui, const gchar *key, gpointer data);
	gpointer new_window_data;

	WidgetData *active_widget;	/* widget with the mouse grab */
	gint in_press;
	gint in_move;

	gint press_x;
	gint press_y;
	gint press_root_x;
	gint press_root_y;

	WidgetData *focus_widget;

	gint frozen;

	gint root_win_idle;

	/* groups */
	UIData *parent;
	GList *children;

	UIData *edit;

	gint destroyed;
};


struct _EditData
{
	GtkWidget *window;
	GtkWidget *table;
	UIData *ui;
	UIData *parent;	/* UI being edited */

	/* the rest is private (and should be made that way) */

	WidgetData *active_widget;
	gint in_press;
	gint in_move;

	gint press_widget_x;
	gint press_widget_y;
	gint press_x;
	gint press_y;

	WidgetData *info_widget;

	GtkWidget *widget_list;
	GtkWidget *spin_x;
	GtkWidget *spin_y;
	GtkWidget *description;
	GtkWidget *key_combo_entry;
	GtkWidget *data_entry;
	GtkWidget *text_id_entry;
	GtkWidget *image_entry;
	GtkWidget *widget_image;
	GtkWidget *clip_mask_entry;

	GtkWidget *anchor_right_button;
	GtkWidget *anchor_bottom_button;

	GtkWidget *detail_vbox;
	GtkWidget *details;
	gpointer detail_data;
	WidgetType detail_type;

	gint click_to_focus;

	GtkWidget *save_path_entry;
	GtkWidget *save_key_entry;

	/* widget addition dialog */
	GtkWidget *widget_window;
	GtkWidget *notebook;

	GtkWidget *import_path_entry;

	GList *widget_id_list;		/* list of 'widget type' data pointers */

	GList *main_list;		/* list of 'add' backgrounds */
	GList *focus_list;
	gpointer main_page;		/* pointer to background page 'add' data */

	/* help dialog */
	GtkWidget *help_window;
	gint help_follow;

	gchar *help_path;

	/* comments */
	GtkWidget *comment_window;
	GtkWidget *comment_title_entry;
	GtkWidget *comment_version_entry;
	GtkWidget *comment_author_entry;
	GtkWidget *comment_email_entry;
	GtkWidget *comment_url_entry;
	GtkWidget *comment_license_entry;
	GtkWidget *comment_misc_entry;
	GtkWidget *comment_build_spin;
	GtkWidget *comment_stamp_label;

	gchar *comment_title;
	gchar *comment_version;
	gchar *comment_author;
	gchar *comment_email;
	gchar *comment_url;
	gchar *comment_license;
	gchar *comment_misc;
	gint comment_build;
	gchar *comment_stamp;
};


/*
 *-------------------------------------
 * SLIK globals
 *-------------------------------------
 */

extern gint slik_smart_placement;
extern gint slik_remember_position;

extern gint slik_focus_enable;

extern gint slik_double_size;

extern gint slik_scale;			/* set this before calling skin_parse to auto-scale a skin */
extern gfloat slik_scale_value;		/* be carefull with value, some skins may crash when scaled */
extern gint slik_allow_shapes;		/* set FALSE to disallow setting of shaped windows */

extern gint slik_colorshift_r;
extern gint slik_colorshift_g;
extern gint slik_colorshift_b;
extern gint slik_colorshift_a;
extern gint slik_colorshift_on;

extern gint slik_transparency_force;
extern gint slik_transparency_force_a;

extern gint debug_mode;
extern gint debug_skin;


#endif




