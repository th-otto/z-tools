top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvmspnt.slb\"

all:: mspaint.ldg zvmspnt.slb

include $(top_srcdir)/rules

mspaint.ldg: mspldg.o mspaint.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvmspnt.slb: header.lo mspslb.lo mspaint.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

mspslb.lo mspaint.lo: ../common/plugin.h ../common/zview.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb
