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
#include "japanese_tag.h"
#include "russian_tag.h"


/* ----------------------------------------------------------
   utility to read ID3 tags, format from the ID3 tag spec
   ----------------------------------------------------------*/

static gchar *genres[] = {
	/* 00 */	"Blues", "Classic Rock", "Country", "Dance", "Disco",
			"Funk", "Grunge", "Hip-Hop", "Jazz", "Metal",
	/* 10 */	"New Age", "Oldies", "Other", "Pop", "R&B",
			"Rap", "Reggae", "Rock", "Techno", "Industrial",
	/* 20 */	"Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack",
			"Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
	/* 30 */	"Fusion", "Trance", "Classical", "Instrumental", "Acid",
			"House", "Game", "Sound Clip", "Gospel", "Noise",
	/* 40 */	"Alternative Rock", "Bass", "Soul", "Punk", "Space",
			"Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic",
	/* 50 */	"Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance",
			"Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
	/* 60 */	"Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native US",
			"Cabaret", "New Wave", "Psychadelic", "Rave", "Showtunes",
	/* 70 */	"Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz",
			"Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock",
	/* 80 */	"Folk", "Folk-Rock", "National Folk", "Swing", "Fast Fusion",
			"Bebob", "Latin", "Revival", "Celtic", "Bluegrass",
	/* 90 */	"Avantgarde", "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock",
			"Slow Rock", "Big Band", "Chorus", "Easy Listening", "Acoustic",
	/* 100 */	"Humour", "Speech", "Chanson", "Opera", "Chamber Music",
			"Sonata", "Symphony", "Booty Bass", "Primus", "Porn Groove",
	/* 110 */	"Satire", "Slow Jam", "Club", "Tango", "Samba",
			"Folklore", "Ballad", "Power Ballad", "Rhytmic Soul", "Freestyle",
	/* 120 */	"Duet", "Punk Rock", "Drum Solo", "Acapella", "Euro-House",
			"Dance Hall", "Goa", "Drum & Bass", "Club-House", "Hardcore",
	/* 130 */	"Terror", "Indie", "BritPop", "Negerpunk", "Polsk Punk",
			"Beat", "Christian Gangsta Rap", "Heavy Metal", "Black Metal", "Crossover",
	/* 140 */	"Contemporary Christian", "Christian Rock", "Merengue", "Salsa", "Trash Metal",
			"Anime", "JPop", "SynthPop"
			};

gchar* get_id3_genre(gint g)
{
	if (g < 0 || g > (gint)((sizeof(genres)/sizeof(gchar*)) - 1)) return NULL;

	return genres[g];
}

static gchar* parse_text(gchar *text, gint l)
{
	gchar buf[32];
	gchar *ptr;

	strncpy(buf, text, l);
	ptr = buf + l - 1;

	while ((ptr[0] == ' ' || ptr[0] == '\0' || ptr[0] == '\n') && ptr > buf) ptr--;
	if (ptr > buf || (ptr == buf && ptr[0] != ' ' && ptr[0] != '\0' && ptr[0] != '\n'))
		{
		ptr++;
		ptr[0] = '\0';
#ifdef ENABLE_JAPANESE_TAGS
		return to_string_euc(buf);
#elif defined(ENABLE_RUSSIAN_TAGS)
		russian_recode(buf, 32);
		return g_strdup(buf);
#else
		return g_strdup(buf);
#endif
		}
	else
		return NULL;
}

ID3_Data *get_id3_tag(const gchar *path)
{
	ID3_Data *d;
	FILE *f;
	gchar buf[130];
	gchar *ptr;
	gchar *pathl;

	if (filesize(path) < 129)
		return NULL;

	pathl = path_from_utf8(path);
	f = fopen(pathl,"r");
	g_free(pathl);
        if (!f)
                {
                printf(_("error opening file for tag: %s\n"), path);
                return NULL;
                }

	if (fseek (f, -128, SEEK_END) != 0)
		{
                printf(_("could not seek in file: %s\n"), path);
		fclose(f);
		return NULL;
		}

	if ( fread(buf, sizeof(gchar), 128, f) < 128)
		{
                printf(_("could not read 128 bytes in file: %s\n"), path);
		fclose(f);
		return NULL;
		}
	fclose(f);

	ptr = buf;
	if (strncmp(buf, "TAG", 3) != 0)
		return NULL;

	d = g_new0(ID3_Data, 1);

	ptr += 3;
	d->title = parse_text(ptr, 30);
	ptr += 30;
	d->artist = parse_text(ptr, 30);
	ptr += 30;
	d->album = parse_text(ptr, 30);
	ptr += 30;
	d->year = parse_text(ptr, 4);
	ptr += 4;
	if (ptr[28] == '\0')
		{
		/* id3 v1.1
		 * (or short v1.0 comment, in which case track field will be 0 for no track)
		 */
		d->comment = parse_text(ptr, 28);
		d->track = ptr[29];
		}
	else
		{
		/* id3 v1.0 */
		d->comment = parse_text(ptr, 30);
		d->track = 0;
		}
	ptr += 30;
	d->genre = ptr[0];
	d->genre_description = get_id3_genre(d->genre);

	return d;
}

gint set_id3_tag(const gchar *path, const gchar *title, const gchar *artist, const gchar *album,
		 const gchar *year, const gchar *comment, guint8 genre, guint8 track)
{
	FILE *f;
	gchar buf[130];
	gint i;
	gchar *pathl;

	pathl = path_from_utf8(path);
	f = fopen(pathl,"r+");
	g_free(pathl);
        if (!f)
                {
                printf(_("error opening file for tag: %s\n"), path);
                return FALSE;
                }

	if (filesize(path) > 128)
		{
		if (fseek (f, -128, SEEK_END) != 0)
			{
	                printf(_("could not seek in file: %s\n"), path);
			fclose(f);
			return FALSE;
			}
		if (fread(buf, sizeof(gchar), 128, f) < 128)
			{
                	printf(_("could not read 128 bytes in file: %s\n"), path);
			fclose(f);
			return FALSE;
			}
		if (strncmp(buf, "TAG", 3) == 0)
			{
			/* file contains a TAG, overwrite starting here */
			if (fseek (f, -128, SEEK_END) != 0)
				{
	        	        printf(_("could not seek in file: %s\n"), path);
				fclose(f);
				return FALSE;
				}
			}
		else
			{
			/* no tag found, seek to append tag to end of file */
			if (fseek (f, 0, SEEK_END) != 0)
				{
	        	        printf(_("could not seek in file: %s\n"), path);
				fclose(f);
				return FALSE;
				}
			}
			
		}
	else
		{
		if (fseek (f, 0, SEEK_END) != 0)
			{
        	        printf(_("could not seek in file: %s\n"), path);
			fclose(f);
			return FALSE;
			}
		}

	for (i=0; i<128; i++)
		{
		buf[i] = '\0';
		}

	strncpy(buf, "TAG", 3);
	if (title) strncpy(buf + 3, title, 30);
	if (artist) strncpy(buf + 33, artist, 30);
	if (album) strncpy(buf + 63, album, 30);
	if (year) strncpy(buf + 93, year, 4);

	if (comment && track == 0 && strlen(comment) > 28)
		{
		/* comment is longer than 28 and no track number,
		 * write a v1.0 tag to enable larger comments
		 */
		strncpy(buf + 97, comment, 30);
		}
	else
		{
		if (comment) strncpy(buf + 97, comment, 28);
		buf[126] = track;
		}

	buf[127] = genre;

	if (debug_mode)printf("Tagline:[%s] g = %d\n", buf, genre);

	if (fwrite(buf, sizeof(gchar), 128, f) < 128)
		{
		printf(_("could not write tag to file: %s\n"), path);
		fclose(f);
		return FALSE;
		}

	fclose(f);
	return TRUE;
}

void free_id3_tag(ID3_Data *d)
{
	if (!d) return;

	g_free(d->title);
	g_free(d->artist);
	g_free(d->album);
	g_free(d->year);
	g_free(d->comment);

	g_free(d);
}
