#!/bin/sh

set -e

VERSION=14.1.0
PATCH_DIR=$(realpath $(dirname -- "$0"))
PROJECT_ROOT=$(realpath "$PATCH_DIR"/../../..)
PREFIX="${IROS_PREFIX:-$PROJECT_ROOT/cross}"
SYSROOT="$PROJECT_ROOT"/build/x86_64/sysroot
NPROC=$(nproc)

curl -L --retry 5 "https://gcc.gnu.org/pub/gcc/releases/gcc-$VERSION/gcc-$VERSION.tar.xz" -o gcc.tar.xz
tar xf gcc.tar.xz
mv gcc-$VERSION src
rm -f gcc.tar.gz

cd src
git apply $PATCH_DIR/*.patch

cd libstdc++-v3
# Use explicit autoconf 2.69 on Ubuntu, but fall-back to the default autoconf for Nix.
autoconf2.69 || autoconf
cd ..
cd ..

mkdir -p build
cd build

# Install libccpp headers
mkdir -p "$SYSROOT"/usr/include
cp -r "$PROJECT_ROOT"/libs/ccpp/include/* "$SYSROOT"/usr/include

# Disable debug symbols.
export CFLAGS="-g0 -O3"
export CXXFLAGS="-g0 -O3"

../src/configure \
    --disable-nls \
    --target=x86_64-iros \
    --prefix="$PREFIX" \
    --with-sysroot="$SYSROOT" \
    --with-build-time-tools="$PREFIX/bin" \
    --enable-languages=c,c++

make all-gcc -j"$NPROC"
make install-gcc -j"$NPROC"

(
    # Build and install Iros libccpp
    cd "$PROJECT_ROOT"
    cmake --preset gcc_iros_x86_64_release_default -DIROS_NeverBuildDocs=ON -DIROS_BuildTests=OFF
    cmake --build --preset gcc_iros_x86_64_release_default --target libs/ccpp/install
    cmake --build --preset gcc_iros_x86_64_release_default --target libs/dius/install
)

make all-target-libgcc -j"$NPROC"
make install-target-libgcc -j"$NPROC"

make all-target-libstdc++-v3 -j"$NPROC"
make install-target-libstdc++-v3 -j"$NPROC"
