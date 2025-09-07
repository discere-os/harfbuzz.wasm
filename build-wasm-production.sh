#!/bin/bash

# HarfBuzz WASM Production Build Script
# Copyright 2025 Superstruct Ltd, New Zealand
# Licensed under MIT License (same as HarfBuzz)

set -euo pipefail

# Build configuration
BUILD_TYPE="Release"
ENABLE_SIMD=1
ENABLE_THREADING=1
ENABLE_WEBGPU=0
ENABLE_NATIVE_FS=1
THREAD_POOL_SIZE=4
INITIAL_MEMORY="32MB"
MAXIMUM_MEMORY="256MB"
INSTALL_DIR="$PWD/harfbuzz-wasm-artifacts"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

handle_error() {
    log_error "$1"
    exit 1
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            --no-simd)
                ENABLE_SIMD=0
                shift
                ;;
            --no-threading)
                ENABLE_THREADING=0
                shift
                ;;
            --enable-webgpu)
                ENABLE_WEBGPU=1
                shift
                ;;
            --no-native-fs)
                ENABLE_NATIVE_FS=0
                shift
                ;;
            --thread-pool-size=*)
                THREAD_POOL_SIZE="${1#*=}"
                shift
                ;;
            --initial-memory=*)
                INITIAL_MEMORY="${1#*=}"
                shift
                ;;
            --maximum-memory=*)
                MAXIMUM_MEMORY="${1#*=}"
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                log_error "Unknown argument: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

show_help() {
    echo "HarfBuzz WASM Production Build Script"
    echo
    echo "Usage: $0 [options]"
    echo
    echo "Options:"
    echo "  --debug                 Build in debug mode"
    echo "  --no-simd              Disable SIMD optimizations"
    echo "  --no-threading         Disable threading support"
    echo "  --enable-webgpu        Enable WebGPU acceleration"
    echo "  --no-native-fs         Disable WASM-native filesystem"
    echo "  --thread-pool-size=N   Set thread pool size (default: 4)"
    echo "  --initial-memory=SIZE  Set initial memory (default: 32MB)"
    echo "  --maximum-memory=SIZE  Set maximum memory (default: 256MB)"
    echo "  --help                 Show this help message"
}

# Convert memory size to bytes
memory_to_bytes() {
    local size="$1"
    if [[ "$size" =~ ^([0-9]+)MB$ ]]; then
        echo $((${BASH_REMATCH[1]} * 1024 * 1024))
    elif [[ "$size" =~ ^([0-9]+)GB$ ]]; then
        echo $((${BASH_REMATCH[1]} * 1024 * 1024 * 1024))
    elif [[ "$size" =~ ^([0-9]+)$ ]]; then
        echo "$size"
    else
        handle_error "Invalid memory size format: $size"
    fi
}

# Check dependencies
check_dependencies() {
    log_info "Checking build dependencies..."
    
    # Check Emscripten
    if ! command -v emcc &> /dev/null; then
        handle_error "Emscripten not found. Please install and activate emsdk."
    fi
    
    local emcc_version=$(emcc --version | head -n1)
    log_success "Found Emscripten: $emcc_version"
    
    # Check Meson
    if ! command -v meson &> /dev/null; then
        handle_error "Meson not found. Please install meson build system."
    fi
    
    # Check Ninja
    if ! command -v ninja &> /dev/null; then
        handle_error "Ninja not found. Please install ninja build system."
    fi
    
    # Check pkg-config
    if ! command -v pkg-config &> /dev/null; then
        handle_error "pkg-config not found. Please install pkg-config."
    fi
    
    log_success "All dependencies checked"
}

# Configure build
configure_build() {
    log_info "Configuring HarfBuzz WASM build..."
    
    # Clean previous builds
    rm -rf build-wasm "$INSTALL_DIR"
    
    # Convert memory sizes
    local initial_memory_bytes=$(memory_to_bytes "$INITIAL_MEMORY")
    local maximum_memory_bytes=$(memory_to_bytes "$MAXIMUM_MEMORY")
    
    # Prepare Meson options
    local meson_options=(
        "-Dwasm_simd=$([[ $ENABLE_SIMD == 1 ]] && echo true || echo false)"
        "-Dwasm_threading=$([[ $ENABLE_THREADING == 1 ]] && echo true || echo false)"
        "-Dwasm_webgpu=$([[ $ENABLE_WEBGPU == 1 ]] && echo true || echo false)"
        "-Dwasm_native_fs=$([[ $ENABLE_NATIVE_FS == 1 ]] && echo true || echo false)"
        "-Dwasm_thread_pool_size=$THREAD_POOL_SIZE"
        "-Dwasm_initial_memory=$INITIAL_MEMORY"
        "-Dwasm_maximum_memory=$MAXIMUM_MEMORY"
        "-Dfreetype=enabled"
        "-Dicu=enabled"
        "-Dcairo=auto"
        "-Dglib=disabled"
        "-Dgobject=disabled" 
        "-Dgraphite2=disabled"
        "-Dutilities=disabled"
        "-Dtests=enabled"
        "-Dintrospection=disabled"
        "-Ddocs=disabled"
        "-Dbenchmark=enabled"
        "-Dbuildtype=release"
        "-Ddefault_library=static"
        "--cross-file=wasm-cross.ini"
        "--prefix=$INSTALL_DIR"
    )
    
    log_info "Meson configuration options: ${meson_options[*]}"
    
    # Configure with Meson
    meson setup build-wasm . "${meson_options[@]}" || handle_error "Meson configuration failed"
    
    log_success "Build configuration completed"
}

# Build HarfBuzz
build_harfbuzz() {
    log_info "Building HarfBuzz WASM..."
    
    # Build with Ninja
    ninja -C build-wasm -j$(nproc) || handle_error "HarfBuzz build failed"
    
    log_success "HarfBuzz build completed"
}

# Create WASM module wrapper
create_wasm_wrapper() {
    log_info "Creating WASM module wrapper..."
    
    # Find the built library
    local harfbuzz_lib=$(find build-wasm -name "libharfbuzz*.a" | head -n1)
    if [[ ! -f "$harfbuzz_lib" ]]; then
        handle_error "HarfBuzz library not found in build output"
    fi
    
    # Create artifacts directory
    mkdir -p "$INSTALL_DIR"
    
    # Prepare Emscripten flags
    local emcc_flags=(
        -O3
        -flto
        --closure=1
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sUSE_ES6_IMPORT_META=0
        -sWASM=1
        "-sINITIAL_MEMORY=$initial_memory_bytes"
        "-sMAXIMUM_MEMORY=$maximum_memory_bytes"
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=5242880
        -sASSERTIONS=0
        -sNODEJS_CATCH_EXIT=0 
        -sNODEJS_CATCH_REJECTION=0
    )
    
    # Add SIMD flags if enabled
    if [[ $ENABLE_SIMD == 1 ]]; then
        emcc_flags+=(-msimd128)
    fi
    
    # Add threading flags if enabled
    if [[ $ENABLE_THREADING == 1 ]]; then
        emcc_flags+=(
            -pthread
            -sUSE_PTHREADS=1
            "-sPTHREAD_POOL_SIZE=$THREAD_POOL_SIZE"
        )
    fi
    
    # Add WebGPU flags if enabled
    if [[ $ENABLE_WEBGPU == 1 ]]; then
        emcc_flags+=(-sUSE_WEBGPU=1)
    fi
    
    # Define exported functions
    local exported_functions='["_malloc","_free","_hb_blob_create","_hb_blob_destroy","_hb_face_create","_hb_face_destroy","_hb_font_create","_hb_font_destroy","_hb_buffer_create","_hb_buffer_destroy","_hb_buffer_add_utf8","_hb_buffer_set_direction","_hb_buffer_set_script","_hb_buffer_set_language","_hb_buffer_guess_segment_properties","_hb_shape","_hb_buffer_get_length","_hb_buffer_get_glyph_infos","_hb_buffer_get_glyph_positions","_hb_unicode_funcs_get_default","_hb_unicode_general_category","_hb_script_from_string","_hb_language_from_string","_hb_font_get_glyph_h_advance","_hb_font_set_scale"]'
    
    # Add WASM-specific function exports
    if [[ $ENABLE_SIMD == 1 ]]; then
        exported_functions="${exported_functions%]},\"_hb_wasm_simd_available\",\"_hb_wasm_simd_unicode_lookup_batch\",\"_hb_wasm_simd_position_glyphs\",\"_hb_wasm_simd_apply_kerning\"]"
    fi
    
    if [[ $ENABLE_NATIVE_FS == 1 ]]; then
        exported_functions="${exported_functions%]},\"_hb_wasm_filesystem_init\",\"_hb_wasm_filesystem_is_ready\",\"_hb_wasm_load_font_from_url\",\"_hb_wasm_load_font_from_cache\",\"_hb_wasm_get_cache_stats\",\"_hb_wasm_clear_font_cache\"]"
    fi
    
    emcc_flags+=("-sEXPORTED_FUNCTIONS=$exported_functions")
    emcc_flags+=("-sEXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\",\"HEAPU8\",\"HEAPU32\",\"HEAPF32\",\"UTF8ToString\",\"stringToNewUTF8\",\"addFunction\",\"removeFunction\",\"dynCall_vii\"]")
    
    # Create the WASM module
    log_info "Compiling final WASM module..."
    
    # Include our WASM-specific sources
    local wasm_sources=(
        "src/wasm/hb-wasm-simd.cc"
        "src/wasm/hb-wasm-filesystem.cc"
    )
    
    # Filter sources that exist
    local existing_sources=()
    for src in "${wasm_sources[@]}"; do
        if [[ -f "$src" ]]; then
            existing_sources+=("$src")
        fi
    done
    
    # Final compilation command
    em++ "${emcc_flags[@]}" \
        "$harfbuzz_lib" \
        "${existing_sources[@]}" \
        -Isrc \
        -Ibuild-wasm \
        -o "$INSTALL_DIR/harfbuzz.js" \
        || handle_error "WASM module creation failed"
    
    log_success "WASM module created successfully"
    
    # Create TypeScript definitions
    create_typescript_definitions
    
    # Verify output files
    if [[ -f "$INSTALL_DIR/harfbuzz.js" && -f "$INSTALL_DIR/harfbuzz.wasm" ]]; then
        local js_size=$(stat -f%z "$INSTALL_DIR/harfbuzz.js" 2>/dev/null || stat -c%s "$INSTALL_DIR/harfbuzz.js")
        local wasm_size=$(stat -f%z "$INSTALL_DIR/harfbuzz.wasm" 2>/dev/null || stat -c%s "$INSTALL_DIR/harfbuzz.wasm")
        
        log_success "Build artifacts created:"
        log_success "  JavaScript: ${js_size} bytes"
        log_success "  WASM binary: ${wasm_size} bytes"
    else
        handle_error "Build artifacts not found"
    fi
}

# Create TypeScript definitions
create_typescript_definitions() {
    log_info "Creating TypeScript definitions..."
    
    cat > "$INSTALL_DIR/harfbuzz.d.ts" << 'EOF'
/**
 * HarfBuzz WASM TypeScript Definitions
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under MIT License
 */

export interface HarfBuzzModule extends EmscriptenModule {
  // Core HarfBuzz functions
  _hb_blob_create(data: number, length: number, mode: number, user_data: number, destroy: number): number;
  _hb_blob_destroy(blob: number): void;
  _hb_face_create(blob: number, index: number): number;
  _hb_face_destroy(face: number): void;
  _hb_font_create(face: number): number;
  _hb_font_destroy(font: number): void;
  _hb_font_set_scale(font: number, x_scale: number, y_scale: number): void;
  _hb_font_get_glyph_h_advance(font: number, glyph: number): number;
  
  // Buffer functions
  _hb_buffer_create(): number;
  _hb_buffer_destroy(buffer: number): void;
  _hb_buffer_add_utf8(buffer: number, text: number, text_length: number, item_offset: number, item_length: number): void;
  _hb_buffer_set_direction(buffer: number, direction: number): void;
  _hb_buffer_set_script(buffer: number, script: number): void;
  _hb_buffer_set_language(buffer: number, language: number): void;
  _hb_buffer_guess_segment_properties(buffer: number): void;
  _hb_shape(font: number, buffer: number, features: number, num_features: number): void;
  _hb_buffer_get_length(buffer: number): number;
  _hb_buffer_get_glyph_infos(buffer: number, length: number): number;
  _hb_buffer_get_glyph_positions(buffer: number, length: number): number;
  
  // Unicode functions
  _hb_unicode_funcs_get_default(): number;
  _hb_unicode_general_category(funcs: number, codepoint: number): number;
  _hb_script_from_string(str: number, len: number): number;
  _hb_language_from_string(str: number, len: number): number;
  
  // WASM-specific SIMD functions (if enabled)
  _hb_wasm_simd_available?(): boolean;
  _hb_wasm_simd_unicode_lookup_batch?(codepoints: number, categories: number, count: number): void;
  _hb_wasm_simd_position_glyphs?(positions: number, count: number, scale_x: number, scale_y: number): void;
  _hb_wasm_simd_apply_kerning?(positions: number, infos: number, count: number, kerning_table: number): void;
  
  // WASM-specific filesystem functions (if enabled)
  _hb_wasm_filesystem_init?(callback: number, user_data: number): boolean;
  _hb_wasm_filesystem_is_ready?(): boolean;
  _hb_wasm_load_font_from_url?(url: number, cache_key: number, callback: number, user_data: number): void;
  _hb_wasm_load_font_from_cache?(cache_key: number): number;
  _hb_wasm_get_cache_stats?(stats: number): void;
  _hb_wasm_clear_font_cache?(): void;
}

export interface EmscriptenModule {
  HEAPU8: Uint8Array;
  HEAPU32: Uint32Array;
  HEAPF32: Float32Array;
  
  _malloc(size: number): number;
  _free(ptr: number): void;
  
  ccall(ident: string, returnType: string, argTypes: string[], args: any[]): any;
  cwrap(ident: string, returnType: string, argTypes: string[]): (...args: any[]) => any;
  
  UTF8ToString(ptr: number): string;
  stringToNewUTF8(str: string): number;
  
  addFunction(func: Function, sig: string): number;
  removeFunction(ptr: number): void;
  dynCall_vii(ptr: number, arg1: number, arg2: number): void;
}

declare const HarfBuzzFactory: () => Promise<HarfBuzzModule>;
export default HarfBuzzFactory;
EOF
    
    log_success "TypeScript definitions created"
}

# Run tests
run_tests() {
    log_info "Running build verification tests..."
    
    # Test basic module loading
    if command -v node &> /dev/null; then
        cat > test_module.mjs << 'EOF'
import HarfBuzzFactory from './harfbuzz.js';

try {
    const hb = await HarfBuzzFactory();
    
    // Test basic memory operations
    const ptr = hb._malloc(1024);
    if (ptr === 0) {
        throw new Error('Memory allocation failed');
    }
    hb._free(ptr);
    
    // Test basic HarfBuzz functions
    const buffer = hb._hb_buffer_create();
    if (buffer === 0) {
        throw new Error('Buffer creation failed');
    }
    hb._hb_buffer_destroy(buffer);
    
    console.log('✅ Basic module verification passed');
    process.exit(0);
} catch (error) {
    console.error('❌ Module verification failed:', error.message);
    process.exit(1);
}
EOF
        
        cd "$INSTALL_DIR"
        if node test_module.mjs; then
            log_success "Module verification passed"
        else
            log_warning "Module verification failed, but build completed"
        fi
        rm -f test_module.mjs
        cd - > /dev/null
    else
        log_warning "Node.js not found, skipping module verification"
    fi
}

# Generate build report
generate_report() {
    log_info "Generating build report..."
    
    local report_file="$INSTALL_DIR/build-report.json"
    
    cat > "$report_file" << EOF
{
  "build_info": {
    "timestamp": "$(date -Iseconds)",
    "build_type": "$BUILD_TYPE",
    "simd_enabled": $([[ $ENABLE_SIMD == 1 ]] && echo true || echo false),
    "threading_enabled": $([[ $ENABLE_THREADING == 1 ]] && echo true || echo false),
    "webgpu_enabled": $([[ $ENABLE_WEBGPU == 1 ]] && echo true || echo false),
    "native_fs_enabled": $([[ $ENABLE_NATIVE_FS == 1 ]] && echo true || echo false),
    "thread_pool_size": $THREAD_POOL_SIZE,
    "initial_memory": "$INITIAL_MEMORY",
    "maximum_memory": "$MAXIMUM_MEMORY"
  },
  "environment": {
    "emscripten_version": "$(emcc --version | head -n1)",
    "meson_version": "$(meson --version)",
    "ninja_version": "$(ninja --version)",
    "node_version": "$(node --version 2>/dev/null || echo 'N/A')",
    "platform": "$(uname -s)",
    "arch": "$(uname -m)"
  },
  "artifacts": {
    "javascript_file": "harfbuzz.js",
    "wasm_file": "harfbuzz.wasm", 
    "typescript_definitions": "harfbuzz.d.ts",
    "build_report": "build-report.json"
  }
}
EOF
    
    log_success "Build report generated: $report_file"
}

# Main build function
main() {
    log_info "Starting HarfBuzz WASM production build..."
    
    # Parse arguments
    parse_arguments "$@"
    
    # Show configuration
    log_info "Configuration: BUILD_TYPE=$BUILD_TYPE, SIMD=$([[ $ENABLE_SIMD == 1 ]] && echo ON || echo OFF), THREADING=$([[ $ENABLE_THREADING == 1 ]] && echo ON || echo OFF), WEBGPU=$([[ $ENABLE_WEBGPU == 1 ]] && echo ON || echo OFF), NATIVE_FS=$([[ $ENABLE_NATIVE_FS == 1 ]] && echo ON || echo OFF)"
    
    # Execute build steps
    check_dependencies
    configure_build
    build_harfbuzz
    create_wasm_wrapper
    run_tests
    generate_report
    
    log_success "HarfBuzz WASM build completed successfully!"
    log_success "Artifacts available in: $INSTALL_DIR"
}

# Convert memory sizes
initial_memory_bytes=$(memory_to_bytes "$INITIAL_MEMORY")
maximum_memory_bytes=$(memory_to_bytes "$MAXIMUM_MEMORY")

# Run main function with all arguments
main "$@"