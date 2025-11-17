/**
 * HarfBuzz SIMD Performance Benchmarks
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under MIT License
 */

import HarfBuzzModule from "../install/wasm/harfbuzz-main.js";

console.log("[Benchmark] HarfBuzz SIMD optimizations");

// Load the WASM module
const module = await HarfBuzzModule();

console.log("Module loaded successfully");

// Test data: Arabic text (complex shaping)
const arabicText = "مرحبا بك في اختبار الأداء";
const arabicCodepoints = Array.from(arabicText).map(c => c.codePointAt(0)!);

console.log(`\n[Test Data] Arabic text: "${arabicText}"`);
console.log(`[Test Data] Codepoints: ${arabicCodepoints.length} characters`);

// Allocate memory for test data
const input_ptr = module._malloc(arabicCodepoints.length * 4);
const output_ptr = module._malloc(arabicCodepoints.length * 4);

const input_arr = new Uint32Array(
    module.HEAPU8.buffer,
    input_ptr,
    arabicCodepoints.length
);
input_arr.set(arabicCodepoints);

// Benchmark glyph positioning
console.log("\n[Test 1] Glyph positioning (SIMD vs Scalar)...");

const POSITION_COUNT = 1000;
const POSITION_ITERATIONS = 10000;

const pos_x = new Int32Array(POSITION_COUNT);
const pos_y = new Int32Array(POSITION_COUNT);
for (let i = 0; i < POSITION_COUNT; i++) {
    pos_x[i] = Math.random() * 100 | 0;
    pos_y[i] = Math.random() * 100 | 0;
}

const pos_x_ptr = module._malloc(POSITION_COUNT * 4);
const pos_y_ptr = module._malloc(POSITION_COUNT * 4);
const out_x_ptr = module._malloc(POSITION_COUNT * 4);
const out_y_ptr = module._malloc(POSITION_COUNT * 4);

module.HEAP32.set(pos_x, pos_x_ptr / 4);
module.HEAP32.set(pos_y, pos_y_ptr / 4);

// Scalar baseline
console.log(`  Running ${POSITION_ITERATIONS} iterations (scalar)...`);
const scalar_start = performance.now();
for (let i = 0; i < POSITION_ITERATIONS; i++) {
    module._hb_scalar_apply_positions(
        pos_x_ptr, pos_y_ptr, out_x_ptr, out_y_ptr, POSITION_COUNT);
}
const scalar_time = performance.now() - scalar_start;

// SIMD optimized
console.log(`  Running ${POSITION_ITERATIONS} iterations (SIMD)...`);
const simd_start = performance.now();
for (let i = 0; i < POSITION_ITERATIONS; i++) {
    module._hb_simd_apply_positions(
        pos_x_ptr, pos_y_ptr, out_x_ptr, out_y_ptr, POSITION_COUNT);
}
const simd_time = performance.now() - simd_start;

const position_speedup = scalar_time / simd_time;
console.log(`  Scalar time: ${scalar_time.toFixed(0)}ms`);
console.log(`  SIMD time: ${simd_time.toFixed(0)}ms`);
console.log(`  Speedup: ${position_speedup.toFixed(1)}x`);

if (position_speedup < 3.0) {
    console.warn(`  ⚠️  WARNING: Glyph positioning speedup ${position_speedup.toFixed(1)}x < 3.0x target`);
} else {
    console.log(`  ✅ PASSED: Glyph positioning achieved ${position_speedup.toFixed(1)}x speedup`);
}

// Benchmark buffer operations
console.log("\n[Test 2] Buffer copy operations (SIMD vs memcpy)...");

const BUFFER_SIZE = 4096;
const BUFFER_ITERATIONS = 5000;

const src_ptr = module._malloc(BUFFER_SIZE);
const dst_ptr = module._malloc(BUFFER_SIZE);

// Fill source buffer with random data
const src_data = new Uint8Array(BUFFER_SIZE);
for (let i = 0; i < BUFFER_SIZE; i++) {
    src_data[i] = Math.random() * 256 | 0;
}
module.HEAPU8.set(src_data, src_ptr);

// Standard memcpy baseline
console.log(`  Running ${BUFFER_ITERATIONS} iterations (memcpy)...`);
const memcpy_start = performance.now();
for (let i = 0; i < BUFFER_ITERATIONS; i++) {
    // JavaScript memcpy equivalent
    const src = new Uint8Array(module.HEAPU8.buffer, src_ptr, BUFFER_SIZE);
    const dst = new Uint8Array(module.HEAPU8.buffer, dst_ptr, BUFFER_SIZE);
    dst.set(src);
}
const memcpy_time = performance.now() - memcpy_start;

// SIMD optimized copy
console.log(`  Running ${BUFFER_ITERATIONS} iterations (SIMD)...`);
const simd_copy_start = performance.now();
for (let i = 0; i < BUFFER_ITERATIONS; i++) {
    module._hb_simd_copy_buffer(dst_ptr, src_ptr, BUFFER_SIZE);
}
const simd_copy_time = performance.now() - simd_copy_start;

const copy_speedup = memcpy_time / simd_copy_time;
console.log(`  memcpy time: ${memcpy_time.toFixed(0)}ms`);
console.log(`  SIMD time: ${simd_copy_time.toFixed(0)}ms`);
console.log(`  Speedup: ${copy_speedup.toFixed(1)}x`);

if (copy_speedup < 2.0) {
    console.warn(`  ⚠️  WARNING: Buffer copy speedup ${copy_speedup.toFixed(1)}x < 2.0x target`);
} else {
    console.log(`  ✅ PASSED: Buffer copy achieved ${copy_speedup.toFixed(1)}x speedup`);
}

// Cleanup
module._free(input_ptr);
module._free(output_ptr);
module._free(pos_x_ptr);
module._free(pos_y_ptr);
module._free(out_x_ptr);
module._free(out_y_ptr);
module._free(src_ptr);
module._free(dst_ptr);

// Summary
console.log("\n" + "=".repeat(60));
console.log("BENCHMARK SUMMARY");
console.log("=".repeat(60));
console.log(`Glyph positioning: ${position_speedup.toFixed(1)}x speedup`);
console.log(`Buffer operations: ${copy_speedup.toFixed(1)}x speedup`);

const overall_pass = position_speedup >= 3.0 && copy_speedup >= 2.0;
if (overall_pass) {
    console.log("\n✅ All HarfBuzz SIMD benchmarks passed!");
} else {
    console.log("\n❌ Some benchmarks did not meet target performance");
    Deno.exit(1);
}
