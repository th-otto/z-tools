top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

all:: d_graph.ldg zvdgraph.slb

include $(top_srcdir)/rules

d_graph.ldg: ldg.o d_graph.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvdgraph.slb: header.lo slb.lo d_graph.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo d_graph.lo: ../common/plugin.h ../common/zview.h exports.h
d_graph.lo d_graph.o: d_graph.c ../crackart/ca_unpac.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
