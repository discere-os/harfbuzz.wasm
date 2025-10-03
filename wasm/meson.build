hb_wasm_cargs = ['-msimd128', '-DHB_WASM=1']

hb_main_link_args = [
  '-sWASM=1',
  '-sMODULARIZE=1',
  '-sEXPORT_ES6=1',
  '-sEXPORT_NAME=HarfBuzzModule',
  '-sALLOW_MEMORY_GROWTH=1',
  '-sENVIRONMENT=web,webview,worker',
  '-sNO_FILESYSTEM=1',
  '-sEXPORTED_RUNTIME_METHODS=["cwrap","ccall","UTF8ToString"]',
  '-O3',
  '-flto',
]

executable(
  'harfbuzz-main',
  files('harfbuzz_wasm_module.c'),
  dependencies: [libharfbuzz_dep],
  c_args: hb_wasm_cargs,
  link_args: hb_main_link_args,
  install: true,
  install_dir: get_option('bindir'),
)

hb_side_link_args = [
  '-sSIDE_MODULE=2',
  '-fPIC',
  '-O3',
  '-flto',
  '-sERROR_ON_UNDEFINED_SYMBOLS=0',  # Host provides dependencies (freetype, glib, etc.)
  '-sEXPORTED_FUNCTIONS=["_harfbuzz_wasm_version"]',
]

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

