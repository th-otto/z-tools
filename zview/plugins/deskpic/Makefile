top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: deskpic.ldg zvgfb.slb

include $(top_srcdir)/rules

deskpic.ldg: ldg.o deskpic.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvgfb.slb: header.lo slb.lo deskpic.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo deskpic.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
