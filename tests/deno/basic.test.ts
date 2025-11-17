import { assert, assertEquals, assertExists } from "https://deno.land/std@0.220.0/assert/mod.ts";
import HarfBuzz from "../../src/lib/index.ts";

Deno.test("Deno runtime features", () => {
  assert(typeof Deno !== 'undefined', "Deno runtime should be available");
  assert(typeof WebAssembly !== 'undefined', "WebAssembly should be available");
});

Deno.test("WASM file accessibility", async () => {
  try {
    const wasmFile = await Deno.stat("./install/wasm/harfbuzz-main.wasm");
    assert(wasmFile.isFile, "WASM file should exist");
    assert(wasmFile.size > 0, "WASM file should not be empty");
    console.log(`✅ Found WASM file: ${wasmFile.size} bytes`);
  } catch (error) {
    console.warn("⚠️  WASM file not found - run 'deno task build:wasm' first");
    throw error;
  }
});

Deno.test("TypeScript module imports", async () => {
  const { default: Module } = await import("../../src/lib/index.ts");
  assertExists(Module, "Module class should be importable");
  assert(typeof Module === 'function', "Module should be a constructor function");
});

Deno.test("HarfBuzz module loading", async () => {
  const hb = new HarfBuzz();
  await hb.initialize();
  assert(hb !== null, "HarfBuzz instance should be created");
});

Deno.test("Web capabilities detection", async () => {
  const hb = new HarfBuzz();
  await hb.initialize();

  const caps = hb.getCapabilities();
  assertExists(caps, "Capabilities should be returned");

  console.log("📊 Web-Native Capabilities:");
  console.log(`  WASM SIMD: ${caps.has_wasm_simd ? '✅' : '❌'}`);
  console.log(`  WebGPU: ${caps.has_webgpu ? '✅' : '❌'}`);
  console.log(`  Web Crypto: ${caps.has_web_crypto ? '✅' : '❌'}`);
  console.log(`  OPFS: ${caps.has_opfs ? '✅' : '❌'}`);
  console.log(`  Workers: ${caps.has_workers ? '✅' : '❌'}`);
  console.log(`  Chrome Version: ${caps.chrome_version}`);

  // Verify expected capabilities in Deno environment
  assert(caps.has_wasm_simd, "WASM SIMD should be available (Chrome 113+)");
  assert(typeof caps.chrome_version === 'number', "Chrome version should be a number");
});

Deno.test("Browser requirements verification", async () => {
  const hb = new HarfBuzz();
  await hb.initialize();

  const meetsRequirements = hb.verifyRequirements();
  assert(
    meetsRequirements,
    "Browser should meet minimum requirements (Chrome 113+ with SIMD)"
  );
});

Deno.test("HarfBuzz version retrieval", async () => {
  const hb = new HarfBuzz();
  await hb.initialize();

  const version = hb.getVersion();
  assertExists(version, "Version should be returned");
  assert(version.length > 0, "Version should not be empty");
  console.log(`📦 HarfBuzz version: ${version}`);
});

Deno.test("Error handling - uninitialized access", () => {
  const hb = new HarfBuzz();

  try {
    hb.getVersion(); // Should throw
    assert(false, "Should have thrown error");
  } catch (error) {
    assert(error instanceof Error, "Should throw Error");
    assert(
      error.message.includes("not initialized"),
      "Error message should mention initialization"
    );
  }
});

Deno.test("Resource disposal", async () => {
  const hb = new HarfBuzz();
  await hb.initialize();

  hb.dispose();

  try {
    hb.getVersion(); // Should throw after disposal
    assert(false, "Should have thrown error");
  } catch (error) {
    assert(error instanceof Error, "Should throw Error after disposal");
  }
});
