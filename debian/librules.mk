# librules.mk - a library of convenient rules and macros for debian/rules files
#
# Copyright © 1999, 2000, 2002, 2003 Antti-Juhani Kaijanaho.
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this file, to deal in this file without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of this file, and to permit persons to whom this file is furnished
# to do so, subject to the following condition: The above copyright
# notice and this permission notice shall be included in all copies or
# substantial portions of this file.
#
# THIS FILE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FIT- NESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT.  IN NO EVENT SHALL SOFTWARE IN THE PUBLIC INTEREST,
# INC.  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
# IN CONNECTION WITH THIS FILE OR THE USE OR OTHER DEALINGS IN THIS
# FILE.
#
# Except as contained in this notice, the name of the author(s) of
# this file shall not be used in advertising or otherwise to promote
# the sale, use or other dealings in this file without prior written
# authorization from the author(s).

# This file is set up to be compliant with Debian Standards Version
# 3.6.0.

# Changes:
#  2003-08-10 ajk  Add $(etcdir)
#  2002-10-16 ajk  Don't force installing the prerm/postinst scripts
#                  Instead, install them if present.
#                  INCOMPATIBLE CHANGE; updated interface to 2

default:
	@echo You need to specify a target.
	@exit 1

librules_a_variable_just_to_run_the_commands1 := $(shell mkdir debian/stamp)
librules_a_variable_just_to_run_the_commands2 := $(shell mkdir debian/stamp/binary)

# Make sure we don't get used by an incompatible debian/rules
# WHEN YOU MAKE INCOMPATIBLE CHANGES, EDIT THIS!
ifneq ($(librules_interface),2)
$(error incompatible debian/rules)
endif

# Standard interface targets
build: debian/stamp/build
binary: binary-indep binary-arch
binary-arch: debian/stamp/binary/arch
binary-indep: debian/stamp/binary/indep
clean: clean-build clean-binary clean-std


# Nonstandard interface targets
clean-build: clean-build-std
clean-binary: clean-binary-std

# Stamp targets for the standard binary targets
debian/stamp/binary/arch: debian/stamp/build
	touch $@

debian/stamp/binary/indep: debian/stamp/build
	touch $@

.PHONY: default build binary binary-arch binary-indep \
	clean clean-build clean-binary

ifeq ($(librules_need_archvars),yes)
DEB_BUILD_ARCH       = $(shell dpkg-architecture -qDEB_BUILD_ARCH)
DEB_BUILD_GNU_TYPE   = $(shell dpkg-architecture -qDEB_BUILD_GNU_TYPE)
DEB_BUILD_GNU_CPU    = $(shell dpkg-architecture -qDEB_BUILD_GNU_CPU)
DEB_BUILD_GNU_SYSTEM = $(shell dpkg-architecture -qDEB_BUILD_GNU_SYSTEM)
DEB_HOST_ARCH        = $(shell dpkg-architecture -qDEB_HOST_ARCH)
DEB_HOST_GNU_TYPE    = $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)
DEB_HOST_GNU_CPU     = $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM  = $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
endif

CFLAGS = -O2 -Wall
STRIP =
ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
CFLAGS += -g
endif
ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTIONS)))
STRIP = -s
endif

export CFLAGS

install         := install -o root -g root
install_exec    := $(install) -m 0755 $(STRIP)
install_nonex   := $(install) -m 0644
install_dir     := $(install) -m 0755 -d
install_script  := $(install) -m 0755
install_symlink := ln -s
gzip            := gzip -9
strip_lib       := strip --strip-unneeded

tmpdir := $(shell pwd)/debian/tmp

# These must not be :='s!
rootdir = $(tmpdir)/$(package)
ctldir = $(rootdir)/DEBIAN
etcdir = $(rootdir)/etc
bindir = $(rootdir)/usr/bin
sbindir = $(rootdir)/usr/sbin
docdir = $(rootdir)/usr/share/doc/$(package)
exampledir = $(docdir)/examples
mandir = $(rootdir)/usr/share/man
elispdir = $(rootdir)/usr/share/emacs/site-lisp
emacs_d_dir = $(etcdir)/emacs/site-start.d
man1dir = $(mandir)/man1
man2dir = $(mandir)/man2
man3dir = $(mandir)/man3
man4dir = $(mandir)/man4
man5dir = $(mandir)/man5
man6dir = $(mandir)/man6
man7dir = $(mandir)/man7
man8dir = $(mandir)/man8
sharedir = $(rootdir)/usr/share/$(package)
libdir = $(rootdir)/usr/lib/$(package)
docbasedir = $(rootdir)/usr/share/doc-base
usrlib = $(rootdir)/usr/lib
includedir = $(rootdir)/usr/include

ifeq ($(librules_native_pkg),yes)
librules_changelog=changelog
else
librules_changelog=changelog.Debian
endif

install_prerm    = $(install_script) debian/prerm.$(package) $(ctldir)/prerm
install_postrm   = $(install_script) debian/postrm.$(package) $(ctldir)/postrm
install_preinst  = $(install_script) debian/preinst.$(package) $(ctldir)/preinst
install_postinst = $(install_script) debian/postinst.$(package) $(ctldir)/postinst


define prebinary
	$(RM) -r $(rootdir)
	$(install_dir) $(ctldir)
	$(install_dir) $(docdir)
	$(install_nonex) debian/copyright $(docdir)
	$(install_nonex) debian/changelog $(docdir)/$(librules_changelog)
	$(gzip) $(docdir)/$(librules_changelog)
endef

define postbinary
	chmod -R g-s $(rootdir)
	dpkg-gencontrol -isp -p$(package) -P$(rootdir) $(gencontrol_options)
	dpkg --build $(rootdir) ..
endef

clean-build-std:
	rm -f debian/stamp/build

clean-binary-std:
	rm -f debian/stamp/binary/*
	rm -f debian/files debian/substvars
	rm -rf $(tmpdir)

clean-std:
	rm -rf debian/stamp/

# Patching support
ifeq ($(librules_patch_support),yes)
debian/stamp/build: debian/stamp/patch
clean-build: unpatch

debian/stamp/patch:
	$(foreach patch, $(librules_patches), patch -fs < $(patch) && )true
	touch $@

unpatch: debian/stamp/patch
	$(foreach patch, $(librules_patches), patch -fsR < $(patch) && )true
	rm -f debian/stamp/patch
endif
