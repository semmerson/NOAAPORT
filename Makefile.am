# Copyright 2011 University Corporation for Atmospheric Research
#
# This file is part of the Unidata NOAAPORT package.  See the file COPYRIGHT in
# the top-level source-directory of the package for copying and redistribution
# conditions.
#
## Process this file with automake(1) to produce file Makefile.in

################################################################################

LDMHOME			= @LDMHOME@
LDMSRC			= $(LDMHOME)/src
SUDO			= @SUDO@
SU			= @SU@

BUILT_SOURCES		= shmfifo.h

SUBDIRS			= zlib g2 gempak libpng tables

distName 		= $(PACKAGE)-$(VERSION)
distArchive 		= $(distName).tar.gz
EXTRA_DIST 		= \
    	CHANGE_LOG \
    	COPYRIGHT \
	Doxyfile \
    	extractDecls \
	$(srcdir)/html \
	mainpage.h.in
DISTCLEANFILES		= mainpage.h *.log $(distArchive)

lib_LTLIBRARIES		= libnoaaport.la
libnoaaport_la_SOURCES	= shmfifo.c shmfifo.h
libnoaaport_la_LIBADD	= g2/lib.la gempak/lib.la libpng/libpng.la zlib/lib.la

bin_PROGRAMS		= dvbs_multicast readnoaaport
dist_bin_SCRIPTS	= \
	dvbs_goes \
	dvbs_nwstg \
	dvbs_nwstg2 \
	dvbs_oconus \
	gms-meteo_mps2 \
	goese_mps3 \
	goesw_mps1 \
	nplog_rotate \
	nwstg_mps0

CPPFLAGS		= -I$(LDMHOME)/include
LDADD 			= libnoaaport.la -L$(LDMHOME)/lib -lldm -lxml2

dvbs_multicast_SOURCES	= dvbs_multicast.c

readnoaaport_SOURCES	= \
	config.h \
	dvbs.h \
	grib2name.c \
	gribid.c \
	nport.h \
	png_io.c \
	process_prod.c \
	readnoaaport.c \
	readpdb.c \
	readpdh.c \
	readpsh.c\
	readsbn.c \
	redbook_header.c \
	wgrib.c \
	wmo_header.c \
	wmo_header.h
readnoaaport_CPPFLAGS 	= \
	-I$(top_srcdir)/g2 -I$(top_srcdir)/gempak -I$(top_srcdir)/zlib

TAGS_FILES		= \
	$(LDMSRC)/pq/*.c $(LDMSRC)/pq/*.h \
	$(LDMSRC)/protocol/*.c $(LDMSRC)/protocol/*.h \
	$(LDMSRC)/ulog/*.c $(LDMSRC)/ulog/*.h \
	$(LDMSRC)/misc/*.c $(LDMSRC)/misc/*.h \
	$(LDMSRC)/registry/*.c $(LDMSRC)/registry/*.h \
	$(LDMSRC)/rpc/*.c $(LDMSRC)/rpc/*.h

DISTCHECK_CONFIGURE_FLAGS	= --disable-root-actions --disable-shared

shmfifo.h:	$(srcdir)/shmfifo.h.in \
		$(srcdir)/shmfifo.c \
		$(srcdir)/extractDecls
	./extractDecls $(srcdir)/shmfifo.h.in $(srcdir)/shmfifo.c >$@

libpng/libpng.la:
	cd libpng && $(MAKE) $(AM_MAKEFLAGS) all

install-exec-hook:
if INCLUDE_ROOT_ACTIONS
	@echo
	@echo "\
The next step will perform those installation actions that must be executed \
by the superuser. \
If this step doesn't work (for example, if you don't enter \
root's password), then the superuser will have to manually execute the \
command \"$(MAKE) root-actions\"." \
	| fmt >/dev/tty
	@echo >/dev/tty
if HAVE_SUDO
	$(SUDO) $(MAKE) $(AM_MAKEFLAGS) root-actions 2>/dev/tty
else
if HAVE_SU
	@printf "Enter root's password (or don't): " >/dev/tty
	@$(SU) root -c 'PATH='$$PATH' $(MAKE) $(AM_MAKEFLAGS) root-actions' \
	    </dev/tty 2>/dev/tty
	@echo >/dev/tty
endif
endif
else
	@echo
	@echo "\
NOTE: The command \"$(MAKE) root-actions\" will have to be executed by the \
superuser in order to complete the installation process." \
	| fmt
	@echo
endif

root-actions:		install_setuids

install_setuids:
	chown root $(DESTDIR)$(bindir)/dvbs_multicast
	chmod 4755 $(DESTDIR)$(bindir)/dvbs_multicast
	@if ls -l $(bindir)/dvbs_multicast | grep root >/dev/null; then \
	    : true; \
	else \
	    echo; \
	    echo "\
ERROR: The program (../bin/dvbs_multicast) is not owned by \"root\" or does \
not have the setuid bit enabled.  The command \"make install_setuids\" will \
have to be manually executed by the superuser on a system that allows these \
actions." \
	    | fmt; \
	    echo; \
	    exit 1; \
	fi

installcheck-local:
	pqcreate -c -s 2m /tmp/readnoaaport-test.pq
	$(DESTDIR)$(bindir)/readnoaaport -l- -q /tmp/readnoaaport-test.pq \
	    $(srcdir)/nwstgdump.data
	rm /tmp/readnoaaport-test.pq

check-local:		readnoaaport
	pqcreate -c -s 2m /tmp/readnoaaport-test.pq
	./readnoaaport -l- -q /tmp/readnoaaport-test.pq \
	    $(srcdir)/nwstgdump.data
	rm /tmp/readnoaaport-test.pq

valgrind:		readnoaaport
	pqcreate -c -s 2m /tmp/readnoaaport-test.pq
	$(LIBTOOL) --mode=execute valgrind --leak-check=yes \
	    readnoaaport -l- -q /tmp/readnoaaport-test.pq nwstgdump.data
	rm /tmp/readnoaaport-test.pq

debug:			readnoaaport
	pqcreate -c -s 2m /tmp/readnoaaport-test.pq
	echo 'handle SIGCONT pass noprint nostop' >/tmp/readnoaaport.gdb
	echo 'run -l- -q /tmp/readnoaaport-test.pq nwstgdump.data' \
	    >>/tmp/readnoaaport.gdb
	$(LIBTOOL) --mode=execute gdb -x /tmp/readnoaaport.gdb readnoaaport
	rm /tmp/readnoaaport-test.pq /tmp/readnoaaport.gdb

$(srcdir)/html:		$(srcdir)/mainpage.h.in
	./config.status
	rm -rf $(srcdir)/html
	cd $(srcdir) && doxygen Doxyfile

install-html:		$(srcdir)/html $(DESTDIR)$(htmldir)
	cp -R $(srcdir)/html/* $(DESTDIR)$(htmldir)

$(DESTDIR)$(htmldir):
	mkdir -p $@

install-data-hook:	install-html

uninstall-hook:
	chmod -R +w $(DESTDIR)$(htmldir)
	rm -rf $(DESTDIR)$(htmldir)

FTPDIR	= /web/ftp/pub/$(PACKAGE)
WEBDIR	= /web/content/software/$(PACKAGE)/$(VERSION)

$(FTPDIR) \
$(WEBDIR):
	mkdir -p $@

commit:
	git commit -a

tag:
	git tag -f v$(VERSION)

ftp:			tag dist $(FTPDIR)
	cp $(distArchive) $(FTPDIR)
	chmod u+rw,g+rw,o=r $(FTPDIR)/$(distArchive)
	rm -f $(FTPDIR)/$(PACKAGE).tar.gz
	$(LN_S) $(distArchive) $(FTPDIR)/$(PACKAGE).tar.gz

web-update:		$(srcdir)/html $(WEBDIR)
	cp -R html/* $(WEBDIR)

release:		ftp web-update commit tag

available:		release

.PHONY:		install-html commit tag ftp web-update available release
