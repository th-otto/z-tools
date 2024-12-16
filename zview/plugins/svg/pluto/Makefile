
#CROSS_PREFIX = m68k-atari-mint-
CC				= $(CROSS_PREFIX)gcc
AR 				= $(CROSS_PREFIX)ar

OPTIMISATION = -O2 -fomit-frame-pointer
CPPFLAGS = -I. -I/usr/include/freetype2
CFLAGS   = $(DEBUG) $(CPU_CFLAGS) $(CPPFLAGS) $(OPTIMISATION) -Wall -Wshadow -Wno-pointer-sign -Wstrict-prototypes -Wmissing-prototypes -Wdeclaration-after-statement -Werror
#CFLAGS += -Waggregate-return
LIBS = -lz -lm
LDFLAGS  = -s

all: svg2png

svg2png: svg2png.o plutovg.a
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $^ $(LIBS)

PLUTO_OBJS = \
	plutosvg.o \
	blend.o \
	canvas.o \
	font.o \
	ftmath.o \
	ftraster.o \
	ftstroke.o \
	matrix.o \
	paint.o \
	path.o \
	raster.o \
	surface.o \
	surfacer.o \
	surfacew.o \
	$(empty)

plutovg.a: $(PLUTO_OBJS)
	rm -f $@
	$(AR) rcs $@ $(PLUTO_OBJS)

$(PLUTO_OBJS): plutovg.h plutosvg.h private.h
surfacer.o: image.h
surfacew.o: imagewr.h
font.o: truetype.h

clean::
	$(RM) *.o *.a svg2png
