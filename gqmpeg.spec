Summary: Frontend to mpg123.
Summary(fr): Interface graphique à mpg123
Name: gqmpeg
Version: 0.91.1
Release: 1
Copyright: GPL
Group: Applications/Multimedia
Source: http://prdownloads.sourceforge.net/gqmpeg/gqmpeg-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root

URL: http://gqmpeg.sourceforge.net

Requires: gtk2 >= 2.2.0

%description
GQmpeg is a frontend to mpg123.
It includes playlist support and all
the usual playing features.
Random, repeat, etc.
And supports custom skins (looks)
Uilitizes the GTK library.
Requires mpg123 v 0.59q

%description -l fr
GQmpeg est une interface graphique à mpg123.
Il inclus le support aux listes de lectures et toutes
les capacités usuelles de lecture: aléatoire, répétition, etc.
Egalement inclus le support des revêtements.
Il utilise la librairie GTK et nécessite mpg123 >= 0.59q

%prep
%setup

%build
if [ ! -f configure ]; then
  CFLAGS="$MYCFLAGS" ./autogen.sh $MYARCH_FLAGS --prefix=%{_prefix}
else
  CFLAGS="$MYCFLAGS" ./configure $MYARCH_FLAGS --prefix=%{_prefix}
fi

make

%install
rm -rf $RPM_BUILD_ROOT

make mandir=$RPM_BUILD_ROOT%{_mandir} bindir=$RPM_BUILD_ROOT%{_bindir} \
 prefix=$RPM_BUILD_ROOT%{_prefix} install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)

%doc README COPYING FAQ TODO SKIN-SPECS SKIN-SPECS-V1 plugin/README.plugin
%{_bindir}/gqmpeg
%{_bindir}/gqmpeg-shoutcast-plugin.sh
%{_datadir}/gqmpeg/*
%{_datadir}/locale/*/*/*
%{_datadir}/applications/gqmpeg.desktop
%{_datadir}/pixmaps/gqmpeg.png
%{_mandir}/man?/*

