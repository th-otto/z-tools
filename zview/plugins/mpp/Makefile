top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvmpp.slb\"

all:: mpp.ldg zvmpp.slb

include $(top_srcdir)/rules

mpp.ldg: mppldg.o mpp.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvmpp.slb: header.lo mppslb.lo mpp.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

mppslb.lo mpp.lo: ../common/plugin.h ../common/zview.h
mpp.lo mpp.o: mpp.c mpp2bmp.c pixbuf.c pixbuf.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
