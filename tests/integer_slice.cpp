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

#include "vectorclass.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
Vec4i slice_divide_abi(Vec4i,Divisor_i);
Vec4ui slice_divide_abi(Vec4ui,Divisor_ui);
Vec8s slice_divide_abi(Vec8s,Divisor_s);
Vec8us slice_divide_abi(Vec8us,Divisor_us);
Vec16c slice_divide_abi(Vec16c,Divisor_s);
Vec16uc slice_divide_abi(Vec16uc,Divisor_us);
namespace {
int failures=0;
#define CHECK(x) do { if(!(x)) { if(failures<20) std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
uint64_t rng=0xabc123ffeedd0011ull;
uint64_t random_bits() { rng^=rng<<13; rng^=rng>>7; rng^=rng<<17; return rng; }
template<class T,class U> T bit_cast(U x) { static_assert(sizeof(T)==sizeof(U)); T y; std::memcpy(&y,&x,sizeof(y)); return y; }
template<class T> T value_bits(uint64_t x) { using U=typename std::make_unsigned<T>::type; return bit_cast<T>(U(x)); }
template<class T,class D> T quotient(T a,D d) {
    // A wider signed reference avoids MIN/-1 overflow. Narrow modulo the lane
    // width to check VCL's wrapping quotient, without out-of-range signed casts.
    if constexpr(std::is_signed<T>::value) return value_bits<T>(uint64_t(int64_t(a)/int64_t(d)));
    else return T(uint64_t(a)/uint64_t(d));
}
template<class T> T pattern(int i) {
    constexpr int w=sizeof(T)*8;
    const uint64_t values[]={0,1,2,3,uint64_t(1)<<(w-1),(uint64_t(1)<<(w-1))-1,
        (uint64_t(1)<<(w-1))+1,uint64_t(std::numeric_limits<typename std::make_unsigned<T>::type>::max()),
        uint64_t(std::numeric_limits<typename std::make_unsigned<T>::type>::max())-1};
    return value_bits<T>(values[i%9]);
}
template<class SmallV,class BigV,class Small,class Big> void conversions() {
    constexpr int n=BigV::size();
    static_assert(std::is_same<decltype(extend_low(SmallV())),BigV>::value);
    static_assert(std::is_same<decltype(extend_high(SmallV())),BigV>::value);
    static_assert(std::is_same<decltype(compress(BigV(),BigV())),SmallV>::value);
    static_assert(std::is_same<decltype(compress_saturated(BigV(),BigV())),SmallV>::value);
    for(int trial=0;trial<1000;++trial) {
        Small narrow[2*n]; Big low[n],high[n];
        for(int i=0;i<2*n;++i) narrow[i]=trial<9?pattern<Small>(trial+i):value_bits<Small>(random_bits());
        SmallV a; a.load(narrow); BigV lo=extend_low(a),hi=extend_high(a);
        for(int i=0;i<n;++i) { CHECK(lo[i]==Big(narrow[i])); CHECK(hi[i]==Big(narrow[i+n])); }
        auto back=compress(lo,hi); for(int i=0;i<2*n;++i) CHECK(back[i]==narrow[i]);
        for(int i=0;i<n;++i) {
            low[i]=trial<9?pattern<Big>(trial+i):value_bits<Big>(random_bits());
            high[i]=trial<9?pattern<Big>(trial+i+4):value_bits<Big>(random_bits());
        }
        // Exact endpoints and their neighbours test both saturation boundaries.
        if(trial<6) {
            Big endpoints[]={Big(std::numeric_limits<Small>::min()),Big(std::numeric_limits<Small>::max()),
                Big(Big(std::numeric_limits<Small>::min())-1),Big(Big(std::numeric_limits<Small>::max())+1),Big(0),Big(1)};
            for(int i=0;i<n;++i) { low[i]=endpoints[(trial+i)%6]; high[i]=endpoints[(trial+i+3)%6]; }
        }
        BigV x,y; x.load(low); y.load(high); auto wrapped=compress(x,y),saturated=compress_saturated(x,y);
        for(int i=0;i<2*n;++i) {
            const Big v=i<n?low[i]:high[i-n];
            CHECK(wrapped[i]==value_bits<Small>(uint64_t(v)));
            const Big expected=v<Big(std::numeric_limits<Small>::min())?Big(std::numeric_limits<Small>::min()):
                v>Big(std::numeric_limits<Small>::max())?Big(std::numeric_limits<Small>::max()):v;
            CHECK(saturated[i]==Small(expected));
        }
    }
}
template<class V,class T,class Div> void check_divisor(T const* input,Div divisor,typename std::conditional<std::is_signed<T>::value,int32_t,uint32_t>::type d) {
    V a; a.load(input); const auto q=a/divisor; V assigned=a; auto* identity=&(assigned/=divisor); CHECK(identity==&assigned);
    for(int lane=0;lane<V::size();++lane) { const T expected=quotient(input[lane],d); CHECK(q[lane]==expected); CHECK(assigned[lane]==expected); }
}
template<class V,class T,class Div,class D> void runtime_division() {
    static_assert(std::is_same<decltype(V()/Div()),V>::value);
    static_assert(std::is_same<decltype(V()/D(3)),V>::value);
    static_assert(sizeof(Div)==48 && alignof(Div)==16);
    D divisors[30]; int count=0;
    for(int i=1;i<=16;++i) divisors[count++]=D(i);
    divisors[count++]=std::numeric_limits<D>::max();
    divisors[count++]=D(std::numeric_limits<D>::max()-1);
    if constexpr(std::is_signed<D>::value) {
        for(int d:{-1,-2,-3,-7,-13,-127,-256}) divisors[count++]=D(d);
        divisors[count++]=std::numeric_limits<D>::min();
        divisors[count++]=D(std::numeric_limits<D>::min()+1);
    }
    else { divisors[count++]=D(uint64_t(1)<<(sizeof(D)*8-1)); divisors[count++]=D((uint64_t(1)<<(sizeof(D)*8-1))+1); }
    T input[V::size()];
    for(int index=0;index<count;++index) {
        D d=divisors[index]; Div divisor(d); Div reset; reset.set(d);
        for(int trial=0;trial<300;++trial) {
            for(int lane=0;lane<V::size();++lane) input[lane]=trial<9?pattern<T>(trial+lane):value_bits<T>(random_bits());
            check_divisor<V,T>(input,divisor,d); check_divisor<V,T>(input,reset,d);
            V a; a.load(input); auto implicit=a/d; for(int lane=0;lane<V::size();++lane) CHECK(implicit[lane]==quotient(input[lane],d));
        }
        V abi_input; abi_input.load(input); auto abi_result=slice_divide_abi(abi_input,divisor);
        for(int i=0;i<V::size();++i) CHECK(abi_result[i]==quotient(input[i],d));
        // Reconstruct from the public multiplier/shift/sign getters.
        V a; a.load(input);
        if constexpr(sizeof(D)==4) {
            const Vec4ui shift(divisor.gets1());
            for(int i=1;i<4;++i) CHECK(shift[i]==0);
            if constexpr(std::is_signed<D>::value) {
                const Vec4i mult(divisor.getm()),sign(divisor.getsign()); Div copy(mult[0],int(shift[0]),sign[0]);
                check_divisor<V,T>(input,copy,d);
            }
            else { const Vec4ui mult(divisor.getm()),shift2(divisor.gets2()); Div copy(mult[0],int(shift[0]),int(shift2[0])); check_divisor<V,T>(input,copy,d); }
        }
        else {
            const Vec4ui shift(divisor.gets1()); for(int i=1;i<4;++i) CHECK(shift[i]==0);
            if constexpr(std::is_signed<D>::value) {
                const Vec8s mult(divisor.getm()),sign(divisor.getsign()); Div copy(mult[0],int(shift[0]),sign[0]); check_divisor<V,T>(input,copy,d);
            }
            else { const Vec8us mult(divisor.getm()); const Vec4ui shift2(divisor.gets2()); Div copy(mult[0],int(shift[0]),int(shift2[0])); check_divisor<V,T>(input,copy,d); }
        }
        if constexpr(sizeof(T)<=2) {
            // Exhaust every lane bit pattern for every selected divisor.
            for(uint32_t start=0;start<(uint32_t(1)<<(sizeof(T)*8));start+=V::size()) {
                for(int lane=0;lane<V::size();++lane) input[lane]=value_bits<T>(start+lane);
                check_divisor<V,T>(input,divisor,d);
            }
        }
    }
    // Exercise every 16-bit divisor with boundary and random dividends. For
    // 32-bit divisor classes sample the full range (and exact quotient edges).
    const uint32_t iterations=sizeof(D)==2?65536:12000;
    for(uint32_t i=0;i<iterations;++i) {
        D d=sizeof(D)==2?value_bits<D>(i):value_bits<D>(random_bits()); if(d==0) continue;
        for(int lane=0;lane<V::size();++lane) input[lane]=lane<4?pattern<T>(lane+int(i%9)):value_bits<T>(random_bits());
        check_divisor<V,T>(input,Div(d),d);
        for(int lane=0;lane<V::size();++lane) {
            using U=typename std::make_unsigned<T>::type;
            input[lane]=value_bits<T>(uint64_t(U(random_bits()%100))*uint64_t(d)+uint64_t(lane%3)-1);
        }
        check_divisor<V,T>(input,Div(d),d);
    }
}
template<class V,class T,int32_t d> void signed_constant() {
    for(int trial=0;trial<300;++trial) {
        T input[V::size()]; for(int i=0;i<V::size();++i) input[i]=trial<9?pattern<T>(trial+i):value_bits<T>(random_bits());
        V a; a.load(input); auto q=a/const_int(d); auto assigned=a; assigned/=const_int(d);
        for(int i=0;i<V::size();++i) { CHECK(q[i]==quotient(input[i],d)); CHECK(assigned[i]==quotient(input[i],d)); }
        if constexpr(sizeof(T)>=2) { auto direct=divide_by_i<d>(a); for(int i=0;i<V::size();++i) CHECK(direct[i]==q[i]); }
    }
}
template<class V,class T,uint32_t d> void unsigned_constant() {
    for(int trial=0;trial<300;++trial) {
        T input[V::size()]; for(int i=0;i<V::size();++i) input[i]=trial<9?pattern<T>(trial+i):value_bits<T>(random_bits());
        V a; a.load(input); auto q=a/const_uint(d); auto assigned=a; assigned/=const_uint(d);
        for(int i=0;i<V::size();++i) { CHECK(q[i]==quotient(input[i],d)); CHECK(assigned[i]==quotient(input[i],d)); }
        if constexpr(sizeof(T)>=2) { auto direct=divide_by_ui<d>(a); for(int i=0;i<V::size();++i) CHECK(direct[i]==q[i]); }
    }
}
template<class V,class T> void signed_constants() {
    signed_constant<V,T,1>(); signed_constant<V,T,-1>(); signed_constant<V,T,2>(); signed_constant<V,T,-2>();
    signed_constant<V,T,3>(); signed_constant<V,T,-3>(); signed_constant<V,T,7>(); signed_constant<V,T,-7>();
    signed_constant<V,T,13>(); signed_constant<V,T,-13>(); signed_constant<V,T,127>(); signed_constant<V,T,-128>();
    signed_constant<V,T,256>(); signed_constant<V,T,32767>(); signed_constant<V,T,-32768>();
    if constexpr(sizeof(T)==4) { signed_constant<V,T,INT32_MAX>(); signed_constant<V,T,INT32_MIN>(); signed_constant<V,T,1073741824>(); }
    V a(T(-17)); CHECK((a/const_uint(3))[0]==T(-5)); a/=const_uint(3); CHECK(a[0]==T(-5));
}
template<class V,class T> void unsigned_constants() {
    unsigned_constant<V,T,1>(); unsigned_constant<V,T,2>(); unsigned_constant<V,T,3>(); unsigned_constant<V,T,7>();
    unsigned_constant<V,T,13>(); unsigned_constant<V,T,27>(); unsigned_constant<V,T,127>(); unsigned_constant<V,T,128>();
    unsigned_constant<V,T,255>(); unsigned_constant<V,T,256>(); unsigned_constant<V,T,32768>(); unsigned_constant<V,T,65535>();
    if constexpr(sizeof(T)==4) { unsigned_constant<V,T,UINT32_MAX>(); unsigned_constant<V,T,2147483648u>(); unsigned_constant<V,T,2147483649u>(); }
    V a(T(17)); CHECK((a/const_int(3))[0]==T(5)); a/=const_int(3); CHECK(a[0]==T(5));
}
template<class I,class F,class IV,class FV> void masks() {
    static_assert(std::is_convertible<I,F>::value);
    for(unsigned bits=0;bits<(1u<<I::size());++bits) {
        I i; i.load_bits(bits); F f=i; I back(f); CHECK(to_bits(f)==bits); CHECK(to_bits(back)==bits);
        F assigned(false); assigned=i; I assigned_back(false); assigned_back=assigned;
        CHECK(to_bits(assigned)==bits); CHECK(to_bits(assigned_back)==bits);
        auto chosen=select(f,FV(1),FV(2)); auto int_chosen=select(back,IV(1),IV(2));
        for(int lane=0;lane<I::size();++lane) { CHECK(chosen[lane]==(bits&(1u<<lane)?1:2)); CHECK(int_chosen[lane]==(bits&(1u<<lane)?1:2)); }
        IV broad(i); for(int lane=0;lane<I::size();++lane) CHECK(broad[lane]==(bits&(1u<<lane)?-1:0));
#if VCL_BACKEND == VCL_BACKEND_NEON
        using U=typename std::conditional<sizeof(IV)==16 && IV::size()==4,uint32_t,uint64_t>::type;
        U lanes[I::size()]; neon_detail::traits<U>::store(lanes,f.native());
        for(int lane=0;lane<I::size();++lane) CHECK(lanes[lane]==(bits&(1u<<lane)?U(~U(0)):U(0)));
#endif
    }
}
#if VCL_BACKEND == VCL_BACKEND_NEON
template<class V,class T> void high_products() {
    constexpr int w=sizeof(T)*8;
    for(int trial=0;trial<2000;++trial) {
        T a[V::size()],b[V::size()]; for(int i=0;i<V::size();++i) { a[i]=trial<9?pattern<T>(trial+i):value_bits<T>(random_bits()); b[i]=trial<9?pattern<T>(trial+i+3):value_bits<T>(random_bits()); }
        V x,y; x.load(a); y.load(b); auto result=neon_detail::mul_high(x,y);
        for(int i=0;i<V::size();++i) {
            uint64_t product;
            if constexpr(std::is_signed<T>::value) product=uint64_t(int64_t(a[i])*int64_t(b[i]));
            else product=uint64_t(a[i])*uint64_t(b[i]);
            CHECK(result[i]==value_bits<T>(product>>w));
        }
    }
}
#endif
}
int main() {
    conversions<Vec16c,Vec8s,int8_t,int16_t>(); conversions<Vec16uc,Vec8us,uint8_t,uint16_t>();
    conversions<Vec8s,Vec4i,int16_t,int32_t>(); conversions<Vec8us,Vec4ui,uint16_t,uint32_t>();
    conversions<Vec4i,Vec2q,int32_t,int64_t>(); conversions<Vec4ui,Vec2uq,uint32_t,uint64_t>();
    runtime_division<Vec4i,int32_t,Divisor_i,int32_t>(); runtime_division<Vec4ui,uint32_t,Divisor_ui,uint32_t>();
    runtime_division<Vec8s,int16_t,Divisor_s,int16_t>(); runtime_division<Vec8us,uint16_t,Divisor_us,uint16_t>();
    runtime_division<Vec16c,int8_t,Divisor_s,int16_t>(); runtime_division<Vec16uc,uint8_t,Divisor_us,uint16_t>();
    signed_constants<Vec4i,int32_t>(); signed_constants<Vec8s,int16_t>(); signed_constants<Vec16c,int8_t>();
    unsigned_constants<Vec4ui,uint32_t>(); unsigned_constants<Vec8us,uint16_t>(); unsigned_constants<Vec16uc,uint8_t>();
    masks<Vec4ib,Vec4fb,Vec4i,Vec4f>(); masks<Vec2qb,Vec2db,Vec2q,Vec2d>();
    CHECK((Vec8s(-17)/const_int(65539))[0]==-5);
    CHECK((Vec8us(65535)/const_uint(65539))[0]==21845);
    CHECK((Vec16c(-17)/const_int(65539))[0]==-5);
#if VCL_BACKEND == VCL_BACKEND_NEON
    high_products<Vec8s,int16_t>(); high_products<Vec8us,uint16_t>(); high_products<Vec4i,int32_t>(); high_products<Vec4ui,uint32_t>();
#endif
    std::printf("%d integer slice failures\n",failures); return failures!=0;
}
