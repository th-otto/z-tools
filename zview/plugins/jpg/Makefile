top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

LIB			= -ljpeg -lexif -lm

CPPFLAGS += -DNO_GETENV -DJPEG_LIB_VERSION=80
SFLAGS += -DJPEG_SLB -DEXIF_SLB -DSHAREDLIB=\"zvjpg.slb\"

libexif_OBJS =

libexif_SOBJS = $(libexif_OBJS:.o=.lo)

all:: jpg.ldg zvjpg.slb

include $(top_srcdir)/rules

jpg.ldg: ldg.o jpg.o jpgdsp.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvjpg.slb: header.lo slb.lo jpg.lo jpgdsp.lo $(jpeg_SOBJS) $(libexif_SOBJS)
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) -ljpeg8d.slb -lexif.slb -lm $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo jpg.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb libexif/*.o libexif/*.lo libexif/*/*.o libexif/*/*.lo jpeg-*/*.o jpeg-*/*.lo *.pdb
