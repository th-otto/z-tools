top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

LIB       = -lpng -lz -lm

SFLAGS += -DPNGLIB_SLB -DZLIB_SLB -DSHAREDLIB=\"zvpng.slb\"

LIBPNG_OBJS = libpng/png.o libpng/pngerror.o \
	libpng/pngget.o libpng/pngmem.o libpng/pngpread.o libpng/pngread.o libpng/pngrio.o libpng/pngrtran.o libpng/pngrutil.o \
	libpng/pngset.o libpng/pngtrans.o libpng/pngwio.o libpng/pngwrite.o libpng/pngwtran.o libpng/pngwutil.o

LIBPNG_SOBJS =


all:: png.ldg zvpng.slb

include $(top_srcdir)/rules

png.ldg: ldg.o png.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvpng.slb: header.lo slb.lo png.lo $(LIBPNG_SOBJS)
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lpng.slb -lz.slb -lm $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo png.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb libpng/*.o libpng/*.lo *.pdb
