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

#include "players.h"
#include "playlist.h"
#include "songinfo.h"
#include "ui_fileops.h"

GtkWidget *mod_create_info_window(const gchar *file)
{
	GtkWidget *vbox;
	GtkWidget *vbox1;
	GtkWidget *frame;
	SongData *sd;

	sd = player_module_songdata_init(file);
	if (sd) player_module_songdata_update(sd, TRUE);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);

	songinfo_add_label(vbox, _("Filename"), filename_from_path(file));

	frame = gtk_frame_new(_("Song info"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);
        gtk_widget_show(vbox1);

	if (sd)
		{
		const gchar *tmp;
		songinfo_add_label(vbox1, _("Title"), sd->title ? sd->title : "");
		tmp = xmp_extra_data_get_type(sd);
		songinfo_add_label(vbox1, _("Type"), tmp ? tmp : _("unknown"));
		tmp = xmp_extra_data_get_tracker(sd);
		songinfo_add_label(vbox1, _("Tracker name"), tmp ? tmp : _("unknown"));

		songinfo_add_label_number(vbox1, _("Length (estimated)"), sd->length, _("seconds"));
		songinfo_add_label_number(vbox1, _("Patterns"), xmp_extra_data_get_positions(sd), NULL);
		songinfo_add_label_number(vbox1, _("Unique patterns"), xmp_extra_data_get_patterns(sd), NULL);
		songinfo_add_label_number(vbox1, _("Instruments"), xmp_extra_data_get_instruments(sd), NULL);
		songinfo_add_label_number(vbox1, _("Channels"), xmp_extra_data_get_channels(sd), NULL);
		}

	playlist_data_free(sd);

	return vbox;
}

