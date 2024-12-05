top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: pntworks.ldg zvpwgp.slb

include $(top_srcdir)/rules

pntworks.ldg: pwgpldg.o pntworks.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvpwgp.slb: header.lo pwgpslb.lo pntworks.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

pwgpslb.lo pntworks.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
