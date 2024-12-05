top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

SFLAGS += -DSHAREDLIB=\"zvgrani.slb\"

all:: gfartani.ldg zvgrani.slb

include $(top_srcdir)/rules

gfartani.ldg: gfartldg.o gfartani.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvgrani.slb: header.lo gfartslb.lo gfartani.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

gfartslb.lo gfartani.lo: ../common/plugin.h ../common/zview.h exports.h
gfartimg.lo gfartimg.o: gfartani.c gfatable.c gfadepac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
