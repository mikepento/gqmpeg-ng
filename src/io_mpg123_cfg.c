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
#include "io_mpg123.h"

#include "rcfile.h"
#include "ui_tabcomp.h"

static GtkWidget     *mpg123_vbox = NULL;
static GtkWidget     *mpg123_device_entry;
static GtkAdjustment *mpg123_buffer_adjustment;
static GtkWidget     *mpg123_buffer_spinbutton;
static GtkAdjustment *mpg123_sample_adjustment;
static GtkWidget     *mpg123_sample_spinbutton;
static GtkWidget     *mpg123_to_wav_path_entry;
static GtkWidget     *mpg123_extra_options_entry;

static gint c_mpg123_buffer_enable;
static gint c_mpg123_buffer;
static gint c_mpg123_downsample;
static gint c_mpg123_custom_sample_size;
static gint c_mpg123_mono;
static gint c_mpg123_8bit;
static gint c_mpg123_device_enable;
static gint c_mpg123_aggressive;
static gint c_mpg123_disable_resync;
static gint c_mpg123_to_wav;
static gint c_mpg123_to_raw;
static gint c_mpg123_to_wav_path_enable;
static gint c_mpg123_read_id3_tags;

/*
 *----------------------------------------------------------------------------
 * load / save routines
 *----------------------------------------------------------------------------
 */

void mpg123_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all)
{
	mpg123_buffer_enable = read_bool_option(f, option,
		"enable_buffer", value, mpg123_buffer_enable);
	mpg123_buffer = read_int_option(f, option,
		"buffer_size", value, mpg123_buffer);

	if (!strcasecmp(option,"downsample"))
		{
		if (!strcasecmp(value,"auto")) mpg123_downsample = MPG123_DOWNSAMPLE_AUTO;
		if (!strcasecmp(value,"2:1")) mpg123_downsample = MPG123_DOWNSAMPLE_22;
		if (!strcasecmp(value,"4:1")) mpg123_downsample = MPG123_DOWNSAMPLE_11;
		if (!strcasecmp(value,"custom")) mpg123_downsample = MPG123_DOWNSAMPLE_CUSTOM;
		}
	mpg123_custom_sample_size = read_int_option(f, option,
		"custom_sample_size", value, mpg123_custom_sample_size);

	mpg123_mono = read_bool_option(f, option,
		"enable_mono", value, mpg123_mono);
	mpg123_8bit = read_bool_option(f, option,
		"enable_8bit", value, mpg123_8bit);

	mpg123_device_enable = read_bool_option(f, option,
		"enable_specific_device", value, mpg123_device_enable);
	mpg123_device = read_char_option(f, option,
		"specific_device", value_all, mpg123_device);

	mpg123_aggressive = read_bool_option(f, option,
		"mpg123_aggressive", value, mpg123_aggressive);
	mpg123_disable_resync = read_bool_option(f, option,
		"mpg123_disable_resync", value, mpg123_disable_resync);

	mpg123_to_wav = read_bool_option(f, option,
		"mpg123_output_to_wav", value, mpg123_to_wav);
	mpg123_to_wav_path_enable = read_bool_option(f, option,
		"mpg123_output_to_wav_to_path", value, mpg123_to_wav_path_enable);
	mpg123_to_wav_path = read_char_option(f, option,
		"mpg123_output_to_wav_path", value_all, mpg123_to_wav_path);
	mpg123_to_raw = read_bool_option(f, option,
		"mpg123_output_to_raw", value, mpg123_to_raw);

	mpg123_read_id3_tags = read_bool_option(f, option,
		"mpg123_read_id3_tags", value, mpg123_read_id3_tags);

	mpg123_extra_options = read_char_option(f, option,
		"mpg123_extra_options", value_all, mpg123_extra_options);

	/* this is for backwards compatibility */
	mpg123_read_id3_tags = read_bool_option(f, option,
		"read_header_tags", value, mpg123_read_id3_tags);
	mpg123_read_id3_tags = read_bool_option(f, option,
		"read_id3_tags", value, mpg123_read_id3_tags);


}

void mpg123_config_save(FILE *f)
{
	fprintf(f,"\n##### MPG123 Output Options #####\n\n");

	write_bool_option(f, "enable_buffer", mpg123_buffer_enable);
	write_int_option(f, "buffer_size", mpg123_buffer);
	fprintf(f,"\n");
	fprintf(f,"downsample: ");
	if (mpg123_downsample == MPG123_DOWNSAMPLE_AUTO) fprintf(f,"auto\n");
	if (mpg123_downsample == MPG123_DOWNSAMPLE_22) fprintf(f,"2:1\n");
	if (mpg123_downsample == MPG123_DOWNSAMPLE_11) fprintf(f,"4:1\n");
	if (mpg123_downsample == MPG123_DOWNSAMPLE_CUSTOM) fprintf(f,"custom\n");
	write_int_option(f, "custom_sample_size", mpg123_custom_sample_size);
	fprintf(f,"\n");
	write_bool_option(f, "enable_mono", mpg123_mono);
	write_bool_option(f, "enable_8bit", mpg123_8bit);
	fprintf(f,"\n");
	write_bool_option(f, "enable_specific_device", mpg123_device_enable);
	write_char_option(f, "specific_device", mpg123_device);
	fprintf(f,"\n");
	write_bool_option(f, "mpg123_aggressive", mpg123_aggressive);
	write_bool_option(f, "mpg123_disable_resync", mpg123_disable_resync);

	write_bool_option(f, "mpg123_output_to_wav", mpg123_to_wav);
	write_bool_option(f, "mpg123_output_to_wav_to_path", mpg123_to_wav_path_enable);
	write_char_option(f, "mpg123_output_to_wav_path", mpg123_to_wav_path);
	write_bool_option(f, "mpg123_output_to_raw", mpg123_to_raw);

	write_bool_option(f, "mpg123_read_id3_tags", mpg123_read_id3_tags);

	write_char_option(f, "mpg123_extra_options", mpg123_extra_options);
}

/*
 *----------------------------------------------------------------------------
 * config window routines
 *----------------------------------------------------------------------------
 */

static void checkbox_cb(GtkWidget *w, gpointer data)
{
	gint *c_n = data;

	*c_n = GTK_TOGGLE_BUTTON (w)->active;
}

static GtkWidget *checkbox_new(gchar *text, gint n, gint *c_n)
{
	GtkWidget *button;

	*c_n = n;
	button = gtk_check_button_new_with_label(text);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), n);
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(checkbox_cb), c_n);

	return button;
}

static void downsample_auto_selected(GtkWidget *w, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(w)->active)
		c_mpg123_downsample = MPG123_DOWNSAMPLE_AUTO;
}

static void downsample_22_selected(GtkWidget *w, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(w)->active)
		c_mpg123_downsample = MPG123_DOWNSAMPLE_22;
}

static void downsample_11_selected(GtkWidget *w, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(w)->active)
		c_mpg123_downsample = MPG123_DOWNSAMPLE_11;
}

static void downsample_custom_selected(GtkWidget *w, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(w)->active)
		c_mpg123_downsample = MPG123_DOWNSAMPLE_CUSTOM;
}

static void output_type_cb(GtkWidget *w, gpointer data)
{
	c_mpg123_to_raw = GPOINTER_TO_INT(data);
}

void mpg123_config_apply(void)
{
	const gchar *buf;

	if (!mpg123_vbox) return;

	mpg123_buffer_enable = c_mpg123_buffer_enable;
	mpg123_buffer = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mpg123_buffer_spinbutton));
	mpg123_downsample = c_mpg123_downsample;
	mpg123_custom_sample_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mpg123_sample_spinbutton));
	mpg123_mono = c_mpg123_mono;
	mpg123_8bit = c_mpg123_8bit;
	mpg123_device_enable = c_mpg123_device_enable;
	mpg123_aggressive = c_mpg123_aggressive;
	mpg123_disable_resync = c_mpg123_disable_resync;

	mpg123_to_wav = c_mpg123_to_wav;
	mpg123_to_raw = c_mpg123_to_raw;
	mpg123_to_wav_path_enable = c_mpg123_to_wav_path_enable;
	if (mpg123_to_wav_path)
		{
		g_free(mpg123_to_wav_path);
		mpg123_to_wav_path = NULL;
		}
	buf = gtk_entry_get_text(GTK_ENTRY(mpg123_to_wav_path_entry));
	if (buf && strlen(buf) > 0) mpg123_to_wav_path = g_strdup(buf);

	g_free(mpg123_device);
	mpg123_device = NULL;
	buf = gtk_entry_get_text(GTK_ENTRY(mpg123_device_entry));
	if (buf && strlen(buf) > 0) mpg123_device = g_strdup(buf);

	g_free(mpg123_extra_options);
	mpg123_extra_options = NULL;
	buf = gtk_entry_get_text(GTK_ENTRY(mpg123_extra_options_entry));
	if (buf && strlen(buf) > 0) mpg123_extra_options = g_strdup(buf);

	mpg123_read_id3_tags = c_mpg123_read_id3_tags;
}

void mpg123_config_close(void)
{
	mpg123_vbox = NULL;
}

GtkWidget *mpg123_config_init(void)
{
	GtkWidget *vbox1;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *radiobutton;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *tabcomp;

	mpg123_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(mpg123_vbox), 5);

	button = checkbox_new(_("Use ID3 tags when available"),
		 mpg123_read_id3_tags, &c_mpg123_read_id3_tags);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = checkbox_new(_("Enable output buffer of size(Kbytes):"),
		 mpg123_buffer_enable, &c_mpg123_buffer_enable);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	c_mpg123_buffer = mpg123_buffer;
	mpg123_buffer_adjustment = (GtkAdjustment *)gtk_adjustment_new ((float)c_mpg123_buffer,0.0,8192.0,8.0,8.0,8.0);
	mpg123_buffer_spinbutton = gtk_spin_button_new (mpg123_buffer_adjustment, (float)8, 0);
	gtk_box_pack_start(GTK_BOX(hbox), mpg123_buffer_spinbutton, FALSE, FALSE, 0);
	gtk_widget_show(mpg123_buffer_spinbutton);

	/* downsampling */

	frame = gtk_frame_new(_("Downsampling"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER(frame),vbox1);
	gtk_widget_show(vbox1);

	c_mpg123_downsample = mpg123_downsample;
	radiobutton = gtk_radio_button_new_with_label (NULL,_("Auto or(1:1)"));
	if (c_mpg123_downsample == MPG123_DOWNSAMPLE_AUTO)
		{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton), TRUE);
		}
	g_signal_connect(G_OBJECT(radiobutton), "clicked",
			 G_CALLBACK(downsample_auto_selected), NULL);
	gtk_box_pack_start(GTK_BOX(vbox1), radiobutton, FALSE, FALSE, 0);
	gtk_widget_show(radiobutton);

	button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton)),
						 _("22Khz (2:1)"));
	if (c_mpg123_downsample == MPG123_DOWNSAMPLE_22)
		{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		}
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(downsample_22_selected), NULL);
	gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton)),
						 _("11Khz (4:1)"));
	if (c_mpg123_downsample == MPG123_DOWNSAMPLE_11)
		{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		}
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(downsample_11_selected), NULL);
	gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton)),
						 _("Custom (bits/sec):"));
	if (c_mpg123_downsample == MPG123_DOWNSAMPLE_CUSTOM)
		{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		}
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(downsample_custom_selected), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	c_mpg123_custom_sample_size = mpg123_custom_sample_size;
	mpg123_sample_adjustment = (GtkAdjustment *)gtk_adjustment_new ((float)c_mpg123_custom_sample_size,8000.0,49152.0,8.0,8.0,8.0);
	mpg123_sample_spinbutton = gtk_spin_button_new (mpg123_sample_adjustment, (float)8, 0);
	gtk_box_pack_start(GTK_BOX(hbox), mpg123_sample_spinbutton, FALSE, FALSE, 0);
	gtk_widget_show(mpg123_sample_spinbutton);

	button = checkbox_new(_("Mix both channels into mono output"),
		 mpg123_mono, &c_mpg123_mono);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = checkbox_new(_("8-bit output"),
		 mpg123_8bit, &c_mpg123_8bit);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = checkbox_new(_("Specific output device:"),
		 mpg123_device_enable, &c_mpg123_device_enable);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	mpg123_device_entry = gtk_entry_new();
	if (mpg123_device)
		{
		gtk_entry_set_text(GTK_ENTRY(mpg123_device_entry), mpg123_device);
		}
	gtk_box_pack_start(GTK_BOX(hbox), mpg123_device_entry, FALSE, FALSE, 0);
	gtk_widget_show(mpg123_device_entry);

	button = checkbox_new(_("Attempt to get higher priority. (setuid root mpg123 helps here)"),
		 mpg123_aggressive, &c_mpg123_aggressive);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = checkbox_new(_("disable resync on error (may help VBR files)"),
		 mpg123_disable_resync, &c_mpg123_disable_resync);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	frame = gtk_frame_new(_("Output to Wave"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER(frame),vbox1);
	gtk_widget_show(vbox1);

	button = checkbox_new(_("Output to file"),
		 mpg123_to_wav, &c_mpg123_to_wav);
	gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Format:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	c_mpg123_to_raw = mpg123_to_raw;
	radiobutton = gtk_radio_button_new_with_label (NULL,_("Wave (.wav)"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton), !c_mpg123_to_raw);
	gtk_box_pack_start(GTK_BOX(hbox), radiobutton, FALSE, FALSE, 0);
	gtk_widget_show(radiobutton);

	button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radiobutton)),
						  _("CDR raw (.raw)"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), c_mpg123_to_raw);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	g_signal_connect(G_OBJECT(radiobutton), "clicked",
			 G_CALLBACK(output_type_cb), GINT_TO_POINTER(FALSE));
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(output_type_cb), GINT_TO_POINTER(TRUE));

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	button = checkbox_new(_("Place files in path:"),
		 mpg123_to_wav_path_enable, &c_mpg123_to_wav_path_enable);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	tabcomp = tab_completion_new(&mpg123_to_wav_path_entry, mpg123_to_wav_path, NULL, NULL);
	gtk_entry_set_max_length(GTK_ENTRY(mpg123_to_wav_path_entry), 128);
	gtk_box_pack_start(GTK_BOX(hbox), tabcomp, TRUE, TRUE, 0);
	gtk_widget_show(tabcomp);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("User options:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	mpg123_extra_options_entry = gtk_entry_new();
	if (mpg123_extra_options)
		{
		gtk_entry_set_text(GTK_ENTRY(mpg123_extra_options_entry), mpg123_extra_options);
		}
	gtk_box_pack_start(GTK_BOX(hbox), mpg123_extra_options_entry, TRUE, TRUE, 0);
	gtk_widget_show(mpg123_extra_options_entry);

	label = gtk_label_new(_("Options do not take effect until the next song."));
	gtk_box_pack_start(GTK_BOX(mpg123_vbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	if (!mpg123_enabled) gtk_widget_set_sensitive(mpg123_vbox, FALSE);
	return mpg123_vbox;
}



