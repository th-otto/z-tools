top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zveureka.slb\"

all:: eureka.ldg zveureka.slb

include $(top_srcdir)/rules

eureka.ldg: ldg.o eureka.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zveureka.slb: header.lo slb.lo eureka.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo eureka.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
