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

LUNASVG_SRCS = \
	luna/graphics \
	luna/lunasvg \
	luna/svgelement \
	luna/svggeometryelement \
	luna/svglayoutstate \
	luna/svgpaintelement \
	luna/svgparser \
	luna/svgproperty \
	luna/svgrenderstate \
	luna/svgtextelement \
	$(empty)

PLUTOVG_SRCS = \
	pluto/blend \
	pluto/canvas \
	pluto/font \
	pluto/ftmath \
	pluto/ftraster \
	pluto/ftstroke \
	pluto/matrix \
	pluto/paint \
	pluto/path \
	pluto/raster \
	pluto/surface \
	pluto/surfacer \
	pluto/surfacew \
	$(empty)

PLUTOSVG_SRCS = \
	pluto/plutosvg \
	$(empty)

ifeq ($(USE_LUNA),1)
SVG_MOD   = svgluna
SVG_CC    = $(CXX)
SVG_SRCS  = $(LUNASVG_SRCS) $(PLUTOVG_SRCS)
CPPFLAGS  += -Ipluto -DNDEBUG
LDSHARED  = $(LDSHAREDXX)
LDSHAREDLIBC = $(LDSHAREDLIBCXX)
CXXFLAGS  += -std=gnu++1z
CXXFLAGS  += -Wno-shadow
endif
ifeq ($(USE_PLUTO),1)
SVG_MOD   = svgpluto
SVG_SRCS  = $(PLUTOSVG_SRCS) $(PLUTOVG_SRCS)
CPPFLAGS  += -Ipluto -DNDEBUG
endif
ifeq ($(USE_NANO),1)
SVG_MOD   = svgnano
endif

LIB  = -lz -lm
SFLAGS += -DZLIB_SLB

all:: svg.ldg zvsvg.slb

include $(top_srcdir)/rules

svg.ldg: ldg.o $(SVG_MOD).o $(LDGOBJS) $(SVG_SRCS:=.o)
	$(SVG_CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvsvg.slb: header.lo slb.lo $(SVG_MOD).lo $(SVG_SRCS:=.lo)
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lz.slb -lm $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo svg.lo: ../common/plugin.h ../common/zview.h exports.h
svgnano.o svgnano.lo: nanosvg.h nanorast.h libcmini.c strtol.c strchr.c strstr.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb pluto/*.o pluto/*.lo luna/*.o luna/*.lo
