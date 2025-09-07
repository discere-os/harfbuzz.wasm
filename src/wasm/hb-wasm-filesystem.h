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

#ifndef HB_WASM_FILESYSTEM_H
#define HB_WASM_FILESYSTEM_H

#include "hb.h"

#ifdef HB_WASM_NATIVE_FS
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

HB_BEGIN_DECLS

/* WASM filesystem initialization */
#ifdef HB_WASM_NATIVE_FS
typedef void (*hb_wasm_filesystem_ready_callback_t) (void *user_data);

HB_EXTERN hb_bool_t
hb_wasm_filesystem_init (hb_wasm_filesystem_ready_callback_t callback,
                        void *user_data);

HB_EXTERN hb_bool_t
hb_wasm_filesystem_is_ready (void);
#endif

/* Font loading and caching */
#ifdef HB_WASM_NATIVE_FS
typedef void (*hb_wasm_font_load_callback_t) (hb_blob_t *blob,
                                             void *user_data);

HB_EXTERN void
hb_wasm_load_font_from_url (const char *url,
                           const char *cache_key,
                           hb_wasm_font_load_callback_t callback,
                           void *user_data);

HB_EXTERN hb_blob_t *
hb_wasm_load_font_from_cache (const char *cache_key);

HB_EXTERN hb_bool_t
hb_wasm_cache_font_blob (const char *cache_key,
                        hb_blob_t *blob);

HB_EXTERN void
hb_wasm_preload_system_fonts (void);
#endif

/* Font package management */
#ifdef HB_WASM_NATIVE_FS
typedef struct {
  char name[256];
  char filename[256];
  size_t size;
  char checksum[65]; /* SHA-256 hex string */
} hb_wasm_font_info_t;

typedef struct {
  char name[256];
  char version[64];
  unsigned int font_count;
  hb_wasm_font_info_t *fonts;
} hb_wasm_font_package_t;

HB_EXTERN hb_bool_t
hb_wasm_load_font_package (const char *package_url,
                          const char *cache_prefix);

HB_EXTERN hb_wasm_font_package_t *
hb_wasm_get_font_package (const char *package_name);

HB_EXTERN void
hb_wasm_font_package_destroy (hb_wasm_font_package_t *package);
#endif

/* Cache management */
#ifdef HB_WASM_NATIVE_FS
typedef struct {
  size_t total_size;
  size_t used_size;
  unsigned int entry_count;
  unsigned int hit_count;
  unsigned int miss_count;
} hb_wasm_cache_stats_t;

HB_EXTERN hb_wasm_cache_stats_t
hb_wasm_get_cache_stats (void);

HB_EXTERN void
hb_wasm_clear_font_cache (void);

HB_EXTERN hb_bool_t
hb_wasm_set_cache_size_limit (size_t max_size);

HB_EXTERN hb_bool_t
hb_wasm_set_cache_age_limit (unsigned int max_age_seconds);

HB_EXTERN void
hb_wasm_sync_cache_to_storage (void);
#endif

/* Virtual directory structure */
#ifdef HB_WASM_NATIVE_FS
#define HB_WASM_FONT_CACHE_DIR "/font-cache"
#define HB_WASM_TEMP_DIR "/temp"
#define HB_WASM_CDN_FONTS_DIR "/cdn-fonts"
#define HB_WASM_FONT_PACKAGES_DIR "/font-packages"
#define HB_WASM_USER_FONTS_DIR "/user-fonts"
#define HB_WASM_SYSTEM_FONTS_DIR "/system-fonts"
#endif

/* CDN integration */
#ifdef HB_WASM_NATIVE_FS
typedef enum {
  HB_WASM_CDN_GOOGLE_FONTS,
  HB_WASM_CDN_ADOBE_FONTS,
  HB_WASM_CDN_CUSTOM
} hb_wasm_cdn_provider_t;

HB_EXTERN void
hb_wasm_load_font_from_cdn (hb_wasm_cdn_provider_t provider,
                           const char *font_family,
                           const char *variant,
                           hb_wasm_font_load_callback_t callback,
                           void *user_data);

HB_EXTERN void
hb_wasm_set_custom_cdn_base_url (const char *base_url);
#endif

/* Progressive enhancement utilities */
#ifdef HB_WASM_NATIVE_FS
HB_EXTERN hb_bool_t
hb_wasm_has_indexeddb_support (void);

HB_EXTERN hb_bool_t
hb_wasm_has_fetch_support (void);

HB_EXTERN void
hb_wasm_enable_progressive_font_loading (hb_bool_t enable);
#endif

/* Error handling */
#ifdef HB_WASM_NATIVE_FS
typedef enum {
  HB_WASM_FS_ERROR_NONE = 0,
  HB_WASM_FS_ERROR_INIT_FAILED,
  HB_WASM_FS_ERROR_IDBFS_NOT_SUPPORTED,
  HB_WASM_FS_ERROR_NETWORK_FAILED,
  HB_WASM_FS_ERROR_CACHE_FULL,
  HB_WASM_FS_ERROR_INVALID_FONT,
  HB_WASM_FS_ERROR_CHECKSUM_MISMATCH
} hb_wasm_fs_error_t;

typedef void (*hb_wasm_error_callback_t) (hb_wasm_fs_error_t error,
                                         const char *message,
                                         void *user_data);

HB_EXTERN void
hb_wasm_set_error_callback (hb_wasm_error_callback_t callback,
                           void *user_data);

HB_EXTERN const char *
hb_wasm_error_to_string (hb_wasm_fs_error_t error);
#endif

HB_END_DECLS

#endif /* HB_WASM_FILESYSTEM_H */