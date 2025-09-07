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

#include "hb-wasm-filesystem.h"
#include "hb-blob.hh"

#ifdef HB_WASM_NATIVE_FS
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <ctime>

/* Internal state */
static bool filesystem_ready = false;
static hb_wasm_filesystem_ready_callback_t ready_callback = nullptr;
static void *ready_callback_user_data = nullptr;
static hb_wasm_error_callback_t error_callback = nullptr;
static void *error_callback_user_data = nullptr;

/* Cache management */
static std::unordered_map<std::string, hb_blob_t*> font_cache;
static std::unordered_map<std::string, time_t> cache_timestamps;
static hb_wasm_cache_stats_t cache_stats = {};
static size_t cache_size_limit = 100 * 1024 * 1024; /* 100MB default */
static unsigned int cache_age_limit = 7 * 24 * 60 * 60; /* 7 days default */

/* Font packages */
static std::unordered_map<std::string, hb_wasm_font_package_t*> font_packages;

/* CDN configuration */
static std::string custom_cdn_base_url;

/* Internal utilities */
static void
report_error (hb_wasm_fs_error_t error, const char *message)
{
  if (error_callback)
    error_callback (error, message, error_callback_user_data);
}

static void
update_cache_stats (void)
{
  cache_stats.entry_count = font_cache.size ();
  cache_stats.used_size = 0;
  
  for (const auto &entry : font_cache)
  {
    cache_stats.used_size += hb_blob_get_length (entry.second);
  }
}

static void
evict_old_entries (void)
{
  time_t current_time = time (nullptr);
  std::vector<std::string> keys_to_remove;
  
  for (const auto &entry : cache_timestamps)
  {
    if (current_time - entry.second > cache_age_limit)
    {
      keys_to_remove.push_back (entry.first);
    }
  }
  
  for (const std::string &key : keys_to_remove)
  {
    auto cache_iter = font_cache.find (key);
    if (cache_iter != font_cache.end ())
    {
      hb_blob_destroy (cache_iter->second);
      font_cache.erase (cache_iter);
    }
    cache_timestamps.erase (key);
  }
}

static void
enforce_cache_size_limit (void)
{
  update_cache_stats ();
  
  if (cache_stats.used_size <= cache_size_limit)
    return;
  
  /* Remove oldest entries until under limit */
  std::vector<std::pair<std::string, time_t>> entries_by_age;
  
  for (const auto &entry : cache_timestamps)
  {
    entries_by_age.push_back ({entry.first, entry.second});
  }
  
  std::sort (entries_by_age.begin (), entries_by_age.end (),
            [](const auto &a, const auto &b) { return a.second < b.second; });
  
  for (const auto &entry : entries_by_age)
  {
    auto cache_iter = font_cache.find (entry.first);
    if (cache_iter != font_cache.end ())
    {
      hb_blob_destroy (cache_iter->second);
      font_cache.erase (cache_iter);
    }
    cache_timestamps.erase (entry.first);
    
    update_cache_stats ();
    if (cache_stats.used_size <= cache_size_limit)
      break;
  }
}

/* C callback for filesystem ready */
extern "C" {
  EMSCRIPTEN_KEEPALIVE
  void hb_wasm_filesystem_ready_internal (void)
  {
    filesystem_ready = true;
    if (ready_callback)
      ready_callback (ready_callback_user_data);
  }
}

/* Public API implementation */
hb_bool_t
hb_wasm_filesystem_init (hb_wasm_filesystem_ready_callback_t callback,
                        void *user_data)
{
  ready_callback = callback;
  ready_callback_user_data = user_data;
  
  /* Initialize IDBFS and virtual directory structure */
  EM_ASM({
    try {
      // Create directory structure
      var dirs = ['/font-cache', '/temp', '/cdn-fonts', 
                  '/font-packages', '/user-fonts', '/system-fonts'];
      
      for (var i = 0; i < dirs.length; i++) {
        try {
          FS.mkdir(dirs[i]);
        } catch (e) {
          // Directory might already exist
        }
      }
      
      // Mount IDBFS on font-cache for persistence
      FS.mount(IDBFS, {}, '/font-cache');
      
      // Sync from IndexedDB
      FS.syncfs(true, function(err) {
        if (!err) {
          console.log('HarfBuzz WASM filesystem initialized');
          _hb_wasm_filesystem_ready_internal();
        } else {
          console.error('HarfBuzz WASM filesystem init failed:', err);
          // Continue without persistence
          _hb_wasm_filesystem_ready_internal();
        }
      });
      
    } catch (e) {
      console.error('HarfBuzz WASM filesystem setup failed:', e);
      // Fallback to MEMFS only
      _hb_wasm_filesystem_ready_internal();
    }
  });
  
  return true;
}

hb_bool_t
hb_wasm_filesystem_is_ready (void)
{
  return filesystem_ready;
}

/* Font loading implementation */
struct font_load_context {
  std::string cache_key;
  hb_wasm_font_load_callback_t callback;
  void *user_data;
};

void
hb_wasm_load_font_from_url (const char *url,
                           const char *cache_key,
                           hb_wasm_font_load_callback_t callback,
                           void *user_data)
{
  /* Check cache first */
  hb_blob_t *cached_blob = hb_wasm_load_font_from_cache (cache_key);
  if (cached_blob)
  {
    cache_stats.hit_count++;
    callback (cached_blob, user_data);
    return;
  }
  
  cache_stats.miss_count++;
  
  /* Async load from URL */
  font_load_context *context = new font_load_context {
    std::string (cache_key),
    callback,
    user_data
  };
  
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init (&attr);
  strcpy (attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.userData = context;
  
  attr.onsuccess = [](emscripten_fetch_t *fetch) {
    font_load_context *ctx = static_cast<font_load_context*> (fetch->userData);
    
    /* Create blob from fetched data */
    char *data = static_cast<char*> (malloc (fetch->numBytes));
    memcpy (data, fetch->data, fetch->numBytes);
    
    hb_blob_t *blob = hb_blob_create (data, fetch->numBytes,
                                     HB_MEMORY_MODE_WRITABLE,
                                     data, free);
    
    /* Cache the font */
    hb_wasm_cache_font_blob (ctx->cache_key.c_str (), blob);
    
    /* Callback with the loaded font */
    ctx->callback (blob, ctx->user_data);
    
    delete ctx;
    emscripten_fetch_close (fetch);
  };
  
  attr.onerror = [](emscripten_fetch_t *fetch) {
    font_load_context *ctx = static_cast<font_load_context*> (fetch->userData);
    
    /* Try to load from cache as fallback */
    hb_blob_t *cached_blob = hb_wasm_load_font_from_cache (ctx->cache_key.c_str ());
    if (cached_blob)
    {
      ctx->callback (cached_blob, ctx->user_data);
    }
    else
    {
      report_error (HB_WASM_FS_ERROR_NETWORK_FAILED, "Font loading failed and no cache available");
      ctx->callback (nullptr, ctx->user_data);
    }
    
    delete ctx;
    emscripten_fetch_close (fetch);
  };
  
  emscripten_fetch (&attr, url);
}

hb_blob_t *
hb_wasm_load_font_from_cache (const char *cache_key)
{
  auto iter = font_cache.find (std::string (cache_key));
  if (iter != font_cache.end ())
  {
    /* Update access time */
    cache_timestamps[std::string (cache_key)] = time (nullptr);
    return hb_blob_reference (iter->second);
  }
  
  /* Try to load from persistent storage */
  std::string cache_path = std::string (HB_WASM_FONT_CACHE_DIR) + "/" + cache_key + ".ttf";
  
  FILE *cache_file = fopen (cache_path.c_str (), "rb");
  if (!cache_file)
    return nullptr;
  
  /* Get file size */
  fseek (cache_file, 0, SEEK_END);
  size_t size = ftell (cache_file);
  fseek (cache_file, 0, SEEK_SET);
  
  /* Load into memory */
  char *data = static_cast<char*> (malloc (size));
  fread (data, 1, size, cache_file);
  fclose (cache_file);
  
  /* Create blob and cache it */
  hb_blob_t *blob = hb_blob_create (data, size, HB_MEMORY_MODE_WRITABLE, data, free);
  font_cache[std::string (cache_key)] = hb_blob_reference (blob);
  cache_timestamps[std::string (cache_key)] = time (nullptr);
  
  return blob;
}

hb_bool_t
hb_wasm_cache_font_blob (const char *cache_key,
                        hb_blob_t *blob)
{
  if (!blob)
    return false;
  
  /* Add to memory cache */
  std::string key (cache_key);
  
  /* Remove old entry if exists */
  auto iter = font_cache.find (key);
  if (iter != font_cache.end ())
  {
    hb_blob_destroy (iter->second);
  }
  
  font_cache[key] = hb_blob_reference (blob);
  cache_timestamps[key] = time (nullptr);
  
  /* Enforce cache limits */
  enforce_cache_size_limit ();
  evict_old_entries ();
  
  /* Save to persistent storage */
  std::string cache_path = std::string (HB_WASM_FONT_CACHE_DIR) + "/" + cache_key + ".ttf";
  
  FILE *cache_file = fopen (cache_path.c_str (), "wb");
  if (cache_file)
  {
    unsigned int length;
    const char *data = hb_blob_get_data (blob, &length);
    fwrite (data, 1, length, cache_file);
    fclose (cache_file);
    
    /* Sync to IndexedDB */
    EM_ASM({
      FS.syncfs(false, function(err) {
        if (err) console.warn('Font cache sync failed:', err);
      });
    });
  }
  
  return true;
}

void
hb_wasm_preload_system_fonts (void)
{
  EM_ASM({
    addRunDependency('system-fonts');
    
    // List of essential system fonts to preload
    var system_fonts = [
      'fonts/DejaVuSans.ttf',
      'fonts/DejaVuSans-Bold.ttf',
      'fonts/DejaVuSansMono.ttf',
      'fonts/NotoSans-Regular.ttf',
      'fonts/NotoSansCJK-Regular.ttc',
      'fonts/NotoColorEmoji.ttf'
    ];
    
    var loaded_count = 0;
    var total_count = system_fonts.length;
    
    function font_loaded() {
      loaded_count++;
      if (loaded_count === total_count) {
        removeRunDependency('system-fonts');
      }
    }
    
    system_fonts.forEach(function(font_url, index) {
      fetch(font_url)
        .then(function(response) {
          if (!response.ok) throw new Error('Font not found: ' + font_url);
          return response.arrayBuffer();
        })
        .then(function(data) {
          // Extract filename for cache key
          var filename = font_url.split('/').pop().replace('.ttf', '').replace('.ttc', '');
          var cache_path = '/system-fonts/' + filename + '.ttf';
          
          // Store in virtual filesystem
          FS.writeFile(cache_path, new Uint8Array(data));
          console.log('Preloaded system font:', filename);
          font_loaded();
        })
        .catch(function(error) {
          console.warn('Failed to preload font:', font_url, error);
          font_loaded(); // Continue even if some fonts fail
        });
    });
  });
}

/* Font package management */
hb_bool_t
hb_wasm_load_font_package (const char *package_url,
                          const char *cache_prefix)
{
  /* This would implement loading of font packages (ZIP files containing multiple fonts) */
  /* For now, return false indicating not implemented */
  return false;
}

hb_wasm_font_package_t *
hb_wasm_get_font_package (const char *package_name)
{
  auto iter = font_packages.find (std::string (package_name));
  return (iter != font_packages.end ()) ? iter->second : nullptr;
}

void
hb_wasm_font_package_destroy (hb_wasm_font_package_t *package)
{
  if (!package)
    return;
  
  free (package->fonts);
  delete package;
}

/* CDN integration */
void
hb_wasm_load_font_from_cdn (hb_wasm_cdn_provider_t provider,
                           const char *font_family,
                           const char *variant,
                           hb_wasm_font_load_callback_t callback,
                           void *user_data)
{
  std::string url;
  std::string cache_key = std::string (font_family) + "-" + variant;
  
  switch (provider)
  {
    case HB_WASM_CDN_GOOGLE_FONTS:
      /* Construct Google Fonts URL */
      url = "https://fonts.gstatic.com/s/" + std::string (font_family) + "/v1/" + variant + ".ttf";
      break;
      
    case HB_WASM_CDN_ADOBE_FONTS:
      /* Adobe Fonts would require API key and different URL structure */
      report_error (HB_WASM_FS_ERROR_NETWORK_FAILED, "Adobe Fonts integration not implemented");
      callback (nullptr, user_data);
      return;
      
    case HB_WASM_CDN_CUSTOM:
      if (custom_cdn_base_url.empty ())
      {
        report_error (HB_WASM_FS_ERROR_NETWORK_FAILED, "Custom CDN base URL not set");
        callback (nullptr, user_data);
        return;
      }
      url = custom_cdn_base_url + "/" + font_family + "/" + variant + ".ttf";
      break;
  }
  
  hb_wasm_load_font_from_url (url.c_str (), cache_key.c_str (), callback, user_data);
}

void
hb_wasm_set_custom_cdn_base_url (const char *base_url)
{
  custom_cdn_base_url = std::string (base_url);
}

/* Cache management */
hb_wasm_cache_stats_t
hb_wasm_get_cache_stats (void)
{
  update_cache_stats ();
  return cache_stats;
}

void
hb_wasm_clear_font_cache (void)
{
  /* Clear memory cache */
  for (auto &entry : font_cache)
  {
    hb_blob_destroy (entry.second);
  }
  font_cache.clear ();
  cache_timestamps.clear ();
  
  /* Clear persistent cache */
  EM_ASM({
    try {
      // Remove all files in font-cache directory
      var files = FS.readdir('/font-cache');
      files.forEach(function(file) {
        if (file !== '.' && file !== '..') {
          FS.unlink('/font-cache/' + file);
        }
      });
      
      // Sync to IndexedDB
      FS.syncfs(false, function(err) {
        if (err) console.warn('Cache clear sync failed:', err);
      });
    } catch (e) {
      console.warn('Cache clear failed:', e);
    }
  });
  
  /* Reset stats */
  cache_stats = {};
}

hb_bool_t
hb_wasm_set_cache_size_limit (size_t max_size)
{
  cache_size_limit = max_size;
  enforce_cache_size_limit ();
  return true;
}

hb_bool_t
hb_wasm_set_cache_age_limit (unsigned int max_age_seconds)
{
  cache_age_limit = max_age_seconds;
  evict_old_entries ();
  return true;
}

void
hb_wasm_sync_cache_to_storage (void)
{
  EM_ASM({
    FS.syncfs(false, function(err) {
      if (err) {
        console.error('Manual cache sync failed:', err);
      } else {
        console.log('Cache synced to IndexedDB');
      }
    });
  });
}

/* Progressive enhancement */
hb_bool_t
hb_wasm_has_indexeddb_support (void)
{
  return EM_ASM_INT({
    return (typeof indexedDB !== 'undefined') ? 1 : 0;
  });
}

hb_bool_t
hb_wasm_has_fetch_support (void)
{
  return EM_ASM_INT({
    return (typeof fetch !== 'undefined') ? 1 : 0;
  });
}

void
hb_wasm_enable_progressive_font_loading (hb_bool_t enable)
{
  /* This would control whether fonts are loaded progressively or all at once */
  /* Implementation depends on specific use case requirements */
}

/* Error handling */
void
hb_wasm_set_error_callback (hb_wasm_error_callback_t callback,
                           void *user_data)
{
  error_callback = callback;
  error_callback_user_data = user_data;
}

const char *
hb_wasm_error_to_string (hb_wasm_fs_error_t error)
{
  switch (error)
  {
    case HB_WASM_FS_ERROR_NONE:
      return "No error";
    case HB_WASM_FS_ERROR_INIT_FAILED:
      return "Filesystem initialization failed";
    case HB_WASM_FS_ERROR_IDBFS_NOT_SUPPORTED:
      return "IndexedDB not supported";
    case HB_WASM_FS_ERROR_NETWORK_FAILED:
      return "Network request failed";
    case HB_WASM_FS_ERROR_CACHE_FULL:
      return "Font cache is full";
    case HB_WASM_FS_ERROR_INVALID_FONT:
      return "Invalid font data";
    case HB_WASM_FS_ERROR_CHECKSUM_MISMATCH:
      return "Font checksum mismatch";
    default:
      return "Unknown error";
  }
}

#else /* !HB_WASM_NATIVE_FS */

/* Production implementations with graceful fallback when filesystem support is disabled */
hb_bool_t
hb_wasm_filesystem_init (hb_wasm_filesystem_ready_callback_t callback,
                        void *user_data)
{
  /* Log fallback mode for debugging */
  EM_ASM({
    console.warn('HarfBuzz WASM: Native filesystem disabled, using memory-only fallback');
  });
  
  /* Initialize memory-based font cache as fallback */
  static hb_bool_t memory_cache_initialized = false;
  if (!memory_cache_initialized)
  {
    /* Set up in-memory font cache with reasonable defaults */
    EM_ASM({
      if (typeof Module === 'undefined') Module = {};
      if (!Module.fontCache) {
        Module.fontCache = new Map();
        Module.fontCacheStats = {
          totalSize: 0,
          hitCount: 0,
          missCount: 0,
          entryCount: 0
        };
        console.log('HarfBuzz WASM: Memory-based font cache initialized');
      }
    });
    memory_cache_initialized = true;
  }
  
  /* Immediately call callback to indicate "ready" in fallback mode */
  if (callback)
    callback (user_data);
  return true;
}

hb_bool_t
hb_wasm_filesystem_is_ready (void)
{
  /* Always ready in fallback mode */
  return true;
}

void
hb_wasm_load_font_from_url (const char *url,
                           const char *cache_key,
                           hb_wasm_font_load_callback_t callback,
                           void *user_data)
{
  /* Fallback implementation using memory-based caching */
  EM_ASM({
    const urlStr = UTF8ToString($0);
    const keyStr = UTF8ToString($1);
    const callback = $2;
    const userData = $3;
    
    fetch(urlStr)
      .then(response => response.arrayBuffer())
      .then(buffer => {
        const fontData = new Uint8Array(buffer);
        
        /* Store in memory cache */
        if (Module.fontCache) {
          Module.fontCache.set(keyStr, fontData);
          Module.fontCacheStats.totalSize += fontData.length;
          Module.fontCacheStats.entryCount++;
        }
        
        /* Create HarfBuzz blob and call callback */
        const dataPtr = Module._malloc(fontData.length);
        Module.HEAPU8.set(fontData, dataPtr);
        const blob = Module._hb_blob_create(dataPtr, fontData.length, 2, dataPtr, Module._free);
        
        if (callback) {
          Module.dynCall_vii(callback, blob, userData);
        }
      })
      .catch(error => {
        console.error('HarfBuzz WASM: Font load failed:', error);
        if (callback) {
          Module.dynCall_vii(callback, 0, userData);
        }
      });
  }, url, cache_key, callback, user_data);
}

hb_blob_t *
hb_wasm_load_font_from_cache (const char *cache_key)
{
  /* Memory-based cache lookup */
  int result = EM_ASM_INT({
    const keyStr = UTF8ToString($0);
    const fontData = Module.fontCache ? Module.fontCache.get(keyStr) : null;
    
    if (fontData) {
      Module.fontCacheStats.hitCount++;
      const dataPtr = Module._malloc(fontData.length);
      Module.HEAPU8.set(fontData, dataPtr);
      return Module._hb_blob_create(dataPtr, fontData.length, 2, dataPtr, Module._free);
    } else {
      if (Module.fontCacheStats) Module.fontCacheStats.missCount++;
      return 0;
    }
  }, cache_key);
  
  return (hb_blob_t*)result;
}

hb_wasm_cache_stats_t
hb_wasm_get_cache_stats (void)
{
  hb_wasm_cache_stats_t stats = {0};
  
  EM_ASM({
    const statsPtr = $0;
    if (Module.fontCacheStats) {
      Module.HEAPU32[statsPtr >> 2] = Module.fontCacheStats.totalSize;
      Module.HEAPU32[(statsPtr >> 2) + 1] = Module.fontCacheStats.totalSize;
      Module.HEAPU32[(statsPtr >> 2) + 2] = Module.fontCacheStats.entryCount;
      Module.HEAPU32[(statsPtr >> 2) + 3] = Module.fontCacheStats.hitCount;
      Module.HEAPU32[(statsPtr >> 2) + 4] = Module.fontCacheStats.missCount;
    }
  }, &stats);
  
  return stats;
}

void
hb_wasm_clear_font_cache (void)
{
  EM_ASM({
    if (Module.fontCache) {
      Module.fontCache.clear();
      Module.fontCacheStats = {
        totalSize: 0,
        hitCount: 0,
        missCount: 0,
        entryCount: 0
      };
      console.log('HarfBuzz WASM: Memory cache cleared');
    }
  });
}

#endif /* HB_WASM_NATIVE_FS */