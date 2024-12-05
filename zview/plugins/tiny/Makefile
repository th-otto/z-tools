top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: tiny.ldg zvtiny.slb

include $(top_srcdir)/rules

tiny.ldg: tinyldg.o tiny.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvtiny.slb: header.lo tinyslb.lo tiny.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

tinyslb.lo tiny.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
