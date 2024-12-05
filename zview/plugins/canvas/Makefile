top_srcdir=../../..
include $(top_srcdir)/configvars

SFLAGS += -DSHAREDLIB=\"zvcanvas.slb\"

all:: canvas.ldg zvcanvas.slb

include $(top_srcdir)/rules

canvas.ldg: canvaldg.o canvas.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvcanvas.slb: header.lo canvaslb.lo canvas.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDSHAREDLIBC)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

canvaslb.lo canvas.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
