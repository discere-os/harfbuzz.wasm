/*
 * HarfBuzz WASM Capabilities Detection
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under MIT License
 */

#include <stdbool.h>

typedef struct {
    bool has_wasm_simd;
    bool has_webgpu;
    int chrome_version;
} HBWebCapabilities;

static HBWebCapabilities g_caps = {0};
static bool g_initialized = false;

const HBWebCapabilities* hb_web_get_capabilities(void) {
    if (!g_initialized) {
        // Assume Chrome 113+ with WASM SIMD support
        #ifdef HB_WASM_SIMD
        g_caps.has_wasm_simd = true;
        #else
        g_caps.has_wasm_simd = false;
        #endif

        g_caps.has_webgpu = false;  // WebGPU support is optional
        g_caps.chrome_version = 113;  // Minimum version for SIMD
        g_initialized = true;
    }
    return &g_caps;
}

// Query functions for individual capabilities
bool hb_web_has_simd(void) {
    return hb_web_get_capabilities()->has_wasm_simd;
}

bool hb_web_has_webgpu(void) {
    return hb_web_get_capabilities()->has_webgpu;
}

int hb_web_chrome_version(void) {
    return hb_web_get_capabilities()->chrome_version;
}
