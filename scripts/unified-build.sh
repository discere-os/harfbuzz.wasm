#!/bin/bash
# HarfBuzz Unified Meson Build Script
# Copyright 2025 Superstruct Ltd, New Zealand
set -euo pipefail

BUILD_TYPE="${1:-standard}"

echo "[Build] HarfBuzz unified Meson build: $BUILD_TYPE"

# Validate tools
for cmd in emcc meson ninja; do
    if ! command -v $cmd >/dev/null 2>&1; then
        echo "❌ $cmd not found"
        exit 1
    fi
done

# Set PKG_CONFIG_PATH for dependencies
export PKG_CONFIG_PATH="${PKG_CONFIG_PATH:-}:$PWD/../freetype.wasm/install/wasm/pkgconfig:$PWD/../glib.wasm/install/wasm/pkgconfig"

# Clean previous builds
rm -rf build install

# Configure
echo "[Meson] Configuring build..."
meson setup build \
    --cross-file=emscripten-cross.ini \
    --prefix="$PWD/install" \
    --buildtype=release \
    --libdir=wasm \
    --bindir=wasm \
    -Ddefault_library=static \
    -Dtests=disabled \
    -Dwasm_build_type="$BUILD_TYPE" \
    -Dwasm_simd=true \
    -Dwasm_threading=true \
    -Dglib=disabled \
    -Dgobject=disabled \
    -Dcairo=disabled \
    -Dchafa=disabled \
    -Dicu=disabled \
    -Dgraphite2=disabled \
    -Dfreetype=enabled \
    -Dutilities=disabled \
    -Dintrospection=disabled \
    -Ddocs=disabled

# Compile
echo "[Meson] Compiling..."
meson compile -C build

# Install
echo "[Meson] Installing..."
meson install -C build

# Optimize with wasm-opt if available
if command -v wasm-opt >/dev/null 2>&1; then
    echo "[wasm-opt] Optimizing SIDE_MODULE..."
    if [ -f install/wasm/harfbuzz-side.wasm ]; then
        wasm-opt -O3 -c install/wasm/harfbuzz-side.wasm \
            -o install/wasm/harfbuzz-side.wasm
    fi
fi

# Report sizes
echo ""
echo "✅ Build complete!"
echo ""
echo "Output files:"
ls -lh install/wasm/harfbuzz-* 2>/dev/null || echo "  (no outputs found)"
