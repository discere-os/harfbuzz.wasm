/*
 * Origin Private File System (OPFS) Integration
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Provides 3-4x speedup vs IDBFS for font caching
 */

#include "web_native_capabilities.h"
#include <emscripten.h>

/**
 * OPFS file read (3-4x faster than IDBFS)
 * For HarfBuzz: cached font files, glyph metrics
 */
EM_JS(void, web_opfs_read_async,
    (const char* path, uint8_t* buffer, size_t* size, void (*callback)(int, void*), void* userdata), {

    if (typeof navigator.storage === 'undefined' ||
        typeof navigator.storage.getDirectory !== 'function') {
        console.warn('OPFS not available');
        if (callback) dynCall('vii', callback, [-1, userdata]);
        return;
    }

    const pathStr = UTF8ToString(path);

    navigator.storage.getDirectory()
        .then(root => root.getFileHandle(pathStr))
        .then(handle => handle.getFile())
        .then(file => file.arrayBuffer())
        .then(data => {
            HEAPU8.set(new Uint8Array(data), buffer);
            setValue(size, data.byteLength, 'i32');
            if (callback) dynCall('vii', callback, [0, userdata]);
        })
        .catch(err => {
            console.error('OPFS read error:', err);
            if (callback) dynCall('vii', callback, [-1, userdata]);
        });
});

/**
 * OPFS file write (3-4x faster than IDBFS)
 */
EM_JS(void, web_opfs_write_async,
    (const char* path, const uint8_t* buffer, size_t size, void (*callback)(int, void*), void* userdata), {

    if (typeof navigator.storage === 'undefined' ||
        typeof navigator.storage.getDirectory !== 'function') {
        console.warn('OPFS not available');
        if (callback) dynCall('vii', callback, [-1, userdata]);
        return;
    }

    const pathStr = UTF8ToString(path);
    const data = HEAPU8.slice(buffer, buffer + size);

    navigator.storage.getDirectory()
        .then(root => root.getFileHandle(pathStr, { create: true }))
        .then(handle => handle.createWritable())
        .then(writable => {
            return writable.write(data).then(() => writable.close());
        })
        .then(() => {
            if (callback) dynCall('vii', callback, [0, userdata]);
        })
        .catch(err => {
            console.error('OPFS write error:', err);
            if (callback) dynCall('vii', callback, [-1, userdata]);
        });
});

/**
 * Check OPFS availability
 */
EMSCRIPTEN_KEEPALIVE
int web_has_opfs(void) {
    return web_get_capabilities()->has_opfs;
}
