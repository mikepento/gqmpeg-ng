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

#ifndef UI2_DISPLAY_H
#define UI2_DISPLAY_H


void ui_display_render_area(UIData *ui, gint x, gint y, gint w, gint h, WidgetData *wd);
void ui_display_redraw_area(UIData *ui, gint x, gint y, gint w, gint h, gint update);

void ui_display_draw_focus(UIData *ui, GdkPixbuf *pixbuf, gint x, gint y, gint w, gint h,
			   gint clip_x, gint clip_y, gint clip_w, gint clip_h, GdkPixbuf *clip_pb);

void ui_display_sync_shape(UIData *ui);
void ui_display_sync(UIData *ui, gint update);
void ui_display_sync_all(UIData *ui);

void ui_display_events_init(UIData *ui);


#endif


