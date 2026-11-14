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

// Experimental native AArch64 vectors. Copyright 2026. Apache-2.0.
// Independent of sse2neon. Only the tested subset documented in tests/README.md
// is supported; native() exposes the actual Neon register without SSE aliases.
#ifndef VECTORI128_NEON_H
#define VECTORI128_NEON_H
#include "instrset_neon.h"
#include <algorithm>
#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif
namespace neon_detail {
template<class T> struct traits;
template<> struct traits<int8_t> {
    using reg=int8x16_t; using ureg=uint8x16_t; static constexpr int lanes=16, code=4;
    static reg dup(int8_t x) { return vdupq_n_s8(x); }
    static reg load(int8_t const* p) { return vld1q_s8(p); }
    static void store(int8_t* p, reg x) { vst1q_s8(p,x); }
    static reg add(reg a,reg b) { return vaddq_s8(a,b); }
    static reg sub(reg a,reg b) { return vsubq_s8(a,b); }
    static reg sat_add(reg a,reg b) { return vqaddq_s8(a,b); }
    static reg sat_sub(reg a,reg b) { return vqsubq_s8(a,b); }
    static ureg eq(reg a,reg b) { return vceqq_s8(a,b); }
    static ureg gt(reg a,reg b) { return vcgtq_s8(a,b); }
    static ureg ge(reg a,reg b) { return vcgeq_s8(a,b); }
    static reg shift(reg a,int count) { return vshlq_s8(a,vdupq_n_s8(count)); }
    static reg select(ureg mask,reg a,reg b) { return vbslq_s8(mask,a,b); }
};
template<> struct traits<uint8_t> {
    using reg=uint8x16_t; using ureg=uint8x16_t; static constexpr int lanes=16, code=5;
    static reg dup(uint8_t x) { return vdupq_n_u8(x); }
    static reg load(uint8_t const* p) { return vld1q_u8(p); }
    static void store(uint8_t* p, reg x) { vst1q_u8(p,x); }
    static reg add(reg a,reg b) { return vaddq_u8(a,b); }
    static reg sub(reg a,reg b) { return vsubq_u8(a,b); }
    static reg sat_add(reg a,reg b) { return vqaddq_u8(a,b); }
    static reg sat_sub(reg a,reg b) { return vqsubq_u8(a,b); }
    static ureg eq(reg a,reg b) { return vceqq_u8(a,b); }
    static ureg gt(reg a,reg b) { return vcgtq_u8(a,b); }
    static ureg ge(reg a,reg b) { return vcgeq_u8(a,b); }
    static reg shift(reg a,int count) { return vshlq_u8(a,vdupq_n_s8(count)); }
    static reg select(ureg mask,reg a,reg b) { return vbslq_u8(mask,a,b); }
};
template<> struct traits<int16_t> {
    using reg=int16x8_t; using ureg=uint16x8_t; static constexpr int lanes=8, code=6;
    static reg dup(int16_t x) { return vdupq_n_s16(x); }
    static reg load(int16_t const* p) { return vld1q_s16(p); }
    static void store(int16_t* p, reg x) { vst1q_s16(p,x); }
    static reg add(reg a,reg b) { return vaddq_s16(a,b); }
    static reg sub(reg a,reg b) { return vsubq_s16(a,b); }
    static reg sat_add(reg a,reg b) { return vqaddq_s16(a,b); }
    static reg sat_sub(reg a,reg b) { return vqsubq_s16(a,b); }
    static ureg eq(reg a,reg b) { return vceqq_s16(a,b); }
    static ureg gt(reg a,reg b) { return vcgtq_s16(a,b); }
    static ureg ge(reg a,reg b) { return vcgeq_s16(a,b); }
    static reg shift(reg a,int count) { return vshlq_s16(a,vdupq_n_s16(count)); }
    static reg select(ureg mask,reg a,reg b) { return vbslq_s16(mask,a,b); }
};
template<> struct traits<uint16_t> {
    using reg=uint16x8_t; using ureg=uint16x8_t; static constexpr int lanes=8, code=7;
    static reg dup(uint16_t x) { return vdupq_n_u16(x); }
    static reg load(uint16_t const* p) { return vld1q_u16(p); }
    static void store(uint16_t* p, reg x) { vst1q_u16(p,x); }
    static reg add(reg a,reg b) { return vaddq_u16(a,b); }
    static reg sub(reg a,reg b) { return vsubq_u16(a,b); }
    static reg sat_add(reg a,reg b) { return vqaddq_u16(a,b); }
    static reg sat_sub(reg a,reg b) { return vqsubq_u16(a,b); }
    static ureg eq(reg a,reg b) { return vceqq_u16(a,b); }
    static ureg gt(reg a,reg b) { return vcgtq_u16(a,b); }
    static ureg ge(reg a,reg b) { return vcgeq_u16(a,b); }
    static reg shift(reg a,int count) { return vshlq_u16(a,vdupq_n_s16(count)); }
    static reg select(ureg mask,reg a,reg b) { return vbslq_u16(mask,a,b); }
};
template<> struct traits<int32_t> {
    using reg=int32x4_t; using ureg=uint32x4_t; static constexpr int lanes=4, code=8;
    static reg dup(int32_t x) { return vdupq_n_s32(x); }
    static reg load(int32_t const* p) { return vld1q_s32(p); }
    static void store(int32_t* p, reg x) { vst1q_s32(p,x); }
    static reg add(reg a,reg b) { return vaddq_s32(a,b); }
    static reg sub(reg a,reg b) { return vsubq_s32(a,b); }
    static reg sat_add(reg a,reg b) { return vqaddq_s32(a,b); }
    static reg sat_sub(reg a,reg b) { return vqsubq_s32(a,b); }
    static ureg eq(reg a,reg b) { return vceqq_s32(a,b); }
    static ureg gt(reg a,reg b) { return vcgtq_s32(a,b); }
    static ureg ge(reg a,reg b) { return vcgeq_s32(a,b); }
    static reg shift(reg a,int count) { return vshlq_s32(a,vdupq_n_s32(count)); }
    static reg select(ureg mask,reg a,reg b) { return vbslq_s32(mask,a,b); }
};
template<> struct traits<uint32_t> {
    using reg=uint32x4_t; using ureg=uint32x4_t; static constexpr int lanes=4, code=9;
    static reg dup(uint32_t x) { return vdupq_n_u32(x); }
    static reg load(uint32_t const* p) { return vld1q_u32(p); }
    static void store(uint32_t* p, reg x) { vst1q_u32(p,x); }
    static reg add(reg a,reg b) { return vaddq_u32(a,b); }
    static reg sub(reg a,reg b) { return vsubq_u32(a,b); }
    static reg sat_add(reg a,reg b) { return vqaddq_u32(a,b); }
    static reg sat_sub(reg a,reg b) { return vqsubq_u32(a,b); }
    static ureg eq(reg a,reg b) { return vceqq_u32(a,b); }
    static ureg gt(reg a,reg b) { return vcgtq_u32(a,b); }
    static ureg ge(reg a,reg b) { return vcgeq_u32(a,b); }
    static reg shift(reg a,int count) { return vshlq_u32(a,vdupq_n_s32(count)); }
    static reg select(ureg mask,reg a,reg b) { return vbslq_u32(mask,a,b); }
};
template<> struct traits<int64_t> {
    using reg=int64x2_t; using ureg=uint64x2_t; static constexpr int lanes=2, code=10;
    static reg dup(int64_t x) { return vdupq_n_s64(x); }
    static reg load(int64_t const* p) { return vld1q_s64(p); }
    static void store(int64_t* p, reg x) { vst1q_s64(p,x); }
    static reg add(reg a,reg b) { return vaddq_s64(a,b); }
    static reg sub(reg a,reg b) { return vsubq_s64(a,b); }
    static reg sat_add(reg a,reg b) { return vqaddq_s64(a,b); }
    static reg sat_sub(reg a,reg b) { return vqsubq_s64(a,b); }
    static ureg eq(reg a,reg b) { return vceqq_s64(a,b); }
    static ureg gt(reg a,reg b) { return vcgtq_s64(a,b); }
    static ureg ge(reg a,reg b) { return vcgeq_s64(a,b); }
    static reg shift(reg a,int count) { return vshlq_s64(a,vdupq_n_s64(count)); }
    static reg select(ureg mask,reg a,reg b) { return vbslq_s64(mask,a,b); }
};
template<> struct traits<uint64_t> {
    using reg=uint64x2_t; using ureg=uint64x2_t; static constexpr int lanes=2, code=11;
    static reg dup(uint64_t x) { return vdupq_n_u64(x); }
    static reg load(uint64_t const* p) { return vld1q_u64(p); }
    static void store(uint64_t* p, reg x) { vst1q_u64(p,x); }
    static reg add(reg a,reg b) { return vaddq_u64(a,b); }
    static reg sub(reg a,reg b) { return vsubq_u64(a,b); }
    static reg sat_add(reg a,reg b) { return vqaddq_u64(a,b); }
    static reg sat_sub(reg a,reg b) { return vqsubq_u64(a,b); }
    static ureg eq(reg a,reg b) { return vceqq_u64(a,b); }
    static ureg gt(reg a,reg b) { return vcgtq_u64(a,b); }
    static ureg ge(reg a,reg b) { return vcgeq_u64(a,b); }
    static reg shift(reg a,int count) { return vshlq_u64(a,vdupq_n_s64(count)); }
    static reg select(ureg mask,reg a,reg b) { return vbslq_u64(mask,a,b); }
};
template<class To,class From> inline To bit_cast(From value) {
    static_assert(sizeof(To)==sizeof(From)); To result; std::memcpy(&result,&value,sizeof(result)); return result;
}
}
template<class T> class NeonInt;
template<class U,int Tag=0> class NeonMask;
class Vec128b {
protected:
    uint8x16_t value;
public:
    using register_type=uint8x16_t;
    using registertype=register_type;
    Vec128b()=default;
    Vec128b(int x):value(vreinterpretq_u8_s32(vdupq_n_s32(x))) {}
    constexpr explicit Vec128b(uint8x16_t x):value(x) {}
    Vec128b& operator=(register_type x) { value=x; return *this; }
    uint8x16_t native() const { return value; }
    Vec128b& load(void const* p) { value=vld1q_u8(static_cast<uint8_t const*>(p)); return *this; }
    void load_a(void const* p) { load(p); }
    void store(void* p) const { vst1q_u8(static_cast<uint8_t*>(p),value); }
    void store_a(void* p) const { store(p); }
    void store_nt(void* p) const { store(p); }
    static constexpr int size() { return 128; }
    static constexpr int elementtype() { return 1; }
    friend Vec128b operator&(Vec128b a,Vec128b b) { return Vec128b(vandq_u8(a.value,b.value)); }
    friend Vec128b operator|(Vec128b a,Vec128b b) { return Vec128b(vorrq_u8(a.value,b.value)); }
    friend Vec128b operator^(Vec128b a,Vec128b b) { return Vec128b(veorq_u8(a.value,b.value)); }
    Vec128b& operator&=(Vec128b b) { return *this=*this&b; }
    Vec128b& operator|=(Vec128b b) { return *this=*this|b; }
    Vec128b& operator^=(Vec128b b) { return *this=*this^b; }
    friend Vec128b operator&&(Vec128b a,Vec128b b) { return a&b; }
    friend Vec128b operator||(Vec128b a,Vec128b b) { return a|b; }
    friend Vec128b operator~(Vec128b a) { return Vec128b(vmvnq_u8(a.value)); }
};

inline bool horizontal_and(Vec128b a) { return vminvq_u8(a.native())==255; }
inline bool horizontal_or(Vec128b a) { return vmaxvq_u8(a.native())!=0; }
inline Vec128b andnot(Vec128b a,Vec128b b) { return a&~b; }
inline Vec128b selectb(Vec128b mask,Vec128b a,Vec128b b) { return Vec128b(vbslq_u8(mask.native(),a.native(),b.native())); }

template<class T> class NeonInt : public std::conditional<std::is_unsigned<T>::value,
    NeonInt<typename std::make_signed<T>::type>,Vec128b>::type {
    using tr=neon_detail::traits<T>;
    using U=typename std::make_unsigned<T>::type;
    void set_native(typename tr::reg x) { this->value=neon_detail::bit_cast<uint8x16_t>(x); }
public:
    using register_type=typename tr::reg;
    using registertype=register_type;
    using mask_type=NeonMask<U>;
    using signed_type=NeonInt<typename std::make_signed<T>::type>;
    NeonInt()=default;
    NeonInt(T x) { set_native(tr::dup(x)); }
    explicit NeonInt(register_type x) { set_native(x); }
    NeonInt& operator=(register_type x) { set_native(x); return *this; }
    template<class Raw,typename std::enable_if<std::is_same<Raw,Vec128b>::value,int>::type=0>
    NeonInt(Raw const& x) { set_native(neon_detail::bit_cast<register_type>(x.native())); }
    template<class S> NeonInt& operator=(NeonInt<S> x) { set_native(neon_detail::bit_cast<register_type>(x.native())); return *this; }
    explicit NeonInt(mask_type x) { set_native(neon_detail::bit_cast<register_type>(x.native())); }
    template<class S,typename std::enable_if<!(sizeof(S)==sizeof(T) && std::is_signed<T>::value && std::is_unsigned<S>::value),int>::type=0>
    explicit NeonInt(NeonInt<S> x) { set_native(neon_detail::bit_cast<register_type>(x.native())); }
    template<class... A,typename std::enable_if<sizeof...(A)==tr::lanes,int>::type=0>
    NeonInt(A... x) { T data[]={T(x)...}; set_native(tr::load(data)); }
    register_type native() const { return neon_detail::bit_cast<register_type>(this->value); }
    static constexpr int size() { return tr::lanes; }
    static constexpr int elementtype() { return tr::code; }
    NeonInt& load(void const* p) { set_native(tr::load(static_cast<T const*>(p))); return *this; }
    NeonInt& load_a(void const* p) { return load(p); }
    void store(void* p) const { tr::store(static_cast<T*>(p),native()); }
    void store_a(void* p) const { store(p); }
    void store_nt(void* p) const { store(p); }
    signed_type& load_partial(int n,void const* p) {
        if constexpr(sizeof(T)>=4) { if(unsigned(n)>tr::lanes) return *this; }
        const int count=std::max(0,std::min(n,tr::lanes));
        T data[tr::lanes]={}; if(count) std::memcpy(data,p,count*sizeof(T)); set_native(tr::load(data)); return *this;
    }
    void store_partial(int n,void* p) const {
        const int count=sizeof(T)==8 ? std::max(0,std::min(n,tr::lanes)) : (unsigned(n)>tr::lanes?tr::lanes:n);
        T data[tr::lanes]; store(data); if(count) std::memcpy(p,data,count*sizeof(T));
    }
    signed_type& cutoff(int n) { if(unsigned(n)>=tr::lanes) return *this; T data[tr::lanes]; store(data); for(int i=n;i<tr::lanes;++i) data[i]=0; return load(data); }
    T operator[](int i) const { T data[tr::lanes]; store(data); return data[unsigned(i)&(tr::lanes-1)]; }
    T extract(int i) const { return (*this)[i]; }
    const NeonInt insert(int i,T x) { T data[tr::lanes]; store(data); data[unsigned(i)&(tr::lanes-1)]=x; load(data); return *this; }
    friend NeonInt operator+(NeonInt a,NeonInt b) { return NeonInt(tr::add(a.native(),b.native())); }
    friend NeonInt operator-(NeonInt a,NeonInt b) { return NeonInt(tr::sub(a.native(),b.native())); }
    friend auto operator-(NeonInt a) {
        using S=typename std::make_signed<T>::type; using V=NeonInt<S>;
        return V(neon_detail::bit_cast<typename V::register_type>(tr::sub(tr::dup(0),a.native())));
    }
    friend NeonInt operator*(NeonInt a,NeonInt b) {
        // Unsigned arithmetic preserves modular integer multiplication, including 64-bit.
        U x[tr::lanes],y[tr::lanes],z[tr::lanes]; auto av=a.native(),bv=b.native(); std::memcpy(x,&av,16); std::memcpy(y,&bv,16);
        for(int i=0;i<tr::lanes;++i) z[i]=U(uint64_t(x[i])*uint64_t(y[i]));
        register_type result; std::memcpy(&result,z,16); return NeonInt(result);
    }
    friend NeonInt operator&(NeonInt a,NeonInt b) { return NeonInt(a.native()&b.native()); }
    friend NeonInt operator|(NeonInt a,NeonInt b) { return NeonInt(a.native()|b.native()); }
    friend NeonInt operator^(NeonInt a,NeonInt b) { return NeonInt(a.native()^b.native()); }
    friend NeonInt operator&&(NeonInt a,NeonInt b) { return a&b; }
    friend NeonInt operator||(NeonInt a,NeonInt b) { return a|b; }
    friend mask_type operator!(NeonInt a) { return a==NeonInt(T(0)); }
    friend NeonInt operator~(NeonInt a) { return NeonInt(~a.native()); }
    friend NeonInt operator<<(NeonInt a,int count) {
        if(unsigned(count)>=sizeof(T)*8) return NeonInt(T(0));
        return NeonInt(tr::shift(a.native(),count));
    }
    friend NeonInt operator>>(NeonInt a,int count) {
        count=unsigned(count)>=sizeof(T)*8?int(sizeof(T)*8):count;
        return NeonInt(tr::shift(a.native(),-count));
    }
    friend mask_type operator==(NeonInt a,NeonInt b) { return mask_type(tr::eq(a.native(),b.native())); }
    friend mask_type operator!=(NeonInt a,NeonInt b) { return ~(a==b); }
    friend mask_type operator>(NeonInt a,NeonInt b) { return mask_type(tr::gt(a.native(),b.native())); }
    friend mask_type operator<(NeonInt a,NeonInt b) { return b>a; }
    friend mask_type operator>=(NeonInt a,NeonInt b) { return mask_type(tr::ge(a.native(),b.native())); }
    friend mask_type operator<=(NeonInt a,NeonInt b) { return b>=a; }
    friend NeonInt select(mask_type m,NeonInt a,NeonInt b) { return NeonInt(tr::select(m.native(),a.native(),b.native())); }
    signed_type& operator+=(signed_type b) { if constexpr(std::is_unsigned<T>::value) return static_cast<signed_type&>(*this)+=b; else return *this=*this+b; }
    signed_type& operator-=(signed_type b) { if constexpr(std::is_unsigned<T>::value) return static_cast<signed_type&>(*this)-=b; else return *this=*this-b; }
    signed_type& operator*=(signed_type b) { if constexpr(std::is_unsigned<T>::value) return static_cast<signed_type&>(*this)*=b; else return *this=*this*b; }
    signed_type& operator&=(signed_type b) { if constexpr(std::is_unsigned<T>::value) return static_cast<signed_type&>(*this)&=b; else return *this=*this&b; }
    signed_type& operator|=(signed_type b) { if constexpr(std::is_unsigned<T>::value) return static_cast<signed_type&>(*this)|=b; else return *this=*this|b; }
    signed_type& operator^=(signed_type b) { if constexpr(std::is_unsigned<T>::value) return static_cast<signed_type&>(*this)^=b; else return *this=*this^b; }
    signed_type& operator<<=(int b) { if constexpr(std::is_unsigned<T>::value) return static_cast<signed_type&>(*this)<<=b; else return *this=*this<<b; }
    NeonInt& operator>>=(int b) { return *this=*this>>b; }
    signed_type& operator++() { return *this+=signed_type(1); }
    signed_type operator++(int) { signed_type old=*this; ++*this; return old; }
    signed_type& operator--() { return *this-=signed_type(1); }
    signed_type operator--(int) { signed_type old=*this; --*this; return old; }
};
namespace neon_detail { struct fp_mask_storage { protected: uint8x16_t value; }; }
template<class U,int Tag> class NeonMask : public std::conditional<Tag==0,
    NeonInt<typename std::make_signed<U>::type>,neon_detail::fp_mask_storage>::type {
    using tr=neon_detail::traits<U>;
    using numeric_type=NeonInt<typename std::make_signed<U>::type>;
    using memory_type=typename std::conditional<Tag==0,numeric_type,NeonMask>::type;
    void set_native(typename tr::reg x) { this->value=neon_detail::bit_cast<uint8x16_t>(x); }
public:
    using register_type=typename tr::reg;
    using registertype=register_type;
    using bits_type=typename std::conditional<tr::lanes==16,uint16_t,uint8_t>::type;
    NeonMask()=default;
    NeonMask(bool x) { set_native(tr::dup(x?U(~U(0)):U(0))); }
    explicit NeonMask(register_type x) { set_native(x); }
    explicit NeonMask(Vec128b x) { set_native(neon_detail::bit_cast<register_type>(x.native())); }
    NeonMask& operator=(register_type x) { set_native(x); return *this; }
    template<class S,typename std::enable_if<sizeof(S)==sizeof(U),int>::type=0>
    explicit NeonMask(NeonInt<S> x) { set_native(neon_detail::bit_cast<register_type>(x.native())); }
    template<class S> NeonMask& operator=(NeonInt<S> x) { set_native(neon_detail::bit_cast<register_type>(x.native())); return *this; }
    NeonMask& operator=(Vec128b x) { set_native(neon_detail::bit_cast<register_type>(x.native())); return *this; }
    template<class V,typename std::enable_if<Tag==1 && std::is_same<V,numeric_type>::value,int>::type=0>
    operator V() const { return V(neon_detail::bit_cast<typename V::register_type>(native())); }
    memory_type& load(void const* p) { set_native(tr::load(static_cast<U const*>(p))); return *this; }
    memory_type& load_a(void const* p) { return load(p); }
    void store(void* p) const { tr::store(static_cast<U*>(p),native()); }
    void store_a(void* p) const { store(p); }
    void store_nt(void* p) const { store(p); }
    template<int OtherTag> NeonMask(NeonMask<U,OtherTag> x) { set_native(x.native()); }
    template<int OtherTag> NeonMask& operator=(NeonMask<U,OtherTag> x) { set_native(x.native()); return *this; }
    NeonMask& operator=(bool x) { set_native(tr::dup(x?U(~U(0)):U(0))); return *this; }
    template<class... A,typename std::enable_if<sizeof...(A)==tr::lanes,int>::type=0>
    NeonMask(A... x) { U data[]={U(bool(x)?~U(0):0)...}; set_native(tr::load(data)); }
    register_type native() const { return neon_detail::bit_cast<register_type>(this->value); }
    static constexpr int size() { return tr::lanes; }
    static constexpr int elementtype() { return 3; }
    bool operator[](int i) const { U data[tr::lanes]; tr::store(data,native()); return data[unsigned(i)&(tr::lanes-1)]!=0; }
    bool extract(int i) const { return (*this)[i]; }
    typename std::conditional<Tag==1,const NeonMask,NeonMask&>::type insert(int i,bool x) { U data[tr::lanes]; tr::store(data,native()); data[unsigned(i)&(tr::lanes-1)]=x?U(~U(0)):U(0); set_native(tr::load(data)); return *this; }
    NeonMask& load_bits(bits_type bits) { U data[tr::lanes]; for(int i=0;i<tr::lanes;++i) data[i]=(bits>>i)&1?U(~U(0)):U(0); set_native(tr::load(data)); return *this; }
    NeonMask(int) = delete;
    NeonMask& operator=(int) = delete;
    explicit operator bool() const = delete;
    friend NeonMask operator&(NeonMask a,NeonMask b) { return NeonMask(a.native() & b.native()); }
    friend NeonMask operator|(NeonMask a,NeonMask b) { return NeonMask(a.native() | b.native()); }
    friend NeonMask operator^(NeonMask a,NeonMask b) { return NeonMask(a.native() ^ b.native()); }
    friend NeonMask operator~(NeonMask a) { return NeonMask(~a.native()); }
    friend NeonMask operator!(NeonMask a) { return ~a; }
    friend NeonMask operator&&(NeonMask a,NeonMask b) { return a&b; }
    friend NeonMask operator||(NeonMask a,NeonMask b) { return a|b; }
    friend NeonMask operator==(NeonMask a,NeonMask b) { return ~(a^b); }
    friend NeonMask operator!=(NeonMask a,NeonMask b) { return a^b; }
    NeonMask& operator&=(NeonMask b) { return *this=*this&b; }
    NeonMask& operator|=(NeonMask b) { return *this=*this|b; }
    NeonMask& operator^=(NeonMask b) { return *this=*this^b; }
};
template<class U,int Tag> inline typename NeonMask<U,Tag>::bits_type to_bits(NeonMask<U,Tag> m) {
    U data[m.size()]; neon_detail::traits<U>::store(data,m.native()); uint64_t bits=0;
    for(int i=0;i<m.size();++i) bits|=uint64_t(data[i]>>(sizeof(U)*8-1))<<i;
    return typename NeonMask<U,Tag>::bits_type(bits);
}
template<class U,int Tag> inline bool horizontal_and(NeonMask<U,Tag> m) { return to_bits(m)==((uint64_t(1)<<m.size())-1); }
template<class U,int Tag> inline bool horizontal_or(NeonMask<U,Tag> m) { return to_bits(m)!=0; }
template<class U,int Tag> inline int horizontal_count(NeonMask<U,Tag> m) { return __builtin_popcountll(to_bits(m)); }
template<class U,int Tag> inline int horizontal_find_first(NeonMask<U,Tag> m) { auto bits=to_bits(m); return bits?int(bit_scan_forward(uint32_t(bits))):-1; }
template<class U,int Tag> inline NeonMask<U,Tag> andnot(NeonMask<U,Tag> a,NeonMask<U,Tag> b) { return a&~b; }
using Vec16cb=NeonMask<uint8_t>; using Vec8sb=NeonMask<uint16_t>;
using Vec4ib=NeonMask<uint32_t>; using Vec2qb=NeonMask<uint64_t>;

using Vec16c=NeonInt<int8_t>; using Vec16uc=NeonInt<uint8_t>;
using Vec8s=NeonInt<int16_t>; using Vec8us=NeonInt<uint16_t>;
using Vec4i=NeonInt<int32_t>; using Vec4ui=NeonInt<uint32_t>;
using Vec2q=NeonInt<int64_t>; using Vec2uq=NeonInt<uint64_t>;
// Constant integer bits use the generic carrier, as do reinterpret_i results.
template<uint32_t i0,uint32_t i1,uint32_t i2,uint32_t i3>
inline constexpr Vec128b constant4ui() {
    return Vec128b(vreinterpretq_u8_u32(uint32x4_t{i0,i1,i2,i3}));
}
template<int N> constexpr int min_index(int const (&a)[N]) {
    int result=a[0]; for(int i=1;i<N;++i) if(a[i]<result) result=a[i]; return result;
}
template<int N> constexpr int max_index(int const (&a)[N]) {
    int result=a[0]; for(int i=1;i<N;++i) if(a[i]>result) result=a[i]; return result;
}
inline Vec16c max(Vec16c a,Vec16c b) { return select(a>b,a,b); }
inline Vec16c min(Vec16c a,Vec16c b) { return select(a<b,a,b); }
inline Vec16uc max(Vec16uc a,Vec16uc b) { return select(a>b,a,b); }
inline Vec16uc min(Vec16uc a,Vec16uc b) { return select(a<b,a,b); }
inline Vec8s max(Vec8s a,Vec8s b) { return select(a>b,a,b); }
inline Vec8s min(Vec8s a,Vec8s b) { return select(a<b,a,b); }
inline Vec8us max(Vec8us a,Vec8us b) { return select(a>b,a,b); }
inline Vec8us min(Vec8us a,Vec8us b) { return select(a<b,a,b); }
inline Vec4i max(Vec4i a,Vec4i b) { return select(a>b,a,b); }
inline Vec4i min(Vec4i a,Vec4i b) { return select(a<b,a,b); }
inline Vec4ui max(Vec4ui a,Vec4ui b) { return select(a>b,a,b); }
inline Vec4ui min(Vec4ui a,Vec4ui b) { return select(a<b,a,b); }
inline Vec2q max(Vec2q a,Vec2q b) { return select(a>b,a,b); }
inline Vec2q min(Vec2q a,Vec2q b) { return select(a<b,a,b); }
inline Vec2uq max(Vec2uq a,Vec2uq b) { return select(a>b,a,b); }
inline Vec2uq min(Vec2uq a,Vec2uq b) { return select(a<b,a,b); }
template<class T> inline typename std::conditional<sizeof(T)==1 || (sizeof(T)==2 && std::is_unsigned<T>::value),typename std::conditional<std::is_signed<T>::value,int32_t,uint32_t>::type,T>::type horizontal_add(NeonInt<T> a) {
    using U=typename std::make_unsigned<T>::type; uint64_t sum=0;
    for(int i=0;i<a.size();++i) sum+=U(a[i]);
    U result=U(sum); return neon_detail::bit_cast<T>(result);
}
template<class T> inline auto horizontal_add_x(NeonInt<T> a) {
    static_assert(sizeof(T)<8,"No extended 64-bit reduction");
    using Wide=typename std::conditional<std::is_signed<T>::value,
        typename std::conditional<(sizeof(T)<4),int32_t,int64_t>::type,
        typename std::conditional<(sizeof(T)<4),uint32_t,uint64_t>::type>::type;
    Wide sum=0; for(int i=0;i<a.size();++i) sum+=Wide(a[i]); return sum;
}
inline Vec16c add_saturated(Vec16c a,Vec16c b) { return Vec16c(neon_detail::traits<int8_t>::sat_add(a.native(),b.native())); }
inline Vec16c sub_saturated(Vec16c a,Vec16c b) { return Vec16c(neon_detail::traits<int8_t>::sat_sub(a.native(),b.native())); }
inline Vec16uc add_saturated(Vec16uc a,Vec16uc b) { return Vec16uc(neon_detail::traits<uint8_t>::sat_add(a.native(),b.native())); }
inline Vec16uc sub_saturated(Vec16uc a,Vec16uc b) { return Vec16uc(neon_detail::traits<uint8_t>::sat_sub(a.native(),b.native())); }
inline Vec8s add_saturated(Vec8s a,Vec8s b) { return Vec8s(neon_detail::traits<int16_t>::sat_add(a.native(),b.native())); }
inline Vec8s sub_saturated(Vec8s a,Vec8s b) { return Vec8s(neon_detail::traits<int16_t>::sat_sub(a.native(),b.native())); }
inline Vec8us add_saturated(Vec8us a,Vec8us b) { return Vec8us(neon_detail::traits<uint16_t>::sat_add(a.native(),b.native())); }
inline Vec8us sub_saturated(Vec8us a,Vec8us b) { return Vec8us(neon_detail::traits<uint16_t>::sat_sub(a.native(),b.native())); }
inline Vec4i add_saturated(Vec4i a,Vec4i b) { return Vec4i(neon_detail::traits<int32_t>::sat_add(a.native(),b.native())); }
inline Vec4i sub_saturated(Vec4i a,Vec4i b) { return Vec4i(neon_detail::traits<int32_t>::sat_sub(a.native(),b.native())); }
inline Vec4ui add_saturated(Vec4ui a,Vec4ui b) { return Vec4ui(neon_detail::traits<uint32_t>::sat_add(a.native(),b.native())); }
inline Vec4ui sub_saturated(Vec4ui a,Vec4ui b) { return Vec4ui(neon_detail::traits<uint32_t>::sat_sub(a.native(),b.native())); }
namespace neon_detail {
template<class T> inline NeonInt<T> rotate_left(NeonInt<T> a,int count) {
    using U=typename std::make_unsigned<T>::type;
    const int n=uint32_t(count)&(sizeof(T)*8-1); const NeonInt<U> bits(a);
    return NeonInt<T>((bits<<n)|(bits>>(int(sizeof(T)*8)-n)));
}
}
inline Vec16c rotate_left(Vec16c a,int count) { return neon_detail::rotate_left(a,count); }
inline Vec8s rotate_left(Vec8s a,int count) { return neon_detail::rotate_left(a,count); }
inline Vec4i rotate_left(Vec4i a,int count) { return neon_detail::rotate_left(a,count); }
inline Vec2q rotate_left(Vec2q a,int count) { return neon_detail::rotate_left(a,count); }
inline Vec16c abs(Vec16c a) { return select(a<Vec16c(0),-a,a); }
inline Vec16c abs_saturated(Vec16c a) {
    using T=decltype(a[0]);
    return select(a==Vec16c(std::numeric_limits<T>::min()),Vec16c(std::numeric_limits<T>::max()),abs(a));
}
inline Vec8s abs(Vec8s a) { return select(a<Vec8s(0),-a,a); }
inline Vec8s abs_saturated(Vec8s a) {
    using T=decltype(a[0]);
    return select(a==Vec8s(std::numeric_limits<T>::min()),Vec8s(std::numeric_limits<T>::max()),abs(a));
}
inline Vec4i abs(Vec4i a) { return select(a<Vec4i(0),-a,a); }
inline Vec4i abs_saturated(Vec4i a) {
    using T=decltype(a[0]);
    return select(a==Vec4i(std::numeric_limits<T>::min()),Vec4i(std::numeric_limits<T>::max()),abs(a));
}
inline Vec2q abs(Vec2q a) { return select(a<Vec2q(0),-a,a); }
inline Vec2q abs_saturated(Vec2q a) {
    using T=decltype(a[0]);
    return select(a==Vec2q(std::numeric_limits<T>::min()),Vec2q(std::numeric_limits<T>::max()),abs(a));
}
inline Vec16c if_add(Vec16cb m,Vec16c a,Vec16c b) { return select(m,a+b,a); }
inline Vec16c if_sub(Vec16cb m,Vec16c a,Vec16c b) { return select(m,a-b,a); }
inline Vec16c if_mul(Vec16cb m,Vec16c a,Vec16c b) { return select(m,a*b,a); }
inline Vec16uc if_add(Vec16cb m,Vec16uc a,Vec16uc b) { return select(m,a+b,a); }
inline Vec16uc if_sub(Vec16cb m,Vec16uc a,Vec16uc b) { return select(m,a-b,a); }
inline Vec16uc if_mul(Vec16cb m,Vec16uc a,Vec16uc b) { return select(m,a*b,a); }
inline Vec8s if_add(Vec8sb m,Vec8s a,Vec8s b) { return select(m,a+b,a); }
inline Vec8s if_sub(Vec8sb m,Vec8s a,Vec8s b) { return select(m,a-b,a); }
inline Vec8s if_mul(Vec8sb m,Vec8s a,Vec8s b) { return select(m,a*b,a); }
inline Vec8us if_add(Vec8sb m,Vec8us a,Vec8us b) { return select(m,a+b,a); }
inline Vec8us if_sub(Vec8sb m,Vec8us a,Vec8us b) { return select(m,a-b,a); }
inline Vec8us if_mul(Vec8sb m,Vec8us a,Vec8us b) { return select(m,a*b,a); }
inline Vec4i if_add(Vec4ib m,Vec4i a,Vec4i b) { return select(m,a+b,a); }
inline Vec4i if_sub(Vec4ib m,Vec4i a,Vec4i b) { return select(m,a-b,a); }
inline Vec4i if_mul(Vec4ib m,Vec4i a,Vec4i b) { return select(m,a*b,a); }
inline Vec4ui if_add(Vec4ib m,Vec4ui a,Vec4ui b) { return select(m,a+b,a); }
inline Vec4ui if_sub(Vec4ib m,Vec4ui a,Vec4ui b) { return select(m,a-b,a); }
inline Vec4ui if_mul(Vec4ib m,Vec4ui a,Vec4ui b) { return select(m,a*b,a); }
inline Vec2q if_add(Vec2qb m,Vec2q a,Vec2q b) { return select(m,a+b,a); }
inline Vec2q if_sub(Vec2qb m,Vec2q a,Vec2q b) { return select(m,a-b,a); }
inline Vec2q if_mul(Vec2qb m,Vec2q a,Vec2q b) { return select(m,a*b,a); }
inline Vec2uq if_add(Vec2qb m,Vec2uq a,Vec2uq b) { return select(m,a+b,a); }
inline Vec2uq if_sub(Vec2qb m,Vec2uq a,Vec2uq b) { return select(m,a-b,a); }
inline Vec2uq if_mul(Vec2qb m,Vec2uq a,Vec2uq b) { return select(m,a*b,a); }

// Integer size conversions. Same lane order and wrap/saturation policy as
// vectori128.h (Agner Fog, Apache-2.0), implemented with native Neon operations.
inline Vec8s extend_low(Vec16c a) { return Vec8s(vmovl_s8(vget_low_s8(a.native()))); }
inline Vec8s extend_high(Vec16c a) { return Vec8s(vmovl_high_s8(a.native())); }
inline Vec16c compress(Vec8s low,Vec8s high) { return Vec16c(vcombine_s8(vmovn_s16(low.native()),vmovn_s16(high.native()))); }
inline Vec16c compress_saturated(Vec8s low,Vec8s high) { return Vec16c(vcombine_s8(vqmovn_s16(low.native()),vqmovn_s16(high.native()))); }
inline Vec8us extend_low(Vec16uc a) { return Vec8us(vmovl_u8(vget_low_u8(a.native()))); }
inline Vec8us extend_high(Vec16uc a) { return Vec8us(vmovl_high_u8(a.native())); }
inline Vec16uc compress(Vec8us low,Vec8us high) { return Vec16uc(vcombine_u8(vmovn_u16(low.native()),vmovn_u16(high.native()))); }
inline Vec16uc compress_saturated(Vec8us low,Vec8us high) { return Vec16uc(vcombine_u8(vqmovn_u16(low.native()),vqmovn_u16(high.native()))); }
inline Vec4i extend_low(Vec8s a) { return Vec4i(vmovl_s16(vget_low_s16(a.native()))); }
inline Vec4i extend_high(Vec8s a) { return Vec4i(vmovl_high_s16(a.native())); }
inline Vec8s compress(Vec4i low,Vec4i high) { return Vec8s(vcombine_s16(vmovn_s32(low.native()),vmovn_s32(high.native()))); }
inline Vec8s compress_saturated(Vec4i low,Vec4i high) { return Vec8s(vcombine_s16(vqmovn_s32(low.native()),vqmovn_s32(high.native()))); }
inline Vec4ui extend_low(Vec8us a) { return Vec4ui(vmovl_u16(vget_low_u16(a.native()))); }
inline Vec4ui extend_high(Vec8us a) { return Vec4ui(vmovl_high_u16(a.native())); }
inline Vec8us compress(Vec4ui low,Vec4ui high) { return Vec8us(vcombine_u16(vmovn_u32(low.native()),vmovn_u32(high.native()))); }
inline Vec8us compress_saturated(Vec4ui low,Vec4ui high) { return Vec8us(vcombine_u16(vqmovn_u32(low.native()),vqmovn_u32(high.native()))); }
inline Vec2q extend_low(Vec4i a) { return Vec2q(vmovl_s32(vget_low_s32(a.native()))); }
inline Vec2q extend_high(Vec4i a) { return Vec2q(vmovl_high_s32(a.native())); }
inline Vec4i compress(Vec2q low,Vec2q high) { return Vec4i(vcombine_s32(vmovn_s64(low.native()),vmovn_s64(high.native()))); }
inline Vec4i compress_saturated(Vec2q low,Vec2q high) { return Vec4i(vcombine_s32(vqmovn_s64(low.native()),vqmovn_s64(high.native()))); }
inline Vec2uq extend_low(Vec4ui a) { return Vec2uq(vmovl_u32(vget_low_u32(a.native()))); }
inline Vec2uq extend_high(Vec4ui a) { return Vec2uq(vmovl_high_u32(a.native())); }
inline Vec4ui compress(Vec2uq low,Vec2uq high) { return Vec4ui(vcombine_u32(vmovn_u64(low.native()),vmovn_u64(high.native()))); }
inline Vec4ui compress_saturated(Vec2uq low,Vec2uq high) { return Vec4ui(vcombine_u32(vqmovn_u64(low.native()),vqmovn_u64(high.native()))); }

#ifdef VCL_NAMESPACE
}
#endif
#include "vectori128_neon_div.h"
#include "vectori128_neon_perm.h"
#endif
