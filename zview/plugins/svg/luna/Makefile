top_srcdir=../../../..
include $(top_srcdir)/configvars
#CFLAGS += -Waggregate-return
CFLAGS += -Wdeclaration-after-statement

LUNASVG_SRCS = \
	graphics \
	lunasvg \
	svgelement \
	svggeometryelement \
	svglayoutstate \
	svgpaintelement \
	svgparser \
	svgproperty \
	svgrenderstate \
	svgtextelement \
	$(empty)

PLUTOVG_SRCS = \
	../pluto/blend \
	../pluto/canvas \
	../pluto/font \
	../pluto/ftmath \
	../pluto/ftraster \
	../pluto/ftstroke \
	../pluto/matrix \
	../pluto/paint \
	../pluto/path \
	../pluto/raster \
	../pluto/surface \
	../pluto/surfacer \
	../pluto/surfacew \
	$(empty)

PLUTOSVG_SRCS = \
	../pluto/plutosvg \
	$(empty)

SVG_CC    = $(CXX)
SVG_SRCS  = $(LUNASVG_SRCS) $(PLUTOVG_SRCS)
CPPFLAGS  += -I. -I../pluto -DNDEBUG
CXXFLAGS  += -std=gnu++1z
CXXFLAGS  += -Wno-shadow

LIB  = -lz -lm

all:: svg2png

include $(top_srcdir)/rules

svg2png: svg2png.o $(SVG_SRCS:=.o)
	$(SVG_CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIB)

clean:
	rm -f svg2png *.o *.lo *.a *.pdb ../pluto/*.o ../pluto/*.lo
