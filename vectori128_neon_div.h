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

// Experimental native Neon integer division. Copyright 2026. Apache-2.0.
// Parameter formula and VCL-facing API adapted from vectori128.h:
// (c) Copyright 2012-2023 Agner Fog. Apache License version 2.0 or later.
// Granlund/Montgomery invariant division; all per-lane arithmetic is native
// vector arithmetic. Scalar parameter setup uses unsigned/wider intermediates.
#ifndef VECTORI128_NEON_DIV_H
#define VECTORI128_NEON_DIV_H
#include "vectori128_neon.h"
#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif
namespace neon_detail {
inline Vec8s mul_high(Vec8s a,Vec8s b) {
    return Vec8s(vcombine_s16(vshrn_n_s32(vmull_s16(vget_low_s16(a.native()),vget_low_s16(b.native())),16),
                            vshrn_n_s32(vmull_high_s16(a.native(),b.native()),16)));
}
inline Vec8us mul_high(Vec8us a,Vec8us b) {
    return Vec8us(vcombine_u16(vshrn_n_u32(vmull_u16(vget_low_u16(a.native()),vget_low_u16(b.native())),16),
                             vshrn_n_u32(vmull_high_u16(a.native(),b.native()),16)));
}
inline Vec4i mul_high(Vec4i a,Vec4i b) {
    return Vec4i(vcombine_s32(vshrn_n_s64(vmull_s32(vget_low_s32(a.native()),vget_low_s32(b.native())),32),
                            vshrn_n_s64(vmull_high_s32(a.native(),b.native()),32)));
}
inline Vec4ui mul_high(Vec4ui a,Vec4ui b) {
    return Vec4ui(vcombine_u32(vshrn_n_u64(vmull_u32(vget_low_u32(a.native()),vget_low_u32(b.native())),32),
                             vshrn_n_u64(vmull_high_u32(a.native(),b.native()),32)));
}
template<class T> struct signed_division_parameters { T multiplier; int shift; T sign; };
template<class T> constexpr signed_division_parameters<T> signed_parameters(T d) {
    static_assert(std::is_signed<T>::value && (sizeof(T)==2 || sizeof(T)==4));
    using U=typename std::make_unsigned<T>::type;
    constexpr int width=sizeof(T)*8;
    // Zero is outside the VCL divisor domain; fail instead of invoking scalar UB.
    if(d==0) __builtin_trap();
    const U magnitude=d<0?U(U(0)-U(d)):U(d);
    const int shift=magnitude>1?bit_scan_reverse_const(uint64_t(magnitude)-1):0;
    const U multiplier=magnitude>1?U((uint64_t(1)<<(width+shift))/magnitude-((uint64_t(1)<<width)-1)):U(1);
    return {T(multiplier),shift,T(d<0?-1:0)};
}
template<class T> struct unsigned_division_parameters { T multiplier; int shift1; int shift2; };
template<class T> constexpr unsigned_division_parameters<T> unsigned_parameters(T d) {
    static_assert(std::is_unsigned<T>::value && (sizeof(T)==2 || sizeof(T)==4));
    constexpr int width=sizeof(T)*8;
    if(d==0) __builtin_trap();
    if(d==1) return {1,0,0};
    if(d==2) return {1,1,0};
    const int length=bit_scan_reverse_const(uint64_t(d)-1)+1;
    const T power=T(uint64_t(1)<<length); // 2^width intentionally wraps to zero.
    const T multiplier=T(1+((uint64_t(T(power-d))<<width)/d));
    return {multiplier,1,length-1};
}
}

// Register getters expose Neon types rather than SSE aliases. Shift registers
// retain the original low-count/zero-high layout for reconstruction and reuse.
template<class T,bool Signed=std::is_signed<T>::value> class NeonDivisor;
template<class T> class NeonDivisor<T,true> {
    using tr=neon_detail::traits<T>;
protected:
    typename tr::reg multiplier;
    uint32x4_t shift1;
    typename tr::reg sign;
public:
    NeonDivisor()=default;
    NeonDivisor(T d) { set(d); }
    NeonDivisor(T m,int s1,int sgn):multiplier(tr::dup(m)),shift1(vsetq_lane_u32(uint32_t(s1),vdupq_n_u32(0),0)),sign(tr::dup(T(sgn))) {}
    void set(T d) {
        auto p=neon_detail::signed_parameters(d);
        *this=NeonDivisor(p.multiplier,p.shift,p.sign);
    }
    typename tr::reg getm() const { return multiplier; }
    uint32x4_t gets1() const { return shift1; }
    typename tr::reg getsign() const { return sign; }
};
template<class T> class NeonDivisor<T,false> {
    using tr=neon_detail::traits<T>;
protected:
    typename tr::reg multiplier;
    uint32x4_t shift1;
    uint32x4_t shift2;
public:
    NeonDivisor()=default;
    NeonDivisor(T d) { set(d); }
    NeonDivisor(T m,int s1,int s2):multiplier(tr::dup(m)),shift1(vsetq_lane_u32(uint32_t(s1),vdupq_n_u32(0),0)),shift2(vsetq_lane_u32(uint32_t(s2),vdupq_n_u32(0),0)) {}
    void set(T d) {
        auto p=neon_detail::unsigned_parameters(d);
        *this=NeonDivisor(p.multiplier,p.shift1,p.shift2);
    }
    typename tr::reg getm() const { return multiplier; }
    uint32x4_t gets1() const { return shift1; }
    uint32x4_t gets2() const { return shift2; }
};
using Divisor_i=NeonDivisor<int32_t>;
using Divisor_ui=NeonDivisor<uint32_t>;
using Divisor_s=NeonDivisor<int16_t>;
using Divisor_us=NeonDivisor<uint16_t>;

namespace neon_detail {
template<class T> inline NeonInt<T> divide(NeonInt<T> a,NeonDivisor<T> d) {
    using V=NeonInt<T>;
    const V product=mul_high(a,V(d.getm()));
    const int shift1=int(vgetq_lane_u32(d.gets1(),0));
    if constexpr(std::is_signed<T>::value) {
        const V sign(d.getsign());
        return (((product+a)>>shift1)-((a>>(sizeof(T)*8-1))-sign))^sign;
    }
    else {
        const int shift2=int(vgetq_lane_u32(d.gets2(),0));
        return (((a-product)>>shift1)+product)>>shift2;
    }
}
}
inline Vec4i operator/(Vec4i a,Divisor_i d) { return neon_detail::divide(a,d); }
inline Vec4ui operator/(Vec4ui a,Divisor_ui d) { return neon_detail::divide(a,d); }
inline Vec8s operator/(Vec8s a,Divisor_s d) { return neon_detail::divide(a,d); }
inline Vec8us operator/(Vec8us a,Divisor_us d) { return neon_detail::divide(a,d); }
inline Vec16c operator/(Vec16c a,Divisor_s d) { return compress(extend_low(a)/d,extend_high(a)/d); }
inline Vec16uc operator/(Vec16uc a,Divisor_us d) { return compress(extend_low(a)/d,extend_high(a)/d); }
inline Vec4i& operator/=(Vec4i& a,Divisor_i d) { return a=a/d; }
inline Vec4ui& operator/=(Vec4ui& a,Divisor_ui d) { return a=a/d; }
inline Vec8s& operator/=(Vec8s& a,Divisor_s d) { return a=a/d; }
inline Vec8us& operator/=(Vec8us& a,Divisor_us d) { return a=a/d; }
inline Vec16c& operator/=(Vec16c& a,Divisor_s d) { return a=a/d; }
inline Vec16uc& operator/=(Vec16uc& a,Divisor_us d) { return a=a/d; }

namespace neon_detail {
template<int32_t D,class T> inline NeonInt<T> constant_signed(NeonInt<T> a) {
    static_assert(std::is_signed<T>::value && (sizeof(T)==2 || sizeof(T)==4));
    constexpr T d=T(D); // Match the VCL 16-bit constant divisor's truncation.
    static_assert(d!=0,"Integer division by zero");
    if constexpr(d==0) return a; // Keep invalid constants out of parameter evaluation.
    else if constexpr(d==1) return a;
    else if constexpr(d==-1) return -a;
    else {
        using U=typename std::make_unsigned<T>::type;
        using V=NeonInt<T>;
        constexpr U magnitude=d<0?U(U(0)-U(d)):U(d);
        if constexpr((magnitude&(magnitude-1))==0) {
            constexpr int shift=bit_scan_reverse_const(magnitude);
            const V bias=(a>>(sizeof(T)*8-1))&V(T(magnitude-1));
            const V q=(a+bias)>>shift;
            if constexpr(d<0) return -q;
            else return q;
        }
        else {
            constexpr auto p=signed_parameters(d);
            return a/NeonDivisor<T>(p.multiplier,p.shift,p.sign);
        }
    }
}
template<uint32_t D,class T> inline NeonInt<T> constant_unsigned(NeonInt<T> a) {
    static_assert(std::is_unsigned<T>::value && (sizeof(T)==2 || sizeof(T)==4));
    constexpr T d=T(D);
    static_assert(d!=0,"Integer division by zero");
    if constexpr(d==0 || d==1) return a;
    else if constexpr((d&(d-1))==0) return a>>bit_scan_reverse_const(d);
    else {
        constexpr auto p=unsigned_parameters(d);
        return a/NeonDivisor<T>(p.multiplier,p.shift1,p.shift2);
    }
}
}
template<int32_t d> inline Vec4i divide_by_i(Vec4i a) { return neon_detail::constant_signed<d>(a); }
template<int32_t d> inline Vec8s divide_by_i(Vec8s a) { return neon_detail::constant_signed<d>(a); }
template<uint32_t d> inline Vec4ui divide_by_ui(Vec4ui a) { return neon_detail::constant_unsigned<d>(a); }
template<uint32_t d> inline Vec8us divide_by_ui(Vec8us a) { return neon_detail::constant_unsigned<d>(a); }

template<int32_t d> inline Vec4i operator/(Vec4i a,Const_int_t<d>) { return divide_by_i<d>(a); }
template<uint32_t d> inline Vec4i operator/(Vec4i a,Const_uint_t<d>) {
    static_assert(d<0x80000000u,"Dividing signed by overflowing unsigned"); return divide_by_i<int32_t(d)>(a);
}
template<uint32_t d> inline Vec4ui operator/(Vec4ui a,Const_uint_t<d>) { return divide_by_ui<d>(a); }
template<int32_t d> inline Vec4ui operator/(Vec4ui a,Const_int_t<d>) {
    static_assert(d>=0,"Dividing unsigned integer by negative value is ambiguous"); return divide_by_ui<uint32_t(d)>(a);
}
template<int32_t d> inline Vec8s operator/(Vec8s a,Const_int_t<d>) { return divide_by_i<d>(a); }
template<uint32_t d> inline Vec8s operator/(Vec8s a,Const_uint_t<d>) {
    static_assert(d<0x8000u,"Dividing signed by overflowing unsigned"); return divide_by_i<int32_t(d)>(a);
}
template<uint32_t d> inline Vec8us operator/(Vec8us a,Const_uint_t<d>) { return divide_by_ui<d>(a); }
template<int32_t d> inline Vec8us operator/(Vec8us a,Const_int_t<d>) {
    static_assert(d>=0,"Dividing unsigned integer by negative value is ambiguous"); return divide_by_ui<uint32_t(d)>(a);
}
template<int32_t d> inline Vec16c operator/(Vec16c a,Const_int_t<d>) { return compress(extend_low(a)/Const_int_t<d>(),extend_high(a)/Const_int_t<d>()); }
template<uint32_t d> inline Vec16c operator/(Vec16c a,Const_uint_t<d>) {
    static_assert(uint8_t(d)<0x80u,"Dividing signed integer by overflowing unsigned"); return a/Const_int_t<int32_t(d)>();
}
template<uint32_t d> inline Vec16uc operator/(Vec16uc a,Const_uint_t<d>) { return compress(extend_low(a)/Const_uint_t<d>(),extend_high(a)/Const_uint_t<d>()); }
template<int32_t d> inline Vec16uc operator/(Vec16uc a,Const_int_t<d>) {
    static_assert(int8_t(d)>=0,"Dividing unsigned integer by negative is ambiguous"); return a/Const_uint_t<uint32_t(d)>();
}
// Same six vector types and two constant wrappers as the original overloads.
template<class T,int32_t d,typename std::enable_if<(sizeof(T)<=4),int>::type=0>
inline NeonInt<T>& operator/=(NeonInt<T>& a,Const_int_t<d> b) { return a=a/b; }
template<class T,uint32_t d,typename std::enable_if<(sizeof(T)<=4),int>::type=0>
inline NeonInt<T>& operator/=(NeonInt<T>& a,Const_uint_t<d> b) { return a=a/b; }
#ifdef VCL_NAMESPACE
}
#endif
#endif
