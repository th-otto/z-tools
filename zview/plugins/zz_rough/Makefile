top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: zz_rough.ldg zvrgh.slb

include $(top_srcdir)/rules

zz_rough.ldg: rghldg.o zz_rough.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvrgh.slb: header.lo rghslb.lo zz_rough.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

rghslb.lo zz_rough.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
