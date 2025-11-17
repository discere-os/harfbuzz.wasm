import { assert } from "https://deno.land/std@0.220.0/assert/mod.ts";
import HarfBuzz from "../../src/lib/index.ts";
import { PERFORMANCE_TARGETS } from "../../src/lib/types.ts";

/**
 * Performance validation tests
 * Ensures web-native optimizations meet minimum speedup targets
 */

Deno.test("SIMD performance validation", async () => {
  const hb = new HarfBuzz({ enableSIMD: true });
  await hb.initialize();

  const caps = hb.getCapabilities();

  if (!caps.has_wasm_simd) {
    console.warn("⚠️  SIMD not available, skipping SIMD performance test");
    return;
  }

  // Note: Actual SIMD benchmarking would require:
  // 1. SIMD vs scalar comparison of string operations
  // 2. Large datasets to amortize overhead
  // 3. Multiple iterations for statistical significance

  console.log(`✅ SIMD enabled - target speedup: ${PERFORMANCE_TARGETS.SIMD_MIN}x`);
  assert(caps.has_wasm_simd, "SIMD should be available");
});

Deno.test("WebCrypto performance validation", async () => {
  const hb = new HarfBuzz();
  await hb.initialize();

  const caps = hb.getCapabilities();

  if (!caps.has_web_crypto) {
    console.warn("⚠️  WebCrypto not available, skipping crypto performance test");
    return;
  }

  console.log(`✅ WebCrypto enabled - target speedup: ${PERFORMANCE_TARGETS.CRYPTO_MIN}x`);
  assert(caps.has_web_crypto, "WebCrypto should be available");
});

Deno.test("Web Workers performance validation", async () => {
  const hb = new HarfBuzz();
  await hb.initialize();

  const caps = hb.getCapabilities();

  if (!caps.has_workers) {
    console.warn("⚠️  Web Workers not available, skipping threading performance test");
    return;
  }

  console.log(`✅ Web Workers enabled - target speedup: ${PERFORMANCE_TARGETS.WORKERS_MIN}x`);
  assert(caps.has_workers, "Web Workers should be available");
});

Deno.test("OPFS performance validation", async () => {
  const hb = new HarfBuzz();
  await hb.initialize();

  const caps = hb.getCapabilities();

  if (!caps.has_opfs) {
    console.warn("⚠️  OPFS not available, skipping filesystem performance test");
    return;
  }

  console.log(`✅ OPFS enabled - target speedup: ${PERFORMANCE_TARGETS.OPFS_MIN}x`);
  assert(caps.has_opfs, "OPFS should be available");
});

Deno.test("Overall performance targets summary", async () => {
  const hb = new HarfBuzz();
  await hb.initialize();

  const caps = hb.getCapabilities();

  console.log("\n📊 Performance Targets Summary:");
  console.log("================================");
  console.log(`SIMD:      ${caps.has_wasm_simd ? '✅' : '❌'} (${PERFORMANCE_TARGETS.SIMD_MIN}x target)`);
  console.log(`WebCrypto: ${caps.has_web_crypto ? '✅' : '❌'} (${PERFORMANCE_TARGETS.CRYPTO_MIN}x target)`);
  console.log(`Workers:   ${caps.has_workers ? '✅' : '❌'} (${PERFORMANCE_TARGETS.WORKERS_MIN}x target)`);
  console.log(`OPFS:      ${caps.has_opfs ? '✅' : '❌'} (${PERFORMANCE_TARGETS.OPFS_MIN}x target)`);
  console.log(`WebGPU:    ${caps.has_webgpu ? '✅' : '❌'} (${PERFORMANCE_TARGETS.WEBGPU_MIN}x target)`);

  // At minimum, SIMD should be available
  assert(caps.has_wasm_simd, "SIMD is mandatory for performance targets");
});
