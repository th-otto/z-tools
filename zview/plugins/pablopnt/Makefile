top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: pablopnt.ldg zvppp.slb

include $(top_srcdir)/rules

pablopnt.ldg: ldg.o pablopnt.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvppp.slb: header.lo slb.lo pablopnt.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo pablopnt.lo: ../common/plugin.h ../common/zview.h exports.h
pablopnt.lo pablopnt.o: pablopnt.c pppdepac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
