#include <hb.h>

const char* harfbuzz_wasm_version(void) {
  return hb_version_string();
}

