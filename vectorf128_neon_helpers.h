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

// Experimental native FP/mask helpers. Copyright 2026. Apache-2.0.
// API/bit contracts follow vectorf128.h and instrset.h:
// (c) Copyright 2012-2023 Agner Fog. Apache License version 2.0 or later.
#ifndef VECTORF128_NEON_HELPERS_H
#define VECTORF128_NEON_HELPERS_H
#include "vectorf128_neon.h"
#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif
inline Vec4f reinterpret_f(Vec4f a) { return a; }
inline Vec2d reinterpret_d(Vec2d a) { return a; }
template<class T> inline Vec128b reinterpret_i(NeonInt<T> a) { return Vec128b(a); }
inline Vec128b reinterpret_i(Vec128b a) { return a; }
inline Vec4f reinterpret_f(Vec128b a) { return Vec4f(neon_detail::bit_cast<float32x4_t>(a.native())); }
inline Vec2d reinterpret_d(Vec128b a) { return Vec2d(neon_detail::bit_cast<float64x2_t>(a.native())); }
template<class U,int Tag> inline Vec128b reinterpret_i(NeonMask<U,Tag> a) {
    return Vec128b(neon_detail::bit_cast<uint8x16_t>(a.native()));
}
template<class U,int Tag> inline Vec4f reinterpret_f(NeonMask<U,Tag> a) { return Vec4f(neon_detail::bit_cast<float32x4_t>(a.native())); }
template<class U,int Tag> inline Vec2d reinterpret_d(NeonMask<U,Tag> a) { return Vec2d(neon_detail::bit_cast<float64x2_t>(a.native())); }
template<class T> inline typename NeonFloat<T>::mask_type sign_bit(NeonFloat<T> a) {
    using I=typename neon_detail::fp_traits<T>::I;
    using M=typename NeonFloat<T>::mask_type;
    return M(neon_detail::bit_cast<typename M::register_type>((NeonInt<I>(reinterpret_i(a))>>(sizeof(T)*8-1)).native()));
}
inline Vec4f sign_combine(Vec4f a,Vec4f b) { return a^(b&Vec4f(-0.f)); }
inline Vec2d sign_combine(Vec2d a,Vec2d b) { return a^(b&Vec2d(-0.0)); }
inline Vec4f infinite4f() { return reinterpret_f(Vec4ui(0x7f800000u)); }
inline Vec2d infinite2d() { return reinterpret_d(Vec2uq(0x7ff0000000000000ull)); }
inline Vec4f nan4f(uint32_t payload=0x10) { return nan_vec<Vec4f>(payload); }
inline Vec2d nan2d(uint32_t payload=0x10) { return nan_vec<Vec2d>(payload); }
inline Vec4i exponent(Vec4f a) { return Vec4i(Vec4ui(reinterpret_i(a))<<1>>24)-Vec4i(127); }
inline Vec2q exponent(Vec2d a) { return Vec2q(Vec2uq(reinterpret_i(a))<<1>>53)-Vec2q(1023); }
inline Vec4f fraction(Vec4f a) { return reinterpret_f((Vec4ui(reinterpret_i(a))&Vec4ui(0x007fffffu))|Vec4ui(0x3f800000u)); }
inline Vec2d fraction(Vec2d a) { return reinterpret_d((Vec2uq(reinterpret_i(a))&Vec2uq(0x000fffffffffffffull))|Vec2uq(0x3ff0000000000000ull)); }
inline Vec4f exp2(Vec4i n) { return reinterpret_f((min(max(n,Vec4i(-127)),Vec4i(128))+Vec4i(127))<<23); }
inline Vec2d exp2(Vec2q n) { return reinterpret_d((min(max(n,Vec2q(-1023)),Vec2q(1024))+Vec2q(1023))<<52); }
inline Vec4f mul_sub_x(Vec4f a,Vec4f b,Vec4f c) { return mul_sub(a,b,c); }
inline Vec2d mul_sub_x(Vec2d a,Vec2d b,Vec2d c) { return mul_sub(a,b,c); }

// SSE's invalid signed conversion value is the signed minimum. Convert in
// 64-bit lanes, then narrow only after applying the 32-bit range fixup.
inline int32x2_t neon_truncate_int32(float64x2_t a) {
    auto valid=vandq_u64(vcgeq_f64(a,vdupq_n_f64(-2147483648.0)),vcltq_f64(a,vdupq_n_f64(2147483648.0)));
    auto fixed=vbslq_s64(valid,vcvtq_s64_f64(a),vdupq_n_s64(INT32_MIN));
    return vmovn_s64(fixed);
}
inline Vec4i truncate_to_int32(Vec2d a) { return Vec4i(vcombine_s32(neon_truncate_int32(a.native()),vdup_n_s32(0))); }
inline Vec4i truncate_to_int32(Vec2d a,Vec2d b) { return Vec4i(vcombine_s32(neon_truncate_int32(a.native()),neon_truncate_int32(b.native()))); }
inline Vec4i round_to_int32(Vec2d a) { return Vec4i(vcombine_s32(neon_truncate_int32(vrndiq_f64(a.native())),vdup_n_s32(0))); }
inline Vec4i round_to_int32(Vec2d a,Vec2d b) { return Vec4i(vcombine_s32(neon_truncate_int32(vrndiq_f64(a.native())),neon_truncate_int32(vrndiq_f64(b.native())))); }

template<class V> inline V pow_template_i(V a,int n) {
    if(n==INT32_MIN) return nan_vec<V>();
    const bool reciprocal=n<0;
    uint32_t remaining=reciprocal?uint32_t(0)-uint32_t(n):uint32_t(n);
    V result(1);
    while(true) {
        if(remaining&1) result*=a;
        remaining>>=1;
        if(!remaining) return reciprocal?V(1)/result:result;
        a*=a;
    }
}
template<class V,int n> inline V pow_n(V a) {
    if constexpr(n==INT32_MIN) return nan_vec<V>();
    else if constexpr(n<0) return V(1)/pow_n<V,-n>(a);
    else if constexpr(n==0) return V(1);
    else if constexpr(n>=256) return pow_template_i(a,n);
    else {
        V result=a; bool started=false; uint32_t remaining=n;
        while(remaining) {
            if(remaining&1) { if(started) result*=a; else { result=a; started=true; } }
            remaining>>=1; if(remaining) a*=a;
        }
        return result;
    }
}
template<class N> Vec4f pow(Vec4f,N)=delete;
template<class N> Vec2d pow(Vec2d,N)=delete;
template<> inline Vec4f pow<int>(Vec4f a,int n) { return pow_template_i(a,n); }
template<> inline Vec4f pow<uint32_t>(Vec4f a,uint32_t n) { return pow_template_i(a,int32_t(n)); }
template<> inline Vec2d pow<int>(Vec2d a,int n) { return pow_template_i(a,n); }
template<> inline Vec2d pow<uint32_t>(Vec2d a,uint32_t n) { return pow_template_i(a,int32_t(n)); }
template<int n> inline Vec4f pow(Vec4f a,Const_int_t<n>) { return pow_n<Vec4f,n>(a); }
template<int n> inline Vec2d pow(Vec2d a,Const_int_t<n>) { return pow_n<Vec2d,n>(a); }
#ifdef VCL_NAMESPACE
#define pow_const(x,n) pow(x,VCL_NAMESPACE::Const_int_t<n>())
#else
#define pow_const(x,n) pow(x,Const_int_t<n>())
#endif

// FP32 approximate APIs: one Newton refinement for ordinary normal inputs.
// Boundary/subnormal inputs use correctly rounded division/sqrt fallback under
// the current FP environment. Signed zero, infinities and NaNs are fixed in bits
// so zero inputs do not execute division by zero or invalid refinement products.
inline Vec4f approx_recipr(Vec4f a) {
    const auto finite=is_finite(a),zero=a==Vec4f(0),nan=is_nan(a),inf=is_inf(a);
    const Vec4i e=exponent(a);
    const Vec4fb fast=Vec4fb((e>=Vec4i(-125))&(e<=Vec4i(125)));
    const Vec4f safe=select(fast,a,Vec4f(1));
    auto estimate=vrecpeq_f32(safe.native());
    Vec4f result(vmulq_f32(estimate,vrecpsq_f32(estimate,safe.native())));
    const auto fallback=finite&!zero&!fast;
    if(horizontal_or(fallback)) result=select(fallback,Vec4f(1)/select(fallback,a,Vec4f(1)),result);
    const Vec4ui bits(reinterpret_i(a)),sign=bits&Vec4ui(0x80000000u);
    result=select(zero,reinterpret_f(sign|Vec4ui(0x7f800000u)),result);
    result=select(inf,reinterpret_f(sign),result);
    return select(nan,reinterpret_f(bits|Vec4ui(0x00400000u)),result);
}
inline Vec4f approx_rsqrt(Vec4f a) {
    const auto zero=a==Vec4f(0),nan=is_nan(a),inf=is_inf(a);
    const auto positive=!sign_bit(a);
    const auto fast=is_finite(a)&positive&!is_zero_or_subnormal(a);
    const Vec4f safe=select(fast,a,Vec4f(1));
    auto estimate=vrsqrteq_f32(safe.native());
    Vec4f result(vmulq_f32(estimate,vrsqrtsq_f32(vmulq_f32(safe.native(),estimate),estimate)));
    const auto fallback=is_subnormal(a)&positive;
    if(horizontal_or(fallback)) result=select(fallback,Vec4f(1)/sqrt(select(fallback,a,Vec4f(1))),result);
    const Vec4ui bits(reinterpret_i(a)),sign=bits&Vec4ui(0x80000000u);
    result=select(inf&positive,Vec4f(0),result);
    result=select(!positive&!zero,nan4f(0),result);
    result=select(zero,reinterpret_f(sign|Vec4ui(0x7f800000u)),result);
    return select(nan,reinterpret_f(bits|Vec4ui(0x00400000u)),result);
}
#ifdef VCL_NAMESPACE
}
#endif
#endif
