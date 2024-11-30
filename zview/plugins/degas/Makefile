top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvdegas.slb\"

all:: degas.ldg zvdegas.slb

include $(top_srcdir)/rules

degas.ldg: degasldg.o degas.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdegas.slb: header.lo degasslb.lo degas.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

degasslb.lo degas.lo: ../common/plugin.h ../common/zview.h
degas.lo degas.o: degas.c packbits.c tanglbts.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
