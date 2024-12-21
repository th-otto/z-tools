top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: macpaint.ldg zvmacpnt.slb

include $(top_srcdir)/rules

macpaint.ldg: ldg.o macpaint.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvmacpnt.slb: header.lo slb.lo macpaint.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo macpaint.lo: ../common/plugin.h ../common/zview.h exports.h
macpaint.lo macpaint.o: macpaint.c ../degas/packbits.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
