# Created for Debian by Peter Karlsson <peterk@debian.org>

DESTDIR =
DATADIR = $(DESTDIR)/usr/share/crashmail

all:
	mkdir bin
	make -C src linux

install:
	install -d $(DATADIR)
	install -m 644 doc/AreafixHelp.txt $(DATADIR)

clean:
	make -C src cleanlinux
	-rm -rf bin
