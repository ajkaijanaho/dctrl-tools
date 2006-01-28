
sysconf = /etc

CC = gcc -std=gnu99 
CFLAGS = -O2 -g -Wall -DENABLE_L_DEBUG -D_GNU_SOURCE -DSYSCONF=\"$(sysconf)\"

CFLAGS += -DVERSION=\"$(shell dpkg-parsechangelog | grep '^Version' | cut -b10-)\"
CFLAGS += -DMAINTAINER='"$(shell grep ^Maintainer: debian/control | cut -b13-)"'

#CFLAGS += -pg
#LDFLAGS += -pg

LDLIBS = -lpub

all : grep-dctrl grep-dctrl.1

grep-dctrl : grep-dctrl.o msg.o predicate.o util.o fsaf.o paragraph.o \
	     fieldtrie.o rc.o strutil.o

%.test : %.test.o

%.test.o : %.c
	$(CC) -c $(CFLAGS) -DTESTMAIN $< -o $@

%.1 : %.1.cp
	sed 's*SYSCONF*$(sysconf)*' $< > $@

fsaf.test : fsaf.test.o msg.o

clean :
	$(RM) core grep-dctrl grep-dctrl.1 *.o
