top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: sgx.ldg zvsgx.slb

include $(top_srcdir)/rules

sgx.ldg: $(LDGSTART) ldg.o sgx.o fastlz.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ -nostartfiles $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB) -Wl,-Map,link.map

zvsgx.slb: header.lo slb.lo sgx.lo fastlz.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo sgx.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
