## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS =                                                  \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"                          \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"                        \
	$(HINEDO_APPLET_CFLAGS)

AM_CFLAGS = \
	 -Wall  \
	 -g

libexec_PROGRAMS = hinedo-applet

hinedo_applet_SOURCES = \
	hinedo-applet.c     \
	main.c

hinedo_applet_LDFLAGS = \
	-Wl,--export-dynamic

hinedo_applet_LDADD = $(HINEDO_APPLET_LIBS)

serverdir       = $(libdir)/bonobo/servers
server_in_files = hinedo-applet.server.in
server_DATA     = $(server_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

uidir = $(datadir)/hinedo-applet/ui
ui_DATA = applet-menu.xml

CLEANFILES = $(server_in_files) $(server_DATA)

EXTRA_DIST =                   \
	hinedo-applet.server.in.in \
	$(ui_DATA)


%.server: %.server.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po)
	LC_ALL=C $(INTLTOOL_MERGE) -s -u -c $(top_buildir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@


# Display a message at the end of install
install-data-hook:
	@echo "***"
	@echo "*** If you have installed your applet in a non standard directory. You have to"
	@echo "*** add it in the bonobo server list, so it can find your applet."
	@echo "***"
	@echo "*** Run 'bonobo-activation-sysconf --add-directory=$(serverdir)'"
	@echo "*** Log out from your current GNOME session"
	@echo "*** Run 'bonobo-slay'"
	@echo "*** Check that the bonobo server is not running"
	@echo "*** Log in again, your applet should be available"
	@echo "***"
