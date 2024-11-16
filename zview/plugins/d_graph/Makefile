top_srcdir=../../..
include $(top_srcdir)/configvars
CFLAGS += -Wextra

LDGOBJS = ../../ldglib/ldglib.o ../../ldglib/ldgcook.o ../../ldglib/ldgutil.o
STACKSIZE = 64k

CPPFLAGS		+= -I../../../common -I../common -I../.. -I.
SFLAGS += -DPLUGIN_SLB -DSHAREDLIB=\"zvdgraph.slb\"

all:: d_graph.ldg zvdgraph.slb

include $(top_srcdir)/rules

d_graph.ldg: dgrapldg.o d_graph.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdgraph.slb: header.lo dgrapslb.lo d_graph.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

dgrapslb.lo d_graph.lo: ../common/plugin.h ../common/zview.h
d_graph.lo d_graph.o: d_graph.c ../crackart/ca_unpac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb
