include Makefile.inc

prefix = /usr
bindir = $(prefix)/bin
mandir = $(prefix)/share/man

NLDEFS = -DNODELIST_CMNL -DNODELIST_V7P
MBDEFS_MSG = -DMSGBASE_MSG
MBDEFS_JAM = -DMSGBASE_JAM
MBDEFS = $(MBDEFS_MSG) $(MBDEFS_JAM)

CPPFLAGS += $(MBDEFS) $(NLDEFS)

NLOBJS = \
       crashmail/nl_cmnl.o \
       crashmail/nl_v7p.o \

SHAREDOBJS = \
	shared/expr.o \
	shared/jblist.o \
	shared/jbstrcpy.o \
	shared/mystrncpy.o \
	shared/node4d.o \
	shared/parseargs.o \
	shared/path.o

OBJS = $(SHAREDOBJS) $(NLOBJS) \
       crashmail/areafix.o \
       crashmail/config.o \
       crashmail/crashmail.o \
       crashmail/dupe.o \
       crashmail/filter.o \
       crashmail/handle.o \
       crashmail/logwrite.o \
       crashmail/mb.o \
       crashmail/mb_jam.o \
       crashmail/mb_msg.o \
       crashmail/memmessage.o \
       crashmail/misc.o \
       crashmail/nl.o \
       crashmail/node4dpat.o \
       crashmail/outbound.o \
       crashmail/pkt.o \
       crashmail/safedel.o \
       crashmail/scan.o \
       crashmail/stats.o \
       crashmail/toss.o

JAMLIB = jamlib/jamlib.a 
CMNLLIB = cmnllib/cmnllib.a
OSLIB = oslib_linux/oslib.a

EXE = bin/crashmail \
      bin/crashlist \
      bin/crashstats \
      bin/crashgetnode \
      bin/crashmaint \
      bin/crashwrite \
      bin/crashexport \
      bin/crashlistout

MAN = \
      man/crashmaint.1 \
      man/crashstats.1 \
      man/crashgetnode.1 \
      man/crashlist.1 \
      man/crashexport.1 \
      man/crashwrite.1 \
      man/crashmail.1

all: $(EXE)

install: all
	$(INSTALL) -d -m 755 $(DESTDIR)$(bindir)
	$(INSTALL) -d -m 755 $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 755 $(EXE) $(DESTDIR)$(bindir)
	$(INSTALL) -m 644 $(MAN) $(DESTDIR)$(mandir)/man1

bin:
	mkdir bin

$(EXE): bin $(OBJS) $(JAMLIB) $(CMNLLIB) $(OSLIB)
	@echo Linking $@.
	@$(CC) -o $@ $(OBJS) $(JAMLIB) $(CMNLLIB) $(OSLIB)

.PHONY: jamlib/jamlib.a
jamlib/jamlib.a:
	$(MAKE) -C jamlib

.PHONY: cmnllib/cmnllib.a
cmnllib/cmnllib.a:
	$(MAKE) -C cmnllib

.PHONY: oslib_linux/oslib.a
oslib_linux/oslib.a:
	$(MAKE) -C oslib_linux

clean:
	rm -f $(OBJS)
	$(MAKE) -C jamlib clean
	$(MAKE) -C cmnllib clean
	$(MAKE) -C oslib_linux clean

