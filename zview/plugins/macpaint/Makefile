top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvmacpnt.slb\"

all:: macpaint.ldg zvmacpnt.slb

include $(top_srcdir)/rules

macpaint.ldg: macldg.o macpaint.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvmacpnt.slb: header.lo macslb.lo macpaint.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

macslb.lo macpaint.lo: ../common/plugin.h ../common/zview.h
macpaint.lo macpaint.o: macpaint.c ../degas/packbits.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
