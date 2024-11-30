top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvprism.slb\"

all:: prismpnt.ldg zvprism.slb

include $(top_srcdir)/rules

prismpnt.ldg: prismldg.o prismpnt.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvprism.slb: header.lo prismslb.lo prismpnt.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

prismslb.lo prismpnt.lo: ../common/plugin.h ../common/zview.h
prismpnt.lo prismpnt.o: prismpnt.c ../degas/packbits.c ../degas/tanglbts.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
