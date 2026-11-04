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

#include "players.h"
#include "playlist.h"
#include "songinfo.h"
#include "ui_fileops.h"
#include "ui_utildlg.h"


#if defined(linux) && defined(HAVE_VIDEO4LINUX)
  #include <fcntl.h>
  #include <sys/ioctl.h>
  #include <linux/videodev.h>
  #include <errno.h>
#endif

#define EPSILON 0.01


/* ----------------------------------------------------------
   input / output interface to a radio tuner

   playlist entry format:
   ======================

   radio stations are stored as a file pathname in the format:

     radio:XX.X:<TITLE>

   radio:   identifies this as a radio station entry
   XX.X     the frequency (in MHz, entry of AM KHz is not defined)
   <TITLE>  the text description (optional)

   for no title the second colon can be omitted: radio:XX.X

   Currently suported platforms:
   =============================
     Linux (using the video4linux /dev/radio interface)

   ----------------------------------------------------------*/

#ifndef RADIO_DEVICE 
  #define RADIO_DEVICE "/dev/radio"
#endif


gchar *radio_device = NULL;


/* these are set here, but should be overwritten in radio_real_test() */
static double radio_limit_lower = 88.0;
static double radio_limit_upper = 107.0;

static SongData *radio_current_sd = NULL;

/* # is per second, the interval to check mono/stereo and signal strength */
#define RADIO_STATUS_INTERVAL 2

/* status check handler id */
static gint radio_timeout_id = -1;


/*
 * auto-scan
 */

/* this is the 'settling time' to determine signal strength */
#define RADIO_SCANS_PER_SECOND 8

/* scan station increment (some countries use 0.05 ?) */
#define RADIO_SCAN_INCREMENT 0.10

/* this is the strength at which to accept a station
 * 24% seems to work for me, less and it detects almost all frequencies...
 * if your tuner/driver does not report/support signal strength, auto-scan is useless
 */
#define RADIO_SCAN_SENSITIVITY 24

static gint radio_scan_timeout_id = -1;
static double radio_scan_point = 0.0;


static gchar *radio_freq_to_path(double freq, const gchar *title);
static double radio_path_to_freq(const gchar *path);


/*
 *-----------------------------------------------------------------------------
 * radio controls (ioctl & system specific)
 *-----------------------------------------------------------------------------
 */

#if defined(linux) && defined(HAVE_VIDEO4LINUX)

/*
 * video4linux interface
 */

gint radio_enabled = TRUE;

static gint v4l_enabled = FALSE;	/* found, ioctl seems to work */
static gint v4l_fd = -1;		/* /dev/radio fd */

static const gchar *radio_device_get(void)
{
	if (radio_device) return radio_device;
	return RADIO_DEVICE;
}

static void radio_error_message(void)
{
	printf("Error talking (ioctl) to %s, %s\n", radio_device_get(), strerror(errno));
}

static guint radio_calc_steps(struct video_tuner *t)
{
	if ( (t->flags & VIDEO_TUNER_LOW) )
		{
		return 16000;	/* field is KHz */
		}
	else
		{
		return 16;	/* field is MHz */
		}
}

static gint radio_control_on(void)
{
	if (v4l_fd != -1) return TRUE;

	if ((v4l_fd = open(radio_device_get(), O_RDONLY)) == -1)
		{
		printf("failed to open radio device %s, %s\n", radio_device_get(), strerror(errno));
		v4l_enabled = FALSE;
		v4l_fd = -1;
		return FALSE;
		}

	return TRUE;
}

static gint radio_control_off(void)
{
	if (v4l_fd == -1) return TRUE;

	close(v4l_fd);
	v4l_fd = -1;

	return TRUE;
}

static gint radio_control_set_mute(gint mute)
{
	struct video_audio av;

	if (v4l_fd == -1) return FALSE;

	if (ioctl(v4l_fd, VIDIOCGAUDIO, &av) != 0)
		{
		radio_error_message();
		return FALSE;
		}

	if (mute)
		{
		av.flags |= VIDEO_AUDIO_MUTE;
		}
	else
		{
		if (av.volume == 0) av.volume = 65535;
		av.flags &= ~VIDEO_AUDIO_MUTE;
		}

	if (ioctl(v4l_fd, VIDIOCSAUDIO, &av) != 0)
		{
		radio_error_message();
		return FALSE;
		}

	return TRUE;
}

static gint radio_control_set_freq(double freq)
{
	struct video_tuner t;
	guint32 f;
	guint m;

	if (v4l_fd == -1) return FALSE;

	t.tuner = 0;

	if (ioctl(v4l_fd, VIDIOCGTUNER, &t) == 0)
		{
		m = radio_calc_steps(&t);
		}
	else
		{
		radio_error_message();
		m = 16;			/* dunno */
		}

	/* convert double to guint32 freq, apply resolution */
	f = freq * m;
	if (ioctl(v4l_fd, VIDIOCSFREQ, &f) != 0)
		{
		radio_error_message();
		return FALSE;
		}

	return TRUE;
}

static void radio_error(void)
{
	radio_control_set_mute(TRUE);
	radio_control_off();
	module_playback_error(radio_current_sd);
}

static gint radio_real_startup(double freq)
{
	if (!radio_control_on()) return FALSE;

	if (freq > 0.0)
		{
		if (!radio_control_set_freq(freq) ||
		    !radio_control_set_mute(FALSE))
			{
			radio_error();
			return FALSE;
			}
		}

	return TRUE;
}

static gint radio_real_shutdown(void)
{
	radio_control_set_mute(TRUE);
	return radio_control_off();
}

static gint radio_real_is_active(void)
{
	return (v4l_fd != -1);
}

static gint radio_real_test()
{
	struct video_tuner t;

	if (!isname(radio_device_get()))
		{
		printf(_("Failed to find %s\n"), radio_device_get());
		v4l_enabled = FALSE;
		return FALSE;
		}

	radio_control_off();
	if (!radio_control_on())
		{
		v4l_enabled = FALSE;
		return FALSE;
		}

	t.tuner = 0;

	if (ioctl(v4l_fd, VIDIOCGTUNER, &t) == 0)
		{
		guint g;

		g = radio_calc_steps(&t);
		radio_limit_lower = (double)t.rangelow / (double)g;
		radio_limit_upper = (double)t.rangehigh / (double)g;
		v4l_enabled = TRUE;
		}
	else
		{
		radio_error_message();
		v4l_enabled = FALSE;
		}

	radio_control_off();

	return v4l_enabled;
}

/* stereo is 0 or 1, strength is 0 to 100 (%) */
static gint radio_real_status(gint *stereo, gint *strength)
{
	struct video_tuner t;
	struct video_audio av;

	if (v4l_fd == -1 || !stereo || !strength) return FALSE;

	/* stereo */

	if (ioctl(v4l_fd, VIDIOCGAUDIO, &av) != 0)
		{
		radio_error_message();
		return FALSE;
		}
	if ( (av.mode & VIDEO_SOUND_STEREO) )
		{
		*stereo = TRUE;
		}
	else
		{
		*stereo = FALSE;
		}

	/* strength */

	t.tuner = 0;
	if (ioctl(v4l_fd, VIDIOCGTUNER, &t) != 0)
		{
		radio_error_message();
		return FALSE;
		}
	*strength = (double)t.signal / 65535.0 * 100.0;	/* 16bit scale */

	return TRUE;
}

static gint radio_real_set_freq(double freq)
{
	return radio_control_set_freq(freq);
}

static gint radio_real_set_mute(gint mute)
{
	return radio_control_set_mute(mute);
}


#else


/*
 * OS does not support radio, so give dummy funcs
 */

gint radio_enabled = FALSE;


static gint radio_real_startup(double freq)
{
	return TRUE;
}

static gint radio_real_shutdown(void)
{
	return TRUE;
}

static gint radio_real_is_active(void)
{
	return FALSE;
}

static gint radio_real_test(void)
{
	return FALSE;
}

static gint radio_real_status(gint *stereo, gint *strength)
{
	return FALSE;
}

static gint radio_real_set_freq(double freq)
{
	return TRUE;
}

static gint radio_real_set_mute(gint mute)
{
	return TRUE;
}


#endif


/*
 *-----------------------------------------------------------------------------
 * wrappers to real stuff, plus update routines
 *-----------------------------------------------------------------------------
 */

/* radio test is only called when needed - and then only once,
 * in other words not at start-up so it does not touch the
 * device - which on linux, will load kernel modules on app start. (slows start)
 * added recheck to allow re-detection (only used from preferences)
 */
gint radio_test(gint recheck)
{
	static gint checked = FALSE;
	static gint found = FALSE;

	if (!radio_enabled) return FALSE;

	if (checked && !recheck) return found;
	checked = TRUE;

	if (debug_mode) printf("Checking for radio tuner...\n");

	found = radio_real_test();

	if (!found) printf(_("radio player module disabled. (failed check)\n"));

	return found;
}

static gint radio_status(gint *stereo, gint *strength)
{
	return radio_real_status(stereo, strength);
}

static gint radio_status_cb(gpointer data)
{
	gint stereo, strength;

	if (radio_scan_timeout_id != -1) return TRUE;

	if (!radio_status(&stereo, &strength))
		{
		if (radio_timeout_id != -1) gtk_timeout_remove(radio_timeout_id);
		radio_timeout_id = -1;
		return FALSE;
		}

	output_channels = stereo ? 2 : 1;
	frames = strength;
	frames_remaining = 100 - strength;

	module_playback_data_changed();

	return TRUE;
}

/* sd can be NULL (for simple init for further control) */
static gint radio_startup(SongData *sd)
{
	if (!sd) return FALSE;

	if (radio_scan_timeout_id == -1)
		{
		radio_test(FALSE);

		if (!radio_real_startup(radio_path_to_freq(sd->path))) return FALSE;
		}

	if (radio_timeout_id == -1)
		{
		radio_timeout_id = gtk_timeout_add(1000 / RADIO_STATUS_INTERVAL, radio_status_cb, NULL);
		}

	return TRUE;
}

static gint radio_shutdown(void)
{
	if (radio_scan_timeout_id == -1) radio_test(FALSE);

	if (radio_timeout_id != -1)
		{
		gtk_timeout_remove(radio_timeout_id);
		radio_timeout_id = -1;
		}

	if (radio_scan_timeout_id == -1) return radio_real_shutdown();

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * misc, path parsing generating (radio:0.0:title)
 *-----------------------------------------------------------------------------
 */

static gint radio_path_is_radio(const gchar *path)
{

	if (!path || path[0] != 'r') return FALSE;
	if (strncmp(path, "radio:", 6) == 0) return TRUE;

	return FALSE;
}

static gchar *radio_freq_to_path(double freq, const gchar *title)
{
	gint p = 1;
	guint t;

	/* 1 or 2 digit precision, for 'clean look' */
	t = (freq * 100.0) + EPSILON;
	if (t % 10 == 5) p = 2;	/* lock to steps of 0.05 */

	if (title && strlen(title) > 0)
		return g_strdup_printf("radio:%.*f:%s", p, freq, title);
	else
		return g_strdup_printf("radio:%.*f", p, freq);
}

static double radio_path_to_freq(const gchar *path)
{
	const gchar *ptr;

	if (!radio_path_is_radio(path)) return 0.0;

	ptr = path;
	ptr += 6;

	if (debug_mode) printf("io_radio.c: %s parses to %f\n", path, (double)strtod(ptr, NULL));

	return (double)strtod(ptr, NULL);
}

static gchar *radio_path_to_title(const gchar *path)
{
	const gchar *ptr;

	if (!path) return NULL;

	ptr = path;
	while (*ptr !=':' && *ptr != '\0') ptr++;
	if (*ptr == '\0') return NULL;
	ptr++;
	while (*ptr !=':' && *ptr != '\0') ptr++;
	if (*ptr == '\0') return NULL;
	ptr++;
	return g_strdup(ptr);
}

static GtkWidget *radio_info_window(const gchar *file)
{
	GtkWidget *vbox;
	GtkWidget *vbox1;
	GtkWidget *frame;
	GtkWidget *label;
	gchar *buf;
	double f;

	vbox = gtk_vbox_new(FALSE, 0);
        gtk_widget_show(vbox);

	label = gtk_label_new(_("Radio tuner"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	frame = gtk_frame_new(_("Song info"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);
	gtk_widget_show(vbox1);

	f = radio_path_to_freq(file);
	buf = g_strdup_printf(_("%.2f Mhz"), f);
	songinfo_add_label(vbox1, _("Frequency"), buf);
	g_free(buf);

	buf = radio_path_to_title(file);
	songinfo_add_label(vbox1, _("Title"), buf ? buf : "");
	g_free(buf);

	return vbox;
}

/*
 *----------------------------------------------------------------------------
 * module callback funcs
 *----------------------------------------------------------------------------
 */

static void radio_data_init(SongData *sd)
{
	sd->type_description = _("Radio station");
	sd->live = TRUE;
}

static gint radio_data_set(SongData *sd, gint read_info)
{
	if (read_info && !sd->info_loaded)
		{
		sd->info_loaded = TRUE;

		g_free(sd->title);
		sd->title = radio_path_to_title(sd->path);
		}

	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * auto-scan
 *-----------------------------------------------------------------------------
 */

static void radio_scan_end(void);


static void radio_scan_done(void)
{
	radio_shutdown();
	if (radio_current_sd) radio_startup(radio_current_sd);
}

static void radio_scan_cancel_cb(GenericDialog *gd, gpointer data)
{
	radio_scan_end();
}

static gint radio_scan_cb(gpointer data)
{
	GenericDialog *gd = data;
	GtkWidget *progress;
	GtkWidget *label;
	gint stereo, strength;
	gchar *buf;

	progress = gd->data;

	/* check the current freq */

	if (radio_status(&stereo, &strength) && strength >= RADIO_SCAN_SENSITIVITY)
		{
		gchar *buf;

		buf = radio_freq_to_path(radio_scan_point, NULL);
		playlist_add(buf, TRUE);
		g_free(buf);
		}

	/* increment */

	radio_scan_point += RADIO_SCAN_INCREMENT;
	if (radio_scan_point > radio_limit_upper)
		{
		generic_dialog_close(gd);
		radio_scan_timeout_id = -1;
		radio_scan_done();
		return FALSE;
		}

	radio_real_set_freq(radio_scan_point);

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress),
		(radio_scan_point - radio_limit_lower) / (radio_limit_upper - radio_limit_lower));

	label = g_object_get_data(G_OBJECT(progress), "current_station");
	buf = g_strdup_printf("%.2f", radio_scan_point);
	gtk_label_set_text(GTK_LABEL(label), buf);
	g_free(buf);


	return TRUE;
}

static gint radio_scan_begin(void)
{
	GenericDialog *gd;
	GtkWidget *progress;
	GtkWidget *label;

	if (radio_scan_timeout_id != -1 || !radio_test(FALSE)) return FALSE;

	if (!radio_real_is_active())
		{
		if (!radio_real_startup(0.0)) return FALSE;
		}
	radio_real_set_mute(TRUE);

	gd = generic_dialog_new(_("Radio auto scan"), _("Scanning for available stations..."),
				"GQmpeg", "radio_scan", TRUE,
				radio_scan_cancel_cb, NULL);
	progress = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(gd->vbox), progress, FALSE, FALSE, 0);
	gtk_widget_show(progress);

	label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(gd->vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	g_object_set_data(G_OBJECT(progress), "current_station", label);

	gd->data = progress;

	radio_scan_point = radio_limit_lower;
	radio_real_set_freq(radio_scan_point);
	radio_scan_timeout_id = gtk_timeout_add(1000 / RADIO_SCANS_PER_SECOND, radio_scan_cb, gd);

	gtk_widget_show(gd->dialog);
	return TRUE;
}

static void radio_scan_end(void)
{
	if (radio_scan_timeout_id == -1) return;

	gtk_timeout_remove(radio_scan_timeout_id);
	radio_scan_timeout_id = -1;

	radio_scan_done();
}

/*
 *----------------------------------------------------------------------------
 * station entry, edit
 *----------------------------------------------------------------------------
 */

static void radio_entry_scan_cb(GtkWidget *button, gpointer data)
{
	radio_scan_begin();
}

static GtkWidget *radio_entry_new(double freq, const gchar *title, gint scan)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *spin;
	GtkObject *adj;
	GtkWidget *entry;

	radio_test(FALSE); /* to get lower/upper limits */

	vbox = gtk_vbox_new(FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show(hbox);

	label = gtk_label_new(_("Frequency (MHz):"));
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
        gtk_widget_show(label);

	adj = gtk_adjustment_new((float)freq, (float)radio_limit_lower, (float)radio_limit_upper,
				 0.05, 1.0, 0.5);

	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.5, 2);
        gtk_box_pack_start(GTK_BOX(hbox), spin, TRUE, TRUE, 0);
        gtk_widget_show(spin);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show(hbox);

	label = gtk_label_new(_("Title:"));
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
        gtk_widget_show(label);

	entry = gtk_entry_new();
	if (title) gtk_entry_set_text(GTK_ENTRY(entry), title);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_widget_show(entry);

	if (scan)
		{
		GtkWidget *button;

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        	gtk_widget_show(hbox);

		button = gtk_button_new_with_label(_("Auto scan"));
		g_signal_connect(G_OBJECT(button), "clicked",
				 G_CALLBACK(radio_entry_scan_cb), NULL);
		gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
		gtk_widget_show(button);

		if (!radio_enabled) gtk_widget_set_sensitive(button, FALSE);
		}

	g_object_set_data(G_OBJECT(vbox), "radio_station", spin);
	g_object_set_data(G_OBJECT(vbox), "radio_station_title", entry);

        return vbox;
}

static GtkWidget *radio_entry_setup(const gchar *path)
{
	double freq;
	gchar *title;
	GtkWidget *ret;

	freq = radio_path_to_freq(path);
	title = radio_path_to_title(path);

	ret = radio_entry_new(freq, title, TRUE);
	g_free(title);

	return ret;
}

static GtkWidget *radio_edit(SongData *sd)
{
	double freq;
	gchar *title;
	GtkWidget *ret;

	freq = radio_path_to_freq(sd->path);
	title = radio_path_to_title(sd->path);

	ret = radio_entry_new(freq, title, FALSE);
	g_free(title);

	return ret;
}

static gchar *radio_get_path(GtkWidget *widget)
{
	double freq;
	GtkWidget *spin;
	GtkWidget *entry;
	const gchar *title = NULL;

	spin = g_object_get_data(G_OBJECT(widget), "radio_station");
	if (spin)
		{
		freq = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
		}
	else
		{
		freq = 0.0;
		}

	entry = g_object_get_data(G_OBJECT(widget), "radio_station_title");
	if (entry) title = gtk_entry_get_text(GTK_ENTRY(entry));

	return radio_freq_to_path(freq, title);
}


/*
 *----------------------------------------------------------------------------
 * module play funcs
 *----------------------------------------------------------------------------
 */

static gint radio_start(SongData *sd, gint position)
{
	gint ret;

	if (debug_mode) printf("io_radio.c: sound started\n");
	radio_current_sd = sd;

	ret = radio_startup(sd);

	playback_done_command(EXEC_PLAY, !ret);
	return ret;
}

static gint radio_stop(SongData *sd)
{
	gint ret;

	if (debug_mode) printf("io_radio.c: play stopped\n");
	if (sd != radio_current_sd)
		{
		printf("io_radio.c warning: attempt to stop playback of non matching id\n");
		}
	ret = radio_shutdown();

	radio_current_sd = NULL;

	playback_done_command(EXEC_STOP, !ret);
	return ret;
}

static gint radio_pause(SongData *sd)
{
	gint ret = FALSE;

	if (debug_mode) printf("io_radio.c: sound muted\n");
	if (sd != radio_current_sd)
		{
		printf("io_radio.c warning: attempt to pause playback of non matching id\n");
		}

	ret = radio_shutdown();

	playback_done_command(EXEC_PAUSE, !ret);
        return ret;
}

static gint radio_continue(SongData *sd)
{
	gint ret = FALSE;

	if (debug_mode) printf("io_radio.c: sound unmuted\n");
	if (sd != radio_current_sd)
		{
		printf("io_radio.c warning: attempt to continue playback of non matching id\n");
		}
	radio_current_sd = sd;

	ret = radio_startup(sd);

	playback_done_command(EXEC_CONTINUE, !ret);
	return ret;
}

static gint radio_seek(SongData *sd, gint position)
{
	/* there is no seek in radio !
	 * well, actually there could be (to change station)
	 */

	playback_done_command(EXEC_SEEK, FALSE);
	return TRUE;
}

/*
 *----------------------------------------------------------------------------
 * player module interface routines
 *----------------------------------------------------------------------------
 */

void radio_init(void)
{
	IO_ModuleData *imd;
	gint id;

	if (debug_mode) printf("radio module initing...\n");

	if (!radio_device) radio_device = g_strdup(RADIO_DEVICE);

	imd = g_new0(IO_ModuleData, 1);

	imd->title = "radio";
	imd->description = _("Radio tuner");

	imd->songdata_init_func = radio_data_init;
	imd->songdata_info_func = radio_data_set;

	if (radio_enabled)
		{
		imd->start_func = radio_start;
		imd->stop_func = radio_stop;
		imd->pause_func = radio_pause;
		imd->continue_func = radio_continue;
		imd->seek_func = radio_seek;
		}

	imd->info_func = radio_info_window;

	imd->config_load_func = radio_config_load;
	imd->config_save_func = radio_config_save;
	imd->config_init_func = radio_config_init;
	imd->config_apply_func = radio_config_apply;
	imd->config_close_func = radio_config_close;

	id = player_module_register(imd);

	module_register_misc_type(_("Radio station"), "radio:0.0:title", id, TRUE,
				  radio_path_is_radio, radio_entry_setup, radio_edit, radio_get_path);
}

