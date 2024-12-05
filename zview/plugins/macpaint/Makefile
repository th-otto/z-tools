top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvmacpnt.slb\"

all:: macpaint.ldg zvmacpnt.slb

include $(top_srcdir)/rules

macpaint.ldg: macldg.o macpaint.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvmacpnt.slb: header.lo macslb.lo macpaint.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

macslb.lo macpaint.lo: ../common/plugin.h ../common/zview.h exports.h
macpaint.lo macpaint.o: macpaint.c ../degas/packbits.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
