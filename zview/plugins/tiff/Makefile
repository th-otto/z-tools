top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars

LIB       = -ltiff -ljpeg -llzma -lzstd -lz

SFLAGS += -DTIFF_SLB -DJPEG_SLB -DLZMA_SLB -DZLIB_SLB

all:: tiff.ldg zvtiff.slb

include $(top_srcdir)/rules

tiff.ldg: ldg.o tiff.o $(LDGOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB) -lm

zvtiff.slb: header.lo slb.lo tiff.lo
	$(LDSHARED) $(CFLAGS) -o $@ $^ $(LDFLAGS) -ltiff.slb -ljpeg.slb -llzma.slb -lz.slb $(LDSHAREDLIBC) -lm
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo tiff.lo: ../common/plugin.h ../common/zview.h exports.h

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
