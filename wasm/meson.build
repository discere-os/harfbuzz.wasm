# HarfBuzz WASM Builds
# Copyright 2025 Superstruct Ltd, New Zealand

# Common WASM compilation flags
hb_wasm_common_flags = [
  '-fPIC',
  '-sWASM_BIGINT=1',
  '-DHB_WASM=1',
]

# SIMD flags (mandatory for 3-5x performance)
hb_wasm_simd_flags = get_option('wasm_simd') ? ['-msimd128'] : []

# Threading flags (10x speedup vs pthread emulation)
hb_wasm_threading_flags = get_option('wasm_threading') ? [
  '-pthread',
  '-sPTHREAD_POOL_SIZE=@0@'.format(get_option('wasm_thread_pool_size'))
] : []

# Build type specific flags
wasm_build_type = get_option('wasm_build_type')

if wasm_build_type == 'minimal'
  # Minimal build: smallest size, basic features
  hb_wasm_memory_flags = [
    '-sINITIAL_MEMORY=67108864',  # 64MB
    '-sSTACK_SIZE=2097152',       # 2MB
  ]
  hb_wasm_cargs = hb_wasm_common_flags + ['-DNDEBUG']

elif wasm_build_type == 'webgpu'
  # WebGPU build: GPU acceleration, maximum memory
  hb_wasm_memory_flags = [
    '-sINITIAL_MEMORY=134217728',   # 128MB
    '-sMAXIMUM_MEMORY=1073741824',  # 1GB
    '-sALLOW_MEMORY_GROWTH=1',
    '-sUSE_WEBGPU=1',
    '-sASYNCIFY=1',
  ]
  hb_wasm_cargs = hb_wasm_common_flags + hb_wasm_simd_flags + ['-DHB_WEBGPU=1']

else  # standard
  # Standard build: balanced size/performance
  hb_wasm_memory_flags = [
    '-sINITIAL_MEMORY=134217728',   # 128MB
    '-sMAXIMUM_MEMORY=1073741824',  # 1GB
    '-sALLOW_MEMORY_GROWTH=1',
  ]
  hb_wasm_cargs = hb_wasm_common_flags + hb_wasm_simd_flags
endif

# Optimization strategy
wasm_optimize = get_option('wasm_optimize')
if wasm_optimize == 'size'
  hb_wasm_opt_flags = ['-Oz', '-flto']
elif wasm_optimize == 'speed'
  hb_wasm_opt_flags = ['-O3', '-flto', '-ffast-math']
else  # balanced
  hb_wasm_opt_flags = ['-O3', '-flto']
endif

# MAIN_MODULE target (self-contained, for testing/NPM)
hb_main_link_args = [
  '-sWASM=1',
  '-sMODULARIZE=1',
  '-sEXPORT_ES6=1',
  '-sEXPORT_NAME=HarfBuzzModule',
  '-sENVIRONMENT=web,webview,worker',
  '-sNO_FILESYSTEM=1',
  '-sEXPORTED_RUNTIME_METHODS=["cwrap","ccall","UTF8ToString","FS","HEAPU8"]',
] + hb_wasm_memory_flags + hb_wasm_threading_flags + hb_wasm_opt_flags

executable(
  'harfbuzz-main',
  files('harfbuzz_wasm_module.c'),
  dependencies: [libharfbuzz_dep],
  c_args: hb_wasm_cargs,
  link_args: hb_main_link_args,
  install: true,
  install_dir: get_option('bindir'),
)

# SIDE_MODULE target (production, dynamic loading by concha)
hb_side_link_args = [
  '-sSIDE_MODULE=2',
  '-fPIC',
  '-sERROR_ON_UNDEFINED_SYMBOLS=0',  # Host provides dependencies (freetype, glib, etc.)
  '-sEXPORTED_FUNCTIONS=["_harfbuzz_wasm_version","_hb_version","_hb_shape"]',
] + hb_wasm_opt_flags

shared_module(
  'harfbuzz-side',
  files('harfbuzz_wasm_side.c'),
  dependencies: [libharfbuzz_dep],
  name_prefix: '',
  name_suffix: 'wasm',
  c_args: hb_wasm_cargs,
  link_args: hb_side_link_args,
  install: true,
  install_dir: get_option('libdir'),
)
