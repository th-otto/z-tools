top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: fluff64.ldg zvfluff.slb

include $(top_srcdir)/rules

fluff64.ldg: ldg.o fluff64.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvfluff.slb: header.lo slb.lo fluff64.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo fluff64.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
