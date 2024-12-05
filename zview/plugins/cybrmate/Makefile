top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvdlt.slb\"

all:: cybrmate.ldg zvdlt.slb

include $(top_srcdir)/rules

cybrmate.ldg: cybrldg.o cybrmate.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdlt.slb: header.lo cybrslb.lo cybrmate.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

cybrslb.lo cybrmate.lo: ../common/plugin.h ../common/zview.h exports.h
cybrmate.lo cybrmate.o: cybrmate.c ../degas/packbits.c ../degas/tanglbts.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
