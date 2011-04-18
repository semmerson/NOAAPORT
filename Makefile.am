# Copyright 2011 University Corporation for Atmospheric Research
#
# This file is part of the Unidata NOAAPORT package.  See the file COPYRIGHT in
# the top-level source-directory of the package for copying and redistribution
# conditions.
#
## Process this file with automake(1) to produce file Makefile.in

################################################################################

LDMHOME			= @LDMHOME@
SUDO			= @SUDO@
SU			= @SU@

SUBDIRS			= zlib g2 gempak libpng tables

distName 		= $(PACKAGE)-$(VERSION)
distArchive 		= $(distName).tar.gz
EXTRA_DIST 		= CHANGE_LOG COPYRIGHT INSTALL extractDecls
DISTCLEANFILES		= *.log $(distArchive)

lib_LTLIBRARIES		= libnoaaport.la
libnoaaport_la_SOURCES	= noaaport_version.c shmfifo.c
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
	shmfifo.h \
	wgrib.c \
	wmo_header.c \
	wmo_header.h
readnoaaport_CPPFLAGS 	= \
	-I$(top_srcdir)/g2 -I$(top_srcdir)/gempak -I$(top_srcdir)/zlib

DISTCHECK_CONFIGURE_FLAGS	= --disable-root-actions --disable-shared

libpng/libpng.la:
	cd libpng && $(MAKE) $(AM_MAKEFLAGS) all

#
#installed_programs:	all
#	for prog in $(INSTALLED_PROGRAMS); do \
#	    $(MAKE) $(BINDIR)/$$prog PROG=$$prog LDM_PREFIX=$(LDM_PREFIX) \
#		|| exit 1; \
#	done
#
#$(BINDIR)/$(PROG):	$(PROG)
#	rm -f $@
#	cp $(PROG) $@
#
#install_setuids:
#	chown root $(DESTDIR)$(BINDIR)/dvbs_multicast
#	chmod 4755 $(DESTDIR)$(BINDIR)/dvbs_multicast
#	@if ls -l $(BINDIR)/dvbs_multicast | grep root >/dev/null; then \
#	    : true; \
#	else \
#	    echo; \
#	    echo "\
#ERROR: The program (../bin/dvbs_multicast) is not owned by \"root\" or does \
#not have the setuid bit enabled.  The command \"make install_setuids\" will \
#have to be manually executed by the superuser on a system that allows these \
#actions." \
#	    | fmt; \
#	    echo; \
#	    exit 1; \
#	fi
#
#noaaport_version.c: 	VERSION
#	echo 'const char* version_str = "'`cat VERSION`'";' >$@
#
#readnoaaport: 		$(READNOAAPORT_OBJS) zlib/libz.a libpng/libpng.a \
#			shmfifo.o g2/g2c.a gempak/gemgrib.a
#	$(CC) -o $@ $(CFLAGS) $(READNOAAPORT_OBJS) shmfifo.o g2/g2c.a \
#	    gempak/gemgrib.a $(LDM_LIBRARIES) libpng/libpng.a zlib/libz.a -lm \
#	    $(LIBS)
#
#test: 			readnoaaport
#	test -f /tmp/test.pq || pqcreate -s 2m /tmp/test.pq
#	./readnoaaport -q /tmp/test.pq -l - -nvx nwstgdump.data
#	rm /tmp/test.pq
#
#clean: 			clean_Zlib clean_PNGlib clean_g2lib clean_gemlib
#	rm -f *.o *.i $(BUILT_PROGRAMS)
#
#clean_Zlib:
#	cd zlib ; \
#	make clean
#
#clean_PNGlib:
#	cd libpng ; \
#	make clean
#
#clean_g2lib:
#	cd g2 ; \
#	make clean
#
#clean_gemlib:
#	cd gempak ; \
#	make clean
#
#zlib/libz.a:
#	cd zlib ; \
#	make all CFLAGS="$(CFLAGS)"
#
#libpng/libpng.a:
#	cd libpng ; \
#	make all CFLAGS="$(CFLAGS) -I../zlib"
#
#g2/g2c.a:
#	cd g2 ; \
#	make all CFLAGS="$(CFLAGS)"
#
#gempak/gemgrib.a:
#	cd gempak ; \
#	make all CC='$(CC)' CFLAGS='$(CFLAGS)' \
#	    CPPFLAGS="-I../g2 -I../$(LDM_PREFIX)/include \
#	    -DLDMHOME='\"$$LDMHOME\"'"
#
#tardist: 		clean
#	version=`cat VERSION`; \
#	id=noaaport-$$version \
#	&& cd .. \
#	&& rm -f $$id  \
#	&& ln -s noaaport $$id \
#	&& pax -w -x ustar $$id/* | gzip > noaaport-$$version.tar.gz \
#	&& rm $$id
#
#.c.i:
#	$(COMPILE) $(CPPFLAGS) -E -o $@ $<
