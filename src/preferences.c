/*
 * GQmpeg
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "gqmpeg.h"
#include "preferences.h"

#include "display.h"
#include "display_flyby.h"
#include "filelist.h"
#include "ipc.h"
#include "mixer.h"
#include "players.h"
#include "playlist.h"
#include "playlist-window.h"
#include "ui2_editor.h"
#include "ui2_main.h"
#include "ui_fileops.h"
#include "ui_menu.h"
#include "ui_tabcomp.h"


enum {
	SKIN_COLUMN_PATH = 0,
	SKIN_COLUMN_NAME,
	SKIN_COLUMN_COUNT
};


static GtkWidget *configwindow;
static GtkWidget *mixer_entry;
static GtkWidget *initial_playlist_entry;
static GtkWidget *initial_directory_entry;
static GtkWidget *default_skin_entry;
static GtkWidget *preset_name_entry[10];
static GtkWidget *preset_file_entry[10];

static gint c_initial_playlist_mode;
static gint c_initial_directory_enable;
static gint c_show_playlist_on_startup;
static gint c_start_playing_on_startup;
static gint c_obey_mode_in_playlist;
static gint c_show_dot_files;
static gint c_disable_filtering;
static gint c_read_file_information;
static gint c_slik_smart_placement;
static gint c_slik_remember_position;
static gint c_mixer_device_id;
static gint c_wm_decorations;
static gint c_play_presets;
static gint c_show_extra_winamp_info;
static gint c_slik_double_size;
static gint c_title_scrolls_always;
static gint c_title_show_extension;
static gint c_title_convert_underscores;

static gint c_allow_ipc;
static gint c_prev_includes_current;
static gint c_drop_single_to_playlist;
static gint c_play_next_on_error;

static gint c_slik_colorshift_on;
static gint c_slik_colorshift_r;
static gint c_slik_colorshift_g;
static gint c_slik_colorshift_b;
static gint c_slik_colorshift_a;

static gint c_slik_transparency_force;
static gint c_slik_transparency_force_a;

static gint c_change_delay;

static gint c_flyby_enabled;
static gint c_flyby_location;

static gint c_skinned_menus_enable;
static gint c_slik_focus_enable;


static void apply_configuration_window(void);
static void create_config_window(gint start_tab);


static void spin_cb(GtkObject *adj, gpointer data)
{
	gint *c_n = data;

	*c_n = (gint)(GTK_ADJUSTMENT(adj)->value);
}

static void hscale_centered_cb(GtkObject *adj, gpointer data)
{
	gint *c_n = data;

	*c_n = (gint)(GTK_ADJUSTMENT(adj)->value / 200.0 * 255.0 + 128.0);
}

static void hscale_cb(GtkObject *adj, gpointer data)
{
	gint *c_n = data;

	*c_n = (gint)(GTK_ADJUSTMENT(adj)->value / 100.0 * 255.0);
}

static GtkWidget *hscale_new(GtkWidget *box, const gchar *text, gint n, gint *c_n, gint centered)
{
	GtkObject *adj;
        GtkWidget *hscale;

	*c_n = n;

	if (centered)
		{
		hscale = ui_edit_spin_new_with_scale(box, text, -100, 100, &adj);
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), ((float)n - 128.0) / 255.0 * 200.0);
		}
	else
		{
		hscale = ui_edit_spin_new_with_scale(box, text, 0, 100, &adj);
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), (float)n / 255.0 * 100.0);
		}

	if (centered)
		{
		g_signal_connect(G_OBJECT(adj), "value_changed",
				 G_CALLBACK(hscale_centered_cb), c_n);
		}
	else
		{
		g_signal_connect(G_OBJECT(adj), "value_changed",
				 G_CALLBACK(hscale_cb), c_n);
		}

	return hscale;
}

static void checkbox_cb(GtkWidget *w, gpointer data)
{
	gint *c_n = data;

	*c_n = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
}

static GtkWidget *checkbox_new(GtkWidget *box, gchar *text, gint n, gint *c_n)
{
        GtkWidget *button;

	*c_n = n;
	button = gtk_check_button_new_with_label(text);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), n);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(checkbox_cb), c_n);

	if (box)
		{
		gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
		gtk_widget_show(button);
		}

	return button;
}

static GtkWidget *vframe_new(GtkWidget *box, gint has_frame, const gchar *text)
{
	GtkWidget *vbox;

	vbox = ui_edit_vframe_new(box, has_frame, text);

	if (has_frame)
		{
		GtkWidget *frame;

		frame = gtk_widget_get_parent(vbox);
		gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
		}

	return vbox;
}

static void mixer_device_id_cb(GtkWidget *w, gpointer data)
{
	gchar *dev_text = data;
	gint d = get_mixer_device_id(dev_text);
	if (d >= 0) c_mixer_device_id = d;
}

static void set_current_playlist_clicked(GtkWidget *w, gpointer data)
{
	if (!isfile(playlist_pathname)) return;
	gtk_entry_set_text(GTK_ENTRY(initial_playlist_entry),playlist_pathname);
}

static void set_current_dir_clicked(GtkWidget *w, gpointer data)
{
	if (!isdir(current_path)) return;
	gtk_entry_set_text(GTK_ENTRY(initial_directory_entry), current_path);
}

static void initial_playlist_empty_selected(GtkWidget *w, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(w)->active)
		c_initial_playlist_mode = PLAYLIST_MODE_EMPTY;
}

static void initial_playlist_session_selected(GtkWidget *w, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(w)->active)
		c_initial_playlist_mode = PLAYLIST_MODE_SESSION;
}

static void initial_playlist_file_selected(GtkWidget *w, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(w)->active)
		c_initial_playlist_mode = PLAYLIST_MODE_FILE;
}

static void flyby_location_cb(GtkWidget *w, gpointer data)
{
	c_flyby_location = GPOINTER_TO_INT(data);
}

static void skin_item_selected(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *path;

	if (!gtk_tree_selection_get_selected(selection, &store, &iter)) return;

	gtk_tree_model_get(store, &iter, SKIN_COLUMN_PATH, &path, -1);
	gtk_entry_set_text(GTK_ENTRY(default_skin_entry), (path) ? path : "");
	g_free(path);
}

static gint sort_skin_list_cb(void *a, void *b)
{
	const gchar *fa = filename_from_path((gchar *)a);
	const gchar *fb = filename_from_path((gchar *)b);
	if (!fa || !fb) return 0;
	return strcmp(fa, fb);
}

static GList *skin_list_add_dir(GList *list, gchar *skinpath)
{
	GList *dirs;

	if (!path_list(skinpath, NULL, &dirs)) return list;

	return g_list_concat(list, dirs);
}

static void populate_skin_list(GtkListStore *store)
{
	gchar *skinpath;
	GList *skin_list = NULL;
	GList *work;
	GtkTreeIter iter;

	/* add default skin */
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   SKIN_COLUMN_PATH, NULL,
			   SKIN_COLUMN_NAME, _("None (default)"), -1);

	/* add home dir skins */
	skinpath = g_strconcat(homedir(), "/", GQMPEG_RC_DIR_SKIN, NULL);
	skin_list = skin_list_add_dir(skin_list, skinpath);
	g_free(skinpath);

	/* add system dir skins */
	skin_list = skin_list_add_dir(skin_list, GQMPEG_SKINDIR);

	skin_list = g_list_sort(skin_list, (GCompareFunc) sort_skin_list_cb);

	work = skin_list;
	while (work)
		{
		gchar *path = work->data;
		gchar *skindata_file = g_strconcat(path, "/skindata", NULL);

		if (isfile(skindata_file))
			{
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
					   SKIN_COLUMN_PATH, path,
					   SKIN_COLUMN_NAME, filename_from_path(path), -1);
			}
		else
			{
			g_free(skindata_file);
			skindata_file = g_strconcat(path, "/main.bmp", NULL);
			if (isfile(skindata_file))
				{
				gchar *amp_txt = g_strconcat(filename_from_path(path), " (amp)", NULL);

				gtk_list_store_append(store, &iter);
				gtk_list_store_set(store, &iter,
						   SKIN_COLUMN_PATH, path,
						   SKIN_COLUMN_NAME, amp_txt, -1);
				g_free(amp_txt);
				}
			else
				{
				printf(_("Valid skin not found in:%s\n"), path);
				printf(_("File named skindata (gqmpeg skins) or main.bmp (winamp skins) not found.\n"));
				}
			}
		g_free(skindata_file);
		work = work->next;
		}

	path_list_free(skin_list);
}

void show_configuration_window(void)
{
	if (configwindow)
		{
		gdk_window_raise(configwindow->window);
		return;
		}

	create_config_window(0);
	gtk_widget_show_all(configwindow);
}

static void hide_configuration_window(void)
{
	player_modules_config_close();
	gtk_widget_destroy(configwindow);
	configwindow = NULL;
}

static void ok_configuration_window(void)
{
	apply_configuration_window();
	hide_configuration_window();
}

static void apply_configuration_window(void)
{
	gchar *buf = NULL;
	const gchar *tmp;
	gint i;
	gint skin_changed = FALSE;

	initial_playlist_mode = c_initial_playlist_mode;

	if (initial_playlist_pathname)
		{
		g_free(initial_playlist_pathname);
		initial_playlist_pathname = NULL;
		}
	tmp = gtk_entry_get_text(GTK_ENTRY(initial_playlist_entry));
	if (tmp && strlen(tmp) > 0) initial_playlist_pathname = g_strdup(tmp);

	if ( slik_double_size != c_slik_double_size || 
		show_extra_winamp_info != c_show_extra_winamp_info) skin_changed = TRUE;

	if (slik_colorshift_on != c_slik_colorshift_on)
		{
		slik_colorshift_on = c_slik_colorshift_on;
		skin_changed = TRUE;
		}

	if (slik_colorshift_r != c_slik_colorshift_r ||
	    slik_colorshift_g != c_slik_colorshift_g ||
	    slik_colorshift_b != c_slik_colorshift_b ||
	    slik_colorshift_a != c_slik_colorshift_a )
		{
		slik_colorshift_r = c_slik_colorshift_r;
		slik_colorshift_g = c_slik_colorshift_g;
		slik_colorshift_b = c_slik_colorshift_b;
		slik_colorshift_a = c_slik_colorshift_a;
		if (slik_colorshift_on) skin_changed = TRUE;
		}

	if (slik_transparency_force != c_slik_transparency_force)
		{
		slik_transparency_force = c_slik_transparency_force;
		skin_changed = TRUE;
		}

	if (slik_transparency_force_a != c_slik_transparency_force_a)
		{
		slik_transparency_force_a = c_slik_transparency_force_a;
		if (slik_transparency_force) skin_changed = TRUE;
		}

	slik_double_size = c_slik_double_size;
	show_extra_winamp_info = c_show_extra_winamp_info;

	buf = remove_trailing_slash(gtk_entry_get_text(GTK_ENTRY(default_skin_entry)));
	if ( (buf && main_window->skin_path && strcmp(buf, main_window->skin_path) != 0 && strlen(buf) > 0) ||
		(!main_window->skin_path && buf && strlen(buf) > 0) )
		{
		/* change to the new skin */
		ui_skin_load(main_window, buf, NULL);
		tab_completion_append_to_history(default_skin_entry, buf);
		}
	else if (main_window->skin_path && buf && strlen(buf) == 0)
		{
		ui_skin_load(main_window, NULL, NULL);
		}
	else if (skin_changed)
		{
		ui_skin_load(main_window, main_window->skin_path, main_window->skin_mode_key);
		}
	g_free(buf);

	if (mixer_device_id != c_mixer_device_id)
		set_mixer_device(c_mixer_device_id);
	mixer_device_id = c_mixer_device_id;

	if (mixer_command)
		{
		g_free(mixer_command);
		mixer_command = NULL;
		}
	tmp = gtk_entry_get_text(GTK_ENTRY(mixer_entry));
	if (tmp && strlen(tmp) > 0) mixer_command = g_strdup(tmp);

	start_playing_on_startup = c_start_playing_on_startup;
	show_playlist_on_startup = c_show_playlist_on_startup;

	obey_mode_in_playlist = c_obey_mode_in_playlist;

	slik_smart_placement = c_slik_smart_placement;
	slik_remember_position = c_slik_remember_position;
	wm_decorations = c_wm_decorations;

	skinned_menus_enable = c_skinned_menus_enable;
	slik_focus_enable = c_slik_focus_enable;

	play_presets = c_play_presets;
	drop_single_to_playlist = c_drop_single_to_playlist;
	play_next_on_error = c_play_next_on_error;

	if (title_scrolls_always != c_title_scrolls_always ||
	    title_show_extension != c_title_show_extension  ||
	    title_convert_underscores != c_title_convert_underscores)
		{
		title_scrolls_always = c_title_scrolls_always;
		title_show_extension = c_title_show_extension;
		title_convert_underscores = c_title_convert_underscores;
		display_set_title(current_song_get_title());
		}

	if (read_file_information != c_read_file_information)
		{
		read_file_information = c_read_file_information;
		playlist_update_all_info();
		playlist_window_length_update();
		}

	if (show_dot_files != c_show_dot_files || disable_filtering != c_disable_filtering)
		{
		show_dot_files = c_show_dot_files;
		disable_filtering = c_disable_filtering;
		filelist_refresh();
		}

	initial_directory_enable = c_initial_directory_enable;
	if (initial_directory_path)
		{
		g_free(initial_directory_path);
		initial_directory_path = NULL;
		}
	buf = remove_trailing_slash(gtk_entry_get_text(GTK_ENTRY(initial_directory_entry)));
	if (buf && strlen(buf) > 0) initial_directory_path = g_strdup(buf);
	g_free(buf);

	player_modules_config_apply();

	if (allow_ipc != c_allow_ipc)
		{
		allow_ipc = c_allow_ipc;
		if (allow_ipc)
			ipc_on();
		else
			ipc_off();
		}

	change_delay = c_change_delay;

	flyby_enabled = c_flyby_enabled;
	flyby_location = c_flyby_location;

	prev_includes_current = c_prev_includes_current;

	for (i=0; i<10; i++)
		{
		if (preset_name[i]) g_free(preset_name[i]);
		if (preset_file[i]) g_free(preset_file[i]);

		tmp = gtk_entry_get_text(GTK_ENTRY(preset_name_entry[i]));
		if (strlen(tmp) > 0)
			preset_name[i] = g_strdup(tmp);
		else
			preset_name[i] = NULL;

		tmp = gtk_entry_get_text(GTK_ENTRY(preset_file_entry[i]));
		if (strlen(tmp) > 0)
			preset_file[i] = g_strdup(tmp);
		else
			preset_file[i] = NULL;
		}

	display_presets_refresh();
	gdk_window_set_decorations (main_window->window->window, wm_decorations);
}

static void create_config_window(gint start_tab)
{
	GtkWidget *configvbox;
	GtkWidget *configtabs;
	GtkWidget *radiobutton;
	GtkWidget *button;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *vbox1;
	GtkWidget *label;
	GtkWidget *frame;
	GtkListStore *store;
	GtkWidget *skin_view;
	GtkWidget *table;
	GtkWidget *sep;

        GtkWidget *omenu;
        GtkWidget *menu;
        GtkWidget *item;
	GList *devices;
	GList *list;
	gint lh, cnt, i;

	GtkObject *adj;
	GtkWidget *spin;

	GtkWidget *tabcomp;

	gchar *skin_label_text;

	GtkWidget *playertabs;
	GList *player_list;
	GList *work;

	configwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(configwindow), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_wmclass(GTK_WINDOW(configwindow), "preferences", "GQmpeg");
	g_signal_connect(G_OBJECT(configwindow), "delete_event",
			 G_CALLBACK(hide_configuration_window), NULL);
	gtk_window_set_resizable(GTK_WINDOW(configwindow), FALSE);
	gtk_window_set_title (GTK_WINDOW (configwindow), _("GQmpeg configuration"));
	gtk_container_set_border_width (GTK_CONTAINER (configwindow), 5);

	configvbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(configwindow),configvbox);
	gtk_widget_show(configvbox);

	hbox = gtk_hbutton_box_new();
	gtk_box_set_spacing(GTK_BOX(hbox), 10);
	gtk_box_pack_end(GTK_BOX(configvbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(ok_configuration_window), NULL);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);


	button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(apply_configuration_window), NULL);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_show(button);

	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(hide_configuration_window), NULL);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_show(button);

	configtabs = gtk_notebook_new();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(configtabs), GTK_POS_TOP);
	gtk_box_pack_start (GTK_BOX(configvbox), configtabs, TRUE, TRUE, 0);

	/* general tab */

	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_widget_show(frame);
	label = gtk_label_new(_("General"));
	gtk_notebook_append_page (GTK_NOTEBOOK (configtabs), frame, label);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add (GTK_CONTAINER(frame),vbox);
	gtk_widget_show(vbox);

	vbox1 = vframe_new(vbox, TRUE, _("Song Information"));

	checkbox_new(vbox1, _("Read file headers (enables playlist length calculation)"), read_file_information, &c_read_file_information);

	vbox1 = vframe_new(vbox, TRUE, _("Window"));

	checkbox_new(vbox1, _("Smart window placement"), slik_smart_placement, &c_slik_smart_placement);
	checkbox_new(vbox1, _("Remember window position"), slik_remember_position, &c_slik_remember_position);
	checkbox_new(vbox1, _("Enable window manager decorations"), wm_decorations, &c_wm_decorations);
	checkbox_new(vbox1, _("Double sized window"), slik_double_size, &c_slik_double_size);

	vbox1 = vframe_new(vbox, TRUE, _("Title Display"));

	checkbox_new(vbox1, _("Always scroll title"), title_scrolls_always, &c_title_scrolls_always);
	checkbox_new(vbox1, _("Show filename extension"), title_show_extension, &c_title_show_extension);
	checkbox_new(vbox1, _("Convert underscores to spaces"), title_convert_underscores, &c_title_convert_underscores);

	vbox1 = vframe_new(vbox, TRUE, _("Mixer"));

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("External Mixer:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	gtk_widget_realize(configwindow);

	tabcomp = tab_completion_new(&mixer_entry, mixer_command, NULL, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), tabcomp,TRUE,TRUE,5);
	gtk_widget_show(tabcomp);

	c_mixer_device_id = mixer_device_id;
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Volume controls device:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

        omenu = gtk_option_menu_new ();
        menu = gtk_menu_new ();

	devices = get_mixer_device_list();

	list = devices;

	lh = 0;
	cnt = 0;
	while (list)
		{
		gchar *dev_text = list->data;

		item = menu_item_add(menu, dev_text,
				     G_CALLBACK(mixer_device_id_cb), dev_text);
		if (get_mixer_device_id(dev_text) == mixer_device_id) lh = cnt;
		cnt++;
		list = list->next;
		}

	g_list_free(devices);

	gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), menu);
	gtk_option_menu_set_history(GTK_OPTION_MENU(omenu), lh);
	gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 5);
	gtk_widget_show(omenu);

	checkbox_new(vbox, _("First press of prev restarts song when playing"), prev_includes_current, &c_prev_includes_current);
	checkbox_new(vbox, _("Allow command parsing through ~/.gqmpeg/command"), allow_ipc, &c_allow_ipc);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Delay between songs (seconds):"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	c_change_delay = change_delay;
	adj = gtk_adjustment_new((gfloat)change_delay, 0.0, 300.0, 1.0, 1.0, 1.0);
	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);
	g_signal_connect(adj, "value_changed",
			   G_CALLBACK(spin_cb), &c_change_delay);
	gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
	gtk_widget_show(spin);

	/* Startup tab */

	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_widget_show(frame);
	label = gtk_label_new(_("Startup"));
	gtk_notebook_append_page (GTK_NOTEBOOK (configtabs), frame, label);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add (GTK_CONTAINER(frame),vbox);
	gtk_widget_show(vbox);

	vbox1 = vframe_new(vbox, TRUE, _("Initial playlist"));

	c_initial_playlist_mode = initial_playlist_mode;
	radiobutton = gtk_radio_button_new_with_label (NULL, _("Empty"));
	if (c_initial_playlist_mode == PLAYLIST_MODE_EMPTY)
		{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton), TRUE);
		}
	g_signal_connect(G_OBJECT(radiobutton), "clicked",
			 G_CALLBACK(initial_playlist_empty_selected), NULL);
	gtk_box_pack_start(GTK_BOX(vbox1), radiobutton, FALSE, FALSE, 0);
	gtk_widget_show(radiobutton);

	button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton)),
						 _("Save playlist between sessions"));
	if (c_initial_playlist_mode == PLAYLIST_MODE_SESSION)
		{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		}
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(initial_playlist_session_selected), NULL);
	gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton)),
						 _("Open specific playlist:"));
	if (c_initial_playlist_mode == PLAYLIST_MODE_FILE)
			{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
			}
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(initial_playlist_file_selected), NULL);
	gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	tabcomp = tab_completion_new(&initial_playlist_entry, initial_playlist_pathname, NULL, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), tabcomp,TRUE,TRUE,0);
	gtk_widget_show(tabcomp);

	button = gtk_button_new_with_label(_("Use current"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(set_current_playlist_clicked), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,FALSE,FALSE,0);
	gtk_widget_show(button);

	vbox1 = vframe_new(vbox, TRUE, _("When program starts:"));

	checkbox_new(vbox1, _("Start playing"), start_playing_on_startup, &c_start_playing_on_startup);
	checkbox_new(vbox1, _("Show playlist window"), show_playlist_on_startup, &c_show_playlist_on_startup);
	checkbox_new(vbox1, _("Change to this directory:"), initial_directory_enable, &c_initial_directory_enable);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	tabcomp = tab_completion_new(&initial_directory_entry, initial_directory_path, NULL, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), tabcomp,TRUE,TRUE,0);
	gtk_widget_show(tabcomp);

	button = gtk_button_new_with_label(_("Use current"));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(set_current_dir_clicked), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,FALSE,FALSE,0);
	gtk_widget_show(button);

	/* output player tab */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_widget_show(frame);
	label = gtk_label_new(_("Output"));
	gtk_notebook_append_page (GTK_NOTEBOOK (configtabs), frame, label);

	playertabs = gtk_notebook_new();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(playertabs), GTK_POS_TOP);
	gtk_container_add(GTK_CONTAINER(frame), playertabs);
	gtk_widget_show(playertabs);

	player_list = player_modules_config_init();
	work = player_list;
	while (work)
		{
		GtkWidget *w = work->data;
		GtkWidget *wlabel;
		gchar *wtext = g_object_get_data(G_OBJECT(w), "title");

		wlabel = gtk_label_new(wtext);
		gtk_notebook_append_page (GTK_NOTEBOOK (playertabs), w, wlabel);
		gtk_widget_show(w);
		work = work->next;
		}

	if (player_list) g_list_free(player_list);

	/* playlist tab */

	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_widget_show(frame);
	label = gtk_label_new(_("Playlist"));
	gtk_notebook_append_page (GTK_NOTEBOOK (configtabs), frame, label);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add (GTK_CONTAINER(frame),vbox);
	gtk_widget_show(vbox);

	checkbox_new(vbox, _("Obey shuffle / repeat if saved in playlist"), obey_mode_in_playlist, &c_obey_mode_in_playlist);
	checkbox_new(vbox, _("Show hidden files and directories in playlist window"), show_dot_files, &c_show_dot_files);
	checkbox_new(vbox, _("Show all files in the playlist window"), disable_filtering, &c_disable_filtering);
	checkbox_new(vbox, _("Add single item drops on main window to playlist"), drop_single_to_playlist, &c_drop_single_to_playlist);
	checkbox_new(vbox, _("Play next song on error"), play_next_on_error, &c_play_next_on_error);

	vbox1 = vframe_new(vbox, TRUE, _("Flybys"));

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	checkbox_new(hbox, _("Enable flybys"), flyby_enabled, &c_flyby_enabled);

	c_flyby_location = flyby_location;

	omenu = gtk_option_menu_new();
	menu = gtk_menu_new();

	for (i = 0; i < 4; i++)
		{
		menu_item_add(menu, display_flyby_location_to_text(i),
			      G_CALLBACK(flyby_location_cb), GINT_TO_POINTER(i));
		}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), menu);
	gtk_option_menu_set_history(GTK_OPTION_MENU(omenu), flyby_location);
	gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 5);
	gtk_widget_show(omenu);

	/* preset tab */

	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_widget_show(frame);
	label = gtk_label_new(_("Presets"));
	gtk_notebook_append_page (GTK_NOTEBOOK (configtabs), frame, label);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add (GTK_CONTAINER(frame),vbox);
	gtk_widget_show(vbox);

	table=gtk_table_new(3,11,FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
        gtk_widget_show(table);

	label = gtk_label_new(_("#"));
        gtk_table_attach_defaults(GTK_TABLE (table),label, 0, 1, 0, 1);
        gtk_widget_show(label);
        label = gtk_label_new(_("Preset name"));
        gtk_table_attach_defaults(GTK_TABLE (table),label, 1, 2, 0, 1);
        gtk_widget_show(label);
        label = gtk_label_new(_("Playlist file"));
        gtk_table_attach_defaults(GTK_TABLE (table),label, 2, 3, 0, 1);
        gtk_widget_show(label);

	for (i=0; i<10; i++)
		{
		gchar s[16];
		hbox = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		gtk_widget_show(hbox);

		snprintf(s, sizeof(s), "%d", i + 1);
		label = gtk_label_new(s);
		gtk_table_attach_defaults(GTK_TABLE (table),label,0,1,i+1,i+2);
		gtk_widget_show(label);

		preset_name_entry[i] = gtk_entry_new();
                gtk_widget_set_size_request(preset_name_entry[i], 80, -1);
		if (preset_name[i]) gtk_entry_set_text(GTK_ENTRY(preset_name_entry[i]), preset_name[i]);
		gtk_table_attach_defaults(GTK_TABLE (table),preset_name_entry[i],1,2,i+1,i+2);
                gtk_widget_show(preset_name_entry[i]);

		tabcomp = tab_completion_new(&preset_file_entry[i], preset_file[i], NULL, NULL);
		gtk_table_attach_defaults(GTK_TABLE (table), tabcomp,2,3,i+1,i+2);
                gtk_widget_show(tabcomp);
		}

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	checkbox_new(vbox, _("Play presets immediately"), play_presets, &c_play_presets);

	/* skin tab */

	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_widget_show(frame);
	label = gtk_label_new(_("Skins"));
	gtk_notebook_append_page (GTK_NOTEBOOK (configtabs), frame, label);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add (GTK_CONTAINER(frame),vbox);
	gtk_widget_show(vbox);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Skin:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show (label);

	tabcomp = tab_completion_new_with_history(&default_skin_entry,
						  main_window->skin_path ? main_window->skin_path : "",
						  "select_skin", 16, NULL, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), tabcomp, TRUE, TRUE, 0);
	gtk_widget_show(tabcomp);

	/* skin list */
	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	skin_view = ui_edit_list_new(vbox, store,
				    G_CALLBACK(skin_item_selected), NULL);
	g_object_unref(store);

	ui_edit_list_add_column(skin_view, _("Available skins:"), SKIN_COLUMN_NAME, FALSE, FALSE);

	populate_skin_list(store);

	skin_label_text = g_strconcat(homedir(), "/", GQMPEG_RC_DIR_SKIN, NULL);

	label = gtk_label_new(skin_label_text);
	gtk_box_pack_end(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	g_free(skin_label_text);

	checkbox_new(vbox, _("Display cpu and frame information in amp skins"), show_extra_winamp_info, &c_show_extra_winamp_info);

/*---*/

	vbox1 = vframe_new(vbox, TRUE, _("Skin Adjustment"));

	hbox = ui_edit_frame_new(vbox1, FALSE, NULL);

	checkbox_new(hbox, _("Color tint:"), slik_colorshift_on, &c_slik_colorshift_on);
	hscale_new(hbox, NULL, slik_colorshift_a, &c_slik_colorshift_a, FALSE);

	hscale_new(vbox1, "R", slik_colorshift_r, &c_slik_colorshift_r, TRUE);
	hscale_new(vbox1, "G", slik_colorshift_g, &c_slik_colorshift_g, TRUE);
	hscale_new(vbox1, "B", slik_colorshift_b, &c_slik_colorshift_b, TRUE);

	hbox = ui_edit_frame_new(vbox1, FALSE, NULL);

	checkbox_new(hbox, _("Force transparency"), slik_transparency_force, &c_slik_transparency_force);
	hscale_new(hbox, NULL, slik_transparency_force_a, &c_slik_transparency_force_a, FALSE);

	checkbox_new(vbox, _("Enable menu skins"), skinned_menus_enable, &c_skinned_menus_enable);
	checkbox_new(vbox, _("Enable keyboard focus"), slik_focus_enable, &c_slik_focus_enable);

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);
	gtk_widget_show(sep);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(configtabs), start_tab);
	gtk_widget_show(configtabs);

}

/*
 *-----------------
 * about window
 *-----------------
 */

static GtkWidget *about = NULL;

static gint about_delete_cb(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	gtk_widget_destroy(about);
	about = NULL;

	return TRUE;
}

void show_about_info(void)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	GtkWidget *cred;
	gchar *buf;

	if (about)
		{
		gdk_window_raise(about->window);
		return;
		}

	about = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(about), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_wmclass(GTK_WINDOW(about), "about", "GQmpeg");
	g_signal_connect(G_OBJECT(about), "delete_event",
			 G_CALLBACK(about_delete_cb), NULL);

	gtk_window_set_title(GTK_WINDOW(about), _("About - GQmpeg"));
	gtk_container_set_border_width(GTK_CONTAINER(about), 5);

	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_container_add(GTK_CONTAINER(about), frame);
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);

	pixbuf = gdk_pixbuf_new_from_xpm_data((const gchar **)gqmpeg_icon_xpm);
	image = gtk_image_new_from_pixbuf(pixbuf);
	gdk_pixbuf_unref(pixbuf);

	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);
	gtk_widget_show(image);

	buf = g_strdup_printf(_("GQmpeg %s\nA frontend to mpg123\n\nCopyright (c) %s John Ellis\nwebsite: %s\nemail: %s\n\nReleased under the GNU General Public License"),
			      VERSION,
			      "2004",
			      "gqmpeg.sourceforge.net",
			      "gqview@users.sourceforge.net");
	label = gtk_label_new(buf);
	g_free(buf);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	cred = ui_slik_credit();
	gtk_box_pack_end(GTK_BOX(vbox), cred, FALSE, FALSE, 0);
	gtk_widget_show(cred);

	window_set_icon(about, (const gchar **)gqmpeg_icon_xpm, NULL, NULL);
	gtk_widget_show(about);
}

