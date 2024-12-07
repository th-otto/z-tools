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
	crackart \
	cybrmate \
	dali \
	degas \
	degasx \
	delmpaint \
	deskpic \
	doodle \
	dunegraph \
	duo \
	d_graph \
	eggpaint \
	esm \
	eureka \
	ezartpro \
	flic \
	fluff64 \
	fnt \
	ftc \
	fullshow \
	funnypnt \
	gfartani \
	gfartimg \
	gft \
	gif \
	godpaint \
	grafix \
	hrm \
	iff_dctv \
	iff_rgbn \
	imagelab \
	imagic \
	img \
	imgscan \
	indypnt \
	inshape \
	jpg \
	macpaint \
	megapnt \
	mpp \
	mspaint \
	neoani \
	neochrome \
	pablopnt \
	paintpro \
	pblcpntr \
	pchrome \
	pci \
	pcl \
	pcx \
	picpackr \
	picworks \
	pixart \
	pl4 \
	png \
	pnm \
	pntshop \
	pntworks \
	pofo \
	prismpnt \
	pro89xx \
	psx \
	qpaint \
	rag_d \
	recoil \
	rembrndt \
	rgb \
	seq \
	signum \
	spectrum \
	specx \
	spooky \
	ssb \
	stad \
	sun \
	synarts \
	tga \
	tiff \
	tiny \
	zz_rough \
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
	rm -f */*.ldg */*.slb */*.o */*.lo */*/*.o */*/*.lo */*/*/*.o */*/*/*.lo */*.a */*.pdb 
	$(MAKE) -C libshare clean
