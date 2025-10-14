/**
 * Harfbuzz WASM Benchmarks
 */

import HarfbuzzWASM from "../src/lib/index.ts"

Deno.bench("harfbuzz initialization", {
  baseline: true
}, async () => {
  const lib = new HarfbuzzWASM()
  await lib.initialize()
})
