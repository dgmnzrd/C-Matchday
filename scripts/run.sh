#!/usr/bin/env bash
export PATH="/opt/homebrew/bin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/mbedtls/lib/pkgconfig:$PKG_CONFIG_PATH"
set -e

mkdir -p build
cd build
cmake ..
make
cd ..                # <— volvemos al root
./build/cmatchday 