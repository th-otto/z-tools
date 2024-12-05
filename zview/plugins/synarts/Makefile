top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: synarts.ldg zvsrt.slb

include $(top_srcdir)/rules

synarts.ldg: synldg.o synarts.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvsrt.slb: header.lo synslb.lo synarts.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

synslb.lo synarts.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
