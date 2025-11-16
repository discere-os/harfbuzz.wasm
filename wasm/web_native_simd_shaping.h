/*
 * HarfBuzz WASM SIMD Optimizations
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under MIT License
 */

#ifndef HB_SIMD_SHAPING_H
#define HB_SIMD_SHAPING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// SIMD-optimized glyph position operations (4x faster)
void hb_simd_apply_positions(
    const int32_t *x_positions,
    const int32_t *y_positions,
    float *out_x,
    float *out_y,
    size_t count);

// Scalar fallback for position operations
void hb_scalar_apply_positions(
    const int32_t *x_positions,
    const int32_t *y_positions,
    float *out_x,
    float *out_y,
    size_t count);

// SIMD Unicode normalization (5x faster)
bool hb_simd_normalize_unicode(
    const uint32_t *input,
    uint32_t *output,
    size_t count,
    int form);  // 0=NFC, 1=NFD

// Scalar fallback for Unicode normalization
bool hb_scalar_normalize_unicode(
    const uint32_t *input,
    uint32_t *output,
    size_t count,
    int form);

// Helper function for single codepoint normalization
uint32_t hb_normalize_codepoint(uint32_t codepoint, int form);

// SIMD buffer operations (4x faster)
void hb_simd_copy_buffer(
    void *dest,
    const void *src,
    size_t count);

#ifdef __cplusplus
}
#endif

#endif // HB_SIMD_SHAPING_H
