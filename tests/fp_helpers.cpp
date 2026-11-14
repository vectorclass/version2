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
#include "vectormath_common.h"
#include <array>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
Vec4f fp_helpers_abi(Vec4f,Vec4f,Vec4f,Vec4fb);
Vec2d fp_helpers_abi(Vec2d,Vec2d,Vec2d,Vec2db);
namespace {
int failures;
#define CHECK(x) do { if(!(x)) { if(failures<25) std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
template<class T,class U> T bits(U x) { static_assert(sizeof(T)==sizeof(U)); T y; std::memcpy(&y,&x,sizeof(y)); return y; }
template<class T> using UInt=typename std::conditional<sizeof(T)==4,uint32_t,uint64_t>::type;
template<class T> bool equal_bits(T a,T b) { return bits<UInt<T>>(a)==bits<UInt<T>>(b); }
template<class V,class M> void scalar_select() {
    using T=decltype(V()[0]);
#if VCL_ARCH_ARM64 || INSTRSET < 10
    static_assert(std::is_same<decltype(select(M(true),1,0)),V>::value);
#endif
    // Taking the overload's address requires ordinary namespace lookup too.
    V (*choose)(M,V,V)=select;
    for(unsigned mask=0;mask<(1u<<V::size());++mask) {
        M m; m.load_bits(mask);
#if VCL_ARCH_ARM64 || INSTRSET < 10
        const V integers=select(m,1,0),scalars=select(m,T(3),T(-2));
#else
        // Compact x86 masks are shared between FP and integer shapes, so
        // both scalar choices need a typed overload to identify the result.
        const V integers=choose(m,1,0),scalars=choose(m,T(3),T(-2));
#endif
        const V zeros=choose(m,V(-T(0)),V(T(0)));
        for(int i=0;i<V::size();++i) {
            const bool selected=(mask>>i)&1;
            CHECK(integers[i]==T(selected));
            CHECK(scalars[i]==(selected?T(3):T(-2)));
            CHECK(equal_bits(zeros[i],selected?-T(0):T(0)));
        }
    }
}
uint64_t state=0x12344321abcdef01ull;
uint64_t random_bits() { state^=state<<13; state^=state>>7; state^=state<<17; return state; }
template<class V,class T> void bit_helpers() {
    using U=UInt<T>; constexpr int fraction_bits=sizeof(T)==4?23:52,bias=sizeof(T)==4?127:1023;
    constexpr U sign=U(1)<<(sizeof(T)*8-1),inf=sizeof(T)==4?U(0x7f800000u):U(0x7ff0000000000000ull),frac=sizeof(T)==4?U(0x007fffffu):U(0x000fffffffffffffull);
    U special[]={0,sign,1,sign|1,frac,frac+1,inf,inf|sign,inf|U(1),inf|U(0x1234),inf-U(1)};
    for(int trial=0;trial<2000;++trial) {
        const U raw=trial<int(sizeof(special)/sizeof(U))?special[trial]:U(random_bits()); const T x=bits<T>(raw);
        V a(x); CHECK(equal_bits((-a)[0],bits<T>(raw^sign))); CHECK(equal_bits(abs(a)[0],bits<T>(raw&~sign))); CHECK(horizontal_and(sign_bit(a))==bool(raw&sign));
        auto combined=sign_combine(a,V(T(-0.0))); CHECK(equal_bits(combined[0],bits<T>(raw^sign)));
        auto extracted=exponent(a); CHECK(extracted[0]==int((raw&inf)>>fraction_bits)-bias);
#if VCL_ARCH_ARM64 || INSTRSET < 10
        CHECK(equal_bits(fraction(a)[0],bits<T>((raw&frac)|(U(bias)<<fraction_bits))));
#endif
        auto cls=is_finite(a); CHECK(horizontal_and(cls)==((raw&inf)!=inf));
        CHECK(horizontal_and(is_nan(a))==((raw&inf)==inf && (raw&frac)!=0));
        CHECK(horizontal_and(is_inf(a))==((raw&~sign)==inf));
        CHECK(horizontal_and(is_subnormal(a))==((raw&inf)==0 && (raw&frac)!=0));
        CHECK(horizontal_and(is_zero_or_subnormal(a))==((raw&inf)==0));
    }
    for(uint32_t payload: {0u,1u,0x10u,0x100u,0x123456u,UINT32_MAX}) {
        V q=nan_vec<V>(payload); U expected;
        if constexpr(sizeof(T)==4) expected=U(0x7fc00000u)|(payload&0x003fffffu);
        else expected=U(0x7ff8000000000000ull)|(uint64_t(payload)<<29);
        CHECK(equal_bits(q[0],bits<T>(expected)));
        CHECK(nan_code(q)[0]==(sizeof(T)==4?uint64_t(expected&frac):uint64_t((expected&frac)>>29)));
        CHECK(nan_code(V(T(1)))[0]==0);
    }
    CHECK(equal_bits(infinite_vec<V>()[0],bits<T>(inf)));
    for(int n=-bias-3;n<=bias+4;++n) {
        auto result=exp2(typename std::conditional<sizeof(T)==4,Vec4i,Vec2q>::type(n));
        const int exponent_field=std::max(0,std::min(2*bias+1,n+bias));
        CHECK(equal_bits(result[0],bits<T>(U(exponent_field)<<fraction_bits)));
    }
    for(int k=0;k<V::size();++k) {
        V a(T(2)); auto returned=a.insert(k,T(7)); CHECK(a[k]==T(7) && returned[k]==T(7));
        for(int i=0;i<V::size();++i) CHECK(a[i]==(i==k?T(7):T(2)));
        a.cutoff(k); for(int i=k;i<V::size();++i) CHECK(equal_bits(a[i],T(0)));
    }
    V a(T(3)); V before=a++; CHECK(before[0]==T(3) && a[0]==T(4)); CHECK((++a)[0]==T(5));
    before=a--; CHECK(before[0]==T(5) && a[0]==T(4)); CHECK((--a)[0]==T(3));
    auto not_zero=!V(T(-0.0)); CHECK(horizontal_and(not_zero)); CHECK(!horizontal_or(!V(T(1))));
    V masked=V(T(1)); masked^=V(T(-0.0)); CHECK(masked[0]==T(-1)); masked&=V(T(-0.0)); CHECK(equal_bits(masked[0],T(-0.0)));
    masked|=V(T(1)); CHECK(masked[0]==T(-1));
    static_assert(std::is_same<decltype(std::declval<V&>().insert(0,T(1))),const V>::value);
    alignas(16) T storage[V::size()]={}; V loaded; loaded.load_a(storage); loaded.store_a(storage); loaded.store_nt(storage);
    static_assert(std::is_same<decltype(std::declval<Vec4f&>().load_a(static_cast<float const*>(nullptr))),Vec4f&>::value);
    static_assert(std::is_same<decltype(std::declval<Vec2d&>().load_a(static_cast<double const*>(nullptr))),const Vec2d>::value);
}
template<class V,class T> void numerical_helpers() {
    for(T x:{T(-3),T(-1.25),T(-0.5),T(0),T(0.5),T(1.25),T(3)}) {
        V a(x); CHECK(max(a,T(2))[0]==std::max(x,T(2))); CHECK(min(a,T(-2))[0]==std::min(x,T(-2)));
        CHECK(if_add(typename std::conditional<sizeof(T)==4,Vec4fb,Vec2db>::type(false),a,T(3))[0]==x);
        CHECK(if_mul(typename std::conditional<sizeof(T)==4,Vec4fb,Vec2db>::type(true),a,T(3))[0]==x*T(3));
        for(int n: {-13,-7,-1,0,1,2,3,7,13}) {
            if(x==T(0) && n<0) continue;
            T expected=T(std::pow(static_cast<long double>(x),n));
            T actual=pow(a,n)[0]; CHECK(std::abs(actual-expected)<=std::abs(expected)*T(8)*std::numeric_limits<T>::epsilon());
        }
        CHECK(pow(a,uint32_t(0))[0]==T(1)); CHECK(pow_const(a,0)[0]==T(1));
        CHECK(pow<int>(a,2)[0]==x*x); CHECK(pow<uint32_t>(a,2u)[0]==x*x);
        CHECK(equal_bits(pow_const(a,1)[0],x)); CHECK(pow_const(a,6)[0]==((x*x)*(x*x))*(x*x));
        if(x!=T(0)) CHECK(pow_const(a,-3)[0]==T(1)/(x*(x*x)));
        CHECK(horizontal_and(is_nan(pow(a,INT32_MIN)))); CHECK(horizontal_and(is_nan(pow_const(a,INT32_MIN))));
        CHECK(polynomial_3(a,T(1),T(2),T(3),T(4))[0]==T(1)+T(2)*x+T(3)*x*x+T(4)*x*x*x);
        CHECK(polynomial_13(a,T(1),T(0),T(0),T(0),T(0),T(0),T(0),T(0),T(0),T(0),T(0),T(0),T(0),T(0))[0]==T(1));
        for(double d: {1.0,2.5,7.0}) {
            T mod=fmodulo(a,d)[0]; T rem=fremainder(a,d)[0];
            const T m=T(static_cast<long double>(x)-std::floor(static_cast<long double>(x)/d)*d);
            const T r=T(static_cast<long double>(x)-std::floor(static_cast<long double>(x)/d+0.5L)*d);
            CHECK(mod==m); CHECK(rem==r);
        }
    }
#if VCL_ARCH_ARM64 || INSTRSET < 10
    using M=typename std::conditional<sizeof(T)==4,Vec4fb,Vec2db>::type;
    CHECK(equal_bits(if_add(M(false),V(T(-0.0)),T(5))[0],T(0)));
#endif
    V q=nan_vec<V>(123),a(T(2)); CHECK(horizontal_and(is_nan(maximum(q,a)))); CHECK(horizontal_and(is_nan(minimum(a,q))));
    CHECK(pow(V(T(-1)),INT32_MAX)[0]==T(-1)); CHECK(pow(V(T(-1)),INT32_MIN+1)[0]==T(-1));
    CHECK(equal_bits(pow_const(V(T(-0.0)),1)[0],T(-0.0)));
    CHECK(horizontal_and(is_nan(fremainder(a,0)))); CHECK(horizontal_and(is_nan(fmodulo(a,-1))));
    V data; T lanes[V::size()]; for(int i=0;i<V::size();++i) lanes[i]=T(i+1); data.load(lanes);
    CHECK(horizontal_min(data)==T(1)); CHECK(horizontal_max(data)==T(V::size()));
    data.insert(V::size()-1,q[0]); CHECK(std::isnan(horizontal_min(data))); CHECK(std::isnan(horizontal_max(data)));
#ifdef SIGNED_ZERO
    CHECK(equal_bits(maximum(V(T(-0.0)),V(T(0)))[0],T(0)));
    CHECK(equal_bits(minimum(V(T(0)),V(T(-0.0)))[0],T(-0.0)));
#endif
    for(int trial=0;trial<2000;++trial) {
        T x=T(int(random_bits()%20000)-10000)/T(127),y=T(int(random_bits()%20000)-10000)/T(131),z=x*y;
        T result=mul_sub_x(V(x),V(y),V(z))[0]; T expected=std::fma(x,y,-z);
#if VCL_BACKEND == VCL_BACKEND_NEON || defined(__FMA__)
        CHECK(result==expected);
#else
        CHECK(std::abs(result-expected)<=T(8)*std::numeric_limits<T>::epsilon()*std::abs(x*y));
#endif
        auto out=fp_helpers_abi(V(x),V(y),V(z),typename std::conditional<sizeof(T)==4,Vec4fb,Vec2db>::type(true)); CHECK(out[0]==result);
    }
}
template<class Mask,class IV> void masks() {
    constexpr unsigned all=(1u<<Mask::size())-1;
    using Expected=typename std::conditional<Mask::size()==16,uint16_t,uint8_t>::type;
    static_assert(std::is_same<decltype(to_bits(Mask())),Expected>::value);
    for(unsigned i=0;i<=all;++i) {
        Mask m; m.load_bits(Expected(i)); CHECK(to_bits(m)==i); CHECK(horizontal_count(m)==__builtin_popcount(i));
        CHECK(horizontal_find_first(m)==(i?__builtin_ctz(i):-1)); CHECK(horizontal_and(m)==(i==all)); CHECK(horizontal_or(m)==bool(i));
        CHECK(to_bits(!m)==(all^i)); CHECK(horizontal_and(m==m)); CHECK(!horizontal_or(m!=m));
        IV broad(m);
#if VCL_ARCH_ARM64 || INSTRSET < 10
        Mask back(broad); CHECK(to_bits(back)==i);
#endif
        CHECK(horizontal_min(m)==(i==all)); CHECK(horizontal_max(m)==bool(i));
        for(int lane=0;lane<Mask::size();++lane) CHECK(broad[lane]==(i&(1u<<lane)?-1:0));
        m.insert(0,true); CHECK(to_bits(m)==(i|1)); m.insert(0,false); CHECK(to_bits(m)==(i&~1u));
        m&=Mask(false); CHECK(!horizontal_or(m)); m|=Mask(true); CHECK(horizontal_and(m)); m^=Mask(true); CHECK(!horizontal_or(m));
    }
}
void conversions() {
    const double inputs[]={-2147483649.,-2147483648.5,-2147483648.,-2.5,-1.5,-0.0,0.0,1.5,2.5,2147483647.,2147483647.9,2147483648.,INFINITY,-INFINITY,NAN};
    for(double x:inputs) {
        auto a=truncate_to_int32(Vec2d(x)); auto b=truncate_to_int32(Vec2d(x),Vec2d(1.75));
        const long double t=std::trunc(static_cast<long double>(x));
        const int expected=std::isnan(x)||t<INT32_MIN||t>INT32_MAX?INT32_MIN:int(t);
        CHECK(a[0]==expected && a[1]==expected && a[2]==0 && a[3]==0); CHECK(b[0]==expected && b[1]==expected && b[2]==1 && b[3]==1);
    }
    const int expect[4][4]={{-2,-2,2,2},{-3,-2,1,2},{-2,-1,2,3},{-2,-1,1,2}};
    for(unsigned mode=0;mode<4;++mode) { set_control_word(0x1f80u|(mode<<13)); auto r=round_to_int32(Vec2d(-2.5,-1.5),Vec2d(1.5,2.5)); for(int lane=0;lane<4;++lane) CHECK(r[lane]==expect[mode][lane]); }
    reset_control_word(); auto f=to_float(Vec2d(1.25,-2.5)); CHECK(f[0]==1.25f && f[1]==-2.5f && equal_bits(f[2],0.f) && equal_bits(f[3],0.f));
    Vec16c raw=Vec16c(-1); Vec4f rf= reinterpret_f(raw); Vec2d rd=reinterpret_d(raw); Vec4i back(reinterpret_i(rf)); Vec2q backd(reinterpret_i(rd));
    CHECK(horizontal_and(back==Vec4i(-1))); CHECK(horizontal_and(backd==Vec2q(-1)));
    CHECK(equal_bits(reinterpret_f(rf)[0],rf[0])); CHECK(equal_bits(reinterpret_d(rd)[0],rd[0]));
    auto perm=permute4<3,2,1,0>(Vec4i(Vec4fb(true,false,false,true))); Vec4i p(perm); CHECK(p[0]==-1 && p[1]==0 && p[2]==0 && p[3]==-1);
}
template<class V,class T> void rotations() {
    using U=typename std::make_unsigned<T>::type; constexpr int width=sizeof(T)*8;
    for(int count: {0,1,-1,width-1,width,width+1,INT32_MIN,INT32_MAX}) for(int trial=0;trial<100;++trial) {
        U raw=U(random_bits()); T value=bits<T>(raw); auto result=rotate_left(V(value),count);
        unsigned n=uint32_t(count)&(width-1); U expected=n?U((uint64_t(raw)<<n)|(uint64_t(raw)>>(width-n))):raw;
        CHECK(result[0]==bits<T>(expected));
    }
}
void raw_helpers() {
    Vec128b a=Vec4i(-1),zero=Vec4i(0); CHECK(horizontal_and(a)); CHECK(!horizontal_or(zero));
    CHECK(!horizontal_or(andnot(a,a))); a^=a; CHECK(!horizontal_or(a)); a|=Vec4i(-1); CHECK(horizontal_and(a)); a&=zero; CHECK(!horizontal_or(a));
    Vec128b choice=selectb(Vec128b(Vec4i(-1)),Vec128b(Vec4i(7)),zero); Vec4i result(choice); CHECK(horizontal_and(result==Vec4i(7)));
    Vec16c lanes(-1); lanes.insert(3,0); CHECK(!horizontal_and(Vec128b(lanes))); CHECK(horizontal_or(Vec128b(lanes)));
    Vec4f f(1); Vec2q words=reinterpret_i(f); Vec16c bytes=reinterpret_i(f); CHECK(equal_bits(reinterpret_f(words)[0],f[0])); CHECK(equal_bits(reinterpret_f(bytes)[0],f[0]));
    Vec4i ints=reinterpret_i(words); CHECK(horizontal_and(Vec4ui(ints)==Vec4ui(0x3f800000u)));
#if VCL_BACKEND == VCL_BACKEND_NEON
    Vec4f via_native; via_native=f.native(); CHECK(via_native[0]==1.f);
    Vec4i iv(3),iv_copy; iv_copy=iv.native(); CHECK(iv_copy[0]==3);
#endif
}
bool reciprocal_matches(float result,double reference) {
#if VCL_BACKEND == VCL_BACKEND_X86 && INSTRSET < 9
    // RCPPS flushes tiny estimates independently of FTZ. Near the normal/
    // subnormal boundary its allowed approximation error can straddle the
    // threshold. Accept zero only there or below, with the correct sign;
    // require zero once the entire permitted interval is subnormal.
    constexpr double smallest=0x1p-126,hardware_error=0x1.8p-12;
    const double magnitude=std::abs(reference);
    if(magnitude*(1+hardware_error)<smallest)
        return equal_bits(result,std::signbit(reference)?-0.f:0.f);
    if(result==0.f)
        return magnitude*(1-hardware_error)<smallest && equal_bits(result,std::signbit(reference)?-0.f:0.f);
    if(std::abs(result)<std::numeric_limits<float>::min())return false;
#endif
    return std::abs((double(result)-reference)/reference)<std::ldexp(1.0,-11);
}
void approximations() {
    for(unsigned exponent_bits=1;exponent_bits<255;++exponent_bits) for(unsigned mantissa: {0u,1u,0x12345u,0x3fffffu,0x7fffffu}) {
        float x=bits<float>((exponent_bits<<23)|mantissa);
        const double inv=1.0/double(x),rsqrt=1.0/std::sqrt(double(x));
        auto r=approx_recipr(Vec4f(x)); auto s=approx_rsqrt(Vec4f(x));
        CHECK(reciprocal_matches(r[0],inv)); CHECK(std::abs((double(s[0])-rsqrt)/rsqrt)<std::ldexp(1.0,-11));
        auto negative=approx_recipr(Vec4f(-x)); CHECK(reciprocal_matches(negative[0],-inv));
    }
#if VCL_BACKEND == VCL_BACKEND_NEON
    for(uint32_t raw: {0u,0x80000000u,1u,0x007fffffu,0x7f800000u,0xff800000u,0x7fc12345u,0xff800001u}) {
        float x=bits<float>(raw); auto r=approx_recipr(Vec4f(x)),s=approx_rsqrt(Vec4f(x));
        if((raw&0x7fffffffu)==0) { CHECK(bits<uint32_t>(r[0])==(raw|0x7f800000u)); CHECK(bits<uint32_t>(s[0])==(raw|0x7f800000u)); }
        else if((raw&0x7fffffffu)>0x7f800000u) { CHECK(bits<uint32_t>(r[0])==(raw|0x00400000u)); CHECK(bits<uint32_t>(s[0])==(raw|0x00400000u)); }
        else if(raw==0x7f800000u) { CHECK(equal_bits(r[0],0.f)); CHECK(equal_bits(s[0],0.f)); }
        else if(raw==0xff800000u) { CHECK(equal_bits(r[0],-0.f)); CHECK(std::isnan(s[0])); }
        else { CHECK(r[0]==1.f/x); CHECK(std::abs(double(s[0])*std::sqrt(double(x))-1)<std::ldexp(1.0,-11)); }
    }
    std::feclearexcept(FE_ALL_EXCEPT);
    const float tiny=bits<float>(1u); auto mixed=approx_recipr(Vec4f(0,tiny,-4,INFINITY));
    CHECK(std::isinf(mixed[0]) && std::isinf(mixed[1]) && std::abs(mixed[2]+0.25f)<std::ldexp(1.f,-13) && equal_bits(mixed[3],0.f));
    auto mixed_sqrt=approx_rsqrt(Vec4f(0,tiny,4,-4));
    CHECK(std::isinf(mixed_sqrt[0]) && std::abs(double(mixed_sqrt[1])*std::sqrt(double(tiny))-1)<std::ldexp(1.0,-11));
    CHECK(std::abs(mixed_sqrt[2]-0.5f)<std::ldexp(1.f,-12) && std::isnan(mixed_sqrt[3]));
    CHECK((std::fetestexcept(FE_DIVBYZERO|FE_INVALID))==0);
    std::feclearexcept(FE_ALL_EXCEPT);
    auto inactive_div=if_div(Vec4fb(false),Vec4f(1),Vec4f(0)); auto inactive_mul=if_mul(Vec4fb(false),Vec4f(0),infinite4f());
    CHECK(inactive_div[0]==1 && inactive_mul[0]==0); CHECK((std::fetestexcept(FE_DIVBYZERO|FE_INVALID))==0);
    std::feclearexcept(FE_ALL_EXCEPT); volatile float zero=0.f; auto r=approx_rsqrt(Vec4f(zero)); auto q=approx_recipr(Vec4f(zero));
    CHECK(std::isinf(r[0]) && std::isinf(q[0])); CHECK((std::fetestexcept(FE_DIVBYZERO|FE_INVALID))==0);
#endif
}
}
int main() {
    std::fenv_t saved; std::fegetenv(&saved); reset_control_word();
    scalar_select<Vec4f,Vec4fb>(); scalar_select<Vec2d,Vec2db>();
    bit_helpers<Vec4f,float>(); bit_helpers<Vec2d,double>(); numerical_helpers<Vec4f,float>(); numerical_helpers<Vec2d,double>();
    masks<Vec16cb,Vec16c>(); masks<Vec8sb,Vec8s>(); masks<Vec4ib,Vec4i>(); masks<Vec2qb,Vec2q>(); masks<Vec4fb,Vec4i>(); masks<Vec2db,Vec2q>();
    conversions(); rotations<Vec16c,int8_t>(); rotations<Vec8s,int16_t>(); rotations<Vec4i,int32_t>(); rotations<Vec2q,int64_t>(); raw_helpers(); approximations(); std::fesetenv(&saved);
    std::printf("%d FP/helper slice failures\n",failures); return failures!=0;
}
