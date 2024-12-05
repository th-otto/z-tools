top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvdelm.slb\"

all:: delmpnt.ldg zvdelm.slb

include $(top_srcdir)/rules

delmpnt.ldg: delmldg.o delmpnt.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdelm.slb: header.lo delmslb.lo delmpnt.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

delmslb.lo delmpnt.lo: ../common/plugin.h ../common/zview.h exports.h
delmpnt.lo delmpnt.o: delmpnt.c ../crackart/ca_unpac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
