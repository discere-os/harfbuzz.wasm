#include <emscripten.h>
#include <hb.h>

EMSCRIPTEN_KEEPALIVE
const char* harfbuzz_wasm_version(void) {
  return hb_version_string();
}

