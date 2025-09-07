/*
 * Copyright 2025 Superstruct Ltd, New Zealand
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HB_WASM_SIMD_H
#define HB_WASM_SIMD_H

#include "hb.h"

#ifdef HB_WASM_SIMD
#include <wasm_simd128.h>
#endif

HB_BEGIN_DECLS

/* WASM SIMD feature detection */
#ifdef HB_WASM_SIMD
HB_EXTERN hb_bool_t
hb_wasm_simd_available (void);
#else
#define hb_wasm_simd_available() false
#endif

/* SIMD-optimized Unicode property lookups */
#ifdef HB_WASM_SIMD
HB_EXTERN void
hb_wasm_simd_unicode_lookup_batch (const hb_codepoint_t *codepoints,
                                  hb_unicode_general_category_t *categories,
                                  unsigned int count);

HB_EXTERN void
hb_wasm_simd_unicode_script_batch (const hb_codepoint_t *codepoints,
                                  hb_script_t *scripts,
                                  unsigned int count);
#endif

/* SIMD-optimized glyph positioning */
#ifdef HB_WASM_SIMD
HB_EXTERN void
hb_wasm_simd_position_glyphs (hb_glyph_position_t *positions,
                             unsigned int count,
                             float scale_x,
                             float scale_y);

HB_EXTERN void
hb_wasm_simd_apply_kerning (hb_glyph_position_t *positions,
                           const hb_glyph_info_t *infos,
                           unsigned int count,
                           const int32_t *kerning_table);
#endif

/* SIMD-optimized string operations */
#ifdef HB_WASM_SIMD
HB_EXTERN int
hb_wasm_simd_string_compare (const char *str1,
                            const char *str2,
                            size_t len);

HB_EXTERN void
hb_wasm_simd_string_copy (char *dest,
                         const char *src,
                         size_t len);

HB_EXTERN size_t
hb_wasm_simd_string_length (const char *str,
                           size_t max_len);
#endif

/* SIMD-optimized buffer operations */
#ifdef HB_WASM_SIMD
HB_EXTERN void
hb_wasm_simd_buffer_clear_positions (hb_glyph_position_t *positions,
                                    unsigned int count);

HB_EXTERN void
hb_wasm_simd_buffer_normalize_glyphs (hb_glyph_info_t *infos,
                                     hb_glyph_position_t *positions,
                                     unsigned int count);
#endif

/* SIMD-optimized font metrics calculations */
#ifdef HB_WASM_SIMD
HB_EXTERN void
hb_wasm_simd_font_get_glyph_advances (hb_font_t *font,
                                     const hb_codepoint_t *first_glyph,
                                     unsigned int count,
                                     hb_position_t *first_advance,
                                     unsigned int glyph_stride,
                                     unsigned int advance_stride);

HB_EXTERN void
hb_wasm_simd_font_get_glyph_origins (hb_font_t *font,
                                    const hb_codepoint_t *first_glyph,
                                    unsigned int count,
                                    hb_position_t *first_x_origin,
                                    hb_position_t *first_y_origin,
                                    unsigned int glyph_stride,
                                    unsigned int origin_stride);
#endif

/* Performance measurement utilities */
HB_EXTERN double
hb_wasm_simd_benchmark_unicode_lookup (unsigned int iterations);

HB_EXTERN double
hb_wasm_simd_benchmark_glyph_positioning (unsigned int iterations);

HB_EXTERN double
hb_wasm_simd_benchmark_string_operations (unsigned int iterations);

HB_END_DECLS

#endif /* HB_WASM_SIMD_H */