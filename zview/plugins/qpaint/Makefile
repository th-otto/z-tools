top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvqpbx.slb\"

all:: qpaint.ldg zvqpbx.slb

include $(top_srcdir)/rules

qpaint.ldg: qpbxldg.o qpaint.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvqpbx.slb: header.lo qpbxslb.lo qpaint.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

qpbxslb.lo qpaint.lo: ../common/plugin.h ../common/zview.h
qpaint.lo qpaint.o: qpaint.c ../degas/packbits.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
