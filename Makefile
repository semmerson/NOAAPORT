include ../src/macros.make

PROG_CSRCS = \
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

PROG_OBJS = $(PROG_CSRCS:.c=.o)

PROG = readnoaaport
DVBS_PROG = dvbs_multicast
C_PROGRAMS = $(PROG) $(DVBS_PROG)
SETUID_PROGRAMS = $(DVBS_PROG)

SHELL_PROGRAMS = \
	dvbs_nwstg \
	dvbs_goes \
	dvbs_nwstg2 \
	dvbs_oconus \
	nwstg_mps0 \
	goesw_mps1 \
	gms-meteo_mps2 \
	goese_mps3 \
	nplog_rotate

INCLUDES = -I./g2 -I./gempak -I./zlib -I$(INCDIR) 


LIBZ = zlib/libz.a
LIBPNG = libpng/libpng.a

LIBRARY=-L$(LIBDIR) -lldm
#LDLIBS =  $(LIBPNG) $(LIBZ) -lm 
#LIBS = -L(LIBDIR) -lldm -loncrpc
ZLIBS =  $(LIBPNG) $(LIBZ) -lm 

PROG_LIBS = g2/g2c.a gempak/gemgrib.a

CC= cc

all: programs

install : installed_programs
	cd tables ; \
	make install

noaaport_version.c: VERSION
	echo 'const char* version_str = "'`cat VERSION`'";' >$@

$(PROG): $(PROG_OBJS) Zlib PNGlib shmfifo.o _g2lib _gemlib
	$(CC) -o $@ $(CFLAGS) $(PROG_OBJS) shmfifo.o $(PROG_LIBS) $(LIBS) $(CONFIGURE_LIBS) $(ZLIBS)

$(DVBS_PROG): $(DVBS_PROG).o shmfifo.o noaaport_version.o
	$(CC) -o $@ $(CFLAGS) $@.o noaaport_version.o shmfifo.o $(LIBS) $(CONFIGURE_LIBS) -lrt -lm

test: $(PROG)
	-pqcreate -s 2m /tmp/test.pq
	readnoaaport -q /tmp/test.pq -l - -nvx nwstgdump.data

clean: clean_Zlib clean_PNGlib clean_g2lib clean_gemlib

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
	make all CFLAGS="$(CFLAGS) -I../g2 -I$(INCDIR)"

tardist: clean
	version=`cat VERSION`; \
	id=noaaport-$$version \
	&& cd .. \
	&& rm -f $$id  \
	&& ln -s noaaport $$id \
	&& pax -L -w -x ustar $$id/C[^V]* \
		$$id/[ABD-z]* \
	| gzip > noaaport-$$version.tar.gz

include ../src/rules.make
