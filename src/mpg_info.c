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

#include "playlist.h"
#include "songinfo.h"
#include "ui_fileops.h"

typedef struct _SongInfoDialog SongInfoDialog;
struct _SongInfoDialog
{
	GtkWidget *vbox;
        GtkWidget *title_entry;
        GtkWidget *artist_entry;
        GtkWidget *album_entry;
	GtkWidget *year_entry;
	GtkWidget *comment_entry;
	GtkWidget *genre_combo;
	GtkWidget *track_spin;
	GtkWidget *save_button;
	gchar *path;
};


static void close_song_info_window(GtkWidget *widget, gpointer data)
{
	SongInfoDialog *s = data;
	g_free(s->path);
	g_free(s);
}

static void save_tag_pressed(GtkWidget *w, gpointer data)
{
	SongInfoDialog *s = data;
        const gchar *title;
        const gchar *artist;
        const gchar *album;
	const gchar *year;
	const gchar *comment;
	const gchar *genre_text;
	guint8 genre = 0;
	guint8 track;

        title = gtk_entry_get_text(GTK_ENTRY(s->title_entry));
        artist = gtk_entry_get_text(GTK_ENTRY(s->artist_entry));
        album = gtk_entry_get_text(GTK_ENTRY(s->album_entry));
	year = gtk_entry_get_text(GTK_ENTRY(s->year_entry));
	comment = gtk_entry_get_text(GTK_ENTRY(s->comment_entry));
	genre_text = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(s->genre_combo)->entry));
	track = (guint8)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->track_spin));

	while (get_id3_genre(genre) && strcmp(get_id3_genre(genre), genre_text) != 0) genre++;
	if (get_id3_genre(genre) == NULL) genre = 255;

	set_id3_tag(s->path, title, artist, album, year, comment, genre, track);
	
	playlist_update_by_path(s->path);

	gtk_widget_set_sensitive(s->save_button, FALSE);
}

static void data_changed_cb(GtkObject *o, gpointer data)
{
	SongInfoDialog *s = data;

	gtk_widget_set_sensitive(s->save_button, TRUE);
}

static gint sort_genre_list_cb(void *a, void *b)
{
	return strcmp((gchar *)a, (gchar *)b);
}

static GtkWidget *add_combo_info(GtkWidget *vbox, const gchar *text, gint data, gpointer pdata)
{
	GtkWidget *combo;
	GtkWidget *label;
	GtkWidget *hbox;
	GList *list = NULL;
	gint i = 0;

	while (get_id3_genre(i) != NULL)
		{
		gchar *g = get_id3_genre(i);
		list = g_list_prepend(list, g);
		i++;
		}
	list = g_list_sort(list, (GCompareFunc) sort_genre_list_cb);
	list = g_list_prepend(list, "");

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);	

	label = gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	combo = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
	if (get_id3_genre(data) != NULL)
		{
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), get_id3_genre(data));
		gtk_combo_set_value_in_list(GTK_COMBO(combo),
					    g_list_index(list, get_id3_genre(data)), FALSE);
		}
	else
		{
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), "");
		gtk_combo_set_value_in_list(GTK_COMBO(combo), 0, FALSE);
		}
	g_signal_connect(G_OBJECT(GTK_COMBO(combo)->entry), "changed",
			 G_CALLBACK(data_changed_cb), pdata);
	gtk_box_pack_end(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
	gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(combo)->entry), FALSE);
	gtk_widget_show(combo);

	g_list_free(list);
	return combo;
}

static GtkWidget *mpg_create_http_info_window(const gchar *file)
{
	GtkWidget *vbox;
	GtkWidget *vbox1;
	const gchar *label;
	GtkWidget *frame;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	songinfo_add_label(vbox, "URL", file);

	frame = gtk_frame_new(_("Information"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
        gtk_container_add(GTK_CONTAINER(frame),vbox1);
        gtk_widget_show(vbox1);

	if (strstr(file, "mp3") == NULL)
		label = _("Live stream.");
	else
		label = _("File across network.");
	songinfo_add_label(vbox1, label, NULL);

	return vbox;
}

GtkWidget *mpg_create_info_window(const gchar *file)
{
	SongInfoDialog *s;
	ID3_Data *d;
	Mpg_Data *h;
	GtkWidget *hbox;
	GtkWidget *vbox1;
	GtkWidget *frame;

	if (type_is_http(file)) return mpg_create_http_info_window(file);

	s = g_new0(SongInfoDialog, 1);
	d = get_id3_tag(file);
	h = get_mpg_header_info(file);

	s->path = g_strdup(file);

	s->vbox = gtk_vbox_new(FALSE, 0);
	g_signal_connect(G_OBJECT (s->vbox), "destroy",
			 G_CALLBACK(close_song_info_window), s);
	gtk_container_set_border_width(GTK_CONTAINER(s->vbox), 5);

	songinfo_add_label(s->vbox, _("Filename"), filename_from_path(file));

	frame = gtk_frame_new(_("Song data"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(s->vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
        gtk_container_add(GTK_CONTAINER(frame),vbox1);
        gtk_widget_show(vbox1);

	if (h)
		{
		songinfo_add_label(vbox1, _("Version"), h->version_text);
		songinfo_add_label(vbox1, _("Layer"), h->layer_text);
		songinfo_add_label(vbox1, _("Channel encoding"), h->mode_text);

		songinfo_add_label_number(vbox1, _("Bit Rate"), h->bit_rate, _("Kbits"));
		songinfo_add_label_number(vbox1, _("Sample Rate"), h->sample_rate, _("Hz"));
		songinfo_add_label_number(vbox1, _("File Size"), h->file_length, _("bytes"));
		songinfo_add_label_number(vbox1, _("Length"), h->length, _("seconds"));

		songinfo_add_label_bool(vbox1, _("Copyright"), h->copyright);
		songinfo_add_label_bool(vbox1, _("Original"), h->original);
		songinfo_add_label_bool(vbox1, _("Emphasis"), h->emphasis);
		songinfo_add_label_bool(vbox1, _("Extensions"), h->mode_extension);
		}
	else
		{
		songinfo_add_label(vbox1, _("unable to find a valid header"), NULL);
		}

	frame = gtk_frame_new(_("ID3 data"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(s->vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
        gtk_container_add(GTK_CONTAINER(frame),vbox1);
        gtk_widget_show(vbox1);

	if (d || h)
		{
		s->title_entry = songinfo_add_entry(vbox1, _("Title"), d ? d->title : NULL,
						    30, G_CALLBACK(data_changed_cb), s);
		s->artist_entry = songinfo_add_entry(vbox1, _("Artist"), d ? d->artist : NULL,
						     30, G_CALLBACK(data_changed_cb), s);
		s->album_entry = songinfo_add_entry(vbox1, _("Album"), d ? d->album : NULL,
						    30, G_CALLBACK(data_changed_cb), s);
		s->year_entry = songinfo_add_entry(vbox1, _("Year"), d ? d->year : NULL,
						   4, G_CALLBACK(data_changed_cb), s);
		s->genre_combo = add_combo_info(vbox1, _("Genre"), d ? d->genre : 255, s);
		s->comment_entry = songinfo_add_entry(vbox1, _("Comment"), d ? d->comment : NULL,
						      30, G_CALLBACK(data_changed_cb), s);

		s->track_spin = songinfo_add_spin(vbox1, _("Track"), d ? (gint)d->track : 0,
			       			  0, 255, G_CALLBACK(data_changed_cb), s);

		hbox = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
		gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 5);
		gtk_widget_show(hbox);

		s->save_button = gtk_button_new_with_label(_("Save Tag"));
		g_signal_connect(G_OBJECT (s->save_button), "clicked",
				 G_CALLBACK(save_tag_pressed), s);
		gtk_container_add(GTK_CONTAINER(hbox), s->save_button);
		gtk_widget_set_sensitive(s->save_button, FALSE);
		gtk_widget_show(s->save_button);

		}
	else
		{
		songinfo_add_label(vbox1, _("disabled when no header is found"), NULL);
		}

	free_id3_tag(d);
	if (h) g_free(h);

	return s->vbox;
}



