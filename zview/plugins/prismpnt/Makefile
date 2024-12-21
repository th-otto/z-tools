top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: prismpnt.ldg zvprism.slb

include $(top_srcdir)/rules

prismpnt.ldg: ldg.o prismpnt.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvprism.slb: header.lo slb.lo prismpnt.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo prismpnt.lo: ../common/plugin.h ../common/zview.h exports.h
prismpnt.lo prismpnt.o: prismpnt.c ../degas/packbits.c ../degas/tanglbts.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
