
sysconfdir = /etc
localedir = /usr/share/locale
version := $(shell dpkg-parsechangelog | grep '^Version' | cut -b10-)

CC = gcc -std=gnu99 
CFLAGS = -O2 -g -Wall -DENABLE_L_DEBUG -D_GNU_SOURCE -DSYSCONF=\"$(sysconfdir)\" \
         -DHAVE_GETTEXT -DPACKAGE=\"grep-dctrl\" -DLOCALEDIR=\"$(localedir)\" 

CFLAGS += -DVERSION=\"$(version)\"
CFLAGS += -DMAINTAINER='"$(shell grep ^Maintainer: debian/control | cut -b13-)"'

#CFLAGS += -pg
#LDFLAGS += -pg

LDLIBS = -lgmp

obj = grep-dctrl.o misc.o msg.o predicate.o util.o fsaf.o paragraph.o \
      fieldtrie.o rc.o strutil.o getaline.o fnutil.o
src = $(obj:.o=.c)

# List of translated languages is given in langs.mk
include langs.mk

all : grep-dctrl grep-dctrl.1 sync-available mo

pot : po/grep-dctrl.pot

po : $(foreach f,$(langs),po/$(f).po)

mo : $(foreach f,$(langs),po/$(f).mo)

grep-dctrl : $(obj)

%.test : %.test.o

%.test.o : %.c
	$(CC) -c $(CFLAGS) -DTESTMAIN $< -o $@

%.1 : %.1.cp
	sed 's*SYSCONF*$(sysconf)*' $< > $@

sync-available : sync-available.cp
	sed 's*VERSION*$(version)*' $< > $@
	chmod 755 $@

xgettext_opts=--copyright-holder="Antti-Juhani Kaijanaho" \
	      --msgid-bugs-address="ajk@debian.org" -kN_ -k_

po/%.po : po/grep-dctrl.pot
	msgmerge -q -U --backup=existing $@ $^

po/%.mo : po/%.po
	msgfmt -c --statistics -o $@ $< 

po/grep-dctrl.pot : $(src)
	xgettext $(xgettext_opts) -d grep-dctrl $^
	mv grep-dctrl.po $@

fsaf.test : fsaf.test.o msg.o

clean :
	$(RM) core grep-dctrl grep-dctrl.1 *.o po/*.mo po/*.pot TAGS
	$(RM) sync-available

tags :
	etags *.[hc]
