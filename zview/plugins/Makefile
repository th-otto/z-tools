top_srcdir=../..
include $(top_srcdir)/configvars

PLUGINS = \
	aim \
	animaster \
	animatic \
	arabesque \
	art \
	ascii \
	bmp \
	calamus \
	canvas \
	cel \
	coke \
	colorburst \
	colostar \
	colr \
	compeyes \
	degas \
	doodle \
	esm \
	eureka \
	gif \
	godpaint \
	img \
	jpg \
	neochrome \
	png \
	pnm \
	pofo \
	recoil \
	seq \
	signum \
	spectrum \
	stad \
	tga \
	tiff \
	webp \
	$(empty)

all:: $(PLUGINS)

.PHONY: $(PLUGINS)
$(PLUGINS): libshare
	$(MAKE) -C $@ all

libshare::
	$(MAKE) -C $@ all

dist:
	rm -rf ../_dist/codecs
	mkdir ../_dist/codecs
	rm -rf ../_dist/oldcodecs
	mkdir ../_dist/oldcodecs
	for f in $(PLUGINS); do \
		cp -a $$f/*.slb ../_dist/codecs; \
		cp -a $$f/*.ldg ../_dist/oldcodecs; \
	done
	if test -d purec/$(SLB_SUBDIR); then mkdir ../_dist/oldcodecs/purec; cp -a purec/$(SLB_SUBDIR)/* ../_dist/oldcodecs/purec; fi

clean:
	rm -f */*.ldg */*.slb */*.o */*.a */*.lo
	$(MAKE) -C libshare clean
