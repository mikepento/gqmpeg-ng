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
#include "io_radio.h"

#include "rcfile.h"

static GtkWidget *radio_vbox = NULL;
static GtkWidget *radio_detect_button = NULL;
static GtkWidget *radio_device_entry = NULL;

/*
 *----------------------------------------------------------------------------
 * load / save routines
 *----------------------------------------------------------------------------
 */

void radio_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all)
{
	radio_device = read_char_option(f, option,
		"radio_device", value_all, radio_device);
}

void radio_config_save(FILE *f)
{
	fprintf(f,"\n##### Radio tuner Options #####\n\n");

	write_char_option(f, "radio_device", radio_device);
}

/*
 *----------------------------------------------------------------------------
 * config window routines
 *----------------------------------------------------------------------------
 */

void radio_config_apply(void)
{
	const gchar *new_device;

	if (!radio_vbox) return;

	new_device = gtk_entry_get_text(GTK_ENTRY(radio_device_entry));
	if (strlen(new_device) > 0 &&
	    (!radio_device || strcmp(radio_device, new_device) != 0))
		{
		g_free(radio_device);
		radio_device = g_strdup(new_device);

		/* force radio restart */
		radio_test(TRUE);
		}
}

void radio_config_close(void)
{
	radio_vbox = NULL;
}

static const gchar *detected_text(gint detected)
{
	if (detected) return _("Radio tuner found");

	return _("Radio tuner not found");
}

static void radio_config_redetect_cb(GtkWidget *button, gpointer data)
{
	GtkWidget *label = data;
	gint detected;
	const gchar *new_device;
	gchar *old_device;

	new_device = gtk_entry_get_text(GTK_ENTRY(radio_device_entry));
	if (strlen(new_device) == 0) return;
	old_device = radio_device;
	radio_device = (gchar *)new_device;
	detected = radio_test(TRUE);
	radio_device = old_device;

	gtk_widget_set_sensitive(button, !detected);
	gtk_label_set_text(GTK_LABEL(label), detected_text(detected));
}

static void radio_device_changed_cb(GtkWidget *widget, gpointer data)
{
	gtk_widget_set_sensitive(radio_detect_button, TRUE);
}

GtkWidget *radio_config_init(void)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *label;
	gint detected;

	radio_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(radio_vbox), 5);

	frame = gtk_frame_new(_("Options"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(radio_vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("device:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	radio_device_entry = gtk_entry_new();
	if (radio_device) gtk_entry_set_text(GTK_ENTRY(radio_device_entry), radio_device);
	g_signal_connect(G_OBJECT(radio_device_entry), "changed",
			 G_CALLBACK(radio_device_changed_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), radio_device_entry, FALSE, FALSE, 5);
	gtk_widget_show(radio_device_entry);

	detected = radio_test(FALSE);

	label = gtk_label_new(detected_text(detected));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	radio_detect_button = gtk_button_new_with_label(_("Detect radio"));
	gtk_box_pack_start(GTK_BOX(vbox), radio_detect_button, FALSE, FALSE, 5);
	gtk_widget_show(radio_detect_button);

	gtk_widget_set_sensitive(radio_detect_button, !detected);
	g_signal_connect(G_OBJECT(radio_detect_button), "clicked",
			 G_CALLBACK(radio_config_redetect_cb), label);

	if (!radio_enabled) gtk_widget_set_sensitive(radio_vbox, FALSE);

	return radio_vbox;
}


