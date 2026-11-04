/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "config.h"
#include "ui_pixbuf_save.h"


#ifdef HAVE_LIBPNG
#include <png.h>	/* for png saving, below */
#endif


/*
 *-----------------------------------------------------------------------------
 * simple png save (please read comment)
 *-----------------------------------------------------------------------------
 */

/*
 * This save_pixbuf_to_file utility was copied from the nautilus 0.1 preview,
 * and judging by the message they got it from gnome-iconedit.
 *
 * I have only changed the source to match my coding style in GQview,
 * make it work in here, and rename to pixbuf_to_file_as_png
 *                                                                   -John
 *
 * === message in nautilus source:
 * utility routine for saving a pixbuf to a png file.
 * This was adapted from Iain Holmes' code in gnome-iconedit, and probably
 * should be in a utility library, possibly in gdk-pixbuf itself.
 * ===
 */
gboolean pixbuf_to_file_as_png (GdkPixbuf *pixbuf, char *filename)
{
#ifdef HAVE_LIBPNG

	FILE *handle;
  	char *buffer;
	gboolean has_alpha;
	int width, height, depth, rowstride;
  	guchar *pixels;
  	png_structp png_ptr;
  	png_infop info_ptr;
  	png_text text[2];
  	int i;

	g_return_val_if_fail (pixbuf != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (filename[0] != '\0', FALSE);

        handle = fopen (filename, "wb");
        if (handle == NULL)
		{
        	return FALSE;
		}

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
		{
		fclose (handle);
		return FALSE;
		}

	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL)
		{
		png_destroy_write_struct (&png_ptr, (png_infopp)NULL);
		fclose (handle);
	    	return FALSE;
		}

	if (setjmp (png_ptr->jmpbuf))
		{
		png_destroy_write_struct (&png_ptr, &info_ptr);
		fclose (handle);
		return FALSE;
		}

	png_init_io (png_ptr, handle);

        has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	depth = gdk_pixbuf_get_bits_per_sample (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	png_set_IHDR (png_ptr, info_ptr, width, height,
			depth, PNG_COLOR_TYPE_RGB_ALPHA,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);

	/* Some text to go with the png image */
	text[0].key = "Title";
	text[0].text = filename;
	text[0].compression = PNG_TEXT_COMPRESSION_NONE;
	text[1].key = "Software";
	text[1].text = "GQview Thumbnail";
	text[1].compression = PNG_TEXT_COMPRESSION_NONE;
	png_set_text (png_ptr, info_ptr, text, 2);

	/* Write header data */
	png_write_info (png_ptr, info_ptr);

	/* if there is no alpha in the data, allocate buffer to expand into */
	if (has_alpha)
		{
		buffer = NULL;
		}
	else
		{
		buffer = g_malloc(4 * width);
		}
	
	/* pump the raster data into libpng, one scan line at a time */	
	for (i = 0; i < height; i++)
		{
		if (has_alpha)
			{
			png_bytep row_pointer = pixels;
			png_write_row (png_ptr, row_pointer);
			}
		else
			{
			/* expand RGB to RGBA using an opaque alpha value */
			int x;
			char *buffer_ptr = buffer;
			char *source_ptr = pixels;
			for (x = 0; x < width; x++)
				{
				*buffer_ptr++ = *source_ptr++;
				*buffer_ptr++ = *source_ptr++;
				*buffer_ptr++ = *source_ptr++;
				*buffer_ptr++ = 255;
				}
			png_write_row (png_ptr, (png_bytep) buffer);		
			}
		pixels += rowstride;
		}
	
	png_write_end (png_ptr, info_ptr);
	png_destroy_write_struct (&png_ptr, &info_ptr);
	
	g_free (buffer);
		
	fclose (handle);
	return TRUE;
#else
	printf("PNG saving support not enabled during compile.\n");
	return FALSE;

#endif
}




