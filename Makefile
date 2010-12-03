include ../src/macros.make

PROG			= dummy_prog
BINDIR			= ../bin
READNOAAPORT_CSRCS 	= \
	noaaport_version.c \
	readsbn.c \
	readpdh.c \
	readpsh.c\
	readpdb.c \
	process_prod.c \
	redbook_header.c \
	readnoaaport.c \
	gribid.c \
	wmo_header.c \
	grib2name.c \
	wgrib.c \
	png_io.c
READNOAAPORT_OBJS	= $(READNOAAPORT_CSRCS:.c=.o)
BUILT_PROGRAMS		=  readnoaaport dvbs_multicast
INSTALLED_PROGRAMS 	= \
	$(BUILT_PROGRAMS) \
	dvbs_nwstg \
	dvbs_goes \
	dvbs_nwstg2 \
	dvbs_oconus \
	nwstg_mps0 \
	goesw_mps1 \
	gms-meteo_mps2 \
	goese_mps3 \
	nplog_rotate
CPPFLAGS 		= -Ig2 -Igempak -Izlib -I../include
LIBDIR			= ../lib
LDM_LIBRARY 		= -L$(LIBDIR) -lldm

all:			$(BUILT_PROGRAMS)

install:		installed_programs
	cd tables && $(MAKE) install

installed_programs:	all
	for prog in $(INSTALLED_PROGRAMS); do \
	    $(MAKE) $(BINDIR)/$$prog PROG=$$prog || exit 1; \
	done

$(BINDIR)/$(PROG):	$(PROG)
	rm -f $@
	cp $(PROG) $(BINDIR)

install_setuids:
	chown root $(BINDIR)/dvbs_multicast
	chmod 4755 $(BINDIR)/dvbs_multicast
	@if ls -l $(BINDIR)/dvbs_multicast | grep root >/dev/null; then \
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

noaaport_version.c: 	VERSION
	echo 'const char* version_str = "'`cat VERSION`'";' >$@

readnoaaport: 		$(READNOAAPORT_OBJS) Zlib PNGlib shmfifo.o _g2lib \
			_gemlib
	$(CC) -o $@ $(CFLAGS) $(READNOAAPORT_OBJS) shmfifo.o g2/g2c.a \
	    gempak/gemgrib.a $(LDM_LIBRARY) libpng/libpng.a zlib/libz.a -lm \
	    $(LIBS) -Wl,-rpath,`cd $(LIBDIR) && pwd`

dvbs_multicast: 	dvbs_multicast.o shmfifo.o noaaport_version.o
	$(CC) -o $@ $(CFLAGS) $@.o noaaport_version.o shmfifo.o \
	    $(LDM_LIBRARY) -lrt -lm $(LIBS) -Wl,-rpath,`cd $(LIBDIR) && pwd`

test: 			readnoaaport
	test -f /tmp/test.pq || pqcreate -s 2m /tmp/test.pq
	./readnoaaport -q /tmp/test.pq -l - -nvx nwstgdump.data
	rm /tmp/test.pq

clean: 			clean_Zlib clean_PNGlib clean_g2lib clean_gemlib
	rm -f *.o $(BUILT_PROGRAMS)

clean_Zlib:
	cd zlib ; \
	make clean

clean_PNGlib:
	cd libpng ; \
	make clean

clean_g2lib:
	cd g2 ; \
	make clean

clean_gemlib:
	cd gempak ; \
	make clean

Zlib:
	cd zlib ; \
	make all CFLAGS="$(CFLAGS)"

PNGlib:
	cd libpng ; \
	make all CFLAGS="$(CFLAGS) -I../zlib"

_g2lib:
	cd g2 ; \
	make all CFLAGS="$(CFLAGS)"

_gemlib:
	cd gempak ; \
	make all CPPFLAGS='-I../g2 -I../../include -DLDMHOME=\"$(LDMHOME)\"' \
	    CFLAGS='$(CFLAGS)'

tardist: 		clean
	version=`cat VERSION`; \
	id=noaaport-$$version \
	&& cd .. \
	&& rm -f $$id  \
	&& ln -s noaaport $$id \
	&& pax -L -w -x ustar $$id/C[^V]* \
		$$id/[ABD-z]* \
	| gzip > noaaport-$$version.tar.gz

.SUFFIXES:	.i .c

.c.i:
	$(CC) -E $(CPPFLAGS) $< >$@

FORCE:
