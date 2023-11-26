top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 1k

CPPFLAGS		+= -I../../../common -I../common -I../..
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvstad.slb\"

all:: stad.ldg zvstad.slb

include $(top_srcdir)/rules

stad.ldg: stadldg.o stad.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvstad.slb: header.lo stadslb.lo stad.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo stadslb.lo stad.lo: ../common/plugin.h ../common/zview.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb
