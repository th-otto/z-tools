top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: spec512.ldg zvspc512.slb

include $(top_srcdir)/rules

spec512.ldg: ldg.o spec512.o spectrum.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvspc512.slb: header.lo slb.lo spec512.lo spectrum.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo spec512.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
