#!/bin/sh

# Download tar.gz
curl http://ftp.gnu.org/gnu/gcc/gcc-8.3.0/gcc-8.3.0.tar.gz --output gcc-os_2-8.3.0.tar.gz

# Extract contents
tar -xzvf gcc-os_2-8.3.0.tar.gz

# Apply patch
cd gcc-8.3.0
git init
git apply ../gcc-os_2-8.3.0.patch
cd ..

# Run autoconf
cd gcc-8.3.0
cd libstdc++-v3
autoconf2.64
cd ../..

# Install headers for correct build
cd ../..
make clean install-headers
cd toolchain/gcc-8.3.0

# Build
mkdir -p build-gcc
cd build-gcc
../gcc-8.3.0/configure --target=x86_64-os_2 --prefix=/home/eloc/Workspace/os/os_2/toolchain/cross --disable-nls --disable-lto --with-sysroot=/home/eloc/Workspace/os/os_2/sysroot --enable-languages=c,c++
make all-gcc -j5
make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=large -mno-red-zone' -j5
make all-target-libstdc++-v3 -j5

# Install
mkdir -p ../../cross
make install-gcc install-target-libgcc install-target-libstdc++-v3
cd ..

# Clean Up
rm -rf build-gcc
rm -rf gcc-8.3.0
rm gcc-os_2-8.3.0.tar.gz
