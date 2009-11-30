VER=0.4
GTK2_CFLAGS=`pkg-config gtk+-2.0 --cflags`
GTK2_LIBS=`pkg-config gtk+-2.0 --libs`

OBJS=hinedo.o
PREFIX=$(DESTDIR)/usr

all: $(OBJS)
	cc -o hinedo $(GTK2_LIBS) $(OBJS)

hinedo.o: hinedo.c
	cc -Wall -c -DPREFIX=\"$(PREFIX)\" -DVERSION=\"$(VER)\" $(GTK2_CFLAGS) hinedo.c

clean:
	rm -f hinedo
	rm -f *.o

install: all
	install -s hinedo $(PREFIX)/bin/hinedo
	mkdir -p $(PREFIX)/lib/hinedo
	mkdir -p $(PREFIX)/share/applications
	mkdir -p $(PREFIX)/share/pixmaps
	install hinedo.desktop $(PREFIX)/share/applications/hinedo.desktop
	install hinedo.png $(PREFIX)/share/pixmaps/hinedo.png
	install update $(PREFIX)/lib/hinedo/update

uninstall:
	rm $(PREFIX)/bin/hinedo
	rm -rf $(PREFIX)/lib/hinedo
	rm $(PREFIX)/share/pixmaps/hinedo.png
	rm $(PREFIX)/share/applications/hinedo.desktop

dist:
	rm -rf hinedo-$(VER)
	mkdir hinedo-$(VER)
	cp *.c *.h update *.desktop *.png README COPYING Makefile hinedo-$(VER)
	tar --bzip2 -cf hinedo-$(VER).tar.bz2 hinedo-$(VER)
	rm -rf hinedo-$(VER)

