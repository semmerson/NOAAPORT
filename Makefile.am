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

SUBDIRS			= zlib g2 gempak libpng tables

distName 		= $(PACKAGE)-$(VERSION)
distArchive 		= $(distName).tar.gz
EXTRA_DIST 		= \
    	CHANGE_LOG \
    	COPYRIGHT \
	Doxyfile \
	html \
	mainpage.h \
	mainpage.h.in \
	noaaportLog.h \
	nwstgdump.data
DISTCLEANFILES		= mainpage.h $(distArchive)

lib_LTLIBRARIES		= libnoaaport.la
libnoaaport_la_SOURCES	= shmfifo.c shmfifo.h noaaportLog.c noaaportLog.h
libnoaaport_la_CPPFLAGS	= \
	-I$(top_srcdir)/g2 \
	-I$(top_srcdir)/gempak \
	-I$(top_srcdir)/zlib
libnoaaport_la_LIBADD	= g2/lib.la gempak/lib.la libpng/libpng.la zlib/lib.la

bin_PROGRAMS		= dvbs_multicast readnoaaport noaaportIngester
dist_bin_SCRIPTS	= \
	dvbs_goes \
	dvbs_nwstg \
	dvbs_nwstg2 \
	dvbs_oconus \
	nplog_rotate

CPPFLAGS		= -I$(LDMHOME)/include -I$(LDMHOME)/src/ulog \
			  -I$(LDMHOME)/src
LDADD 			= libnoaaport.la -L$(LDMHOME)/lib -lldm -lxml2

dvbs_multicast_SOURCES	= dvbs_multicast.c

COMMON_SOURCES		= \
	dvbs.h \
	grib2name.c \
	gribid.c \
	ldmProductQueue.c ldmProductQueue.h \
	nport.h \
	png_io.c \
	process_prod.c \
	readpdb.c \
	readpdh.c \
	readpsh.c\
	readsbn.c \
	redbook_header.c \
	wgrib.c \
	wmo_header.c \
	wmo_header.h
COMMON_CPPFLAGS		= \
	-I$(top_srcdir)/g2 -I$(top_srcdir)/gempak -I$(top_srcdir)/zlib
readnoaaport_SOURCES	= readnoaaport.c $(COMMON_SOURCES)
readnoaaport_CPPFLAGS 	= $(COMMON_CPPFLAGS)

noaaportIngester_SOURCES	= noaaportIngester.c $(COMMON_SOURCES) \
	fifo.c fifo.h \
	fileReader.c fileReader.h \
	getFacilityName.c getFacilityName.h \
	multicastReader.c multicastReader.h \
	noaaportLog.c noaaportLog.h \
	productMaker.c productMaker.h \
	reader.c reader.h
noaaportIngester_CPPFLAGS 	= $(COMMON_CPPFLAGS)

TAGS_FILES		= \
	*.c *.h \
	g2/*.c g2/*.h \
	gempak/*.c gempak/*.h \
	$(LDMSRC)/pq/*.c $(LDMSRC)/pq/*.h \
	$(LDMSRC)/protocol/*.c $(LDMSRC)/protocol/*.h \
	$(LDMSRC)/ulog/*.c $(LDMSRC)/ulog/*.h \
	$(LDMSRC)/misc/*.c $(LDMSRC)/misc/*.h \
	$(LDMSRC)/registry/*.c $(LDMSRC)/registry/*.h \
	$(LDMSRC)/rpc/*.c $(LDMSRC)/rpc/*.h

DISTCHECK_CONFIGURE_FLAGS	= --disable-root-actions --disable-shared

if MAINTAINER
all:		$(srcdir)/html/index.html

$(srcdir)/html/index.html:	$(srcdir)/configure.ac $(srcdir)/mainpage.h.in \
		$(srcdir)/Doxyfile
	cd $(srcdir) && doxygen Doxyfile

.hin.h:
	./extractDecls $*.hin $*.c >$@.tmp
	mv -f $@.tmp $@
shmfifo.h:		shmfifo.c shmfifo.hin extractDecls
ldmProductQueue.h:	ldmProductQueue.c ldmProductQueue.hin extractDecls
fifo.h:			fifo.c fifo.hin extractDecls
reader.h:		reader.c reader.hin extractDecls
fileReader.h:		fileReader.c fileReader.hin extractDecls
multicastReader.h:	multicastReader.c multicastReader.hin extractDecls
productMaker.h:		productMaker.c productMaker.hin extractDecls
getFacilityName.h:	getFacilityName.c getFacilityName.hin extractDecls
noaaportLog.h:		noaaportLog.c noaaportLog.hin extractDecls

.c.i:
	$(CC) $(CPPFLAGS) -E $< >$@
endif

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
	$(MAKE) $(AM_MAKEFLAGS) sudo TARGET=root-actions
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
	chown root $(DESTDIR)$(bindir)/noaaportIngester
	chmod 4755 $(DESTDIR)$(bindir)/noaaportIngester
	chown root $(DESTDIR)$(bindir)/dvbs_multicast
	chmod 4755 $(DESTDIR)$(bindir)/dvbs_multicast
	@if ls -l $(bindir)/noaaportIngester | grep root >/dev/null; then \
	    : true; \
	else \
	    echo; \
	    echo "\
ERROR: The program (../bin/noaaportIngester) is not owned by \"root\" or does \
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

check-local:		check-readnoaaport check-noaaportIngester
check-readnoaaport:	readnoaaport
	pqcreate -c -s 2m /tmp/readnoaaport-test.pq
	./readnoaaport -nl- -q /tmp/readnoaaport-test.pq \
	    $(srcdir)/nwstgdump.data
	rm /tmp/readnoaaport-test.pq
check-noaaportIngester:	noaaportIngester
	pqcreate -c -s 2m /tmp/noaaportIngester-test.pq
	$(MAKE) $(AM_MAKEFLAGS) sudo TARGET=root-ni-ck
	rm /tmp/noaaportIngester-test.pq

root-ni-ck:
	./noaaportIngester -b 3 -q /tmp/noaaportIngester-test.pq \
	    <$(srcdir)/nwstgdump.data

valgrind:		valgrind-readnoaaport valgrind-noaaportIngester

valgrind-readnoaaport:		readnoaaport
	pqcreate -c -s 2m /tmp/readnoaaport-test.pq
	$(LIBTOOL) --mode=execute valgrind --leak-check=yes \
	    readnoaaport -l- -q /tmp/readnoaaport-test.pq nwstgdump.data
	rm /tmp/readnoaaport-test.pq

sudo:
if HAVE_SUDO
	$(SUDO) $(MAKE) $(AM_MAKEFLAGS) $(TARGET) 2>/dev/tty
else
if HAVE_SU
	@printf "Enter root's password (or don't): " >/dev/tty
	@$(SU) root -c 'PATH='$$PATH' $(MAKE) $(AM_MAKEFLAGS) $(TARGET)' \
	    </dev/tty 2>/dev/tty
	@echo >/dev/tty
endif
endif
	
valgrind-noaaportIngester:	noaaportIngester
	pqcreate -c -s 2m /tmp/noaaportIngester-test.pq
	$(MAKE) $(AM_MAKEFLAGS) sudo TARGET=root-ni-vg
	rm /tmp/noaaportIngester-test.pq

root-ni-vg:
	$(LIBTOOL) --mode=execute valgrind --leak-check=yes \
	    noaaportIngester -n -q /tmp/noaaportIngester-test.pq \
		<$(srcdir)/nwstgdump.data

debug-readnoaaport:	readnoaaport
	pqcreate -c -s 2m /tmp/readnoaaport-test.pq
	echo 'handle SIGCONT pass noprint nostop' >/tmp/readnoaaport.gdb
	echo 'b 617' >>/tmp/readnoaaport.gdb
	echo 'run -l- -q /tmp/readnoaaport-test.pq nwstgdump.data' \
	    >>/tmp/readnoaaport.gdb
	$(LIBTOOL) --mode=execute gdb -x /tmp/readnoaaport.gdb readnoaaport
	rm /tmp/readnoaaport-test.pq /tmp/readnoaaport.gdb

debug-noaaportIngester:	noaaportIngester
	pqcreate -c -s 2m /tmp/noaaportIngester-test.pq
	echo 'handle SIGCONT pass noprint nostop' >/tmp/noaaportIngester.gdb
	echo 'handle SIGTERM pass nostop' >>/tmp/noaaportIngester.gdb
	echo 'run -q /tmp/noaaportIngester-test.pq -n -m 224.0.1.1' \
	    >>/tmp/noaaportIngester.gdb
	$(MAKE) $(AM_MAKEFLAGS) sudo TARGET=root-ni-db
	rm /tmp/noaaportIngester-test.pq /tmp/noaaportIngester.gdb

root-ni-db:
	$(LIBTOOL) --mode=execute gdb -x /tmp/noaaportIngester.gdb \
	      noaaportIngester

install-html:		$(srcdir)/html/index.html $(DESTDIR)$(htmldir)
	cp -R $(srcdir)/html/* $(DESTDIR)$(htmldir)

$(DESTDIR)$(htmldir):
	mkdir -p $@

install-data-hook:	install-html

uninstall-hook:
	chmod -R +w $(DESTDIR)$(htmldir)
	rm -rf $(DESTDIR)$(htmldir)

FTPDIR	= /web/ftp/pub/$(PACKAGE)
WEBROOT	= /web/content/software/$(PACKAGE)

$(FTPDIR) \
$(WEBROOT):
	mkdir -p $@

releaseCheck:
	git diff `git tag -l v\* | tail -1` >diff.log
	if ! test -s diff.log; then \
	    echo 2>&1 "No changes since last release!"; \
	    rm diff.log; \
	    exit 1; \
	fi
	rm diff.log
	regex=`echo $(VERSION) | sed 's/\./\\./g'`; \
	if git tag -l v\* | grep "^v$$regex$$"; then \
	    echo 2>&1 'Tag "v$(VERSION)" already exists!'; \
	    exit 1; \
	fi

timestamp:
	awk 'NR == 1 {\
		print $$1, "	'"`date --rfc-3339=seconds`"'";\
		next;\
	    }\
	    {print}' CHANGE_LOG >CHANGE_LOG.tmp
	mv CHANGE_LOG.tmp CHANGE_LOG

dist:		configure $(srcdir)/html/index.html

commitAndTag:
	git commit -a -m "v$(VERSION)"
	git tag -f "v$(VERSION)"

release:	releaseCheck
	-git commit -a
	echo 'PUT VERSION HERE' >CHANGE_LOG.tmp
	git log --pretty=full `git tag -l v\* | \
	    sort -t . -k 1.2n,1 -k 2n,2 -k 3n,3 -k 4n,4 | \
	    tail -1`.. >>CHANGE_LOG.tmp
	echo '' >>CHANGE_LOG.tmp
	cat CHANGE_LOG >>CHANGE_LOG.tmp
	vi CHANGE_LOG.tmp
	mv CHANGE_LOG.tmp CHANGE_LOG
	@newVersion=`awk '{print $$1; exit}' CHANGE_LOG`; \
	echo $$newVersion | egrep '^[0-9]+(\.[0-9]+)*$$' >/dev/null || exit 1; \
	sed '/^AC_INIT(/s/[0-9][0-9.]*,/'"$$newVersion"',/' configure.ac \
	    >configure.ac.tmp
	mv configure.ac.tmp configure.ac
	$(MAKE) timestamp dist commitAndTag

ensureRelease:
	-@$(MAKE) release

ftp-actual:		dist $(FTPDIR)
	cp $(distArchive) $(FTPDIR)
	chmod u+rw,g+rw,o=r $(FTPDIR)/$(distArchive)
	rm -f $(FTPDIR)/$(PACKAGE).tar.gz
	$(LN_S) $(distArchive) $(FTPDIR)/$(PACKAGE).tar.gz

ftp:			ensureRelease
	$(MAKE) ftp-actual

software-update:	$(srcdir)/html/index.html $(WEBROOT)
	-mkdir $(WEBROOT)/$(VERSION)
	cp -R html/* $(WEBROOT)/$(VERSION)
	echo 'RemoveOutputFilter LAYOUT html htm' \
	    >$(WEBROOT)/$(VERSION)/.htaccess
	rm -f $(WEBROOT)/current
	$(LN_S) $(VERSION) $(WEBROOT)/current

download_update:
	previous=`sed -n 's/.*"%current%" value="\(.*\)".*/\1/p' \
		 /web/content/downloads/$(PACKAGE)/toc.xml`; \
	if test "$$previous" != "$(VERSION)"; then \
	    sed -e 's/"%current%" value=".*"/"%current%" value="$(VERSION)"/' \
	    -e 's/"%previous%" value=".*"/"%previous%" value="'$$previous'"/' \
		/web/content/downloads/$(PACKAGE)/toc.xml >toc.xml.new; \
	    cp /web/content/downloads/$(PACKAGE)/toc.xml \
		/web/content/downloads/$(PACKAGE)/toc.xml.old; \
	    mv -f toc.xml.new /web/content/downloads/$(PACKAGE)/toc.xml; \
	fi

web-update:		ensureRelease
	$(MAKE) software-update download_update

# Apparently, there's no rule to create $(distArchive).
available:		ensureRelease
	$(MAKE) $(distArchive) ftp web-update

.PHONY:	\
	available \
	commitAndTag \
	download_update \
	ensureRelease \
	ftp \
	ftp-actual \
	install-html \
	release \
	releaseCheck \
	root-ni-ck \
	root-ni-db \
	root-ni-vg \
	software-update \
	sudo \
	timestamp \
	web-update
