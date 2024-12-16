top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars
#CFLAGS += -Waggregate-return
CFLAGS += -Wdeclaration-after-statement

USE_LUNA  = 1
USE_PLUTO = 0
USE_NANO  = 0

SVG_CC    = $(CC)
SVG_LDCC  = $(LDSHARED)

ifeq ($(USE_LUNA),1)
SVG_MOD   = svgluna
SVG_CC    = $(CXX)
SVG_LIB   = -llunasvg -lplutovg
LDSHARED  = $(LDSHAREDXX)
LDSHAREDLIBC = $(LDSHAREDLIBCXX)
endif
ifeq ($(USE_PLUTO),1)
SVG_MOD   = svgpluto
SVG_LIB   = -lplutosvg -lplutovg
endif
ifeq ($(USE_NANO),1)
SVG_MOD   = svgnano
endif

LIB  = $(SVG_LIB) -lz -lm
SFLAGS += -DZLIB_SLB

all:: svg.ldg zvsvg.slb

include $(top_srcdir)/rules

svg.ldg: ldg.o $(SVG_MOD).o $(LDGOBJS)
	$(SVG_CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvsvg.slb: header.lo slb.lo $(SVG_MOD).lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(SVG_LIB) -lz.slb -lm $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo svg.lo: ../common/plugin.h ../common/zview.h exports.h
svgnano.o svgnano.lo: nanosvg.h nanorast.h libcmini.c strtol.c strchr.c strstr.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb pluto/*.o pluto/*.lo
