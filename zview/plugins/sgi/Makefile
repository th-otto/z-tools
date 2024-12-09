top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: sgi.ldg zvsgi.slb

include $(top_srcdir)/rules

sgi.ldg: ldg.o sgi.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvsgi.slb: header.lo slb.lo sgi.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo sgi.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
