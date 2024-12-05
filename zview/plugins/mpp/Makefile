top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvmpp.slb\"

all:: mpp.ldg zvmpp.slb

include $(top_srcdir)/rules

mpp.ldg: mppldg.o mpp.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvmpp.slb: header.lo mppslb.lo mpp.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

mppslb.lo mpp.lo: ../common/plugin.h ../common/zview.h exports.h
mpp.lo mpp.o: mpp.c mpp2bmp.c pixbuf.c pixbuf.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
