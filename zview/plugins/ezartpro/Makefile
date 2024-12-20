top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: ezartpro.ldg zvezap.slb

include $(top_srcdir)/rules

ezartpro.ldg: ldg.o ezartpro.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvezap.slb: header.lo slb.lo ezartpro.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo ezartpro.lo: ../common/plugin.h ../common/zview.h exports.h
ezartpro.lo ezartpro.o: ezartpro.c ../degas/packbits.c ../degas/tanglbts.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
