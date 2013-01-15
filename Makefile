# Created for Debian by Peter Karlsson <peterk@debian.org>

DESTDIR =

all:
	mkdir bin
	make -C src linux

clean:
	make -C src cleanlinux
	-rm -rf bin
