# HarfBuzz WASM build targets
# Copyright 2025 Superstruct Ltd, New Zealand

wasm_build_type = get_option('wasm_build_type')
wasm_simd = get_option('wasm_simd')
wasm_threading = get_option('wasm_threading')

# Common compilation flags
common_flags = ['-O3', '-flto', '-sENVIRONMENT=web,webview,worker', '-DHB_WASM=1']

if wasm_simd
  common_flags += ['-msimd128', '-DHB_WASM_SIMD=1']
endif

if wasm_threading
  common_flags += ['-pthread', '-sPTHREAD_POOL_SIZE=4']
endif

# Add web-native SIMD sources (created as empty files if they don't exist yet)
wasm_simd_sources = []
if wasm_simd
  wasm_simd_sources = files(
    'web_native_simd_shaping.c',
    'web_native_capabilities.c',
  )
endif

# MAIN_MODULE (testing/NPM - self-contained)
hb_main_link_args = common_flags + [
  '-sWASM=1',
  '-sMODULARIZE=1',
  '-sEXPORT_ES6=1',
  '-sEXPORT_NAME=HarfBuzzModule',
  '-sALLOW_MEMORY_GROWTH=1',
  '-sNO_FILESYSTEM=1',
  '-sEXPORTED_RUNTIME_METHODS=["cwrap","ccall","UTF8ToString"]',
  '-sSINGLE_FILE=0',  # Keep separate for easier debugging
]

executable(
  'harfbuzz-main',
  files('harfbuzz_wasm_module.c') + wasm_simd_sources,
  dependencies: [libharfbuzz_dep],
  c_args: common_flags,
  link_args: hb_main_link_args,
  name_suffix: 'js',
  install: true,
  install_dir: get_option('bindir'),
)

# SIDE_MODULE (production - 70-200KB)
hb_side_link_args = common_flags + [
  '-sSIDE_MODULE=2',
  '-fPIC',
  '-sERROR_ON_UNDEFINED_SYMBOLS=0',  # Host provides dependencies (freetype, glib, etc.)
]

shared_module(
  'harfbuzz-side',
  files('harfbuzz_wasm_side.c') + wasm_simd_sources,
  dependencies: [libharfbuzz_dep],
  name_prefix: '',
  name_suffix: 'wasm',
  c_args: common_flags + ['-fPIC'],
  link_args: hb_side_link_args,
  install: true,
  install_dir: get_option('libdir'),
)
