top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: recoil.ldg zvrecoil.slb

include $(top_srcdir)/rules

recoil.ldg: ldg.o recoimod.o recoil.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvrecoil.slb: header.lo slb.lo recoimod.lo recoil.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo recoimod.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
