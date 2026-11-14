/****************************  instrset.h   **********************************
* Author:        Agner Fog
* Date created:  2012-05-30
* Last modified: 2026-09-05
* Version:       2.02.04
* Project:       vector class library
* Description:
* Header file for various compiler-specific tasks as well as common
* macros and templates. This file contains:
*
* > Selection of the supported instruction set
* > Defines compiler version macros
* > Undefines certain macros that prevent function overloading
* > Helper functions that depend on instruction set, compiler, or platform
* > Common templates for permute, blend, etc.
*
* For instructions, see vcl_manual.pdf
*
* (c) Copyright 2012-2026 Agner Fog.
* Apache License version 2.0 or later.
******************************************************************************/

#ifndef INSTRSET_H
#define INSTRSET_H 20200
#include "vcl_config.h"
#if VCL_BACKEND == VCL_BACKEND_NEON
#include "instrset_neon.h"
#else
#if VCL_BACKEND == VCL_BACKEND_SSE2NEON
#include "sse2neon.h"
#ifndef INSTRSET
#define INSTRSET 6
#elif INSTRSET != 6
#error sse2neon requires INSTRSET=6
#endif
#endif

// check if compiled for C++17
#if defined(_MSVC_LANG)  // MS compiler has its own version of __cplusplus with different value
#if _MSVC_LANG < 201703
#error Please compile for C++17 or higher
#endif
#else  // all other compilers
#if __cplusplus < 201703
#error Please compile for C++17 or higher
#endif
#endif

// Allow the use of floating point permute instructions on integer vectors.
// Some CPU's have an extra latency of 1 or 2 clock cycles for this, but
// it may still be faster than alternative implementations:
#define ALLOW_FP_PERMUTE  true


// Macro to indicate 64 bit mode
#if (defined(_M_AMD64) || defined(_M_X64) || defined(__amd64) ) && ! defined(__x86_64__)
#define __x86_64__ 1  // There are many different macros for this, decide on only one
#endif

// The following values of INSTRSET are currently defined:
// 2:  SSE2
// 3:  SSE3
// 4:  SSSE3
// 5:  SSE4.1
// 6:  SSE4.2
// 7:  AVX
// 8:  AVX2
// 9:  AVX512F
// 10: AVX512BW/DQ/VL
// 11: AVX512VBMI and AVX512VBMI2
// 12: AVX512FP16

// Find instruction set from compiler macros if INSTRSET is not defined.
// Note: Some of these macros are not defined in Microsoft compilers
#ifndef INSTRSET

#if defined ( __AVX512VBMI__ ) && defined ( __AVX512VBMI2__ ) && defined ( __AVX512FP16__ )
#define INSTRSET 12
#elif defined ( __AVX512VBMI__ ) && defined ( __AVX512VBMI2__ )
#define INSTRSET 11
#elif defined ( __AVX512VL__ ) && defined ( __AVX512BW__ ) && defined ( __AVX512DQ__ )
#define INSTRSET 10
#elif defined ( __AVX512F__ ) || defined ( __AVX512__ )
#define INSTRSET 9
#elif defined ( __AVX2__ )
#define INSTRSET 8
#elif defined ( __AVX__ )
#define INSTRSET 7
#elif defined ( __SSE4_2__ )
#define INSTRSET 6
#elif defined ( __SSE4_1__ )
#define INSTRSET 5
#elif defined ( __SSSE3__ )
#define INSTRSET 4
#elif defined ( __SSE3__ )
#define INSTRSET 3
#elif defined ( __SSE2__ ) || defined ( __x86_64__ )
#define INSTRSET 2
#elif defined ( __SSE__ )
#define INSTRSET 1
#elif defined ( _M_IX86_FP )           // Defined in MS compiler. 1: SSE, 2: SSE2
#define INSTRSET _M_IX86_FP
#else
#define INSTRSET 0
#endif // instruction set defines
#endif // INSTRSET


#if INSTRSET >= 8 && !defined(__FMA__)
// Assume that all processors that have AVX2 also have FMA3
#if defined (__GNUC__) && ! defined (__INTEL_COMPILER)
// Prevent error message in g++ and Clang when using FMA intrinsics with avx2:
#if !defined(DISABLE_WARNING_AVX2_WITHOUT_FMA)
#pragma message "It is recommended to specify also option -mfma when using -mavx2 or higher"
#endif
#elif ! defined (__clang__)
#define __FMA__  1
#endif
#endif

// Header files for non-vector intrinsic functions including _BitScanReverse(int), __cpuid(int[4],int), _xgetbv(int)
#ifdef _MSC_VER                        // Microsoft compiler or compatible Intel compiler
#include <intrin.h>
#pragma warning(disable: 6323 4514 4710 4711) // Diasble annoying warnings
#elif __x86_64__
#include <x86intrin.h>                 // Gcc or Clang compiler
#endif

#include <stdint.h>                    // Define integer types with known size
#include <limits.h>                    // Define INT_MAX
#include <stdlib.h>                    // define abs(int)


// functions in instrset_detect.cpp:
#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif
    constexpr bool has_neon() { return VCL_ARCH_ARM64 != 0; }
    int  instrset_detect(void);        // tells which instruction sets are supported
    bool hasFMA3(void);                // true if FMA3 instructions supported
    bool hasFMA4(void);                // true if FMA4 instructions supported
    bool hasXOP(void);                 // true if XOP  instructions supported
    bool hasAVX512ER(void);            // true if AVX512ER instructions supported
    bool hasAVX512VBMI(void);          // true if AVX512VBMI instructions supported
    bool hasAVX512VBMI2(void);         // true if AVX512VBMI2 instructions supported
    bool hasF16C(void);                // true if F16C instructions supported
    bool hasAVX512FP16(void);          // true if AVX512_FP16 instructions supported

    // function in physical_processors.cpp:
    int physicalProcessors(int * logical_processors = 0);

#ifdef VCL_NAMESPACE
}
#endif


// GCC version
#if defined(__GNUC__) && !defined (GCC_VERSION) && !defined (__clang__)
#define GCC_VERSION  ((__GNUC__) * 10000 + (__GNUC_MINOR__) * 100 + (__GNUC_PATCHLEVEL__))
#endif

// Clang version
#if defined (__clang__)
#define CLANG_VERSION  ((__clang_major__) * 10000 + (__clang_minor__) * 100 + (__clang_patchlevel__))
// Problem: The version number is not consistent across platforms
// http://llvm.org/bugs/show_bug.cgi?id=12643
// Apple bug 18746972
#endif

// Fix problem with non-overloadable macros named min and max in WinDef.h
#ifdef _MSC_VER
#if defined (_WINDEF_) && defined(min) && defined(max)
#undef min
#undef max
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// warning for poor support for AVX512F in MS compiler
#if !defined(__INTEL_COMPILER) && !defined(__clang__)
#if INSTRSET == 9
#pragma message("Warning: MS compiler cannot generate code for AVX512F without AVX512DQ")
#endif
#if _MSC_VER < 1920 && INSTRSET > 8
#pragma message("Warning: Your compiler has poor support for AVX512. Code may be erroneous.\nPlease use a newer compiler version or a different compiler!")
#endif
#endif // __INTEL_COMPILER
#endif // _MSC_VER

#if defined(__INTEL_COMPILER) && __INTEL_COMPILER < 2021
#error The Intel compiler version 19.00 cannot compile VCL version 2
#endif

/* Clang problem:
The Clang compiler treats the intrinsic vector types __m128, __m128i, and __m128d as identical.
See the bug report at https://bugs.llvm.org/show_bug.cgi?id=17164
Additional problem: The version number is not consistent across platforms. The Apple build has
different version numbers. We have to rely on __apple_build_version__ on the Mac platform:
http://llvm.org/bugs/show_bug.cgi?id=12643
We have to make switches here when - hopefully - the error some day has been fixed.
We need different version checks with and whithout __apple_build_version__
*/
#if (defined (__clang__) || defined(__apple_build_version__)) && !defined(__INTEL_COMPILER)
#define FIX_CLANG_VECTOR_ALIAS_AMBIGUITY
#endif

#if defined (__GNUC__) && __GNUC__ < 10 && !defined(__clang__)
// Gcc 9 and earlier donot have _mm256_zextsi128_si256 and similar functions for xero-extending vector registers
#define ZEXT_MISSING
#endif


#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif

// Constant for indicating don't care in permute and blend functions.
// V_DC is -256 in Vector class library version 1.xx
// V_DC can be any value less than -1 in Vector class library version 2.00
constexpr int V_DC = -256;



/*****************************************************************************
*
*    Helper functions that depend on instruction set, compiler, or platform
*
*****************************************************************************/

// Define interface to cpuid instruction.
// input:  functionnumber = leaf (eax), ecxleaf = subleaf(ecx)
// output: output[0] = eax, output[1] = ebx, output[2] = ecx, output[3] = edx
static inline void cpuid(int output[4], int functionnumber, int ecxleaf = 0) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    // Original x86/x86_64 implementation
    #if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
        int a, b, c, d;
        __asm("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(functionnumber), "c"(ecxleaf) : );
        output[0] = a;
        output[1] = b;
        output[2] = c;
        output[3] = d;
    #elif defined (_MSC_VER)
        __cpuidex(output, functionnumber, ecxleaf);
    #else
        __asm {
            mov eax, functionnumber
            mov ecx, ecxleaf
            cpuid;
            mov esi, output
            mov[esi], eax
            mov[esi + 4], ebx
            mov[esi + 8], ecx
            mov[esi + 12], edx
        }
    #endif
#elif defined(__aarch64__) || defined(__arm__)
    // ARM has no x86 CPUID leaves.
    (void)functionnumber; (void)ecxleaf;
    output[0] = output[1] = output[2] = output[3] = 0;
#else
    #error Unsupported platform
#endif
}


// Define popcount function. Gives sum of bits
#if INSTRSET >= 6   // SSE4.2
// The popcnt instruction is not officially part of the SSE4.2 instruction set,
// but available in all known processors with SSE4.2
static inline uint32_t vml_popcnt(uint32_t a) {
    return (uint32_t)_mm_popcnt_u32(a);  // Intel intrinsic. Supported by gcc and clang
}
#ifdef __x86_64__
static inline int64_t vml_popcnt(uint64_t a) {
    return _mm_popcnt_u64(a);            // Intel intrinsic.
}
#else   // 32 bit mode
static inline int64_t vml_popcnt(uint64_t a) {
    return _mm_popcnt_u32(uint32_t(a >> 32)) + _mm_popcnt_u32(uint32_t(a));
}
#endif
#else  // no SSE4.2
static inline uint32_t vml_popcnt(uint32_t a) {
    // popcnt instruction not available
    uint32_t b = a - ((a >> 1) & 0x55555555);
    uint32_t c = (b & 0x33333333) + ((b >> 2) & 0x33333333);
    uint32_t d = (c + (c >> 4)) & 0x0F0F0F0F;
    uint32_t e = d * 0x01010101;
    return   e >> 24;
}
static inline int32_t vml_popcnt(uint64_t a) {
    return (int32_t)(vml_popcnt(uint32_t(a >> 32)) + vml_popcnt(uint32_t(a)));
}
#endif

// Define bit-scan-forward function. Gives index to lowest set bit
#if (defined (__GNUC__) || defined(__clang__)) && !defined (_MSC_VER)
// _BitScanForward intrinsics are defined only under Windows and only when _MSC_VER is defined

// Use inline assembly for gcc and Clang
#if defined(__clang_major__) && __clang_major__ < 10
    // fix bug in Clang version 6. (not detected in version 8 and later)
    // Clang version 6 uses a k register as parameter a when inlined from horizontal_find_first
__attribute__((noinline))
#endif
#ifdef __x86_64__
static uint32_t bit_scan_forward(uint32_t a) {
    uint32_t r;
    __asm("bsfl %1, %0" : "=r"(r) : "r"(a) : );
    return r;
}
#else
static uint32_t bit_scan_forward(uint32_t a) {
    return __builtin_ctz(a);
}
#endif
static inline uint32_t bit_scan_forward(uint64_t a) {
    uint32_t lo = uint32_t(a);
    if (lo) return bit_scan_forward(lo);
    uint32_t hi = uint32_t(a >> 32);
    return bit_scan_forward(hi) + 32;
}
#else  // MS compatible compilers under Windows
static inline uint32_t bit_scan_forward(uint32_t a) {
    unsigned long r;
    _BitScanForward(&r, a);            // defined in intrin.h for MS and Intel compilers
    return r;
}
#ifdef __x86_64__
static inline uint32_t bit_scan_forward(uint64_t a) {
    unsigned long r;
    _BitScanForward64(&r, a);          // defined in intrin.h for MS and Intel compilers
    return (uint32_t)r;
}
#else
static inline uint32_t bit_scan_forward(uint64_t a) {
    uint32_t lo = uint32_t(a);
    if (lo) return bit_scan_forward(lo);
    uint32_t hi = uint32_t(a >> 32);
    return bit_scan_forward(hi) + 32;
}
#endif
#endif


// Define bit-scan-reverse function. Gives index to highest set bit = floor(log2(a))
#if (defined (__GNUC__) || defined(__clang__)) && !defined (_MSC_VER)
// _BitScanReverse intrinsics are defined only under Windows and only when _MSC_VER is defined

// Use inline assembly for gcc and Clang

static inline uint32_t bit_scan_reverse(uint32_t a) __attribute__((pure));
static inline uint32_t bit_scan_reverse(uint32_t a) {
    // As before, the input must be nonzero.
    return 31u - uint32_t(__builtin_clz(a));
}
#ifdef __x86_64__
static inline uint32_t bit_scan_reverse(uint64_t a) {
    return 63u - uint32_t(__builtin_clzll(a));
}
#else   // 32 bit mode
static inline uint32_t bit_scan_reverse(uint64_t a) {
    uint64_t ahi = a >> 32;
    if (ahi == 0) return bit_scan_reverse(uint32_t(a));
    else return bit_scan_reverse(uint32_t(ahi)) + 32;
}
#endif
#else  // MS compatible compilers under Windows
static inline uint32_t bit_scan_reverse(uint32_t a) {
    unsigned long r;
    _BitScanReverse(&r, a);            // defined in intrin.h for MS compatible compilers
    return r;
}
#ifdef __x86_64__
static inline uint32_t bit_scan_reverse(uint64_t a) {
    unsigned long r;
    _BitScanReverse64(&r, a);          // defined in intrin.h for MS compatible compilers
    return r;
}
#else   // 32 bit mode
static inline uint32_t bit_scan_reverse(uint64_t a) {
    uint64_t ahi = a >> 32;
    if (ahi == 0) return bit_scan_reverse(uint32_t(a));
    else return bit_scan_reverse(uint32_t(ahi)) + 32;
}
#endif
#endif 

// Same function, for compile-time constants
constexpr int bit_scan_reverse_const(uint64_t const n) {
    if (n == 0) return -1;
    uint64_t a = n, b = 0, j = 64, k = 0;
    do {
        j >>= 1;
        k = (uint64_t)1 << j;
        if (a >= k) {
            a >>= j;
            b += j;
        }
    } while (j > 0);
    return int(b);
}


/*****************************************************************************
*
*    Common templates
*
*****************************************************************************/

#ifdef VCL_NAMESPACE
#define NAMESPACEPREFIX VCL_NAMESPACE::
#else 
#define NAMESPACEPREFIX
#endif

template <int32_t  n> class Const_int_t {};                // represent compile-time signed integer constant
template <uint32_t n> class Const_uint_t {};               // represent compile-time unsigned integer constant
#define const_int(n)  (NAMESPACEPREFIX Const_int_t <n>())  // n must be compile-time integer constant
#define const_uint(n) (NAMESPACEPREFIX Const_uint_t<n>())  // n must be compile-time unsigned integer constant


// template for producing quiet NAN
template <class VTYPE>
static inline VTYPE nan_vec(uint32_t payload = 0x100) {
    if constexpr (VTYPE::elementtype() == 17) {  // double
        union {
            uint64_t q;
            double f;
        } ud;
        // n is left justified to avoid loss of NAN payload when converting to float
        ud.q = 0x7FF8000000000000 | uint64_t(payload) << 29;
        return VTYPE(ud.f);
    }
    if constexpr (VTYPE::elementtype() == 16) {  // float
        union {
            uint32_t i;
            float f;
        } uf;
        uf.i = 0x7FC00000 | (payload & 0x003FFFFF);
        return VTYPE(uf.f);
    }
    /*  // defined in vectorfp16.h
    if constexpr (VTYPE::elementtype() == 15) {  // _Float16
        union {
            uint16_t i;
            _Float16 f;  // error if _Float16 not defined
        } uf;
        uf.i = 0x7C00 | (payload & 0x03FF);
        return VTYPE(uf.f);
    } */
}


// Test if a parameter is a compile-time constant
/* Unfortunately, this works only for macro parameters, not for inline function parameters.
   I hope that some solution will appear in the future, but for now it appears to be
   impossible to check if a function parameter is a compile-time constant.
   This would be useful in operator / and in function pow:
   #if defined(__GNUC__) || defined (__clang__)
   #define is_constant(a) __builtin_constant_p(a)
   #else
   #define is_constant(a) false
   #endif
*/


/*****************************************************************************
*
*    Helper functions for permute and blend functions
*
******************************************************************************
Rules for constexpr functions:

> All variable declarations must include initialization

> Do not put variable declarations inside a for-clause, e.g. avoid: for (int i=0; ..
  Instead, you have to declare the loop counter before the for-loop.

> Do not make constexpr functions that return vector types. This requires type
  punning with a union, which is not allowed in constexpr functions under C++17.

*****************************************************************************/


#ifdef VCL_NAMESPACE
}
#endif


#include "instrset_helpers.h"

#endif // backend
#endif // INSTRSET_H
