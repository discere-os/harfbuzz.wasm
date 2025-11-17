/*
 * HarfBuzz WASM SIMD Optimizations
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under MIT License
 */

#include "web_native_simd_shaping.h"

#ifdef __EMSCRIPTEN__
#include <wasm_simd128.h>
#endif

#include <string.h>

// Scalar fallback for position operations
void
hb_scalar_apply_positions(
    const int32_t *x_positions,
    const int32_t *y_positions,
    float *out_x,
    float *out_y,
    size_t count)
{
    for (size_t i = 0; i < count; i++) {
        out_x[i] = (float)x_positions[i];
        out_y[i] = (float)y_positions[i];
    }
}

#ifdef __EMSCRIPTEN__

void
hb_simd_apply_positions(
    const int32_t *x_positions,
    const int32_t *y_positions,
    float *out_x,
    float *out_y,
    size_t count)
{
    if (count < 8) {
        // Scalar fallback for small buffers
        hb_scalar_apply_positions(x_positions, y_positions, out_x, out_y, count);
        return;
    }

    // Process 4 positions at a time using SIMD
    size_t simd_count = count / 4;

    for (size_t i = 0; i < simd_count; i++) {
        // Load 4 x positions (i32)
        v128_t x_i32 = wasm_v128_load(&x_positions[i * 4]);
        v128_t y_i32 = wasm_v128_load(&y_positions[i * 4]);

        // Convert i32 to f32
        v128_t x_f32 = wasm_f32x4_convert_i32x4(x_i32);
        v128_t y_f32 = wasm_f32x4_convert_i32x4(y_i32);

        // Store results
        wasm_v128_store(&out_x[i * 4], x_f32);
        wasm_v128_store(&out_y[i * 4], y_f32);
    }

    // Handle remainder with scalar
    for (size_t i = simd_count * 4; i < count; i++) {
        out_x[i] = (float)x_positions[i];
        out_y[i] = (float)y_positions[i];
    }
}

#else

// Non-WASM fallback
void
hb_simd_apply_positions(
    const int32_t *x_positions,
    const int32_t *y_positions,
    float *out_x,
    float *out_y,
    size_t count)
{
    hb_scalar_apply_positions(x_positions, y_positions, out_x, out_y, count);
}

#endif

// Unicode normalization helper
uint32_t
hb_normalize_codepoint(uint32_t codepoint, int form)
{
    // Simplified normalization - full implementation would use Unicode tables
    // For now, just return the codepoint as-is
    // TODO: Implement proper Unicode normalization tables
    return codepoint;
}

// Scalar fallback for Unicode normalization
bool
hb_scalar_normalize_unicode(
    const uint32_t *input,
    uint32_t *output,
    size_t count,
    int form)
{
    for (size_t i = 0; i < count; i++) {
        output[i] = hb_normalize_codepoint(input[i], form);
    }
    return true;
}

#ifdef __EMSCRIPTEN__

bool
hb_simd_normalize_unicode(
    const uint32_t *input,
    uint32_t *output,
    size_t count,
    int form)
{
    if (count < 8) {
        return hb_scalar_normalize_unicode(input, output, count, form);
    }

    // Process 4 codepoints at once
    size_t chunks = count / 4;

    for (size_t i = 0; i < chunks; i++) {
        v128_t codepoints = wasm_v128_load((const v128_t*)&input[i * 4]);

        // Apply normalization rules using SIMD
        // For NFC: Check for combining sequences, compose if possible
        // For NFD: Decompose composed characters
        // Simplified - full implementation would use Unicode tables

        v128_t normalized = codepoints;  // Simplified - full impl needs tables

        wasm_v128_store((v128_t*)&output[i * 4], normalized);
    }

    // Scalar remainder
    for (size_t i = chunks * 4; i < count; i++) {
        output[i] = hb_normalize_codepoint(input[i], form);
    }

    return true;
}

#else

bool
hb_simd_normalize_unicode(
    const uint32_t *input,
    uint32_t *output,
    size_t count,
    int form)
{
    return hb_scalar_normalize_unicode(input, output, count, form);
}

#endif

#ifdef __EMSCRIPTEN__

void
hb_simd_copy_buffer(void *dest, const void *src, size_t count)
{
    if (count < 32) {
        memcpy(dest, src, count);
        return;
    }

    // Copy 16 bytes at a time
    v128_t *dst_vec = (v128_t*)dest;
    const v128_t *src_vec = (const v128_t*)src;
    size_t chunks = count / 16;

    for (size_t i = 0; i < chunks; i++) {
        wasm_v128_store(dst_vec + i, wasm_v128_load(src_vec + i));
    }

    // Remainder
    size_t remainder = count % 16;
    if (remainder > 0) {
        memcpy((char*)dest + chunks * 16,
               (const char*)src + chunks * 16,
               remainder);
    }
}

#else

void
hb_simd_copy_buffer(void *dest, const void *src, size_t count)
{
    memcpy(dest, src, count);
}

#endif
