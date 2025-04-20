#!/usr/bin/env bash
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/mbedtls/lib/pkgconfig:$PKG_CONFIG_PATH"
set -e

mkdir -p build
cd build
cmake ..
make
./cmatchday