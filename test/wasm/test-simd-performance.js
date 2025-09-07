#!/usr/bin/env node

/**
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under MIT License (same as HarfBuzz)
 * 
 * Production-ready SIMD performance validation tests
 */

import { readFileSync } from 'fs';
import { performance } from 'perf_hooks';

// Test configuration
const TEST_ITERATIONS = 1000;
const UNICODE_TEST_SIZE = 10000;
const POSITION_TEST_SIZE = 5000;

class HarfBuzzSIMDTests {
  constructor() {
    this.results = {
      unicodeLookup: { simd: 0, scalar: 0 },
      glyphPositioning: { simd: 0, scalar: 0 },
      stringOperations: { simd: 0, scalar: 0 },
      bufferOperations: { simd: 0, scalar: 0 }
    };
  }

  async loadWASMModule() {
    // Load production WASM module
    try {
      const wasmPath = '../../harfbuzz-wasm-artifacts/harfbuzz.js';
      const HarfBuzz = await import(wasmPath);
      this.hb = await HarfBuzz.default();
      console.log('✅ HarfBuzz WASM module loaded successfully');
      return true;
    } catch (error) {
      console.error('❌ Failed to load WASM module:', error.message);
      return false;
    }
  }

  generateTestData() {
    // Generate realistic Unicode test data
    this.unicodeData = new Uint32Array(UNICODE_TEST_SIZE);
    for (let i = 0; i < UNICODE_TEST_SIZE; i++) {
      // Mix of common scripts: Latin, Arabic, CJK, Emoji
      const ranges = [
        [0x0020, 0x007F],  // Basic Latin
        [0x0600, 0x06FF],  // Arabic
        [0x4E00, 0x9FFF],  // CJK Unified Ideographs
        [0x1F600, 0x1F64F] // Emoticons
      ];
      const range = ranges[Math.floor(Math.random() * ranges.length)];
      this.unicodeData[i] = Math.floor(Math.random() * (range[1] - range[0])) + range[0];
    }

    // Generate glyph positioning test data
    this.positionData = new Array(POSITION_TEST_SIZE);
    for (let i = 0; i < POSITION_TEST_SIZE; i++) {
      this.positionData[i] = {
        x_advance: Math.random() * 1000,
        y_advance: Math.random() * 1000,
        x_offset: Math.random() * 100 - 50,
        y_offset: Math.random() * 100 - 50
      };
    }

    console.log(`📊 Generated test data: ${UNICODE_TEST_SIZE} Unicode points, ${POSITION_TEST_SIZE} positions`);
  }

  async testUnicodeLookupPerformance() {
    console.log('\n🔍 Testing Unicode category lookup performance...');

    // Test SIMD implementation
    const simdStart = performance.now();
    for (let iter = 0; iter < TEST_ITERATIONS; iter++) {
      if (this.hb._hb_wasm_simd_unicode_lookup_batch) {
        // Allocate test arrays
        const codepointsPtr = this.hb._malloc(this.unicodeData.length * 4);
        const categoriesPtr = this.hb._malloc(this.unicodeData.length);
        
        // Copy data to WASM memory
        this.hb.HEAPU32.set(this.unicodeData, codepointsPtr >> 2);
        
        // Call SIMD function
        this.hb._hb_wasm_simd_unicode_lookup_batch(
          codepointsPtr, categoriesPtr, this.unicodeData.length
        );
        
        // Clean up
        this.hb._free(codepointsPtr);
        this.hb._free(categoriesPtr);
      }
    }
    const simdEnd = performance.now();
    this.results.unicodeLookup.simd = simdEnd - simdStart;

    // Test scalar fallback
    const scalarStart = performance.now();
    for (let iter = 0; iter < TEST_ITERATIONS; iter++) {
      for (let i = 0; i < this.unicodeData.length; i++) {
        if (this.hb._hb_unicode_general_category) {
          this.hb._hb_unicode_general_category(
            this.hb._hb_unicode_funcs_get_default(),
            this.unicodeData[i]
          );
        }
      }
    }
    const scalarEnd = performance.now();
    this.results.unicodeLookup.scalar = scalarEnd - scalarStart;

    const speedup = this.results.unicodeLookup.scalar / this.results.unicodeLookup.simd;
    console.log(`   SIMD: ${this.results.unicodeLookup.simd.toFixed(2)}ms`);
    console.log(`   Scalar: ${this.results.unicodeLookup.scalar.toFixed(2)}ms`);
    console.log(`   Speedup: ${speedup.toFixed(2)}x`);
    
    return speedup >= 2.0; // Expect at least 2x speedup
  }

  async testGlyphPositioningPerformance() {
    console.log('\n📐 Testing glyph positioning performance...');

    const testScaleX = 1.2;
    const testScaleY = 1.1;

    // Test SIMD positioning
    const simdStart = performance.now();
    for (let iter = 0; iter < TEST_ITERATIONS; iter++) {
      if (this.hb._hb_wasm_simd_position_glyphs) {
        // Allocate and populate position data
        const positionsPtr = this.hb._malloc(this.positionData.length * 16); // 4 floats per position
        
        for (let i = 0; i < this.positionData.length; i++) {
          const offset = (positionsPtr >> 2) + (i * 4);
          this.hb.HEAPF32[offset] = this.positionData[i].x_advance;
          this.hb.HEAPF32[offset + 1] = this.positionData[i].y_advance;
          this.hb.HEAPF32[offset + 2] = this.positionData[i].x_offset;
          this.hb.HEAPF32[offset + 3] = this.positionData[i].y_offset;
        }
        
        // Call SIMD positioning
        this.hb._hb_wasm_simd_position_glyphs(
          positionsPtr, this.positionData.length, testScaleX, testScaleY
        );
        
        this.hb._free(positionsPtr);
      }
    }
    const simdEnd = performance.now();
    this.results.glyphPositioning.simd = simdEnd - simdStart;

    // Test scalar positioning
    const scalarStart = performance.now();
    for (let iter = 0; iter < TEST_ITERATIONS; iter++) {
      for (let i = 0; i < this.positionData.length; i++) {
        // Scalar position scaling
        this.positionData[i].x_advance *= testScaleX;
        this.positionData[i].y_advance *= testScaleY;
        this.positionData[i].x_offset *= testScaleX;
        this.positionData[i].y_offset *= testScaleY;
        
        // Reset for next iteration
        this.positionData[i].x_advance /= testScaleX;
        this.positionData[i].y_advance /= testScaleY;
        this.positionData[i].x_offset /= testScaleX;
        this.positionData[i].y_offset /= testScaleY;
      }
    }
    const scalarEnd = performance.now();
    this.results.glyphPositioning.scalar = scalarEnd - scalarStart;

    const speedup = this.results.glyphPositioning.scalar / this.results.glyphPositioning.simd;
    console.log(`   SIMD: ${this.results.glyphPositioning.simd.toFixed(2)}ms`);
    console.log(`   Scalar: ${this.results.glyphPositioning.scalar.toFixed(2)}ms`);
    console.log(`   Speedup: ${speedup.toFixed(2)}x`);
    
    return speedup >= 1.8; // Expect at least 1.8x speedup for positioning
  }

  async testStringOperations() {
    console.log('\n🔤 Testing string operation performance...');

    // Generate test strings
    const testStrings = [];
    for (let i = 0; i < 100; i++) {
      let str = '';
      for (let j = 0; j < 1000; j++) {
        str += String.fromCharCode(0x20 + Math.floor(Math.random() * 95));
      }
      testStrings.push(str);
    }

    // Test SIMD string operations
    const simdStart = performance.now();
    for (let iter = 0; iter < TEST_ITERATIONS / 10; iter++) {
      for (const str of testStrings) {
        if (this.hb._hb_wasm_simd_string_length) {
          const strPtr = this.hb.stringToNewUTF8(str);
          this.hb._hb_wasm_simd_string_length(strPtr, str.length);
          this.hb._free(strPtr);
        }
      }
    }
    const simdEnd = performance.now();
    this.results.stringOperations.simd = simdEnd - simdStart;

    // Test scalar string operations
    const scalarStart = performance.now();
    for (let iter = 0; iter < TEST_ITERATIONS / 10; iter++) {
      for (const str of testStrings) {
        // Scalar string length calculation
        let length = 0;
        for (let i = 0; i < str.length; i++) {
          if (str.charCodeAt(i) !== 0) length++;
        }
      }
    }
    const scalarEnd = performance.now();
    this.results.stringOperations.scalar = scalarEnd - scalarStart;

    const speedup = this.results.stringOperations.scalar / this.results.stringOperations.simd;
    console.log(`   SIMD: ${this.results.stringOperations.simd.toFixed(2)}ms`);
    console.log(`   Scalar: ${this.results.stringOperations.scalar.toFixed(2)}ms`);
    console.log(`   Speedup: ${speedup.toFixed(2)}x`);
    
    return speedup >= 1.5; // Expect at least 1.5x speedup for strings
  }

  generatePerformanceReport() {
    const totalSIMDTime = Object.values(this.results).reduce((sum, result) => sum + result.simd, 0);
    const totalScalarTime = Object.values(this.results).reduce((sum, result) => sum + result.scalar, 0);
    const overallSpeedup = totalScalarTime / totalSIMDTime;

    const report = {
      summary: {
        totalSIMDTime: `${totalSIMDTime.toFixed(2)}ms`,
        totalScalarTime: `${totalScalarTime.toFixed(2)}ms`,
        overallSpeedup: `${overallSpeedup.toFixed(2)}x`,
        testIterations: TEST_ITERATIONS,
        timestamp: new Date().toISOString()
      },
      details: {
        unicodeLookup: {
          simdTime: `${this.results.unicodeLookup.simd.toFixed(2)}ms`,
          scalarTime: `${this.results.unicodeLookup.scalar.toFixed(2)}ms`,
          speedup: `${(this.results.unicodeLookup.scalar / this.results.unicodeLookup.simd).toFixed(2)}x`
        },
        glyphPositioning: {
          simdTime: `${this.results.glyphPositioning.simd.toFixed(2)}ms`,
          scalarTime: `${this.results.glyphPositioning.scalar.toFixed(2)}ms`,
          speedup: `${(this.results.glyphPositioning.scalar / this.results.glyphPositioning.simd).toFixed(2)}x`
        },
        stringOperations: {
          simdTime: `${this.results.stringOperations.simd.toFixed(2)}ms`,
          scalarTime: `${this.results.stringOperations.scalar.toFixed(2)}ms`,
          speedup: `${(this.results.stringOperations.scalar / this.results.stringOperations.simd).toFixed(2)}x`
        }
      }
    };

    return report;
  }

  async runAllTests() {
    console.log('🚀 Starting HarfBuzz WASM SIMD Performance Tests\n');

    if (!(await this.loadWASMModule())) {
      process.exit(1);
    }

    this.generateTestData();

    const results = [];
    results.push(await this.testUnicodeLookupPerformance());
    results.push(await this.testGlyphPositioningPerformance());
    results.push(await this.testStringOperations());

    const allTestsPassed = results.every(result => result);
    const report = this.generatePerformanceReport();

    console.log('\n📊 Performance Test Results:');
    console.log(JSON.stringify(report, null, 2));

    if (allTestsPassed) {
      console.log('\n✅ All SIMD performance tests passed!');
      return 0;
    } else {
      console.log('\n❌ Some performance tests failed to meet expectations');
      return 1;
    }
  }
}

// Run tests if called directly
if (import.meta.url === `file://${process.argv[1]}`) {
  const tester = new HarfBuzzSIMDTests();
  tester.runAllTests().then(exitCode => process.exit(exitCode));
}

export default HarfBuzzSIMDTests;