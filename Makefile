# Created for Debian by peter karlsson <pk@softwolves.pp.se>

DESTDIR =
BINDIR = $(DESTDIR)/usr/bin
DOCDIR = $(DESTDIR)/usr/share/doc/crashmail
MANDIR = $(DESTDIR)/usr/share/man/man1

all:
	make -C src linux

install:
	install -d $(BINDIR)
	install bin/crashmail $(BINDIR)
	install bin/crashstats $(BINDIR)
	install bin/crashlist $(BINDIR)
	install bin/crashgetnode $(BINDIR)
	install bin/crashmaint $(BINDIR)
	install bin/crashwrite $(BINDIR)
	install bin/crashexport $(BINDIR)
	install -d $(MANDIR)
	cp -a man/* $(MANDIR)/
	install -d $(DOCDIR)
	install -m 644 doc/AreafixHelp.txt $(DOCDIR)
	install -m 644 doc/ReadMe.txt $(DOCDIR)
	install -d $(DOCDIR)/examples
	install -m 644 doc/example.prefs $(DOCDIR)/examples

clean:
	make -C src cleanlinux
	-rm bin/*
