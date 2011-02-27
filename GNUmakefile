
version := $(shell dpkg-parsechangelog | grep '^Version' | cut -b10-)
maintainer := "$(shell grep ^Maintainer: debian/control | cut -b13-)"
prefix = /usr/local
exec_prefix = $(prefix)
sysconfdir = $(prefix)/etc
bindir = $(exec_prefix)/bin
sbindir = $(exec_prefix)/sbin
datarootdir = $(prefix)/share
docdir = $(datarootdir)/doc/dctrl-tools
mandir = $(datarootdir)/man
man1dir = $(mandir)/man1
man8dir = $(mandir)/man8
localedir = $(datarootdir)/locale

CC = gcc 
CFLAGS = -g -Wall -Werror
ALL_CFLAGS = $(CFLAGS) -std=gnu99 -Ilib \
	 -DENABLE_L_DEBUG -D_GNU_SOURCE -DSYSCONF=\"$(sysconfdir)\" \
         -DHAVE_GETTEXT -DPACKAGE=\"dctrl-tools\" -DLOCALEDIR=\"$(localedir)\" 

ALL_CFLAGS += -DVERSION=\"$(version)\"
ALL_CFLAGS += -DMAINTAINER='$(maintainer)'

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m644
INSTALL_DIR = $(INSTALL) -d

PO4A = po4a
PO4A_CONFIG = man/po4a/po4a.cfg

libsrc = $(wildcard lib/*.c)
libobj = $(libsrc:.c=.o)

src = $(libsrc) \
      $(wildcard grep-dctrl/*.c) \
      $(wildcard sort-dctrl/*.c) \
      $(wildcard tbl-dctrl/*.c) \
      $(wildcard join-dctrl/*.c)

obj = $(src:.c=.o)

exe =  	grep-dctrl/grep-dctrl \
	sort-dctrl/sort-dctrl \
	tbl-dctrl/tbl-dctrl \
	join-dctrl/join-dctrl

LDLIBS = -L. -ldctrl

# List of translated languages is given in langs.mk
include langs.mk

all :	all-no-mo mo translated-man

all-no-mo :	sync-available/sync-available \
		man/grep-dctrl.1 \
		$(exe)
aliases = grep-status grep-available grep-aptavail grep-debtags

install :
	$(INSTALL_DIR) $(DESTDIR)$(sysconfdir)
	$(INSTALL_DIR) $(DESTDIR)$(sbindir)
	$(INSTALL_DIR) $(DESTDIR)$(bindir)
	$(INSTALL_DIR) $(DESTDIR)$(docdir)
	$(INSTALL_DIR) $(DESTDIR)$(man1dir)
	$(INSTALL_DIR) $(DESTDIR)$(man8dir)
	$(INSTALL_DATA) grep-dctrl/grep-dctrl.rc $(DESTDIR)$(sysconfdir)
	$(INSTALL_DATA) grep-dctrl/grep-dctrl.rc $(DESTDIR)$(sysconfdir)
	$(INSTALL_PROGRAM) sync-available/sync-available $(DESTDIR)$(sbindir)
	$(INSTALL_PROGRAM) join-dctrl/join-dctrl $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) tbl-dctrl/tbl-dctrl $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) sort-dctrl/sort-dctrl $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) grep-dctrl/grep-dctrl $(DESTDIR)$(bindir)
	set -e ; for dest in $(aliases) ; do \
		ln -s grep-dctrl $(DESTDIR)$(bindir)/$$dest ; \
	 done
	$(INSTALL_DATA) man/sync-available.8 $(DESTDIR)$(man8dir)/
	gzip -9 $(DESTDIR)$(man8dir)/sync-available.8
	$(INSTALL_DATA) man/sort-dctrl.1 $(DESTDIR)$(man1dir)/
	gzip -9 $(DESTDIR)$(man1dir)/sort-dctrl.1
	$(INSTALL_DATA) man/tbl-dctrl.1 $(DESTDIR)$(man1dir)/
	gzip -9 $(DESTDIR)$(man1dir)/tbl-dctrl.1
	$(INSTALL_DATA) man/join-dctrl.1 $(DESTDIR)$(man1dir)/
	gzip -9 $(DESTDIR)$(man1dir)/join-dctrl.1
	$(INSTALL_DATA) man/grep-dctrl.1 $(DESTDIR)$(man1dir)/
	gzip -9 $(DESTDIR)$(man1dir)/grep-dctrl.1
	set -e ; for dest in $(aliases) ; do \
		ln -s grep-dctrl.1.gz $(DESTDIR)$(man1dir)/$$dest.1.gz ; \
	done
	set -e ; for d in man/translated/*; do \
		lang=`echo $$d | cut -c16-`; \
		if [ -e $$d/sync-available.8 ]; then \
			$(INSTALL_DIR) $(DESTDIR)$(mandir)/$$lang/man8; \
			$(INSTALL_DATA) $$d/sync-available.8 $(DESTDIR)$(mandir)/$$lang/man8/; \
			gzip -9 $(DESTDIR)$(mandir)/$$lang/man8/sync-available.8; \
		fi; \
		for file in sort-dctrl.1 tbl-dctrl.1 join-dctrl.1 grep-dctrl.1; do \
			if [ -e $$d/$$file ]; then \
				if ! [ -d $(DESTDIR)$(mandir)/$$lang/man1 ]; then \
					$(INSTALL_DIR) $(DESTDIR)$(mandir)/$$lang/man1; \
				fi; \
				$(INSTALL_DATA) $$d/$$file $(DESTDIR)$(mandir)/$$lang/man1/; \
				gzip -9 $(DESTDIR)$(mandir)/$$lang/man1/$$file; \
			fi; \
		done; \
		if [ -e $(DESTDIR)$(mandir)$$lang/man1/grep-dctrl.1.gz ]; then \
			for dest in $(aliases) ; do \
				ln -s grep-dctrl.1.gz $(DESTDIR)$(mandir)/$$lang/man1/$$dest.1.gz ; \
			done; \
		fi; \
	done;
	$(INSTALL_DATA) TODO README $(DESTDIR)$(docdir)
	set -e ; for lang in $(langs) ; do \
		$(INSTALL_DIR) $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES ; \
		$(INSTALL_DATA) po/$$lang.mo \
		  $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/dctrl-tools.mo ; \
	done


pot : po/dctrl-tools.pot man/po4a/po/dctrl-tools-man.pot

po : $(foreach f,$(langs),po/$(f).po)

mo : $(foreach f,$(langs),po/$(f).mo)

grep-dctrl/grep-dctrl : grep-dctrl/grep-dctrl.o grep-dctrl/rc.o libdctrl.a

sort-dctrl/sort-dctrl : sort-dctrl/sort-dctrl.o libdctrl.a

tbl-dctrl/tbl-dctrl : tbl-dctrl/tbl-dctrl.o libdctrl.a

join-dctrl/join-dctrl : join-dctrl/join-dctrl.o libdctrl.a

% : %.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o : %.c
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

%.d: %.c
	$(CC) -M $(ALL_CFLAGS) $< > $@.$$$$; \
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
	sed 's*SYSCONF*$(sysconfdir)*' $< > $@

sync-available/sync-available : sync-available/sync-available.cp
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

# create (or update) dctrl-tools-man.pot
man/po4a/po/dctrl-tools-man.pot :
	touch man/po4a/po/dctrl-tools-man.pot
	$(PO4A) --force --no-translations $(PO4A_CONFIG)

# build translated manpages in man/translated/$lang/
po4a :
	touch man/po4a/po/dctrl-tools-man.pot
	$(PO4A) --force --no-backups $(PO4A_CONFIG)

# handle the "%.1 : %.1.cp" rule if and only if the grep-dctrl.1.cp file exists
translated-man : po4a
	set -e ; for d in man/translated/*; do \
		if [ -e $$d/grep-dctrl.1.cp ]; then \
			sed 's*SYSCONF*$(sysconfdir)*' \
			$$d/grep-dctrl.1.cp > $$d/grep-dctrl.1; \
		fi; \
       done

fsaf.test : fsaf.test.o msg.o

test :	all-no-mo
	sh ./tester.sh

clean :
	$(RM) core $(exe) man/grep-dctrl.1 $(obj) so/*.o libdctrl.a libdctrl.so
	$(RM) po/*.mo TAGS *.d */*.d
	$(RM) sync-available/sync-available
	$(RM) -r man/translated

distclean : clean

maintainer-clean : distclean
	$(RM) po/*.pot man/po4a/po/*.pot

tags :
	etags *.[hc]

ifeq ($(MAKECMDGOALS),clean)
else ifeq ($(MAKECMDGOALS),distclean)
else ifeq  ($(MAKECMDGOALS),maintainer-clean)
else
include $(obj:.o=.d)
endif
