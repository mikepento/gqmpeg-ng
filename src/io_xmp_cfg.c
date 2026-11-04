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
#include "io_xmp.h"

#include "rcfile.h"

static GtkWidget *xmp_vbox = NULL;
#if 0
static GtkWidget *xmp_to_wav_path_entry;
#endif
gint c_xmp_frequency;
gint c_xmp_8bit;
gint c_xmp_interpolate;

gint c_xmp_mono;
gint c_xmp_pan_percent;

gint c_xmp_output_to_wav;
gint c_xmp_output_to_wav_to_path;

/*
 *----------------------------------------------------------------------------
 * load / save routines
 *----------------------------------------------------------------------------
 */

void xmp_config_load(FILE *f, const gchar *option, const gchar *value, const gchar *value_all)
{

	xmp_frequency = read_int_option(f, option,
		"xmp_frequency", value, xmp_frequency);
	xmp_8bit = read_bool_option(f, option,
		"xmp_8bit", value, xmp_8bit);
	xmp_interpolate = read_bool_option(f, option,
		"xmp_interpolate", value, xmp_interpolate);

	xmp_mono = read_bool_option(f, option,
		"xmp_mono", value, xmp_mono);
	xmp_pan_percent = read_int_option(f, option,
		"xmp_panning_percent", value, xmp_pan_percent);

	xmp_output_to_wav = read_bool_option(f, option,
		"xmp_to_wav", value, xmp_output_to_wav);
	xmp_output_to_wav_to_path = read_bool_option(f, option,
		"xmp_to_wav_path_enable", value, xmp_output_to_wav_to_path);
	xmp_output_to_wav_path = read_char_option(f, option,
                "xmp_to_wav_path", value_all, xmp_output_to_wav_path);
}

void xmp_config_save(FILE *f)
{
	fprintf(f,"\n##### XMP Output Options #####\n\n");

	write_int_option(f, "xmp_frequency", xmp_frequency);
	write_bool_option(f, "xmp_8bit", xmp_8bit);
	write_bool_option(f, "xmp_interpolate", xmp_interpolate);
	fprintf(f,"\n");
	write_bool_option(f, "xmp_mono", xmp_mono);
	write_int_option(f, "xmp_panning_percent", xmp_pan_percent);
	fprintf(f,"\n");
	write_bool_option(f, "xmp_to_wav", xmp_output_to_wav);
	write_bool_option(f, "xmp_to_wav_path_enable", xmp_output_to_wav_to_path);
	write_char_option(f, "xmp_to_wav_path", xmp_output_to_wav_path);
}

/*
 *----------------------------------------------------------------------------
 * config window routines
 *----------------------------------------------------------------------------
 */

static void hscale_cb(GtkObject *adj, gpointer data)
{
	gint *c_n = data;

	*c_n = (gint)GTK_ADJUSTMENT(adj)->value;
}

static GtkWidget *hscale_new(gchar *text, gint n, gint *c_n, gint lower, gint upper)
{
	GtkObject *adj;
	GtkWidget *hscale;

	*c_n = n;
	adj = gtk_adjustment_new((float)n, (float)lower, (float)upper, 1.0, 1.0, 0.0);
	hscale = gtk_hscale_new (GTK_ADJUSTMENT(adj));
	gtk_range_set_update_policy (GTK_RANGE (hscale), GTK_UPDATE_DELAYED);
	gtk_scale_set_digits (GTK_SCALE (hscale), 0);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(hscale_cb), c_n);

	if (text)
		{
		GtkWidget *hbox;
		GtkWidget *label;

		hbox = gtk_hbox_new (FALSE, 0);

		label = gtk_label_new(text);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		gtk_widget_show(label);

		gtk_box_pack_start(GTK_BOX(hbox), hscale, TRUE, TRUE, 0);
		gtk_widget_show(hscale);

		return hbox;
		}

	return hscale;
}

static void checkbox_cb(GtkWidget *w, gpointer data)
{
	gint *c_n = data;

	*c_n = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
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

void xmp_config_apply(void)
{
#if 0
	gchar *buf;
#endif
	if (!xmp_vbox) return;

	xmp_frequency = c_xmp_frequency;
	xmp_8bit = c_xmp_8bit;
	xmp_interpolate = c_xmp_interpolate;

	xmp_mono = c_xmp_mono;
	xmp_pan_percent = c_xmp_pan_percent;

	xmp_output_to_wav = c_xmp_output_to_wav;
	xmp_output_to_wav_to_path = c_xmp_output_to_wav_to_path;

#if 0
	if (xmp_output_to_wav_path)
                {
                g_free(xmp_output_to_wav_path);
                xmp_output_to_wav_path = NULL;
                }
        buf = gtk_entry_get_text(GTK_ENTRY(xmp_to_wav_path_entry));
        if (buf && strlen(buf) > 0) xmp_output_to_wav_path = g_strdup(buf);
#endif
}

void xmp_config_close(void)
{
	xmp_vbox = NULL;
}

GtkWidget *xmp_config_init(void)
{
	GtkWidget *vbox1;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *hscale;

	xmp_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(xmp_vbox), 5);

	frame = gtk_frame_new(_("Sound Options"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(xmp_vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER(frame),vbox1);
	gtk_widget_show(vbox1);

	button = checkbox_new(_("Mono output"), xmp_mono, &c_xmp_mono);
	gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	hscale = hscale_new(_("Panning Percent:"), xmp_pan_percent, &c_xmp_pan_percent, 0, 100);
	gtk_box_pack_start(GTK_BOX(vbox1), hscale, FALSE, FALSE, 0);
	gtk_widget_show(hscale);

	frame = gtk_frame_new(_("Software Mixer"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(xmp_vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER(frame),vbox1);
	gtk_widget_show(vbox1);

	hscale = hscale_new(_("Mixing rate (hz):"), xmp_frequency, &c_xmp_frequency, 8000, 44100);
	gtk_box_pack_start(GTK_BOX(vbox1), hscale, FALSE, FALSE, 0);
	gtk_widget_show(hscale);

	button = checkbox_new(_("8 bit"), xmp_8bit, &c_xmp_8bit);
	gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = checkbox_new(_("Enable interpolation"), xmp_interpolate, &c_xmp_interpolate);
	gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	if (!xmp_enabled) gtk_widget_set_sensitive(xmp_vbox, FALSE);
	return xmp_vbox;
}

