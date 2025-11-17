/*
 * Web-Native Capabilities Detection
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Part of the Discere OS unified WASM system
 */

#ifndef WEB_NATIVE_CAPABILITIES_H
#define WEB_NATIVE_CAPABILITIES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Web-native capabilities structure
 * Detects browser features for optimal performance routing
 */
typedef struct {
    bool has_wasm_simd;           // WASM SIMD (3-5x string ops)
    bool has_webgpu;              // WebGPU (10x+ compute)
    bool has_shared_array_buffer; // SharedArrayBuffer (threading)
    bool has_web_crypto;          // Web Crypto API (5-15x crypto)
    bool has_opfs;                // Origin Private File System (3-4x I/O)
    bool has_workers;             // Web Workers (10x threading)
    bool has_fetch_streaming;     // Fetch API streaming
    bool has_weak_ref;            // WeakRef for GC integration
    int chrome_version;           // Chrome version (0 if not Chrome)
} WebCapabilities;

/**
 * Get web capabilities (singleton, lazy init)
 * @return Pointer to capabilities struct (never NULL)
 */
const WebCapabilities* web_get_capabilities(void);

/**
 * Verify minimum requirements (Chrome 113+)
 * @return true if all mandatory features available
 */
bool web_verify_requirements(void);

#ifdef __cplusplus
}
#endif

#endif /* WEB_NATIVE_CAPABILITIES_H */
