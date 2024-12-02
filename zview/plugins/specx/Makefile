top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvspx.slb\"

all:: specx.ldg zvspx.slb

include $(top_srcdir)/rules

specx.ldg: spxldg.o specx.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvspx.slb: header.lo spxslb.lo specx.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

specxslb.lo specx.lo: ../common/plugin.h ../common/zview.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
