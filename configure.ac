dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(hinedo-applet,
        0.9,
        [http://github.com/vicamo/Hinedo/issues],
        [hinedo-applet],
        [http://github.com/vicamo/Hinedo])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

LIBPANELAPPLET_MIN_VERSION=2.14
LIBSOUP_MIN_VERSION=2.29.3
JSON_GLIB_MIN_VERSION=0.7.6
GSTREAMER_MIN_VERSION=0.10.25

PKG_CHECK_MODULES(LIBSOUP_GNOME,
		  [libsoup-gnome-2.4 >= 2.26],
		  [LIBSOUP_CFLAGS="$LIBSOUP_GNOME_CFLAGS"
		   LIBSOUP_LIBS="$LIBSOUP_GNOME_LIBS"
		   AC_DEFINE(HAVE_LIBSOUP_GNOME, 1, [Have libsoup-gnome])],
		  [PKG_CHECK_MODULES(LIBSOUP, [libsoup-2.4 >= 2.26])])
AC_SUBST(LIBSOUP_CFLAGS)
AC_SUBST(LIBSOUP_LIBS)

PKG_CHECK_MODULES(LIBPANELAPPLET,
                  [libpanelapplet-2.0 >= $LIBPANELAPPLET_MIN_VERSION])
AC_SUBST(LIBPANELAPPLET_CFLAGS)
AC_SUBST(LIBPANELAPPLET_LIBS)

PKG_CHECK_MODULES(JSON_GLIB,
                  [json-glib-1.0 >= $JSON_GLIB_MIN_VERSION])
AC_SUBST(JSON_GLIB_CFLAGS)
AC_SUBST(JSON_GLIB_LIBS)

PKG_CHECK_MODULES(GSTREAMER,
                  [gstreamer-0.10 >= $GSTREAMER_MIN_VERSION])
AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)

HINEDO_APPLET_CFLAGS="$LIBPANELAPPLET_CFLAGS $LIBSOUP_CFLAGS $JSON_GLIB_CFLAGS $GSTREAMER_CFLAGS"
HINEDO_APPLET_LIBS="$LIBPANELAPPLET_LIBS $LIBSOUP_LIBS $JSON_GLIB_LIBS $GSTREAMER_LIBS"
AC_SUBST(HINEDO_APPLET_CFLAGS)
AC_SUBST(HINEDO_APPLET_LIBS)

AS_AC_EXPAND(LIBEXECDIR, $libexecdir)
AC_SUBST(LIBEXECDIR)


dnl ***************************************************************************
dnl Internatinalization
dnl ***************************************************************************
GETTEXT_PACKAGE=hinedo-applet
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AC_DEFINE_DIR(GNOMELOCALEDIR, "${datadir}/locale", [locale directory])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])


AC_OUTPUT([
Makefile
data/Makefile
src/Makefile
src/hinedo-applet.server.in
po/Makefile.in
])
