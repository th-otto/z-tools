#!/bin/bash -xe

echo rvm_autoupdate_flag=0 >> ~/.rvmrc

sudo apt-get update
sudo apt-get install -y pkg-config curl xz-utils libjson-perl libwww-perl dos2unix

DOWNLOAD_DIR=http://tho-otto.de/snapshots
URL=http://tho-otto.de/download
SYSROOT_DIR=/usr/m68k-atari-mint/sys-root
sudo mkdir -p $SYSROOT_DIR/usr

PKG_CONFIG_LIBDIR=/usr/m68k-atari-mint/lib/pkgconfig
sudo mkdir -p $PKG_CONFIG_LIBDIR

mkdir -p tmp
cd tmp

#
# get & install the sharedlibs
#
for f in bzip2108-mint.zip exif0622-mint.tar.bz2 freetype2101-mint.zip lzma544-mint.zip tiff450-mint.zip zlib13-mint.zip jpeg8d-mint.tbz png1639-mint.tbz
do
	wget -q $URL/$f || exit 1
	case $f in
	*.zip)
		unzip $f >/dev/null
		;;
	*)
		tar xf $f >/dev/null
		;;
	esac
done

sudo cp -pr */usr/. $SYSROOT_DIR/usr/

#
# get & install a package with pkg-config files
# These are only needed to pass the configure script of xpdf
#
for f in shared-pkgconfigs.tar.bz2; do
	wget -q -O - $URL/$f | sudo tar --strip-components=1 -C / -xjf - || exit 1
done

#
# Get & install binutils & compiler
#
for package in binutils-mint-current gcc-mint-current mintbin-mint-latest; do
	wget -q -O - "$DOWNLOAD_DIR/${package%%-*}/${package}.tar.bz2" | sudo tar -C / -xjf - || exit 1
done

#
# Get & libraries
#
URL=$URL/mint

for f in mintlib-0.60.1-mint-dev.tar.xz \
	fdlibm-mint-dev.tar.xz \
	gemlib-0.44.0-mint-20230212-dev.tar.xz \
	windom-2.0.1-mint-dev.tar.xz \
	windom1-1.21.3-mint-dev.tar.xz \
	giflib-5.1.4-mint-dev.tar.xz \
	zstd-1.5.5-mint-dev.tar.xz \
	libwebp-1.3.2-mint-dev.tar.xz \
	plutosvg-0.0.3-mint-dev.tar.xz \
	plutovg-0.0.9-mint-dev.tar.xz \
	lunasvg-3.0.1-mint-dev.tar.xz 
do
	wget -q -O - $URL/$f | sudo tar -C / -xJf - || exit 1
done

cd ..
