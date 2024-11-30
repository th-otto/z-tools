top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvppp.slb\"

all:: pablopnt.ldg zvppp.slb

include $(top_srcdir)/rules

pablopnt.ldg: pppldg.o pablopnt.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvppp.slb: header.lo pppslb.lo pablopnt.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

pppslb.lo pablopnt.lo: ../common/plugin.h ../common/zview.h
pablopnt.lo pablopnt.o: pablopnt.c pppdepac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
