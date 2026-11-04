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
#include "dock.h"

#include <gdk/gdkx.h>
#include <X11/Xlib.h>

/*
 *-----------------------------------------------------------------------------
 * these are the dock routines, for embedding into other programs, panels, etc.
 *-----------------------------------------------------------------------------
 */ 

void windowmaker_setup_dock(void)
{
	Display *dpy;
	Window win;
	XWMHints *hints;
	XEvent event;

	dpy = GDK_DISPLAY();
	/* Get the X Window corresponding to mainwindow */
	win = GDK_WINDOW_XWINDOW(main_window->window->window);

	/* Find our window group leader */
	hints = XGetWMHints(dpy, win);
	if( hints == NULL || !(hints->flags & WindowGroupHint) )
		{
		printf("Could not get XWMHints of mainwindow. Docking disabled!\n");
		dock_mode = FALSE;
		return;
		}

	/* Set the size of the mainwindow. This is needed because Window Maker
	 * uses the size to center the window on the tile but gtk sets the
	 * size only when the widget is realized, but this also maps the
	 * window, and we have to map the leader as the first window. */
	XResizeWindow(dpy, win, main_window->skin->width, main_window->skin->height);

	/* Set WM hints of the leader */
	/* Set the window group hint on the leader too. (This is needed!) */
	hints->flags = StateHint|WindowGroupHint|IconWindowHint;
	hints->initial_state = WithdrawnState; /* Initial state is withdrawn */
	hints->icon_window = win;   /* Use the mainwindow as the icon window */
	XSetWMHints(dpy, hints->window_group, hints);

	/* Map the leader as our first window, before mainwindow. The leader
	 * will not stay mapped because of the withdrawn initial state. */
	XMapWindow(dpy, hints->window_group);
	XFree(hints);

	/* Wait until our window is mapped by WM */
	do {
		XWindowEvent(dpy, win, StructureNotifyMask, &event);
	   } while (event.type != MapNotify);
}

