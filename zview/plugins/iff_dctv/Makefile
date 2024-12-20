top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: iff_dctv.ldg zvdctv.slb

include $(top_srcdir)/rules

iff_dctv.ldg: ldg.o iff_dctv.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdctv.slb: header.lo slb.lo iff_dctv.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo iff_dctv.lo: ../common/plugin.h ../common/zview.h exports.h

iff_dctv.lo iff_dctv.o: iff_dctv.c cvtdctv.c dctv.h dctvtabs.h ../degas/packbits.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
