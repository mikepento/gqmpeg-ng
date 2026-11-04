## Process this file with automake to produce Makefile.in.

SUBDIRS = m4 intl src plugin skins po
DIST_SUBDIRS = m4 intl src plugin skins po

man_MANS = gqmpeg.1

EXTRA_DIST = mkinstalldirs  config.rpath  \
	gqmpeg.desktop	\
	gqmpeg.png		\
	gqmpeg.spec.in		\
	gqmpeg.1		\
	ChangeLog FAQ		\
	SKIN-SPECS		\
	SKIN-SPECS-V1

#create the empty skins directory
skindir = $(prefix)/share/gqmpeg/skins
skin_DATA = SKIN-SPECS README

desktopdir = $(datadir)/applications
desktop_DATA = gqmpeg.desktop

icondir = $(datadir)/pixmaps
icon_DATA = gqmpeg.png

dist-hook: gqmpeg.spec
	cp $(top_builddir)/gqmpeg.spec $(distdir)

ACLOCAL_AMFLAGS = -I m4
