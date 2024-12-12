top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars
CFLAGS += -Waggregate-return
CFLAGS += -Wdeclaration-after-statement

LIB       = -lz -lm
SFLAGS += -DZLIB_SLB

all:: svg.ldg zvsvg.slb

include $(top_srcdir)/rules

svg.ldg: ldg.o svg.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvsvg.slb: header.lo slb.lo svg.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lz.slb -lm $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo svg.lo: ../common/plugin.h ../common/zview.h exports.h
svg.o svg.lo: nanosvg.h nanorast.h libcmini.c strtol.c strchr.c strstr.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
