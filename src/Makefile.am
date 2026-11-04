SUBDIRS = default_skin
DIST_SUBDIRS = default_skin

INCLUDES = -I$(srcdir) -I$(builddir)    \
	-I$(top_srcdir) -I$(top_builddir)       \
	-DLOCALEDIR=\""$(localedir)"\"		\
	-I$(includedir) $(GTK_CFLAGS)

module_SLIK = \
	ui2_button.c	\
	ui2_button.h	\
	ui2_button_edit.c	\
	ui2_button_edit.h	\
	ui2_decal.c	\
	ui2_decal.h	\
	ui2_decal_edit.c	\
	ui2_decal_edit.h	\
	ui2_dial.c	\
	ui2_dial.h	\
	ui2_dial_edit.c	\
	ui2_dial_edit.h	\
	ui2_display.c	\
	ui2_display.h	\
	ui2_editor.c	\
	ui2_editor.h	\
	ui2_includes.h	\
	ui2_item.c	\
	ui2_item.h	\
	ui2_item_edit.c	\
	ui2_item_edit.h	\
	ui2_list.c	\
	ui2_list.h	\
	ui2_list_edit.c	\
	ui2_list_edit.h	\
	ui2_logo.h	\
	ui2_main.c	\
	ui2_main.h	\
	ui2_menu.c	\
	ui2_menu.h	\
	ui2_number.c	\
	ui2_number.h	\
	ui2_number_edit.c	\
	ui2_number_edit.h	\
	ui2_parse.c	\
	ui2_parse.h	\
	ui2_skin.c	\
	ui2_skin.h	\
	ui2_slider.c	\
	ui2_slider.h	\
	ui2_slider_edit.c	\
	ui2_slider_edit.h	\
	ui2_text.c	\
	ui2_text.h	\
	ui2_text_edit.c	\
	ui2_text_edit.h	\
	ui2_typedefs.h	\
	ui2_util.c	\
	ui2_util.h	\
	ui2_widget.c	\
	ui2_widget.h	\
	ui_bookmark.c	\
	ui_bookmark.h	\
	ui_fileops.c	\
	ui_fileops.h	\
	ui_help.c	\
	ui_help.h	\
	ui_menu.c	\
	ui_menu.h	\
	ui_pathsel.c	\
	ui_pathsel.h	\
	ui_pixbuf_ops.c	\
	ui_pixbuf_ops.h	\
	ui_pixbuf_save.c	\
	ui_pixbuf_save.h	\
	ui_spinner.c	\
	ui_spinner.h	\
	ui_tabcomp.c	\
	ui_tabcomp.h	\
	ui_tree_edit.c	\
	ui_tree_edit.h	\
	ui_utildlg.c	\
	ui_utildlg.h

extra_ICONS = \
	ui_spinner.png	\
	ui_tabcomp.png

extra_SLIK = \
	$(extra_ICONS)

ICON_PAIRS = \
	icon_spinner		$(srcdir)/ui_spinner.png	\
	icon_tabcomp		$(srcdir)/ui_tabcomp.png

ui_icons.h: $(extra_ICONS)
	gdk-pixbuf-csource --raw --extern --build-list $(ICON_PAIRS) > ui_icons.h

ui_spinner.c: ui_icons.h

noinst_DATA = ui_icons.h
CLEANFILES = $(noinst_DATA)

module_radio = \
	io_radio.c	\
	io_radio.h	\
	io_radio_cfg.c

module_ogg123 = \
	ogg_info.c	\
	io_ogg123.c	\
	io_ogg123.h	\
	io_ogg123_cfg.c

module_xmp = \
	io_xmp.c	\
	io_xmp.h	\
	io_xmp_cfg.c	\
	mod_info.c

module_mpg123 = \
	io_mpg123.c	\
	io_mpg123.h	\
	io_mpg123_cfg.c	\
	mpg_hdrutil.c	\
	mpg_info.c	\
	mpg_tagutil.c

bin_PROGRAMS = gqmpeg

gqmpeg_SOURCES = \
	$(module_SLIK)	\
	btn_funcs.c	\
	btn_funcs.h	\
	cpu_perc.c	\
	cpu_perc.h	\
	display.c	\
	display.h	\
	display_flyby.c	\
	display_flyby.h	\
	display_menu.c	\
	display_menu.h	\
	dndutil.c	\
	dndutil.h	\
	dock.c		\
	dock.h		\
	filelist.c	\
	filelist.h	\
	globals.c	\
	gqmpeg.h	\
	intl.h		\
	ipc.c		\
	ipc.h		\
	japanese_tag.c	\
	japanese_tag.h	\
	main.c		\
	menus.c		\
	menus.h		\
	mixer.c		\
	mixer.h		\
	players.c	\
	players.h	\
	playlist-dlg.c	\
	playlist-dlg.h	\
	playlist-window.c	\
	playlist-window.h	\
	playlist.c	\
	playlist.h	\
	preferences.c	\
	preferences.h	\
	preset-dlg.c	\
	preset-dlg.h	\
	rcfile.c	\
	rcfile.h	\
	russian_tag.c	\
	russian_tag.h	\
	skin_compat.c	\
	skin_compat.h	\
	skin_default.c	\
	skin_default.h	\
	skin_wamp.c	\
	skin_wamp.h	\
	songinfo.c	\
	songinfo.h	\
	typedefs.h	\
	types.c		\
	types.h		\
	utilops.c	\
	utilops.h	\
	window.c	\
	window.h	\
	$(module_mpg123) $(module_xmp) $(module_ogg123) $(module_radio)

gqmpeg_LDADD = $(GTK_LIBS) $(LIBPNG)

EXTRA_DIST = \
	$(extra_SLIK)	\
	icon.xpm	\
	icon-alert.xpm	\
	icon-custom.xpm	\
	icon-question.xpm	\
	icon-rarrow.xpm

