top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: eggpaint.ldg zveggpnt.slb

include $(top_srcdir)/rules

eggpaint.ldg: ldg.o eggpaint.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zveggpnt.slb: header.lo slb.lo eggpaint.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo eggpaint.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
