top_srcdir=../../..
include $(top_srcdir)/configvars
include ../pluginvars
CFLAGS += -Waggregate-return
CFLAGS += -Wdeclaration-after-statement

LIB       = -lz -llunasvg -lplutovg -lm
SFLAGS += -DZLIB_SLB

all:: svg.ldg zvsvg.slb

include $(top_srcdir)/rules

svg.ldg: ldg.o svgluna.o $(LDGOBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,--stack,$(STACKSIZE) $(LIB)

zvsvg.slb: header.lo slb.lo svgluna.lo
	$(LDSHAREDXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) -llunasvg -lplutovg -lz.slb -lm $(LDSHAREDLIBCXX)
	$(STRIPEX) $@

header.lo: ../common/header.S exports.h
	$(CC) $(CFLAGS) $(SFLAGS) -c $< -o $@

slb.lo svg.lo: ../common/plugin.h ../common/zview.h exports.h
svgnano.o svgnano.lo: nanosvg.h nanorast.h libcmini.c strtol.c strchr.c strstr.c

clean:
	rm -f *.o *.lo *.a *.ldg *.slb *.pdb
