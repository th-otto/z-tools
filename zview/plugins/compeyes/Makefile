top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: compeyes.ldg zvceyes.slb

include $(top_srcdir)/rules

compeyes.ldg: ldg.o compeyes.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvceyes.slb: header.lo slb.lo compeyes.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo compeyes.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
