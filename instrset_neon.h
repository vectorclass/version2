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

// Experimental AArch64 helpers. Copyright 2026. Apache-2.0.
#ifndef INSTRSET_NEON_H
#define INSTRSET_NEON_H
#include "vcl_config.h"
#if VCL_BACKEND != VCL_BACKEND_NEON
#error Native Neon headers require VCL_BACKEND_NEON
#endif
#include <arm_neon.h>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <limits>
#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif
// The constant wrappers and macro spelling are shared with the original VCL API.
template<int32_t n> class Const_int_t {};
template<uint32_t n> class Const_uint_t {};
#ifdef VCL_NAMESPACE
#define const_int(n) (VCL_NAMESPACE::Const_int_t<n>())
#define const_uint(n) (VCL_NAMESPACE::Const_uint_t<n>())
#else
#define const_int(n) (Const_int_t<n>())
#define const_uint(n) (Const_uint_t<n>())
#endif
constexpr int bit_scan_reverse_const(uint64_t n) {
    int result=-1;
    while(n) { ++result; n>>=1; }
    return result;
}
constexpr int V_DC = -256; // Compile-time permutation/blend don't-care lane.
template<class V> inline V nan_vec(uint32_t payload=0x100) {
    static_assert(V::elementtype()==16 || V::elementtype()==17,"FP32/FP64 vector required");
    if constexpr(V::elementtype()==16) {
        const uint32_t bits=0x7fc00000u|(payload&0x003fffffu); float x; std::memcpy(&x,&bits,4); return V(x);
    }
    else {
        const uint64_t bits=0x7ff8000000000000ull|(uint64_t(payload)<<29); double x; std::memcpy(&x,&bits,8); return V(x);
    }
}
constexpr bool has_neon() { return true; }
int instrset_detect(void);
bool hasFMA3(void); bool hasFMA4(void); bool hasXOP(void);
bool hasAVX512ER(void); bool hasAVX512VBMI(void); bool hasAVX512VBMI2(void);
bool hasF16C(void); bool hasAVX512FP16(void);
inline void cpuid(int output[4],int leaf,int subleaf=0) {
    (void)leaf; (void)subleaf;
    output[0]=output[1]=output[2]=output[3]=0;
}
inline uint32_t bit_scan_forward(uint32_t x) { return uint32_t(__builtin_ctz(x)); }
inline uint32_t bit_scan_forward(uint64_t x) { return uint32_t(__builtin_ctzll(x)); }
inline uint32_t bit_scan_reverse(uint32_t x) { return 31u-uint32_t(__builtin_clz(x)); }
inline uint32_t bit_scan_reverse(uint64_t x) { return 63u-uint32_t(__builtin_clzll(x)); }
inline uint32_t vml_popcnt(uint32_t x) { return uint32_t(__builtin_popcount(x)); }
inline int64_t vml_popcnt(uint64_t x) { return __builtin_popcountll(x); }
// MXCSR-shaped adapter: rounding and unified FZ/DAZ only. Exception flags and
// masks are unsupported and read as zero, matching the compatibility backend.
inline uint32_t get_control_word() {
    uint64_t fpcr; __asm__ volatile("mrs %0, fpcr" : "=r"(fpcr));
    unsigned rm=unsigned((fpcr>>22)&3);
    rm=((rm&1)<<1)|((rm&2)>>1); // ARM: +inf,-inf; SSE: -inf,+inf
    return (rm<<13) | ((fpcr&(1ull<<24)) ? 0x8040u : 0u);
}
inline void set_control_word(uint32_t value) {
    uint64_t fpcr; __asm__ volatile("mrs %0, fpcr" : "=r"(fpcr));
    unsigned rm=(value>>13)&3; rm=((rm&1)<<1)|((rm&2)>>1);
    fpcr=(fpcr&~((3ull<<22)|(1ull<<24))) | (uint64_t(rm)<<22) | ((value&0x8040u)?1ull<<24:0);
    __asm__ volatile("msr fpcr, %0\n\tisb" :: "r"(fpcr) : "memory");
}
inline void no_subnormals() { set_control_word(get_control_word()|0x8040u); }
inline void reset_control_word() { set_control_word(0x1f80u); }
#ifdef VCL_NAMESPACE
}
#endif
#include "instrset_helpers.h"
#endif
