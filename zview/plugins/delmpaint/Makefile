top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvdelm.slb\"

all:: delmpnt.ldg zvdelm.slb

include $(top_srcdir)/rules

delmpnt.ldg: delmldg.o delmpnt.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdelm.slb: header.lo delmslb.lo delmpnt.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

delmslb.lo delmpnt.lo: ../common/plugin.h ../common/zview.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb
