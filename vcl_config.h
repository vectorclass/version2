/*
 * Copyright 2026 DigitalExpressions Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Backend selection. All translation units exchanging VCL objects must agree.
#ifndef VCL_CONFIG_H
#define VCL_CONFIG_H
#define VCL_BACKEND_X86 1
#define VCL_BACKEND_SSE2NEON 2
#define VCL_BACKEND_NEON 3
#if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#define VCL_ARCH_ARM64 1
#else
#define VCL_ARCH_ARM64 0
#endif
#ifndef VCL_BACKEND
#if VCL_ARCH_ARM64
#define VCL_BACKEND VCL_BACKEND_SSE2NEON
#else
#define VCL_BACKEND VCL_BACKEND_X86
#endif
#endif
#if VCL_BACKEND != 1 && VCL_BACKEND != 2 && VCL_BACKEND != 3
#error Invalid VCL_BACKEND
#endif
#if VCL_ARCH_ARM64 && VCL_BACKEND == VCL_BACKEND_X86
#error The x86 backend requires an x86 target
#elif !VCL_ARCH_ARM64 && VCL_BACKEND != VCL_BACKEND_X86
#error ARM backends require an AArch64 target
#endif
#ifndef MAX_VECTOR_SIZE
#define MAX_VECTOR_SIZE 512
#endif
#if MAX_VECTOR_SIZE != 128 && MAX_VECTOR_SIZE != 256 && MAX_VECTOR_SIZE != 512
#error MAX_VECTOR_SIZE must be 128, 256, or 512
#endif
#if VCL_BACKEND == VCL_BACKEND_SSE2NEON
// Accurate division/sqrt and SSE operand ordering for min/max are the baseline.
#ifndef SSE2NEON_PRECISE_DIV
#define SSE2NEON_PRECISE_DIV 1
#endif
#ifndef SSE2NEON_PRECISE_SQRT
#define SSE2NEON_PRECISE_SQRT 1
#endif
#ifndef SSE2NEON_PRECISE_MINMAX
#define SSE2NEON_PRECISE_MINMAX 1
#endif
#endif
#if VCL_BACKEND == VCL_BACKEND_NEON && !defined(VCL_ENABLE_EXPERIMENTAL_NEON)
#error Define VCL_ENABLE_EXPERIMENTAL_NEON to enable the incomplete native backend
#endif
#endif
