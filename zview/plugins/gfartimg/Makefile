top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvgrimg.slb\"

all:: gfartimg.ldg zvgrimg.slb

include $(top_srcdir)/rules

gfartimg.ldg: gfartldg.o gfartimg.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvgrimg.slb: header.lo gfartslb.lo gfartimg.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

gfartslb.lo gfartimg.lo: ../common/plugin.h ../common/zview.h
gfartimg.lo gfartimg.o: gfartimg.c ../gfartani/gfatable.c ../gfartani/gfadepac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
