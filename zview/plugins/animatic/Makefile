top_srcdir=../../..
include $(top_srcdir)/configvars

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 1k

CPPFLAGS		+= -I../../../common -I../common -I../..
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvflm.slb\"

all:: animatic.ldg zvflm.slb

include $(top_srcdir)/rules

animatic.ldg: animaldg.o anima.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvflm.slb: header.lo animaslb.lo anima.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo animaslb.lo anima.lo: ../common/plugin.h ../common/zview.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb
