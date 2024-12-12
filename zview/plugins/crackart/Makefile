top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvcart.slb\"

all:: crackart.ldg zvcart.slb

include $(top_srcdir)/rules

crackart.ldg: ldg.o crackart.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvcart.slb: header.lo slb.lo crackart.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

ca_unpac.o: ca_unpac.s Makefile
	$(CC) $(ASFLAGS) -c $< -o $@

slb.lo crackart.lo: ../common/plugin.h ../common/zview.h exports.h
crackart.lo crackart.o: crackart.c ca_unpac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
