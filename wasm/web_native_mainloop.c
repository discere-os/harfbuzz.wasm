/*
 * RequestAnimationFrame Loop Integration
 * Copyright 2025 Superstruct Ltd, New Zealand
 * For UI libraries: smooth 60fps rendering
 */

#include "web_native_capabilities.h"
#include <emscripten.h>

/**
 * Setup RAF-based main loop (for UI/rendering use cases)
 * Note: HarfBuzz is typically not UI-bound, but useful for demos
 */
EM_JS(void, web_setup_raf_loop, (void (*callback)(double), void* userdata), {
    let lastTime = 0;

    function loop(timestamp) {
        const deltaTime = timestamp - lastTime;
        lastTime = timestamp;

        if (callback) {
            dynCall('vd', callback, [deltaTime]);
        }

        requestAnimationFrame(loop);
    }

    requestAnimationFrame(loop);
});

/**
 * One-shot RAF callback (for async rendering)
 */
EM_JS(void, web_request_animation_frame, (void (*callback)(double), void* userdata), {
    requestAnimationFrame(timestamp => {
        if (callback) {
            dynCall('vd', callback, [timestamp]);
        }
    });
});
