top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: pblcpntr.ldg zvcmp.slb

include $(top_srcdir)/rules

pblcpntr.ldg: ldg.o pblcpntr.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvcmp.slb: header.lo slb.lo pblcpntr.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo pblcpntr.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
