top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: qoi.ldg zvqoi.slb

include $(top_srcdir)/rules

qoi.ldg: ldg.o qoi.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvqoi.slb: header.lo slb.lo qoi.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo qoi.lo: ../common/plugin.h ../common/zview.h exports.h
qoi.o qoi.lo: qoi.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
