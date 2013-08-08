#!/bin/bash

set -ex

uname -a
free

BUILD=${WORKSPACE}/build
mkdir -p ${BUILD}/lib64
rm -f ${BUILD}/lib
ln -s ${BUILD}/lib64 ${BUILD}/lib

export PATH=$PATH:${BUILD}/bin
export CFLAGS="-I ${BUILD}/include"
export PKG_CONFIG_PATH="${BUILD}/lib/pkgconfig"

if [ ! -f ${BUILD}/bin/hd ]; then
    wget https://github.com/kazum/build-sheepdog/raw/master/hd.c
    gcc -o ${BUILD}/bin/hd hd.c
fi

#if [ ! -f ${BUILD}/lib/pkgconfig/libffi.pc ]; then
if [ $(pkg-config --exists libffi || echo true) ]; then

wget ftp://sourceware.org:/pub/libffi/libffi-3.0.13.tar.gz > /dev/null 2>&1
tar zxf libffi-3.0.13.tar.gz
cd libffi-3.0.13
./configure --prefix=${BUILD} > /dev/null 2>&1
make install -j4 > build.log 2>&1
make clean > /dev/null 2>&1
cd ..

fi

#if [ ! -f ${BUILD}/include/glib-2.0/glib.h ]; then
if [ $(pkg-config --exists glib-2.0 || echo true) ]; then

wget http://ftp.gnome.org/pub/gnome/sources/glib/2.34/glib-2.34.3.tar.xz > /dev/null 2>&1
tar Jxf glib-2.34.3.tar.xz
cd glib-2.34.3
./configure --prefix=${BUILD} --disable-modular-tests --disable-dependency-tracking --disable-gtk-doc-html > /dev/null 2>&1
make install -j4 > build.log 2>&1
make clean > /dev/null 2>&1
cd ..

fi

#if [ ! -f ${BUILD}/include/pixman-1/pixman.h ]; then
if [ $(pkg-config --exists pixman-1 || echo true) ]; then

wget http://cairographics.org/releases/pixman-0.28.2.tar.gz > /dev/null 2>&1
tar zxf pixman-0.28.2.tar.gz
cd pixman-0.28.2
./configure --prefix=${BUILD} --disable-dependency-tracking > /dev/null 2>&1
make install -j4 > build.log 2>&1
make clean > /dev/null 2>&1
cd ..

fi

if [ ! -f ${BUILD}/bin/qemu-io ]; then

wget https://github.com/kazum/qemu/archive/master.zip -O qemu.zip > /dev/null 2>&1
rm -rf qemu-master
unzip qemu.zip > /dev/null 2>&1
cd qemu-master
./configure --prefix=${BUILD} --target-list= > /dev/null 2>&1
make qemu-io -j4 > build.log 2>&1
make qemu-img -j4 > build.log 2>&1
cp qemu-io ${BUILD}/bin/
cp qemu-img ${BUILD}/bin/
make clean > /dev/null 2>&1

cd ..

fi

#if [ ! -f ${BUILD}/include/urcu/uatomic.h ]; then
if [ $(pkg-config --exists liburcu || echo true) ]; then

wget http://lttng.org/files/urcu/userspace-rcu-0.7.6.tar.bz2 > /dev/null 2>&1
tar jxf userspace-rcu-0.7.6.tar.bz2
cd userspace-rcu-0.7.6
./configure --prefix=${BUILD} --disable-dependency-tracking > /dev/null 2>&1
make install -j4 > build.log 2>&1
make clean > /dev/null 2>&1
sed -i 's/void _*uatomic_link_error *()/void __uatomic_link_error(void)/g' ${BUILD}/include/urcu/uatomic/generic.h

cd ..

fi


# if [ ! -f ${BUILD}/bin/valgrind ]; then
# wget http://valgrind.org/downloads/valgrind-3.8.1.tar.bz2 > /dev/null 2>&1
# tar jxf valgrind-3.8.1.tar.bz2
# cd valgrind-3.8.1
# ./configure --prefix=${BUILD} --disable-dependency-tracking > /dev/null 2>&1
# make install > /dev/null 2>&1
# make clean > /dev/null 2>&1
# cd ..
# fi

./autogen.sh
./configure --disable-corosync --enable-debug --enable-fatal-warnings
make

export WD=${WORKSPACE}/results
./tests/functional/check
./tests/functional/check -g md -md
