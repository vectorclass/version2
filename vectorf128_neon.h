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

// Experimental native FP32/FP64 slice. Copyright 2026. Apache-2.0.
#ifndef VECTORF128_NEON_H
#define VECTORF128_NEON_H
#include "vectori128_neon.h"
#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif
namespace neon_detail {
template<class T> struct fp_traits;
template<> struct fp_traits<float> {
    using reg=float32x4_t; using U=uint32_t; using I=int32_t;
    static constexpr int lanes=4,code=16;
    static reg dup(float x) { return vdupq_n_f32(x); }
    static reg load(float const* p) { return vld1q_f32(p); }
    static void store(float* p,reg x) { vst1q_f32(p,x); }
    static reg add(reg a,reg b) { return vaddq_f32(a,b); }
    static reg sub(reg a,reg b) { return vsubq_f32(a,b); }
    static reg mul(reg a,reg b) { return vmulq_f32(a,b); }
    static reg div(reg a,reg b) { return vdivq_f32(a,b); }
    static reg sqrt(reg a) { return vsqrtq_f32(a); }
    static reg abs(reg a) { return vabsq_f32(a); }
    static reg round(reg a) { return vrndnq_f32(a); }
    static reg truncate(reg a) { return vrndq_f32(a); }
    static reg floor(reg a) { return vrndmq_f32(a); }
    static reg ceil(reg a) { return vrndpq_f32(a); }
    static reg neg(reg a) { return vnegq_f32(a); }
    static uint32x4_t eq(reg a,reg b) { return vceqq_f32(a,b); }
    static uint32x4_t gt(reg a,reg b) { return vcgtq_f32(a,b); }
    static uint32x4_t ge(reg a,reg b) { return vcgeq_f32(a,b); }
    static reg select(uint32x4_t m,reg a,reg b) { return vbslq_f32(m,a,b); }
    static reg fma(reg a,reg b,reg c) { return vfmaq_f32(c,a,b); }
    static reg fms(reg a,reg b,reg c) { return vfmsq_f32(c,a,b); }
    static float sum(reg a) { return vaddvq_f32(a); }
};
template<> struct fp_traits<double> {
    using reg=float64x2_t; using U=uint64_t; using I=int64_t;
    static constexpr int lanes=2,code=17;
    static reg dup(double x) { return vdupq_n_f64(x); }
    static reg load(double const* p) { return vld1q_f64(p); }
    static void store(double* p,reg x) { vst1q_f64(p,x); }
    static reg add(reg a,reg b) { return vaddq_f64(a,b); }
    static reg sub(reg a,reg b) { return vsubq_f64(a,b); }
    static reg mul(reg a,reg b) { return vmulq_f64(a,b); }
    static reg div(reg a,reg b) { return vdivq_f64(a,b); }
    static reg sqrt(reg a) { return vsqrtq_f64(a); }
    static reg abs(reg a) { return vabsq_f64(a); }
    static reg round(reg a) { return vrndnq_f64(a); }
    static reg truncate(reg a) { return vrndq_f64(a); }
    static reg floor(reg a) { return vrndmq_f64(a); }
    static reg ceil(reg a) { return vrndpq_f64(a); }
    static reg neg(reg a) { return vnegq_f64(a); }
    static uint64x2_t eq(reg a,reg b) { return vceqq_f64(a,b); }
    static uint64x2_t gt(reg a,reg b) { return vcgtq_f64(a,b); }
    static uint64x2_t ge(reg a,reg b) { return vcgeq_f64(a,b); }
    static reg select(uint64x2_t m,reg a,reg b) { return vbslq_f64(m,a,b); }
    static reg fma(reg a,reg b,reg c) { return vfmaq_f64(c,a,b); }
    static reg fms(reg a,reg b,reg c) { return vfmsq_f64(c,a,b); }
    static double sum(reg a) { return vaddvq_f64(a); }
};
}
using Vec4fb=NeonMask<uint32_t,1>; using Vec2db=NeonMask<uint64_t,1>;
namespace neon_detail {
// Keep partial-access control flow out of full-vector callers. Clang's
// register-preserving convention also limits spills around these cold calls.
#if defined(__clang__)
#if __has_attribute(preserve_most)
#define VCL_NEON_PARTIAL_ATTR __attribute__((noinline,cold,preserve_most))
#else
#define VCL_NEON_PARTIAL_ATTR __attribute__((noinline,cold))
#endif
#elif defined(__GNUC__)
#define VCL_NEON_PARTIAL_ATTR __attribute__((noinline,cold))
#elif defined(_MSC_VER)
#define VCL_NEON_PARTIAL_ATTR __declspec(noinline)
#else
#define VCL_NEON_PARTIAL_ATTR
#endif
// The public methods validate/clamp n to [0,4] before calling these helpers.
VCL_NEON_PARTIAL_ATTR inline float32x4_t partial_load_f32(int n,float const* p) {
    auto data=vdupq_n_f32(0);
    if(n>0) data=vld1q_lane_f32(p,data,0);
    if(n>1) data=vld1q_lane_f32(p+1,data,1);
    if(n>2) data=vld1q_lane_f32(p+2,data,2);
    if(n>3) data=vld1q_lane_f32(p+3,data,3);
    return data;
}
VCL_NEON_PARTIAL_ATTR inline void partial_store_f32(int n,float* p,float32x4_t data) {
    if(n>0) vst1q_lane_f32(p,data,0);
    if(n>1) vst1q_lane_f32(p+1,data,1);
    if(n>2) vst1q_lane_f32(p+2,data,2);
    if(n>3) vst1q_lane_f32(p+3,data,3);
}
// Grouped helpers accept n in [0,16] and never touch inactive addresses.
// Use an output reference and preserve SIMD registers around cold calls to
// avoid imposing aggregate-return register pressure on full-vector callers.
#if MAX_VECTOR_SIZE >= 512
#if defined(__clang__)
#if __has_attribute(preserve_all)
#define VCL_NEON_GROUPED_PARTIAL_ATTR __attribute__((noinline,cold,preserve_all))
#endif
#endif
#ifndef VCL_NEON_GROUPED_PARTIAL_ATTR
#define VCL_NEON_GROUPED_PARTIAL_ATTR VCL_NEON_PARTIAL_ATTR
#endif
VCL_NEON_GROUPED_PARTIAL_ATTR inline void partial_load_f32x4(int n,float const* p,float32x4x4_t& result) {
    const auto zero=vdupq_n_f32(0);
    result={{zero,zero,zero,zero}};
    const auto load=[](int count,float const* source) {
        auto data=vdupq_n_f32(0);
        if(count>=4) return vld1q_f32(source);
        if(count>0) data=vld1q_lane_f32(source,data,0);
        if(count>1) data=vld1q_lane_f32(source+1,data,1);
        if(count>2) data=vld1q_lane_f32(source+2,data,2);
        return data;
    };
    result.val[0]=load(n,p);
    if(n>4) result.val[1]=load(n-4,p+4);
    if(n>8) result.val[2]=load(n-8,p+8);
    if(n>12) result.val[3]=load(n-12,p+12);
}
VCL_NEON_GROUPED_PARTIAL_ATTR inline void partial_store_f32x4(int n,float* p,float32x4x4_t data) {
    const auto store=[](int count,float* target,float32x4_t value) {
        if(count>=4) { vst1q_f32(target,value);return; }
        if(count>0) vst1q_lane_f32(target,value,0);
        if(count>1) vst1q_lane_f32(target+1,value,1);
        if(count>2) vst1q_lane_f32(target+2,value,2);
    };
    store(n,p,data.val[0]);
    if(n>4) store(n-4,p+4,data.val[1]);
    if(n>8) store(n-8,p+8,data.val[2]);
    if(n>12) store(n-12,p+12,data.val[3]);
}
#undef VCL_NEON_GROUPED_PARTIAL_ATTR
#endif
#undef VCL_NEON_PARTIAL_ATTR
}
template<class T> class NeonFloat {
    using tr=neon_detail::fp_traits<T>;
    typename tr::reg value;
public:
    using register_type=typename tr::reg;
    using registertype=register_type;
    using mask_type=NeonMask<typename tr::U,1>;
    NeonFloat()=default;
    NeonFloat(T x):value(tr::dup(x)) {}
    explicit NeonFloat(register_type x):value(x) {}
    NeonFloat& operator=(register_type x) { value=x; return *this; }
    explicit NeonFloat(mask_type x):value(neon_detail::bit_cast<register_type>(x.native())) {}
    template<class... A,typename std::enable_if<sizeof...(A)==tr::lanes,int>::type=0>
    NeonFloat(A... x) { T data[]={T(x)...}; value=tr::load(data); }
    register_type native() const { return value; }
    static constexpr int size() { return tr::lanes; }
    static constexpr int elementtype() { return tr::code; }
    NeonFloat& load(T const* p) { value=tr::load(static_cast<T const*>(p)); return *this; }
    typename std::conditional<sizeof(T)==8,const NeonFloat,NeonFloat&>::type load_a(T const* p) { load(p); return *this; }
    void store(T* p) const { tr::store(static_cast<T*>(p),value); }
    void store_a(T* p) const { store(p); }
    void store_nt(T* p) const { store(p); }
    NeonFloat& load_partial(int n,T const* p) {
        // Preserve the native count policies; inactive lanes never access memory.
        if constexpr(sizeof(T)==4) {
            if(unsigned(n)>tr::lanes) return *this;
            value=neon_detail::partial_load_f32(n,p);
        } else {
            if(n==1) value=vcombine_f64(vld1_f64(p),vdup_n_f64(0));
            else if(n==2) value=tr::load(p);
            else value=vdupq_n_f64(0);
        }
        return *this;
    }
    void store_partial(int n,T* p) const {
        if constexpr(sizeof(T)==4) {
            const int count=unsigned(n)>tr::lanes?tr::lanes:n;
            if(count) neon_detail::partial_store_f32(count,p,value);
        } else {
            if(n==1) vst1q_lane_f64(p,value,0);
            else if(n>1) tr::store(p,value);
        }
    }
    NeonFloat& cutoff(int n) { if(unsigned(n)>=tr::lanes) return *this; T data[tr::lanes]; store(data); for(int i=n;i<tr::lanes;++i) data[i]=0; return load(data); }
    T operator[](int i) const { T data[tr::lanes]; store(data); return data[unsigned(i)&(tr::lanes-1)]; }
    T extract(int i) const { return (*this)[i]; }
    const NeonFloat insert(int i,T x) { T data[tr::lanes]; store(data); data[unsigned(i)&(tr::lanes-1)]=x; return load(data); }
    friend NeonFloat operator+(NeonFloat a,NeonFloat b) { return NeonFloat(tr::add(a.value,b.value)); }
    friend NeonFloat operator-(NeonFloat a,NeonFloat b) { return NeonFloat(tr::sub(a.value,b.value)); }
    friend NeonFloat operator*(NeonFloat a,NeonFloat b) { return NeonFloat(tr::mul(a.value,b.value)); }
    friend NeonFloat operator/(NeonFloat a,NeonFloat b) { return NeonFloat(tr::div(a.value,b.value)); }
    friend NeonFloat operator-(NeonFloat a) { return NeonFloat(tr::neg(a.value)); }
    friend mask_type operator!(NeonFloat a) { return a==NeonFloat(T(0)); }
    friend mask_type operator==(NeonFloat a,NeonFloat b) { return mask_type(tr::eq(a.value,b.value)); }
    friend mask_type operator!=(NeonFloat a,NeonFloat b) { return ~(a==b); }
    friend mask_type operator>(NeonFloat a,NeonFloat b) { return mask_type(tr::gt(a.value,b.value)); }
    friend mask_type operator<(NeonFloat a,NeonFloat b) { return b>a; }
    friend mask_type operator>=(NeonFloat a,NeonFloat b) { return mask_type(tr::ge(a.value,b.value)); }
    friend mask_type operator<=(NeonFloat a,NeonFloat b) { return b>=a; }
    friend NeonFloat select(mask_type m,NeonFloat a,NeonFloat b) { return NeonFloat(tr::select(m.native(),a.value,b.value)); }
    friend NeonFloat operator&(NeonFloat a,NeonFloat b) {
        auto x=neon_detail::bit_cast<uint8x16_t>(a.value),y=neon_detail::bit_cast<uint8x16_t>(b.value);
        return NeonFloat(neon_detail::bit_cast<register_type>(vandq_u8(x,y)));
    }
    friend NeonFloat operator|(NeonFloat a,NeonFloat b) {
        auto x=neon_detail::bit_cast<uint8x16_t>(a.value),y=neon_detail::bit_cast<uint8x16_t>(b.value);
        return NeonFloat(neon_detail::bit_cast<register_type>(vorrq_u8(x,y)));
    }
    friend NeonFloat operator^(NeonFloat a,NeonFloat b) {
        auto x=neon_detail::bit_cast<uint8x16_t>(a.value),y=neon_detail::bit_cast<uint8x16_t>(b.value);
        return NeonFloat(neon_detail::bit_cast<register_type>(veorq_u8(x,y)));
    }
    friend NeonFloat operator&(NeonFloat a,mask_type m) { return select(m,a,NeonFloat(T(0))); }
    friend NeonFloat operator&(mask_type m,NeonFloat a) { return a&m; }
    NeonFloat& operator&=(NeonFloat b) { return *this=*this&b; }
    NeonFloat& operator|=(NeonFloat b) { return *this=*this|b; }
    NeonFloat& operator^=(NeonFloat b) { return *this=*this^b; }
    NeonFloat& operator++() { return *this+=NeonFloat(T(1)); }
    NeonFloat operator++(int) { auto old=*this; ++*this; return old; }
    NeonFloat& operator--() { return *this-=NeonFloat(T(1)); }
    NeonFloat operator--(int) { auto old=*this; --*this; return old; }
    NeonFloat& operator+=(NeonFloat b) { return *this=*this+b; }
    NeonFloat& operator-=(NeonFloat b) { return *this=*this-b; }
    NeonFloat& operator*=(NeonFloat b) { return *this=*this*b; }
    NeonFloat& operator/=(NeonFloat b) { return *this=*this/b; }
};
using Vec4f=NeonFloat<float>; using Vec2d=NeonFloat<double>;
// Make the hidden friends visible when both choices are scalar broadcasts.
inline Vec4f select(Vec4fb m,Vec4f a,Vec4f b);
inline Vec2d select(Vec2db m,Vec2d a,Vec2d b);
inline Vec4f max(Vec4f a,Vec4f b) { return select(a>b,a,b); }
inline Vec4f min(Vec4f a,Vec4f b) { return select(a<b,a,b); }
inline Vec2d max(Vec2d a,Vec2d b) { return select(a>b,a,b); }
inline Vec2d min(Vec2d a,Vec2d b) { return select(a<b,a,b); }
template<class T> inline NeonFloat<T> sqrt(NeonFloat<T> a) { return NeonFloat<T>(neon_detail::fp_traits<T>::sqrt(a.native())); }
template<class T> inline NeonFloat<T> abs(NeonFloat<T> a) { return NeonFloat<T>(neon_detail::fp_traits<T>::abs(a.native())); }
template<class T> inline NeonFloat<T> round(NeonFloat<T> a) { return NeonFloat<T>(neon_detail::fp_traits<T>::round(a.native())); }
template<class T> inline NeonFloat<T> truncate(NeonFloat<T> a) { return NeonFloat<T>(neon_detail::fp_traits<T>::truncate(a.native())); }
template<class T> inline NeonFloat<T> floor(NeonFloat<T> a) { return NeonFloat<T>(neon_detail::fp_traits<T>::floor(a.native())); }
template<class T> inline NeonFloat<T> ceil(NeonFloat<T> a) { return NeonFloat<T>(neon_detail::fp_traits<T>::ceil(a.native())); }
template<class T> inline T horizontal_add(NeonFloat<T> a) {
    // VCL SSE reduction order is (a0+a2)+(a1+a3), rather than Neon vaddvq's order.
    if constexpr(sizeof(T)==4) { auto x=a.native(); auto pairs=vadd_f32(vget_low_f32(x),vget_high_f32(x)); return vaddv_f32(pairs); }
    else return vaddvq_f64(a.native());
}
inline Vec4f mul_add(Vec4f a,Vec4f b,Vec4f c) { return Vec4f(neon_detail::fp_traits<float>::fma(a.native(),b.native(),c.native())); }
inline Vec4f mul_sub(Vec4f a,Vec4f b,Vec4f c) { return mul_add(a,b,-c); }
inline Vec4f nmul_add(Vec4f a,Vec4f b,Vec4f c) { return Vec4f(neon_detail::fp_traits<float>::fms(a.native(),b.native(),c.native())); }
inline Vec2d mul_add(Vec2d a,Vec2d b,Vec2d c) { return Vec2d(neon_detail::fp_traits<double>::fma(a.native(),b.native(),c.native())); }
inline Vec2d mul_sub(Vec2d a,Vec2d b,Vec2d c) { return mul_add(a,b,-c); }
inline Vec2d nmul_add(Vec2d a,Vec2d b,Vec2d c) { return Vec2d(neon_detail::fp_traits<double>::fms(a.native(),b.native(),c.native())); }
template<class T> inline NeonFloat<T> square(NeonFloat<T> a) { return a*a; }
template<class T> inline Vec128b reinterpret_i(NeonFloat<T> a) {
    return Vec128b(neon_detail::bit_cast<uint8x16_t>(a.native()));
}
template<class T> inline Vec4f reinterpret_f(NeonInt<T> a) { return Vec4f(neon_detail::bit_cast<float32x4_t>(a.native())); }
template<class T> inline Vec2d reinterpret_d(NeonInt<T> a) { return Vec2d(neon_detail::bit_cast<float64x2_t>(a.native())); }
inline Vec4f reinterpret_f(Vec2d a) { return Vec4f(neon_detail::bit_cast<float32x4_t>(a.native())); }
inline Vec2d reinterpret_d(Vec4f a) { return Vec2d(neon_detail::bit_cast<float64x2_t>(a.native())); }
namespace neon_detail {
template<class T> inline NeonInt<typename fp_traits<T>::U> magnitude(NeonFloat<T> a) {
    using U=typename fp_traits<T>::U; using V=NeonInt<U>;
    auto bits=neon_detail::bit_cast<typename V::register_type>(a.native());
#if (defined(__GNUC__) || defined(__clang__)) && defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
    // Keep classification in integer bits when the optimizer assumes finite FP.
    __asm__ volatile("" : "+w"(bits));
#endif
    return V(bits) & V(std::numeric_limits<U>::max()>>1);
}
}
template<class T> inline typename NeonFloat<T>::mask_type is_nan(NeonFloat<T> a) {
    using U=typename neon_detail::fp_traits<T>::U; using V=NeonInt<U>;
    constexpr U inf=sizeof(T)==4?U(0x7f800000u):U(0x7ff0000000000000ull);
    return typename NeonFloat<T>::mask_type((neon_detail::magnitude(a)>V(inf)).native());
}
template<class T> inline typename NeonFloat<T>::mask_type is_inf(NeonFloat<T> a) {
    using U=typename neon_detail::fp_traits<T>::U; using V=NeonInt<U>;
    constexpr U inf=sizeof(T)==4?U(0x7f800000u):U(0x7ff0000000000000ull);
    return typename NeonFloat<T>::mask_type((neon_detail::magnitude(a)==V(inf)).native());
}
template<class T> inline typename NeonFloat<T>::mask_type is_finite(NeonFloat<T> a) {
    using U=typename neon_detail::fp_traits<T>::U; using V=NeonInt<U>;
    constexpr U inf=sizeof(T)==4?U(0x7f800000u):U(0x7ff0000000000000ull);
    return typename NeonFloat<T>::mask_type((neon_detail::magnitude(a)<V(inf)).native());
}
template<class T> inline typename NeonFloat<T>::mask_type is_subnormal(NeonFloat<T> a) {
    using U=typename neon_detail::fp_traits<T>::U; using V=NeonInt<U>;
    constexpr U normal=sizeof(T)==4?U(0x00800000u):U(0x0010000000000000ull);
    auto bits=neon_detail::magnitude(a);
    return typename NeonFloat<T>::mask_type(((bits<V(normal)) & (bits!=V(0))).native());
}
template<class T> inline typename NeonFloat<T>::mask_type is_zero_or_subnormal(NeonFloat<T> a) {
    using U=typename neon_detail::fp_traits<T>::U; using V=NeonInt<U>;
    constexpr U normal=sizeof(T)==4?U(0x00800000u):U(0x0010000000000000ull);
    return typename NeonFloat<T>::mask_type((neon_detail::magnitude(a)<V(normal)).native());
}
inline Vec4f if_add(Vec4fb m,Vec4f a,Vec4f b) { return a+(Vec4f(m)&b); }
inline Vec4f if_sub(Vec4fb m,Vec4f a,Vec4f b) { return a-(Vec4f(m)&b); }
inline Vec4f if_mul(Vec4fb m,Vec4f a,Vec4f b) { return a*select(m,b,Vec4f(1)); }
inline Vec4f if_div(Vec4fb m,Vec4f a,Vec4f b) { return a/select(m,b,Vec4f(1)); }
inline Vec2d if_add(Vec2db m,Vec2d a,Vec2d b) { return a+(Vec2d(m)&b); }
inline Vec2d if_sub(Vec2db m,Vec2d a,Vec2d b) { return a-(Vec2d(m)&b); }
inline Vec2d if_mul(Vec2db m,Vec2d a,Vec2d b) { return a*select(m,b,Vec2d(1)); }
inline Vec2d if_div(Vec2db m,Vec2d a,Vec2d b) { return a/select(m,b,Vec2d(1)); }

inline Vec4f to_float(Vec4i a) { return Vec4f(vcvtq_f32_s32(a.native())); }
inline Vec4f to_float(Vec4ui a) { return Vec4f(vcvtq_f32_u32(a.native())); }
inline Vec2d to_double(Vec2q a) { return Vec2d(vcvtq_f64_s64(a.native())); }
inline Vec2d to_double(Vec2uq a) { return Vec2d(vcvtq_f64_u64(a.native())); }
// ARM saturates invalid conversions; VCL/SSE returns the signed minimum.
inline Vec4i truncatei(Vec4f a) {
    auto valid=(a>=Vec4f(-2147483648.f))&(a<Vec4f(2147483648.f));
    return select(Vec4ib(valid.native()),Vec4i(vcvtq_s32_f32(a.native())),Vec4i(INT32_MIN));
}
inline Vec2q truncatei(Vec2d a) {
    auto valid=(a>=Vec2d(-9223372036854775808.0))&(a<Vec2d(9223372036854775808.0));
    return select(Vec2qb(valid.native()),Vec2q(vcvtq_s64_f64(a.native())),Vec2q(INT64_MIN));
}
inline Vec4i roundi(Vec4f a) { return truncatei(Vec4f(vrndiq_f32(a.native()))); }
inline Vec2q roundi(Vec2d a) { return truncatei(round(a)); }
inline Vec2d to_double_low(Vec4i a) { return Vec2d(vcvtq_f64_s64(vmovl_s32(vget_low_s32(a.native())))); }
inline Vec2d to_double_high(Vec4i a) { return Vec2d(vcvtq_f64_s64(vmovl_s32(vget_high_s32(a.native())))); }
inline Vec4f compress(Vec2d low,Vec2d high) { return Vec4f(vcombine_f32(vcvt_f32_f64(low.native()),vcvt_f32_f64(high.native()))); }
inline Vec2d extend_low(Vec4f a) { return Vec2d(vcvt_f64_f32(vget_low_f32(a.native()))); }
inline Vec2d extend_high(Vec4f a) { return Vec2d(vcvt_f64_f32(vget_high_f32(a.native()))); }

template<int... I> inline Vec4f permute4(Vec4f a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec4f blend4(Vec4f a,Vec4f b) { return neon_detail::blend<I...>(a,b); }
template<int... I> inline Vec2d permute2(Vec2d a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec2d blend2(Vec2d a,Vec2d b) { return neon_detail::blend<I...>(a,b); }
inline Vec4f lookup4(Vec4i index,Vec4f t0) { return neon_detail::register_lookup<1,Vec4f>(index,{{t0}}); }
inline Vec4f lookup8(Vec4i index,Vec4f t0,Vec4f t1) { return neon_detail::register_lookup<2,Vec4f>(index,{{t0,t1}}); }
template<int n> inline Vec4f lookup(Vec4i index,float const* table) { return neon_detail::memory_lookup<n,Vec4f>(index,table); }
template<int i0,int i1,int i2,int i3> inline Vec4f gather4f(void const* table) { return neon_detail::gather<Vec4f,i0,i1,i2,i3>(table); }
template<int i0,int i1,int i2,int i3> inline void scatter(Vec4f data,float* destination) { neon_detail::scatter_fixed<i0,i1,i2,i3>(data,destination); }
inline void scatter(Vec4i index,uint32_t limit,Vec4f data,float* destination) { neon_detail::scatter_variable(index,limit,data,destination); }
inline Vec2d lookup2(Vec2q index,Vec2d t0) { return neon_detail::register_lookup<1,Vec2d>(index,{{t0}}); }
inline Vec2d lookup4(Vec2q index,Vec2d t0,Vec2d t1) { return neon_detail::register_lookup<2,Vec2d>(index,{{t0,t1}}); }
template<int n> inline Vec2d lookup(Vec2q index,double const* table) { return neon_detail::memory_lookup<n,Vec2d>(index,table); }
template<int i0,int i1> inline Vec2d gather2d(void const* table) { return neon_detail::gather<Vec2d,i0,i1>(table); }
template<int i0,int i1> inline void scatter(Vec2d data,double* destination) { neon_detail::scatter_fixed<i0,i1>(data,destination); }
inline void scatter(Vec2q index,uint32_t limit,Vec2d data,double* destination) { neon_detail::scatter_variable(index,limit,data,destination); }
template<int i0,int i1,int i2,int i3> inline Vec4f change_sign(Vec4f a) {
    const uint32_t mask[4]={i0?0x80000000u:0,i1?0x80000000u:0,i2?0x80000000u:0,i3?0x80000000u:0};
    return Vec4f(vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a.native()),vld1q_u32(mask))));
}
template<int i0,int i1> inline Vec2d change_sign(Vec2d a) {
    const uint64_t mask[2]={i0?0x8000000000000000ull:0,i1?0x8000000000000000ull:0};
    return Vec2d(vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(a.native()),vld1q_u64(mask))));
}
#ifdef VCL_NAMESPACE
}
#endif
#include "vectorf128_neon_helpers.h"
#endif
