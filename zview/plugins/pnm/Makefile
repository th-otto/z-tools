top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvpnm.slb\"

all:: pnm.ldg zvpnm.slb

include $(top_srcdir)/rules

pnm.ldg: ldg.o pnm.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvpnm.slb: header.lo slb.lo pnm.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo pnm.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
