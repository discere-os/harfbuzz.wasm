/*
 * Web-Native Capabilities Detection
 * Copyright 2025 Superstruct Ltd, New Zealand
 */

#include "web_native_capabilities.h"
#include <emscripten.h>
#include <stdio.h>

static WebCapabilities g_caps = {0};
static bool g_initialized = false;

EMSCRIPTEN_KEEPALIVE
const WebCapabilities* web_get_capabilities(void) {
    if (!g_initialized) {
        // Detect WASM SIMD (3-5x speedup for string operations)
        g_caps.has_wasm_simd = EM_ASM_INT({
            try {
                return typeof WebAssembly.validate !== 'undefined' &&
                       WebAssembly.validate(new Uint8Array([
                         0,97,115,109,1,0,0,0,1,4,1,96,0,0,3,2,1,0,10,9,1,7,0,65,0,253,15,26,11
                       ]));
            } catch(e) { return 0; }
        });

        // Detect WebGPU (10x+ speedup for parallel compute)
        g_caps.has_webgpu = EM_ASM_INT({
            return typeof navigator !== 'undefined' &&
                   typeof navigator.gpu !== 'undefined' ? 1 : 0;
        });

        // Detect SharedArrayBuffer (required for threading)
        g_caps.has_shared_array_buffer = EM_ASM_INT({
            return typeof SharedArrayBuffer !== 'undefined' ? 1 : 0;
        });

        // Detect Web Crypto API (5-15x speedup for crypto ops)
        g_caps.has_web_crypto = EM_ASM_INT({
            return typeof crypto !== 'undefined' &&
                   typeof crypto.subtle !== 'undefined' ? 1 : 0;
        });

        // Detect OPFS (3-4x faster than IDBFS)
        g_caps.has_opfs = EM_ASM_INT({
            return typeof navigator !== 'undefined' &&
                   typeof navigator.storage !== 'undefined' &&
                   typeof navigator.storage.getDirectory === 'function' ? 1 : 0;
        });

        // Detect Web Workers (10x speedup vs pthread emulation)
        g_caps.has_workers = EM_ASM_INT({
            return typeof Worker !== 'undefined' ? 1 : 0;
        });

        // Detect Fetch API with streaming
        g_caps.has_fetch_streaming = EM_ASM_INT({
            return typeof fetch !== 'undefined' &&
                   typeof ReadableStream !== 'undefined' ? 1 : 0;
        });

        // Detect WeakRef for GC integration
        g_caps.has_weak_ref = EM_ASM_INT({
            return typeof WeakRef !== 'undefined' ? 1 : 0;
        });

        // Detect Chrome version (target: 113+)
        g_caps.chrome_version = EM_ASM_INT({
            const match = navigator.userAgent.match(/Chrome\\/(\\d+)/);
            return match ? parseInt(match[1]) : 0;
        });

        g_initialized = true;

        // Log capabilities for debugging
        EM_ASM({
            console.log('🌐 Web-Native Capabilities:', {
                simd: !!$0,
                webgpu: !!$1,
                sharedArrayBuffer: !!$2,
                webCrypto: !!$3,
                opfs: !!$4,
                workers: !!$5,
                chromeVersion: $6
            });
        }, g_caps.has_wasm_simd, g_caps.has_webgpu, g_caps.has_shared_array_buffer,
           g_caps.has_web_crypto, g_caps.has_opfs, g_caps.has_workers, g_caps.chrome_version);
    }
    return &g_caps;
}

EMSCRIPTEN_KEEPALIVE
bool web_verify_requirements(void) {
    const WebCapabilities* caps = web_get_capabilities();

    // Mandatory: Chrome 113+ with WASM SIMD
    bool meets_requirements = caps->has_wasm_simd && caps->chrome_version >= 113;

    if (!meets_requirements) {
        EM_ASM({
            console.warn('⚠️  Browser does not meet minimum requirements:', {
                required: 'Chrome 113+ with WASM SIMD',
                detected: {
                    simd: !!$0,
                    chromeVersion: $1
                }
            });
        }, caps->has_wasm_simd, caps->chrome_version);
    }

    return meets_requirements;
}
