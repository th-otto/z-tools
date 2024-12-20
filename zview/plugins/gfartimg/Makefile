top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: gfartimg.ldg zvgrimg.slb

include $(top_srcdir)/rules

gfartimg.ldg: ldg.o gfartimg.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvgrimg.slb: header.lo slb.lo gfartimg.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo gfartimg.lo: ../common/plugin.h ../common/zview.h exports.h
gfartimg.lo gfartimg.o: gfartimg.c ../gfartani/gfatable.c ../gfartani/gfadepac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
