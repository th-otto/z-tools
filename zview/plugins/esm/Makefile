top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvesm.slb\"

all:: esm.ldg zvesm.slb

include $(top_srcdir)/rules

esm.ldg: ldg.o esm.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvesm.slb: header.lo slb.lo esm.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo esm.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
