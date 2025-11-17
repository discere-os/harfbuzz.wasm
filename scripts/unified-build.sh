#!/bin/bash
# Unified Build System for HarfBuzz.wasm
# Copyright 2025 Superstruct Ltd, New Zealand
# Part of the Discere OS unified build system

set -euo pipefail

BUILD_TYPE="${1:-standard}"
CLEAN="${CLEAN:-false}"
FETCH_ONLY="${FETCH_ONLY:-false}"

echo "🔨 Unified Build System for harfbuzz.wasm"
echo "Build Type: $BUILD_TYPE"
echo "======================================"

# 1. Validate tools
echo "📋 Validating build tools..."
command -v emcc >/dev/null 2>&1 || { echo "❌ emcc not found. Install Emscripten first."; exit 1; }
command -v meson >/dev/null 2>&1 || { echo "❌ meson not found. Install Meson first."; exit 1; }
command -v ninja >/dev/null 2>&1 || { echo "❌ ninja not found. Install Ninja first."; exit 1; }
echo "✅ All build tools available"

# Print versions
echo "   emcc version: $(emcc --version | head -n1)"
echo "   meson version: $(meson --version)"

# 2. Clean if requested
if [[ "$CLEAN" == "true" ]]; then
  echo "🧹 Cleaning build artifacts..."
  rm -rf build build-* install
  echo "✅ Cleaned build artifacts"
fi

# 3. Fetch or build dependencies
if [[ -f "dependencies.json" ]]; then
  echo "📦 Fetching dependencies..."
  if [[ -f "scripts/fetch-dependencies.sh" ]]; then
    bash scripts/fetch-dependencies.sh || echo "⚠️  Dependency fetch failed, will build locally"
  else
    echo "⚠️  No dependency fetch script found, building locally"
  fi
fi

if [[ "$FETCH_ONLY" == "true" ]]; then
  echo "✅ Dependencies fetched"
  exit 0
fi

# 4. Configure Meson
echo "⚙️  Configuring Meson build..."
meson setup build \
  --cross-file=emscripten-cross.ini \
  --prefix="$(pwd)/install" \
  -Dwasm_build_type="$BUILD_TYPE" \
  -Ddefault_library=static \
  -Dtests=disabled \
  -Dbuildtype=release \
  -Dlibdir=wasm \
  -Dbindir=wasm \
  || { echo "❌ Meson configuration failed"; exit 1; }

echo "✅ Meson configured successfully"

# 5. Build
echo "🔨 Building WASM binaries..."
meson compile -C build || { echo "❌ Build failed"; exit 1; }
echo "✅ Build completed"

# 6. Install
echo "📦 Installing to $(pwd)/install/wasm..."
meson install -C build || { echo "❌ Install failed"; exit 1; }
echo "✅ Installation completed"

# 7. Post-process with wasm-opt (if available)
if command -v wasm-opt >/dev/null 2>&1; then
  echo "⚡ Optimizing WASM binaries with wasm-opt..."
  for wasm_file in install/wasm/*.wasm; do
    if [[ -f "$wasm_file" ]]; then
      wasm-opt -O3 "$wasm_file" -o "$wasm_file.opt"
      mv "$wasm_file.opt" "$wasm_file"
      echo "   ✅ Optimized $(basename $wasm_file)"
    fi
  done
else
  echo "ℹ️  wasm-opt not found, skipping additional optimization"
fi

# 8. Generate manifest
echo "📄 Generating manifest..."
cat > install/wasm/manifest.json <<EOF
{
  "library": "harfbuzz",
  "version": "$(git describe --tags --always 2>/dev/null || echo 'unknown')",
  "buildType": "$BUILD_TYPE",
  "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "files": $(cd install/wasm && ls -1 | jq -R . | jq -s . 2>/dev/null || echo '[]')
}
EOF

# 9. Display build summary
echo ""
echo "✅ Build complete: install/wasm/"
echo "======================================"
ls -lh install/wasm/ 2>/dev/null || echo "No files found"
echo ""
echo "📊 Build Summary:"
for wasm_file in install/wasm/*.wasm; do
  if [[ -f "$wasm_file" ]]; then
    size=$(stat -c%s "$wasm_file" 2>/dev/null || stat -f%z "$wasm_file" 2>/dev/null || echo "0")
    size_kb=$((size / 1024))
    echo "   $(basename $wasm_file): ${size_kb}KB"
  fi
done

echo ""
echo "🎉 Build successful! Use 'deno task test' to verify."
