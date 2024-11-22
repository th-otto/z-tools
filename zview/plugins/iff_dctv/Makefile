top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvdctv.slb\"

all:: iff_dctv.ldg zvdctv.slb

include $(top_srcdir)/rules

iff_dctv.ldg: iffldg.o iff_dctv.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdctv.slb: header.lo iffslb.lo iff_dctv.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

iffslb.lo iff_dctv.lo: ../common/plugin.h ../common/zview.h

iff_dctv.lo iff_dctv.o: iff_dctv.c cvtdctv.c dctv.h dctvtabs.h ../degas/packbits.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb
