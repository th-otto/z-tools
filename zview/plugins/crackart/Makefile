top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvcart.slb\"

all:: crackart.ldg zvcart.slb

include $(top_srcdir)/rules

crackart.ldg: cartldg.o crackart.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvcart.slb: header.lo cartslb.lo crackart.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

ca_unpac.o: ca_unpac.s Makefile
	$(CC) $(ASFLAGS) -c $< -o $@

cartslb.lo crackart.lo: ../common/plugin.h ../common/zview.h
crackart.lo crackart.o: crackart.c ca_unpac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
