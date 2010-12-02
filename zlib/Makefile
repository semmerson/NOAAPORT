AR = ar rv

OBJS = adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o \
	zutil.o inflate.o infblock.o inftrees.o infcodes.o infutil.o inffast.o

all: libz.a

libz.a: $(OBJS) 
	$(AR) $@ $(OBJS)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

clean:
	-rm $(OBJS) libz.a
