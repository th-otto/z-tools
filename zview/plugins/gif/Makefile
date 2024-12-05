top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

GIFLIB = -lgif
LIB       = $(GIFLIB)

SFLAGS += -DSHAREDLIB=\"zvgif.slb\"

all:: gif.ldg zvgif.slb

include $(top_srcdir)/rules

gif.ldg: gifldg.o gif.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvgif.slb: header.lo gifslb.lo gif.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(GIFLIB) $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

gifslb.lo gif.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
