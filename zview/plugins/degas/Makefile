top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvdegas.slb\"

all:: degas.ldg zvdegas.slb

include $(top_srcdir)/rules

degas.ldg: degasldg.o degas.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdegas.slb: header.lo degasslb.lo degas.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

degasslb.lo degas.lo: ../common/plugin.h ../common/zview.h exports.h
degas.lo degas.o: degas.c packbits.c tanglbts.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
