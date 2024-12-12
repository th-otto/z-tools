top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvqpbx.slb\"

all:: qpaint.ldg zvqpbx.slb

include $(top_srcdir)/rules

qpaint.ldg: ldg.o qpaint.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvqpbx.slb: header.lo slb.lo qpaint.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo qpaint.lo: ../common/plugin.h ../common/zview.h exports.h
qpaint.lo qpaint.o: qpaint.c ../degas/packbits.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
