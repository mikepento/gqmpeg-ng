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

#include "ui_fileops.h"


/* ----------------------------------------------------------
               utility to read mpg header info
   ----------------------------------------------------------*/

#define MPG_HDR_CHUNK_SIZE	1024
#define MPG_HDR_MAX_READS	64

unsigned int bitrates[3][3][15] = {
	{
	{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
	{0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
	},
	{
	{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
	},
	{
	{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
	}
	};

unsigned int sample_freq[3][4] = {
	{44100, 48000, 32000, 0},
	{22050, 24000, 16000, 0},
	{11025, 8000, 8000, 0}
	};

gchar *version_names[3] = {"MPEG-1", "MPEG-2 LSF", "MPEG-2.5"};
gchar *version_numbers[3] = {"1", "2", "2.5"};
gchar *layer_names[3] = {"I","II","III"};
gchar *mode_names[5] = {"stereo", "joint-stereo", "dual-channel",
			"single-channel", "multi-channel"};

static gint header_is_valid(unsigned char *hdr)
{
	if((hdr[0] & 0xff) != 0xff || (hdr[1] & 0xe0) != 0xe0) return FALSE;
	if(!((hdr[1] >> 1) & 0x3)) return FALSE;
	if( ((hdr[2] >> 4) & 0xf) == 0xf) return FALSE;
	if( ((hdr[2] >> 2) & 0x3) == 0x3) return FALSE;
	return TRUE;
}

static Mpg_Data *parse_mpg_header(unsigned char *hdr, const gchar *path)
{
	Mpg_Data *md = NULL;
	gint version;
	gint bitrate_idx;
	gint sample_idx;

	switch ((hdr[1] >> 3 & 0x3))
		{
		case 3:
			version = 0;
			break;
		case 2:
			version = 1;
			break;
		case 0:
			version = 2;
			break;
		default:
			return NULL;
		}

	md = g_new0(Mpg_Data, 1);

	md->version = version;
	md->version_text = version_names[version];
	md->version_number_text = version_numbers[version];

	md->layer = 4 - ((hdr[1] >> 1) & 0x03);
	md->layer_text = layer_names[md->layer - 1];

	md->error_protection = !(hdr[1] & 0x01);

	bitrate_idx = (hdr[2] >> 4) & 0x0f;
	sample_idx = (hdr[2] >> 2) & 0x03;
	md->bit_rate = bitrates[md->version][md->layer - 1][bitrate_idx];
	md->sample_rate = sample_freq[md->version][sample_idx];

	md->padding = (hdr[2] >> 1) & 0x01;
	md->extension = hdr[2] & 0x01;

	md->channel_mode = (hdr[3] >> 6) & 0x03;
	md->mode_text = mode_names[md->channel_mode];
	md->stereo = !(md->channel_mode == 3);

	md->mode_extension = (hdr[3] >> 4) & 0x03;

	md->copyright = (hdr[3] >> 3) & 0x01;
	md->original = (hdr[3] >> 2) & 0x01;
        md->emphasis = (hdr[3]) & 0x3;

	md->file_length = filesize(path);

	if (md->bit_rate)
		md->length = md->file_length / (md->bit_rate * 125); /* 125 is 1000 / 8 */
	else
		md->length = 0;

	return md;
}

Mpg_Data *get_mpg_header_info(const gchar *path)
{
	Mpg_Data *md = NULL;
	gchar *pathl;
	unsigned char buf[MPG_HDR_CHUNK_SIZE];
	FILE *f = NULL;
	gint done = FALSE;
	gint count = 0;

	pathl = path_from_utf8(path);
	f = fopen(pathl, "r");
	g_free(pathl);
	if (!f)
		{
		printf(_("unable to open file for read: %s\n"), path);
		return NULL;
		}

	while (!done && !md && count < MPG_HDR_MAX_READS)
		{
		gint c;

		c = fread(buf, sizeof(unsigned char), MPG_HDR_CHUNK_SIZE, f);
		if (c < 3)
			{
			done = TRUE;
			}
		else
			{
			unsigned char *ptr = buf;

			while (ptr < buf + c - 4 && !header_is_valid(ptr)) ptr++;
			if (header_is_valid(ptr))
				{
				md = parse_mpg_header(ptr, path);
				done = TRUE;
				}
			else if (count == 0)
				{
				/* if this is a file with an ID3v2 tag... keep going */
				done = !(strncmp(buf, "ID3", 3) == 0);
				}
			}
		count++;
		}

	fclose(f);

	return md;
}

