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


#include "gqmpeg.h"
#include "io_ogg123.h"

#include "rcfile.h"
#include "ui_menu.h"

static GtkWidget *ogg123_vbox = NULL;
static GtkWidget *ogg123_device_options_entry = NULL;
static GtkWidget *ogg123_extra_options_entry = NULL;

gint c_ogg123_device;

/*
 *----------------------------------------------------------------------------
 * load / save routines
 *----------------------------------------------------------------------------
 */

void ogg123_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all)
{

	ogg123_device = read_int_option(f, option,
		"ogg123_device", value, ogg123_device);
	ogg123_device_options = read_char_option(f, option,
		"ogg123_device_options", value_all, ogg123_device_options);
	ogg123_extra_options = read_char_option(f, option,
		"ogg123_extra_options", value_all, ogg123_extra_options);
}

void ogg123_config_save(FILE *f)
{
	fprintf(f,"\n##### ogg123 Output Options #####\n\n");

	write_int_option(f, "ogg123_device", ogg123_device);
	write_char_option(f, "ogg123_device_options", ogg123_device_options);
	write_char_option(f, "ogg123_extra_options", ogg123_extra_options);
}

/*
 *----------------------------------------------------------------------------
 * config window routines
 *----------------------------------------------------------------------------
 */

void ogg123_config_apply(void)
{
	const gchar *buf;

	if (!ogg123_vbox) return;

	buf = gtk_entry_get_text(GTK_ENTRY(ogg123_device_options_entry));
	g_free(ogg123_device_options);
	ogg123_device_options = NULL;
	if (buf && strlen(buf) > 0) ogg123_device_options = g_strdup(buf);

	ogg123_device = c_ogg123_device;

	buf = gtk_entry_get_text(GTK_ENTRY(ogg123_extra_options_entry));
	g_free(ogg123_extra_options);
	ogg123_extra_options = NULL;
	if (buf && strlen(buf) > 0) ogg123_extra_options = g_strdup(buf);
}

void ogg123_config_close(void)
{
	ogg123_vbox = NULL;
}

static void ogg123_device_id_cb(GtkWidget *widget, gpointer data)
{
	c_ogg123_device = GPOINTER_TO_INT(data);
}

static GtkWidget *ogg123_device_menu(gint device)
{
	GtkWidget *omenu;
	GtkWidget *menu;
	GList *list;
	GList *work;
	gint c;

	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();

	list = ogg123_get_device_list();
	work = list;
	c = 0;
	while(work)
		{
		gchar *text = work->data;

		menu_item_add(menu, text,
			      G_CALLBACK(ogg123_device_id_cb), GINT_TO_POINTER(c));
		c++;
		work = work->next;
		}

	g_list_free(list);

	gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), menu);
	gtk_option_menu_set_history(GTK_OPTION_MENU(omenu), device);

	return omenu;
}

GtkWidget *ogg123_config_init(void)
{
	GtkWidget *vbox1;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *omenu;

	ogg123_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ogg123_vbox), 5);

	frame = gtk_frame_new(_("Device Options"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(ogg123_vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);
	gtk_widget_show(vbox1);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Device:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	c_ogg123_device = ogg123_device;
	omenu = ogg123_device_menu(ogg123_device);
	gtk_box_pack_start(GTK_BOX(hbox), omenu, FALSE, FALSE, 0);
	gtk_widget_show(omenu);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Device options:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	ogg123_device_options_entry = gtk_entry_new();
	if (ogg123_device_options) gtk_entry_set_text(GTK_ENTRY(ogg123_device_options_entry), ogg123_device_options);
	gtk_box_pack_start(GTK_BOX(hbox), ogg123_device_options_entry, TRUE, TRUE, 0);
	gtk_widget_show(ogg123_device_options_entry);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_box_pack_start(GTK_BOX(ogg123_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("User options:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	ogg123_extra_options_entry = gtk_entry_new();
	if (ogg123_extra_options)
		{
		gtk_entry_set_text(GTK_ENTRY(ogg123_extra_options_entry), ogg123_extra_options);
		}
	gtk_box_pack_start(GTK_BOX(hbox), ogg123_extra_options_entry, TRUE, TRUE, 0);
	gtk_widget_show(ogg123_extra_options_entry);

	label = gtk_label_new(_("Options do not take effect until the next song."));
	gtk_box_pack_start(GTK_BOX(ogg123_vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	if (!ogg123_enabled) gtk_widget_set_sensitive(ogg123_vbox, FALSE);
	return ogg123_vbox;
}

