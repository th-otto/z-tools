top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: delmpnt.ldg zvdelm.slb

include $(top_srcdir)/rules

delmpnt.ldg: ldg.o delmpnt.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdelm.slb: header.lo slb.lo delmpnt.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo delmpnt.lo: ../common/plugin.h ../common/zview.h exports.h
delmpnt.lo delmpnt.o: delmpnt.c ../crackart/ca_unpac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
