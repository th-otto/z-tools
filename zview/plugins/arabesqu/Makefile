top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvabm.slb\"

all:: arabesqu.ldg zvabm.slb

include $(top_srcdir)/rules

arabesqu.ldg: ldg.o abm.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvabm.slb: header.lo slb.lo abm.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo abm.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
