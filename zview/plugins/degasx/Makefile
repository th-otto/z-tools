top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: degasx.ldg zvdegasx.slb

include $(top_srcdir)/rules

degasx.ldg: ldg.o degasx.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdegasx.slb: header.lo slb.lo degasx.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo degasx.lo: ../common/plugin.h ../common/zview.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
