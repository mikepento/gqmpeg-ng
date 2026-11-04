/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2003 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#include "ui2_includes.h"
#include "ui2_typedefs.h"
#include "ui2_main.h"

#include "ui2_button.h"
#include "ui2_decal.h"
#include "ui2_display.h"
#include "ui2_parse.h"
#include "ui2_skin.h"
#include "ui2_util.h"
#include "ui2_widget.h"
#include "ui_fileops.h"

#include "ui2_logo.h"

/* these two includes are _only_ used in ui_iconify() */
#include <gdk/gdkx.h>
#include <X11/Xlib.h>


/*
 *-------------
 * yes, these are globals
 *-------------
 */

gint slik_smart_placement = TRUE;
gint slik_remember_position = FALSE;

gint slik_focus_enable = TRUE;

gint slik_double_size = FALSE;

gint slik_scale = FALSE;
gfloat slik_scale_value = 1.0;
gint slik_allow_shapes = TRUE;

gint slik_colorshift_r = 128;
gint slik_colorshift_g = 128;
gint slik_colorshift_b = 128;
gint slik_colorshift_a = 128;
gint slik_colorshift_on = FALSE;

gint slik_transparency_force = FALSE;
gint slik_transparency_force_a = 192;

gint debug_mode = FALSE;
gint debug_skin = FALSE;

/* list of all windows created with ui_new() */
static GList *slik_ui_list = NULL;


/*
 *-------------
 * misc
 *-------------
 */

void window_set_icon(GtkWidget *window, const char **icon, const gchar *file, GdkPixbuf *pixbuf)
{
	GdkPixbuf *pb;

	if (icon)
		{
		pb = gdk_pixbuf_new_from_xpm_data(icon);
		}
	else if (file)
		{
		pb = util_pixbuf_new_from_file(file);
		}
	else
		{
		pb = pixbuf;
		if (pb)
			{
			gdk_pixbuf_ref(pb);
			}
		else
			{
			pb = ui_slik_logo();
			}
		}

	if (!pb) return;
	gtk_window_set_icon(GTK_WINDOW(window), pb);
	gdk_pixbuf_unref(pb);
}

void ui_set_icon(UIData *ui, const char **icon, const gchar *file, GdkPixbuf *pixbuf)
{
	if (!ui || !ui->window) return;

	window_set_icon(ui->window, icon, file, pixbuf);
}

void ui_iconify(UIData *ui)
{
        Window xwindow;

        if (!ui || !ui->window->window) return;

        xwindow = GDK_WINDOW_XWINDOW(ui->window->window);
        XIconifyWindow (GDK_DISPLAY (), xwindow, DefaultScreen (GDK_DISPLAY ()));
}

static gint ui_mode_unique(UIData *ui, const gchar *mode_key)
{
	GList *work;

	if (!mode_key) return FALSE;

	if (ui->parent) ui = ui->parent;

	if (strcmp(mode_key, "skindata") == 0) return FALSE;
	if (ui->skin_mode_key && strcmp(ui->skin_mode_key, mode_key) == 0) return FALSE;
	if (ui->key && strcmp(ui->key, mode_key) == 0) return FALSE;

	work = ui->children;
	while (work)
		{
		UIData *child;

		child = work->data;
		work = work->next;

		if (child->key && strcmp(child->key, mode_key) == 0) return FALSE;
		}

	return TRUE;
}

/*
 *-------------
 * these are the reserved widgets for window resizing and skin mode toggles.
 *-------------
 */

static void ui_skin_toggle_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	UIData *ui = data;
	const gchar *mode_key;

	/* luckily, the editor disables this button, no need to handle ui->edit */

	mode_key = ui_widget_get_data_by_widget(ui, button);

	if (debug_mode) printf("Setting skin mode key = \"%s\"\n", mode_key);

	ui_skin_mode_set(ui, mode_key);
}

static gint ui_skin_expand_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	UIData *ui = data;
	SkinData *skin;

	skin = ui->skin;

	return (skin->width != skin->width_def || skin->height != skin->height_def);
}

static void ui_skin_expand_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	UIData *ui = data;
	const gchar *text;
	gint w, h;

	text = ui_widget_get_data_by_widget(ui, button);

	/* we must support the editor */
	if (!text && ui->edit && ui_widget_get_by_widget(ui, button) == NULL)
		{
		text = ui_widget_get_data_by_widget(ui->edit, button);
		ui = ui->edit;
		}

	if (debug_mode) printf("Skin expand data = \"%s\"\n", text);

	if (text && sscanf(text, "%i %i", &w, &h) == 2)
		{
		SkinData *skin = ui->skin;
		gint state;

		if (skin->width == skin->width_def && skin->height == skin->height_def)
			{
			util_size(&w);
			util_size(&h);

			/* do the expand (can be negative too) */
			skin_resize(ui, skin->width + w, skin->height + h);
			state = TRUE;
			}
		else
			{
			/* assume expanded, snap back to default */
			skin_resize(ui, skin->width_def, skin->height_def);
			state = FALSE;
			}
		button_state_set("skin_expand", ui, state);
		}
}

UIData *ui_spawn_child(UIData *parent, const gchar *key, const gchar *title, const gchar *mode_key)
{
	UIData *child;
	SkinData *skin;

	if (!key) return NULL;

	if (parent->parent) parent = parent->parent;

	if (debug_mode) printf("ui2_main.c: opening skin \"%s\"for addition as \"%s\"\n", (mode_key) ? mode_key : "default", key);

	if (parent->skin_path)
		{
		gchar *datafile;

		datafile = g_strconcat(parent->skin_path, "/", (mode_key) ? mode_key : "skindata", NULL);
		skin = skin_parse(parent->skin_path, datafile, FALSE);
		g_free(datafile);
		}
	else
		{
		if (parent->skin_func)
			{
			skin = parent->skin_func(parent, mode_key, parent->skin_data);
			}
		else
			{
			skin = NULL;
			}
		}

	if (!skin)
		{
		printf("ui2_main.c: failed to open skin \"%s\"for spawned window\n", mode_key);
		return NULL;
		}

	child = ui_new(parent->class, key, parent->decorations, parent->class);
	ui_title_set(child, title);
	child->allow_move = parent->allow_move;

	/* copy a few things from parent, it is up to the application to determine if
	 * the UI in the callback(s) is the parent.
	 */
	child->click_func = parent->click_func;
	child->click_data = parent->click_data;

	child->skin_func = parent->skin_func;
	child->skin_data = parent->skin_data;

	child->back_func = parent->back_func;
	child->back_data = parent->back_data;

	child->new_window_func = parent->new_window_func;
	child->new_window_data = parent->new_window_data;

	ui_group_set_child(parent, child);

	ui_skin_set(child, skin, parent->skin_path, mode_key);

	if (parent->new_window_func)
		{
		parent->new_window_func(child, child->key, parent->new_window_data);
		}

	return child;
}

static void ui_skin_open_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	UIData *ui = data;
	UIData *child;
	const gchar *mode_key;

	mode_key = ui_widget_get_data_by_widget(ui, button);

	if (!mode_key || strlen(mode_key) == 0)
		{
		printf("ui2_main.c: skin_open requires valid mode key\n");
		return;
		}

	if (!ui_mode_unique(ui, mode_key)) return;

	if (debug_mode) printf("ui2_main.c: opening skin \"%s\"for addition\n", mode_key);

	child = ui_spawn_child(ui, mode_key, NULL, mode_key);
	if (child)
		{
		gtk_widget_show(child->window);
		}
}

static void ui_skin_close_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	UIData *ui = data;

	if (!ui->parent) return;

	if (debug_mode) printf("ui2_main.c: closing skin \"%s\"\n", ui->skin_mode_key);

	ui_close(ui);
}

static void ui_skin_iconify_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	UIData *ui = data;

	if (ui->allow_move) ui_iconify(ui);
}

static void ui_skin_sticky_update_cb(WidgetData *wd, gpointer data, UIData *ui)
{
	ui_widget_draw(ui, wd, TRUE, FALSE);
}

static void ui_skin_sticky_update(UIData *ui)
{
	ui_widget_for_each_key_one(ui, "skin_sticky", button_type_id(),
				   ui_skin_sticky_update_cb, NULL);
}

static gint ui_skin_sticky_status_cb(ButtonData *button, const gchar *key, gpointer data)
{
	UIData *ui = data;

	return ui->sticky;
}

static void ui_skin_sticky_click_cb(ButtonData *button, const gchar *key, gpointer data)
{
	UIData *ui = data;

	if (!ui->window) return;

	if (debug_mode) printf("setting sticky state to %d\n", !ui->sticky);

	if (ui->sticky)
		{
		gtk_window_unstick(GTK_WINDOW(ui->window));
		ui->sticky = FALSE;
		}
	else
		{
		gtk_window_stick(GTK_WINDOW(ui->window));
		ui->sticky = TRUE;
		}

	ui_skin_sticky_update(ui);
}

static gboolean ui_window_state_cb(GtkWidget *widget, GdkEventWindowState *event, gpointer data)
{
	UIData *ui = data;

	if (event->changed_mask & GDK_WINDOW_STATE_STICKY)
		{
		ui->sticky = (event->new_window_state & GDK_WINDOW_STATE_STICKY);
		ui_skin_sticky_update(ui);

		if (debug_mode) printf("sticky changed: %d\n", ui->sticky);
		}

	return FALSE;
}

static void ui_add_reserved(UIData *ui)
{
	/* ensure decals can be recognized */
        decal_type_init();

	if (!ui_registered_key_exists(ui, "skin_toggle", button_type_id()))
		{
		button_register_key("skin_toggle", ui,
				     NULL, NULL,
				     ui_skin_toggle_click_cb, ui,
				     NULL, NULL,
				     NULL, NULL);
		}
	if (!ui_registered_key_exists(ui, "skin_size", button_type_id()))
		{
		/* this is really a dummy register, but it makes the key show in the editor */
		button_register_key("skin_size", ui,
				     NULL, NULL,
				     NULL, NULL,
				     NULL, NULL,
				     NULL, NULL);
		}
	if (!ui_registered_key_exists(ui, "skin_expand", button_type_id()))
		{
		button_register_key("skin_expand", ui,
				     ui_skin_expand_status_cb, ui,
				     ui_skin_expand_click_cb, ui,
				     NULL, NULL,
				     NULL, NULL);
		}

	if (!ui_registered_key_exists(ui, "skin_open", button_type_id()))
		{
		button_register_key("skin_open", ui,
				     NULL, NULL,
				     ui_skin_open_click_cb, ui,
				     NULL, NULL,
				     NULL, NULL);
		}
	if (!ui_registered_key_exists(ui, "skin_close", button_type_id()))
		{
		button_register_key("skin_close", ui,
				     NULL, NULL,
				     ui_skin_close_click_cb, ui,
				     NULL, NULL,
				     NULL, NULL);
		}
	if (!ui_registered_key_exists(ui, "skin_iconify", button_type_id()))
		{
		button_register_key("skin_iconify", ui,
				     NULL, NULL,
				     ui_skin_iconify_click_cb, ui,
				     NULL, NULL,
				     NULL, NULL);
		}
	if (!ui_registered_key_exists(ui, "skin_sticky", button_type_id()))
		{
		button_register_key("skin_sticky", ui,
				     ui_skin_sticky_status_cb, ui,
				     ui_skin_sticky_click_cb, ui,
				     NULL, NULL,
				     NULL, NULL);
		}
}

/*
 *-------------
 * ui
 *-------------
 */

void ui_free(UIData *ui)
{
	if (!ui) return;

	if (!ui->destroyed)
		{
		printf("warning: free called for ui \"%s\" not marked as destroyed\n", ui->key);
		}

	/* close children */
	while (ui->children)
		{
		UIData *child = ui->children->data;
		ui_close(child);
		}

	/* if child, break from parent */
	ui_group_unset_child(ui);

	slik_ui_list = g_list_remove(slik_ui_list, ui);

	if (ui->root_win_idle != -1) gtk_idle_remove(ui->root_win_idle);
	ui_register_free_all(ui);
	skin_free(ui->skin);
	g_free(ui->skin_path);
	g_free(ui->skin_mode_key);
	g_free(ui->class);
	g_free(ui->key);
	g_free(ui);
}

static void ui_hide_cb(GtkWidget *widget, gpointer data)
{
	UIData *ui = data;
	gint x, y, w, h;

	if (ui_geometry_get(ui, &x, &y, &w, &h))
		{
		if (debug_mode) printf("hidden \"%s\"\n", ui->key);
		ui_state_set(filename_from_path(ui->skin_path), ui->skin_mode_key, x, y, w, h);
		}
}

static void ui_destroy_cb(GtkWidget *widget, gpointer data)
{
	UIData *ui = data;

	if (debug_mode) printf("UI destroyed \"%s\"\n", ui->key);

	ui->destroyed = TRUE;
	ui_free(ui);
}

static UIData *ui_real_new(const gchar *class, const gchar *key,
			   gint decorations, const gchar *title, GtkWindowType type)
{
	UIData *ui;

	ui = g_new0(UIData, 1);

	ui->decorations = decorations;
	ui->root_win_idle = -1;
	ui->edit = NULL;
	ui->skin = NULL;
	ui->class = g_strdup((class) ? class : "SLIK");
	ui->key = g_strdup(key);
	ui->allow_move = TRUE;
	ui->focus_enable = FALSE;

	ui->parent = NULL;
	ui->children = NULL;

	ui->window = gtk_window_new(type);
	gtk_window_set_resizable(GTK_WINDOW(ui->window), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(ui->window), 0);
	gtk_window_set_wmclass(GTK_WINDOW(ui->window), ui->key, ui->class);
	gtk_window_set_title(GTK_WINDOW(ui->window), title);

	g_signal_connect(G_OBJECT(ui->window), "hide",
			 G_CALLBACK(ui_hide_cb), ui);
	g_signal_connect(G_OBJECT(ui->window), "destroy",
			 G_CALLBACK(ui_destroy_cb), ui);
	g_signal_connect(G_OBJECT(ui->window), "window_state_event",
			 G_CALLBACK(ui_window_state_cb), ui);

	ui->display = gtk_drawing_area_new();
	ui_display_events_init(ui);
	gtk_container_add(GTK_CONTAINER(ui->window), ui->display);
	gtk_widget_show(ui->display);

	if (GTK_WIDGET_CAN_FOCUS(ui->display)) gtk_widget_grab_focus(ui->display);

	if (type != GTK_WINDOW_POPUP && !ui->decorations)
		{
		gtk_window_set_decorated(GTK_WINDOW(ui->window), 0);
		}

	/* add the default reserved keys */
	ui_add_reserved(ui);

	slik_ui_list = g_list_append(slik_ui_list, ui);

	return ui;
}

UIData *ui_new(const gchar *class, const gchar *subclass, gint decorations, const gchar *title)
{
	return ui_real_new(class, subclass, decorations, title, GTK_WINDOW_TOPLEVEL);
}

UIData *ui_new_dialog(const gchar *class, const gchar *subclass, gint decorations, const gchar *title)
{
	UIData *ui;

	ui = ui_real_new(class, subclass, decorations, title, GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(ui->window), GDK_WINDOW_TYPE_HINT_DIALOG);

	return ui;
}

UIData *ui_new_into_container(const gchar *class, const gchar *key, GtkWidget *widget)
{
	UIData *ui;

	ui = g_new0(UIData, 1);

	ui->decorations = FALSE;
	ui->root_win_idle = -1;
	ui->edit = NULL;
	ui->skin = NULL;
	ui->class = g_strdup((class) ? class : "SLIK");
	ui->key = g_strdup(key);
	ui->allow_move = FALSE;
	ui->focus_enable = FALSE;

	ui->parent = NULL;
	ui->children = NULL;

	ui->window = NULL;

	ui->display = gtk_drawing_area_new();
	g_signal_connect(G_OBJECT(ui->display), "destroy",
			 G_CALLBACK(ui_destroy_cb), ui);

	ui_display_events_init(ui);
	if (widget)
		{
		gtk_container_add(GTK_CONTAINER(widget), ui->display);
		gtk_widget_realize(ui->display);
		}

	gtk_widget_show(ui->display);

	/* add the default reserved keys */
	ui_add_reserved(ui);

	slik_ui_list = g_list_append(slik_ui_list, ui);

	return ui;
}

static gint ui_close_cb(gpointer data)
{
	UIData *ui = data;

	if (ui->window)
		{
		gtk_widget_destroy(ui->window);
		}
	else
		{
		gtk_widget_destroy(ui->display);
		}

	return FALSE;
}

void ui_close(UIData *ui)
{
	if (!ui || ui->destroyed) return;

	/* This may be called in the middle of a widget event (like a button press handler),
	 * and bad things may happen if the ui vanishes from under the handler, so hide the window
	 * then clean up in an idle function.
	 */

	ui->destroyed = TRUE;
	if (ui->window && GTK_WIDGET_VISIBLE(ui->window)) gtk_widget_hide(ui->window);

	/* close children.
	 * this may be duplicating effort in ui free,
	 * but has the benefit immediately hiding the windows.
	 */
	while (ui->children)
		{
		UIData *child = ui->children->data;
		ui_close(child);
		}

	/* if a child, break from parent */
	ui_group_unset_child(ui);

	gtk_idle_add(ui_close_cb, ui);
}

void ui_set_mouse_callback(UIData *ui, void (*func)(UIData *ui, gint button, guint32 time, gpointer data), gpointer data)
{
	ui->click_func = func;
	ui->click_data = data;
}

void ui_set_skin_callback(UIData *ui, SkinData *(*func)(UIData *ui, const gchar *key, gpointer data), gpointer data)
{
	ui->skin_func = func;
	ui->skin_data = data;
}

void ui_set_back_callback(UIData *ui, gint (*func)(UIData *ui, GdkPixbuf *pixbuf, gpointer data), gpointer data)
{
	ui->back_func = func;	
	ui->back_data = data;
}

void ui_set_new_window_callback(UIData *ui, void (*func)(UIData *ui, const gchar *key, gpointer data), gpointer data)
{
	ui->new_window_func = func;
	ui->new_window_data = data;
}


/*
 *-------------
 * app side keys
 *-------------
 */

RegisterData *ui_register_key(UIData *ui, const gchar *key, WidgetType type, gpointer callbacks, guint length)
{
	RegisterData *rd;

	rd = g_new(RegisterData, 1);
	rd->type = type;
	rd->key = g_strdup(key);
	rd->callbacks = callbacks;
	rd->callbacks_l = length;
	rd->private = FALSE;
	rd->private_widget = NULL;

	ui->register_list = g_list_prepend(ui->register_list, rd);

	return rd;
}

RegisterData *ui_register_key_private(UIData *ui, const gchar *key, WidgetType type,
				      gpointer callbacks, guint length, gpointer widget)
{
	RegisterData *rd;

	rd = ui_register_key(ui, key, type, callbacks, length);
	rd->private = TRUE;
	rd->private_widget = widget;
	return rd;
}

static void ui_register_free(RegisterData *rd)
{
	g_free(rd->callbacks);
	g_free(rd->key);
	g_free(rd);
}

void ui_register_free_all(UIData *ui)
{
	while(ui->register_list)
		{
		RegisterData *rd = ui->register_list->data;
		ui->register_list = g_list_remove(ui->register_list, rd);
		ui_register_free(rd);
		}
}

void ui_register_free_private(UIData *ui)
{
	GList *work = ui->register_list;

	while(work)
		{
		RegisterData *rd = work->data;
		work = work->next;

		if (rd->private)
			{
			ui->register_list = g_list_remove(ui->register_list, rd);
			ui_register_free(rd);
			}
		}
}

/* this will remove all keys with private_widget that match the widget,
 * needed only when removing a widget
 */
void ui_unregister_key_private_for_widget(UIData *ui, gpointer widget)
{
	GList *work;

	if (!widget) return;

	work = ui->register_list;
	while (work)
		{
		RegisterData *rd = work->data;
		work = work->next;

		if (rd->private && rd->private_widget == widget)
			{
			ui->register_list = g_list_remove(ui->register_list, rd);
			ui_register_free(rd);
			}
		}
}

static RegisterData *ui_registered_by_key(UIData *ui, const gchar *key, WidgetType type)
{
	GList *work;

	work = ui->register_list;
	while (work)
		{
		RegisterData *rd = work->data;
		if (rd->type == type && strcmp(rd->key, key) == 0) return rd;
		work = work->next;
		}
	return NULL;
}

gpointer ui_get_registered_callbacks(UIData *ui, const gchar *key, WidgetType type)
{
	RegisterData *rd;

	rd = ui_registered_by_key(ui, key, type);
	if (rd) return rd->callbacks;

	if (ui->parent)
		{
		return ui_get_registered_callbacks(ui->parent, key, type);
		}

	return NULL;
}

gint ui_registered_key_exists(UIData *ui, const gchar *key, WidgetType type)
{
	return (ui_registered_by_key(ui, key, type) != NULL);
}

gint ui_widget_exists(UIData *ui, const gchar *key, WidgetType type)
{
	if (!ui->skin) return FALSE;

	return (skin_widget_get_by_key(ui->skin, key, type) != NULL);
}

gint ui_registered_key_is_private(UIData *ui, const gchar *key, WidgetType type)
{
	GList *work;

	/* walk the loop ourselves, for speed only strcmp if private and type match */

	work = ui->register_list;
	while (work)
		{
		RegisterData *rd = work->data;
		if (rd->private && rd->type == type && strcmp(rd->key, key) == 0) return TRUE;
		work = work->next;
		}
	return FALSE;
}

static void ui_debug_print_register_list(UIData *ui)
{
	GList *work;

	printf("-------------------------\n");
	printf("UI registered keys (%3d):\n", g_list_length(ui->register_list));
	printf("-[key]------------------------[type]-----\n");

	work = ui->register_list;
	while (work)
		{
		RegisterData *rd = work->data;
		work = work->next;

		printf("%-30s %-10s %s\n", rd->key, ui_widget_type_to_text(rd->type), rd->private ? "(widget)" : "");
		}
}

void ui_debug_print_registered_keys(UIData *ui)
{
	if (!ui) return;

	if (ui->parent)
		{
		printf("=========================\n");
		printf("UI is \"%s\" is a child of \"%s\"\n", ui->key, ui->parent->key);
		printf("printing out keys starting at the parent\n");
		printf("-------------------------\n");
		ui_debug_print_registered_keys(ui->parent);
		return;
		}

	printf("=========================\n");
	printf("UI is \"%s\"\n", ui->key);
	printf("-------------------------\n");
	printf("    skin: \"%s\"\n", ui->skin_path);
	printf("mode key: \"%s\"\n", ui->skin_mode_key);

	ui_debug_print_register_list(ui);

	if (ui->children)
		{
		GList *work;

		printf("-------------------------\n");
		printf("children: %d\n", g_list_length(ui->children));

		work = ui->children;
		while (work)
			{
			UIData *child;

			child = work->data;
			work = work->next;

			printf("=========================\n");
			printf("   child: \"%s\"\n", child->key);
			printf("mode key: \"%s\"\n", child->skin_mode_key);

			ui_debug_print_register_list(child);
			}
		}

	printf("=========================\n");
}

void ui_debug_print_all_keys(UIData *ui)
{
	GList *work;

	ui_debug_print_registered_keys(ui);

	if (ui->parent) ui = ui->parent;

	skin_debug_print_registered_keys(ui->skin);

	work = ui->children;
	while (work)
		{
		UIData *child;

		child = work->data;
		work = work->next;

		printf("=========================\n");
		printf("child: \"%s\"\n", child->key);
		printf("-------------------------\n");

		skin_debug_print_registered_keys(child->skin);
		}

	printf("=========================\n");
}

/*
 *-------------
 * ui_misc
 *-------------
 */

gint ui_geometry_get(UIData *ui, gint *x, gint *y, gint *w, gint *h)
{
	gint rx, ry, rw, rh;

	if (!ui || !ui->skin ||
	    !ui->window || !ui->window->window ||
	    !GTK_WIDGET_REALIZED(ui->window)) return FALSE;

	gdk_window_get_position(ui->window->window, &rx, &ry);
	gtk_window_get_size(GTK_WINDOW(ui->window), &rw, &rh);

	if (x) *x = rx;
	if (y) *y = ry;
	if (w) *w = rw;
	if (h) *h = rh;

	ui_state_set(filename_from_path(ui->skin_path), ui->skin_mode_key, rx, ry, rw, rh);

	return TRUE;
}

void ui_title_set(UIData *ui, const gchar *text)
{
	gtk_window_set_title(GTK_WINDOW(ui->window), text);
}

void ui_moveable_set(UIData *ui, gint moveable)
{
	ui->allow_move = moveable;
}

void ui_focus_set(UIData *ui, gint enable)
{
	ui->focus_enable = enable;
}

void ui_skin_set(UIData *ui, SkinData *skin, const gchar *path, const gchar *mode_key)
{
	gchar *tmp;

	if (!skin) return;

	/* handle children, closing any that were opened
	 * with the skin_open button, these are identified by
	 * having the same key and skin_mode_key.
	 */
	if (( (path == NULL) != (ui->skin_path == NULL) ) ||
	    (path && strcmp(path, ui->skin_path) != 0) )
		{
		GList *work;

		work = ui->children;
		while (work)
			{
			UIData *child;

			child = work->data;
			work = work->next;

			if (child->key && child->skin_mode_key &&
			    strcmp(child->key, child->skin_mode_key) == 0)
				{
				ui_close(child);
				}
			}
		}

	/* save current state of old skin */
	ui_geometry_get(ui, NULL, NULL, NULL, NULL);

	/* remove old internal widget signals */
	ui_register_free_private(ui);

	skin_free(ui->skin);
	ui->skin = skin;
	skin->ui = ui;

	ui->active_widget = NULL;
	ui->focus_widget = NULL;

	/* use tmp, may be src = dest */

	tmp = g_strdup(path);
	g_free(ui->skin_path);
	ui->skin_path = tmp;

	tmp = g_strdup(mode_key);
	g_free(ui->skin_mode_key);
	ui->skin_mode_key = tmp;

	/* set up new internal widget signals */
	skin_widgets_init(skin, ui);

	/* restore skin states */
	if (slik_remember_position)
		{
		gint x;
		gint y;
		gint w;
		gint h;

		if (ui_state_get(filename_from_path(ui->skin_path), ui->skin_mode_key, &x, &y, &w, &h))
			{
			/* if not visible, this is the first skin, set position */
			if (ui->window && !GTK_WIDGET_VISIBLE(ui->window) && ui->allow_move)
				{
				/* ensure that window is at least visible */
				if (x < 0 - skin->width) x = 0;
				if (x > gdk_screen_width()) x = gdk_screen_width() - skin->width;
				if (y < 0 - skin->height) y = 0;
				if (y > gdk_screen_height()) y = gdk_screen_height() - skin->height;

				if (debug_mode) printf("setting window position %d, %d\n", x, y);

				gtk_window_move(GTK_WINDOW(ui->window), x, y);
				}
			
			/* only do this if the skin is actually sizeable in some way */
			if ((skin->sizeable && ui_widget_exists(ui, "skin_size", button_type_id())) ||
			    ui_widget_exists(ui, "skin_expand", button_type_id()) )
				{
				w = CLAMP(w, skin->width_min, skin->width_max);
				h = CLAMP(h, skin->height_min, skin->height_max);
				skin_resize(ui, w, h);
				}
			}
		}

	ui_display_sync_all(ui);
}

SkinData *ui_skin_load_default(UIData *ui, const gchar *key)
{
	SkinData *skin = NULL;

	if (ui->skin_func)
		{
		skin = ui->skin_func(ui, key, ui->skin_data);
		}
	if (!skin)
		{
		/* well, return something! */
		skin = skin_new();

		skin->real_overlay = ui_slik_logo();
		skin->width = gdk_pixbuf_get_width(skin->real_overlay);
		skin->height = gdk_pixbuf_get_height(skin->real_overlay);

		/* add some widgets (a text message maybe? ) */

		skin->width_def = skin->width_min = skin->width_max = skin->width;
		skin->height_def = skin->height_min = skin->height_max = skin->height;
		}

	return skin;
}

gint ui_skin_load(UIData *ui, const gchar *path, const gchar *mode_key)
{
	SkinData *skin;
	gint success = FALSE;
	gchar *cpath;
	gchar *ckey;

	if (!mode_key) mode_key = "skindata";

	/* copy, since loading a new skin may free the source ? */
	cpath = g_strdup(path);
	ckey = g_strdup(mode_key);

	if (cpath)
		{
		if (!isdir(cpath) && isfile(cpath))
			{
			gchar *dirbuf = remove_level_from_path(path);

			g_free(ckey);
			ckey = g_strdup(filename_from_path(path));

			skin = skin_parse(dirbuf, cpath, FALSE);

			g_free(cpath);
			cpath = dirbuf;
			}
		else
			{
			gchar *datafile;

			datafile = g_strconcat(cpath, "/", ckey, NULL);
			skin = skin_parse(cpath, datafile, FALSE);
			g_free(datafile);
			}

		}
	else
		{
		skin = ui_skin_load_default(ui, ckey);
		}

	if (skin)
		{
		ui_skin_set(ui, skin, cpath, ckey);
		success = TRUE;
		}

	g_free(cpath);
	g_free(ckey);

	return success;
}

gint ui_skin_mode_set(UIData *ui, const gchar *mode_key)
{
	gint success;
	gint ox, oy, ow, oh;

	if (ui->window)
		{
		gdk_window_get_position(ui->window->window, &ox, &oy);
		}
	else
		{
		ox = oy = 0;
		}
	ow = ui->skin->width;
	oh = ui->skin->height;

	success = ui_skin_load(ui, ui->skin_path, mode_key);

	if (slik_smart_placement && success && ui->window)
		{
		gint x, y;
		gint move = FALSE;

		x = y = 0;

		if (oy + (oh / 2) > gdk_screen_height() / 2)
			{
			move = TRUE;
			if (oh > ui->skin->height)
				{
				x = ox;
				y = oy + oh - ui->skin->height;
				if (y > gdk_screen_height() - ui->skin->height)
					{
					y = gdk_screen_height() - ui->skin->height;
					}
				}
			else
				{
				x = ox;
				y = oy + oh - ui->skin->height;
				}
			}
		else if (oy < 0)
			{
			move = TRUE;
			x = ox;
			y = 0;
			}
		if (move) gdk_window_move(ui->window->window, x, y);
		}

	return success;
}

void ui_set_underlay(UIData *ui, GdkPixbuf *pb)
{
	if (!ui || !ui->skin) return;

	skin_set_underlay(ui->skin, ui, pb);
	ui_display_sync(ui, FALSE);
}

void ui_sync_states(void)
{
	GList *work;

	work = slik_ui_list;
	while (work)
		{
		UIData *ui = work->data;
		gint x, y, w, h;

		if (ui_geometry_get(ui, &x, &y, &w, &h))
			{
			ui_state_set(filename_from_path(ui->skin_path), ui->skin_mode_key, x, y, w, h);
			}
		
		work = work->next;
		}
}

/*
 *-------------
 * ui_misc text based utils
 *-------------
 */

UIData *ui_find_by_key(const gchar *key)
{
	GList *work;

	if (!key) return NULL;

	work = slik_ui_list;
	while(work)
		{
		UIData *ui = work->data;

		if (ui->key && strcmp(ui->key, key) == 0) return ui;

		work = work->next;
		}
	return NULL;
}

/*
 *-------------
 * grouping (children) support
 *-------------
 */

void ui_group_set_child(UIData *parent, UIData *child)
{
	if (!parent || !child) return;

	if (child->parent) return;

	/* children are linear,
	 * that is one parent and the rest are children of that parent (no complex trees!)
	 */
	if (parent->parent) parent = parent->parent;

	child->parent = parent;
	parent->children = g_list_append(parent->children, child);

	if (debug_mode) printf("setting %s as a child of %s\n", child->skin_mode_key, parent->skin_mode_key);
}

void ui_group_unset_child(UIData *child)
{
	UIData *parent;
	
	if (!child || !child->parent) return;

	parent = child->parent;
	parent->children = g_list_remove(parent->children, child);
	child->parent = NULL;

	if (debug_mode) printf("removed %s as a child of %s\n", child->skin_mode_key, parent->skin_mode_key);
}

UIData *ui_group_get_parent(UIData *ui)
{
	if (!ui) return NULL;

	return ui->parent;
}

/*
 *-------------
 * SLIK credits / about / logo
 *-------------
 */

GdkPixbuf *ui_slik_logo(void)
{
	return gdk_pixbuf_new_from_inline(-1, icon_slik_logo, FALSE, NULL);
}

static GtkWidget *slik_about = NULL;

static gint ui_slik_about_delete_cb(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	gtk_widget_destroy(slik_about);
	slik_about = NULL;
	return TRUE;
}

static void slik_about_toggle_cb(GtkWidget *button, gpointer data)
{
	gint *val = data;
	*val = GTK_TOGGLE_BUTTON(button)->active;

	if (val == &debug_mode)
		{
		printf("Debug set: %d\n", debug_mode);
		}
	else if (val == &debug_skin)
		{
		printf("Skin coordinates set: %d\n", debug_skin);
		}
}

static void ui_slik_about(void)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	gchar *buf;

	if (slik_about)
		{
		gdk_window_raise(slik_about->window);
		return;
		}

	slik_about = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(slik_about), GDK_WINDOW_TYPE_HINT_DIALOG);
	g_signal_connect(G_OBJECT(slik_about), "delete_event",
			 G_CALLBACK(ui_slik_about_delete_cb), NULL);
	gtk_window_set_wmclass(GTK_WINDOW(slik_about), "about", "SLIK");
	gtk_window_set_title(GTK_WINDOW(slik_about), _("About - SLIK"));
	gtk_container_set_border_width(GTK_CONTAINER(slik_about), 5);

	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_container_add(GTK_CONTAINER(slik_about), frame);
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);
	gtk_widget_realize(slik_about);

	pixbuf = ui_slik_logo();
	image = gtk_image_new_from_pixbuf(pixbuf);
	gdk_pixbuf_unref(pixbuf);

	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);
	gtk_widget_show(image);

	buf = g_strdup_printf(_("SLIK\nSimpLIstic sKin interface\n%s\nCopyright (c) %s John Ellis\nwebsite: %s\nemail: %s\n\nReleased under the GNU General Public License"),
			      SLIK_VERSION,
			      "2004",
			      "gqmpeg.sourceforge.net",
			      "gqview@users.sourceforge.net");
	label = gtk_label_new(buf);
	g_free(buf);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = gtk_toggle_button_new();
	gtk_widget_set_size_request(button, 8, 8);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), debug_mode);
	g_signal_connect(G_OBJECT(button), "toggled",
			 G_CALLBACK(slik_about_toggle_cb), &debug_mode);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 4);
	gtk_widget_show(button);

	button = gtk_toggle_button_new();
	gtk_widget_set_size_request(button, 8, 8);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), debug_skin);
	g_signal_connect(G_OBJECT(button), "toggled",
			 G_CALLBACK(slik_about_toggle_cb), &debug_skin);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 4);
	gtk_widget_show(button);

	window_set_icon(slik_about, NULL, NULL, NULL);

	gtk_widget_show(slik_about);
}

GtkWidget *ui_slik_credit(void)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *image;
	GdkPixbuf *pb;
	gchar *buf;

	frame = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(frame), GTK_RELIEF_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
	g_signal_connect(G_OBJECT(frame), "clicked",
			 G_CALLBACK(ui_slik_about), NULL);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	gtk_widget_show(hbox);

	pb = ui_slik_logo();
	image = gtk_image_new_from_pixbuf(pb);
	gdk_pixbuf_unref(pb);

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_widget_show(image);

	buf = g_strdup_printf(_("utilizes SLIK %s\nSimpLIstic sKin interface"), SLIK_VERSION);
	label = gtk_label_new(buf);
	g_free(buf);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	return frame;
}


