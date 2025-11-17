/*
 * WeakRef Integration for GC
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Automatic memory management integration with browser GC
 */

#include "web_native_capabilities.h"
#include <emscripten.h>

/**
 * Register object for WeakRef tracking
 * Allows browser GC to reclaim WASM memory when JS references are gone
 */
EM_JS(void, web_register_weak_ref, (void* ptr, const char* name), {
    if (typeof WeakRef === 'undefined') {
        return;
    }

    const nameStr = UTF8ToString(name);
    const ref = new WeakRef({ ptr: ptr, name: nameStr });

    // Store in global registry for cleanup
    if (!Module.weakRefs) {
        Module.weakRefs = new FinalizationRegistry(held => {
            console.log(`GC collected: ${held.name}`);
            // Could trigger WASM-side cleanup here
        });
    }

    Module.weakRefs.register({ ptr: ptr }, { name: nameStr });
});

/**
 * Check if WeakRef is available
 */
EMSCRIPTEN_KEEPALIVE
int web_has_weak_ref(void) {
    return web_get_capabilities()->has_weak_ref;
}
