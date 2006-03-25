
sysconfdir = /etc
localedir = /usr/share/locale
version := $(shell dpkg-parsechangelog | grep '^Version' | cut -b10-)

CC = gcc -std=gnu99
CFLAGS = -g -Wall -DENABLE_L_DEBUG -D_GNU_SOURCE -DSYSCONF=\"$(sysconfdir)\" \
         -DHAVE_GETTEXT -DPACKAGE=\"dctrl-tools\" -DLOCALEDIR=\"$(localedir)\" 

CFLAGS += -DVERSION=\"$(version)\"
CFLAGS += -DMAINTAINER='"$(shell grep ^Maintainer: debian/control | cut -b13-)"'

#CFLAGS += -DNDEBUG

#CFLAGS += -pg
#LDFLAGS += -pg

libobj = misc.o msg.o predicate.o util.o fsaf.o paragraph.o \
         fieldtrie.o rc.o strutil.o getaline.o fnutil.o para_pool.o \
	 ifile.o para_bundle.o sorter.o version.o

obj = $(libobj) grep-dctrl.o sort-dctrl.o tbl-dctrl.o
src = $(obj:.o=.c)

LDLIBS = -L. -ldctrl

# List of translated languages is given in langs.mk
include langs.mk

all :	grep-dctrl sort-dctrl tbl-dctrl sync-available \
	grep-dctrl.1 sort-dctrl.1 mo

pot : po/dctrl-tools.pot

po : $(foreach f,$(langs),po/$(f).po)

mo : $(foreach f,$(langs),po/$(f).mo)

grep-dctrl : grep-dctrl.o libdctrl.a

sort-dctrl : sort-dctrl.o libdctrl.a

tbl-dctrl : tbl-dctrl.o libdctrl.a

% : %.o
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

%.d: %.c
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	   sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	   rm -f $@.$$$$


libdctrl.a : $(libobj)
	ar cr $@ $^
	ranlib $@

libdctrl.so : $(soobj)
	$(LD) -shared -o $@ $^ -lc $(SOLDLIBS)

%.test : %.test.o

%.test.o : %.c
	$(CC) -c $(CFLAGS) -DTESTMAIN $< -o $@

so/%.o : %.c
	$(CC) -fPIC $(CFLAGS) -c $< -o $@

%.1 : %.1.cp
	sed 's*SYSCONF*$(sysconf)*' $< > $@

sync-available : sync-available.cp
	sed 's*VERSION*$(version)*' $< > $@
	chmod 755 $@

xgettext_opts=--copyright-holder="Antti-Juhani Kaijanaho" \
	      --msgid-bugs-address="ajk@debian.org" -kN_ -k_

po/%.po : po/dctrl-tools.pot
	msgmerge -q -U --backup=existing $@ $^

po/%.mo : po/%.po
	msgfmt -c --statistics -o $@ $< 

po/dctrl-tools.pot : $(src)
	xgettext $(xgettext_opts) -d grep-dctrl $^
	mv grep-dctrl.po $@

fsaf.test : fsaf.test.o msg.o

clean :
	$(RM) core grep-dctrl grep-dctrl.1 *.o so/*.o libdctrl.a libdctrl.so
	$(RM) po/*.mo TAGS *.d
	$(RM) sync-available

distclean : clean

maintainer-clean : distclean
	$(RM) po/*.pot

tags :
	etags *.[hc]

include $(obj:.o=.d)