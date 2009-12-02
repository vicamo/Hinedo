dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(hinedo-applet, 0.9)

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC




PKG_CHECK_MODULES(HINEDO_APPLET, [libpanelapplet-2.0 >= 2.14    ])


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
src/Makefile
src/hinedo-applet.server.in
po/Makefile.in
])
