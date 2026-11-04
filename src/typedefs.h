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


#ifndef TYPEDEFS_H
#define TYPEDEFS_H

typedef enum
{
	EXEC_NONE = 0,
	EXEC_PLAY,
	EXEC_PAUSE,
	EXEC_CONTINUE,
	EXEC_STOP,
	EXEC_SEEK,
	EXEC_NEXT
} PlayerCommand;

/*
 *----------------------------------------------------------------------------
 * song structure
 *----------------------------------------------------------------------------
 */

typedef enum
{
	SONG_FLAG_PLAYING	= 1 << 1,
	SONG_FLAG_NOT_FOUND	= 1 << 2,
	SONG_FLAG_PLAY_FAIL	= 1 << 3,
	SONG_FLAG_UNKNOWN_TYPE	= 1 << 4,
	SONG_FLAG_CUSTOMIZED	= 1 << 5
} SongFlags;

typedef struct _SongData SongData;
struct _SongData
{
	gchar *path;			/* path or location */

	SongFlags flags;		/* attributes */

	gint type;			/* module type (location in io_list) */
	gchar *type_description;	/* file type description */

	gint custom;			/* if TRUE, not a standard file */
	gint custom_type;		/* location in typelist */

	gint info_loaded;		/* if the title, artist, length, etc. was loaded */

	gchar *title;
	gchar *artist;
	gchar *album;
	gchar *genre;
	gchar *comment;
	gchar *year;
	guint8 track;

	gint length;			/* seconds, -1 for unknown or live */
	gint live;			/* TRUE if live streaming */
	gint bit_rate;			/* for example mp3: 128 = 128Kbits */
	gint bit_depth;			/* usually 8 or 16 */
	gint khz_rate;			/* example: 44 = 44 Khz */
	gint channels;			/* usually 1 or 2, mono or stereo */

	gint customized;		/* whether extra data was loaded from the playlist
					 * or the user customized the data.
					 * used to save it back to the playlist
					 */
	gint custom_title;		/* the customization flags */
	gint custom_artist;
	gint custom_album;
	gint custom_comment;
	gint custom_length;

	gpointer data;			/* slot for misc internal use of io_modules */
	void (* free_data_func)(gpointer);	/* func to free data slot */
};

#endif

