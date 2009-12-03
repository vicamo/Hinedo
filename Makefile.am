## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = data src po

ACLOCAL_AMFLAGS = \
	-I m4

hinedo_appletdocdir = ${prefix}/doc/hinedo-applet
hinedo_appletdoc_DATA = \
	README              \
	COPYING             \
	AUTHORS             \
	ChangeLog           \
	INSTALL             \
	NEWS


INTLTOOL_FILES =        \
	intltool-extract.in \
	intltool-merge.in   \
	intltool-update.in

EXTRA_DIST =                 \
	$(hinedo_appletdoc_DATA) \
	$(INTLTOOL_FILES)

DISTCLEANFILES =     \
	intltool-extract \
	intltool-merge   \
	intltool-update  \
	po/.intltool-merge-cache


