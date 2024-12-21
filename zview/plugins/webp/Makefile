top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

LIB       = -lwebpdemux -lwebp -lz -lm

# libwebp headers don't have any version info, extract it from pkg-config
export PKG_CONFIG_PATH := $(shell CROSS_PREFIX=$(CROSS_PREFIX); CROSS_PREFIX=$${CROSS_PREFIX%-}; echo /usr/$${CROSS_PREFIX}/lib/pkgconfig)
WEBP_VERSION_STR := $(shell pkg-config --modversion libwebp)
CPPFLAGS += -DWEBP_VERSION_STR=\"$(WEBP_VERSION_STR)\"

all:: webp.ldg zvwebp.slb

include $(top_srcdir)/rules

webp.ldg: ldg.o webp.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvwebp.slb: header.lo slb.lo webp.lo $(webp_SOBJS)
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lwebpdemux -lwebp -lm $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo webp.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
