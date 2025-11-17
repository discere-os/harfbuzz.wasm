/**
 * Type definitions for HarfBuzz.wasm
 * Copyright 2025 Superstruct Ltd, New Zealand
 */

/**
 * Emscripten WASM module interface
 */
export interface HarfBuzzModule {
  ccall: (funcName: string, returnType: string, argTypes: string[], args: unknown[]) => unknown;
  cwrap: (funcName: string, returnType: string, argTypes: string[]) => Function;
  FS: unknown;
  HEAPU8: Uint8Array;
  HEAP8: Int8Array;
  HEAP16: Int16Array;
  HEAP32: Int32Array;
  HEAPF32: Float32Array;
  HEAPF64: Float64Array;
  UTF8ToString: (ptr: number) => string;
  _malloc: (size: number) => number;
  _free: (ptr: number) => void;
  setValue: (ptr: number, value: number, type: string) => void;
  getValue: (ptr: number, type: string) => number;
}

/**
 * Web-native capabilities structure
 */
export interface WebCapabilities {
  /** WASM SIMD support (3-5x string operations) */
  has_wasm_simd: boolean;

  /** WebGPU support (10x+ compute) */
  has_webgpu: boolean;

  /** SharedArrayBuffer support (required for threading) */
  has_shared_array_buffer: boolean;

  /** Web Crypto API support (5-15x crypto) */
  has_web_crypto: boolean;

  /** Origin Private File System (3-4x I/O) */
  has_opfs: boolean;

  /** Web Workers support (10x threading) */
  has_workers: boolean;

  /** Fetch API with streaming */
  has_fetch_streaming: boolean;

  /** WeakRef for GC integration */
  has_weak_ref: boolean;

  /** Chrome version (0 if not Chrome, target: 113+) */
  chrome_version: number;
}

/**
 * HarfBuzz configuration options
 */
export interface HarfBuzzConfig {
  /** Enable SIMD optimizations (3-5x speedup) */
  enableSIMD?: boolean;

  /** Enable WebGPU acceleration (10x+ speedup) */
  enableWebGPU?: boolean;

  /** Initial memory size in bytes (default: 128MB) */
  initialMemory?: number;

  /** Maximum memory size in bytes (default: 1GB) */
  maximumMemory?: number;
}

/**
 * Performance metrics for web-native features
 */
export interface PerformanceMetrics {
  /** SIMD speedup multiplier (target: 3-5x) */
  simdSpeedup: number;

  /** WebGPU speedup multiplier (target: 10x+) */
  webgpuSpeedup: number;

  /** Workers speedup multiplier (target: 10x) */
  workersSpeedup: number;

  /** Crypto speedup multiplier (target: 5-15x) */
  cryptoSpeedup: number;

  /** OPFS I/O speedup multiplier (target: 3-4x) */
  opfsSpeedup: number;
}

/**
 * HarfBuzz error class
 */
export class HarfBuzzError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'HarfBuzzError';
  }
}

/**
 * Browser requirements for optimal performance
 */
export interface BrowserRequirements {
  /** Minimum Chrome version */
  minChromeVersion: number;

  /** Required features */
  requiredFeatures: string[];

  /** Optional features for enhanced performance */
  optionalFeatures: string[];
}

/**
 * Default browser requirements
 */
export const DEFAULT_BROWSER_REQUIREMENTS: BrowserRequirements = {
  minChromeVersion: 113,
  requiredFeatures: [
    'wasm_simd',
    'bigint',
  ],
  optionalFeatures: [
    'webgpu',
    'shared_array_buffer',
    'web_crypto',
    'opfs',
    'workers',
  ],
};

/**
 * Performance targets for validation
 */
export const PERFORMANCE_TARGETS = {
  /** SIMD minimum speedup */
  SIMD_MIN: 3.0,

  /** WebCrypto minimum speedup */
  CRYPTO_MIN: 5.0,

  /** Web Workers minimum speedup */
  WORKERS_MIN: 10.0,

  /** WebGPU minimum speedup */
  WEBGPU_MIN: 10.0,

  /** OPFS minimum speedup */
  OPFS_MIN: 3.0,
} as const;
