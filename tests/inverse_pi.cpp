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

#include "vectormath_trig.h"
#include "inverse_pi_reference.h"
#include <algorithm>
#include <array>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>
#include <vector>
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
namespace {
int failures;
template<class T> using UInt=std::conditional_t<sizeof(T)==4,uint32_t,uint64_t>;
template<class T> T from_bits(uint64_t b) { UInt<T> u=UInt<T>(b); T x; std::memcpy(&x,&u,sizeof(x)); return x; }
template<class T> UInt<T> bits(T x) { UInt<T> u; std::memcpy(&u,&x,sizeof(x)); return u; }
template<class T> uint64_t distance(T a,T b) {
    if(a==b) return 0;
    constexpr UInt<T> sign=UInt<T>(1)<<(sizeof(T)*8-1);
    auto ordered=[&](T x) { auto u=bits(x); return UInt<T>(u&sign?~u:u|sign); };
    auto x=ordered(a),y=ordered(b); return x>y?uint64_t(x-y):uint64_t(y-x);
}
#define CHECK(...) do { if(!(__VA_ARGS__)) { if(failures<30) std::fprintf(stderr,"line %d: %s\n",__LINE__,#__VA_ARGS__); ++failures; } } while(0)
template<class V> V evaluate(unsigned op,V a,V b) {
    switch(op) {
    case 0:return asin(a); case 1:return acos(a); case 2:return atan(a); case 3:return atan2(a,b);
    case 4:return sinpi(a); case 5:return cospi(a); case 6:return tanpi(a);
    case 7:{ V c; return sincospi(&c,a); } default:{ V c; sincospi(&c,a); return c; }
    }
}
template<class V> void check_halves(unsigned op,V a,V b,V result) {
    if constexpr(sizeof(V)>16) {
        V combined(evaluate(op,a.get_low(),b.get_low()),evaluate(op,a.get_high(),b.get_high()));
        for(int i=0;i<V::size();++i) CHECK((std::isnan(combined[i]) && std::isnan(result[i])) || bits(combined[i])==bits(result[i]));
    }
}
template<class T> bool matches(T actual,T expected,uint64_t budget=0) {
    if(std::isnan(expected)) return std::isnan(actual);
    if(std::isinf(expected)) return actual==expected;
    return std::isfinite(actual) && distance(actual,expected)<=budget;
}
template<class V> void reference_cases() {
    using T=decltype(V()[0]); constexpr unsigned width=sizeof(T)*8;
    for(unsigned op=0;op<9;++op) {
        std::vector<InversePiReference> cases;
        for(auto c:inverse_pi_reference) if(c.width==width && c.op==(op<7?op:op-3)) {
#if VCL_BACKEND != VCL_BACKEND_NEON
            // The inherited path doubles before reducing, so overflow to NaN
            // above max/2 is characterized separately from the native fix.
            if(op>=4 && std::abs(from_bits<T>(c.a))>std::numeric_limits<T>::max()/T(2)) continue;
#endif
            cases.push_back(c);
        }
        uint64_t worst=0; const uint64_t gate=op==6?4:3;
        for(size_t offset=0;offset<cases.size();offset+=V::size()) {
            T first[V::size()],second[V::size()];
            for(int i=0;i<V::size();++i) {
                auto c=cases[(offset+i)%cases.size()]; first[i]=from_bits<T>(c.a); second[i]=from_bits<T>(c.b);
            }
            V a,b; a.load(first); b.load(second); V result=evaluate(op,a,b); check_halves(op,a,b,result);
            for(int i=0;i<V::size();++i) {
                T expected=from_bits<T>(cases[(offset+i)%cases.size()].expected);
                if(std::isfinite(expected) && std::isfinite(result[i])) worst=std::max(worst,distance(result[i],expected));
                if(!matches(result[i],expected,gate)) {
                    if(failures<30) std::fprintf(stderr,"inverse/pi op=%u f%u lanes=%d a=%a b=%a got=%a expected=%a ulp=%llu gate=%llu\n",op,width,V::size(),double(first[i]),double(second[i]),double(result[i]),double(expected),(unsigned long long)distance(result[i],expected),(unsigned long long)gate);
                    ++failures;
                }
                if(op>=7) CHECK((std::isnan(result[i]) && std::isnan(evaluate(op-3,a,b)[i])) || bits(result[i])==bits(evaluate(op-3,a,b)[i]));
            }
        }
        std::printf("inverse/pi op=%u f%u lanes=%d Decimal_cases=%zu max_ulp=%llu gate=%llu\n",op,width,V::size(),cases.size(),(unsigned long long)worst,(unsigned long long)gate);
    }
}
template<class V> void exceptional_cases() {
    using T=decltype(V()[0]); const T inf=std::numeric_limits<T>::infinity();
    const T tiny=std::numeric_limits<T>::denorm_min(),normal=std::numeric_limits<T>::min();
    const T nan=from_bits<T>(sizeof(T)==4?0x7fc12345ull:0x7ff8123456789abcull);
    std::vector<T> values={T(0),-T(0),tiny,-tiny,normal,-normal,T(-1),T(-.5),T(.5),T(1),T(-2),T(2),inf,-inf,nan,-nan,
        std::nextafter(T(1),inf),std::nextafter(T(-1),-inf),std::numeric_limits<T>::max(),-std::numeric_limits<T>::max()};
    for(unsigned op=0;op<9;++op) if(op!=3) for(T x:values) for(int lane=0;lane<V::size();++lane) {
        V a(T(.25)),b(T(1)); a.insert(lane,x); V result=evaluate(op,a,b); T y=result[lane];
        check_halves(op,a,b,result);
        for(int i=0;i<V::size();++i) if(i!=lane) CHECK(bits(result[i])==bits(evaluate(op,V(T(.25)),b)[i]));
        if(std::isnan(x)) CHECK(std::isnan(y));
        else if(op<2 && std::abs(x)>T(1)) CHECK(std::isnan(y));
        else if(op<3) {
            T expected=op==0?std::asin(x):op==1?std::acos(x):std::atan(x);
            CHECK(matches(y,expected,3));
            if(x==0 && op!=1) CHECK(std::signbit(y)==std::signbit(x));
        } else if(std::isinf(x)) CHECK(std::isnan(y));
#if VCL_BACKEND != VCL_BACKEND_NEON
        else if(std::abs(x)>std::numeric_limits<T>::max()/T(2)) CHECK(!std::isfinite(y));
#endif
        else if(x==0) {
            if(op==5||op==8) CHECK(y==T(1));
            else { CHECK(y==0); if(op==4||op==7) CHECK(std::signbit(y)==std::signbit(x)); }
        } else if(op==6 && std::abs(x)==T(.5)) CHECK(y==std::copysign(inf,x));
        else CHECK(std::isfinite(y));
        if(op>=7) CHECK((std::isnan(y)&&std::isnan(evaluate(op-3,a,b)[lane])) || bits(y)==bits(evaluate(op-3,a,b)[lane]));
    }
    // atan2 quadrants, all zero-sign combinations, axes, infinities, NaNs,
    // and subnormal ratios, with each pair rotated through every lane.
    for(T a:values) for(T b:values) for(int lane=0;lane<V::size();++lane) {
        V x(T(.25)),y(T(1)); x.insert(lane,a); y.insert(lane,b); V z=atan2(x,y);
        check_halves(3,x,y,z); T expected=std::atan2(a,b);
        if(!matches(z[lane],expected,3)) {
            if(failures<30) std::fprintf(stderr,"atan2 f%zu a=%a b=%a got=%a expected=%a\n",sizeof(T)*8,double(a),double(b),double(z[lane]),double(expected));
            ++failures;
        }
        if(expected==0) CHECK(std::signbit(z[lane])==std::signbit(expected));
        for(int i=0;i<V::size();++i) if(i!=lane) CHECK(bits(z[i])==bits(atan2(V(T(.25)),V(T(1)))[i]));
    }
    // Exact binary integers/half-integers must not rely on multiplying a large
    // argument by pi. Their values are exact; zero signs are checked only where
    // VCL promises them (sine at input zero and atan2 above).
    const T parity=std::ldexp(T(1),std::numeric_limits<T>::digits-1);
    for(T x:{T(.5),T(1),T(1.5),T(2),T(3),parity-T(.5),parity,parity+T(1),parity*T(2),std::numeric_limits<T>::max()}) for(T sign:{T(-1),T(1)}) {
        T a=x*sign; V input(a); auto s=sinpi(input),c=cospi(input),t=tanpi(input);
#if VCL_BACKEND != VCL_BACKEND_NEON
        if(std::abs(a)>std::numeric_limits<T>::max()/T(2)) { CHECK(!std::isfinite(s[0])); CHECK(!std::isfinite(c[0])); CHECK(!std::isfinite(t[0])); continue; }
#endif
        T reduced=std::remainder(a,T(2));
        if(reduced==0 || std::abs(reduced)==1) { CHECK(s[0]==0); CHECK(c[0]==(reduced==0?T(1):T(-1))); CHECK(t[0]==0); }
        else if(std::abs(reduced)==T(.5)) { CHECK(s[0]==std::copysign(T(1),reduced)); CHECK(c[0]==0); CHECK(t[0]==std::copysign(inf,reduced)); }
    }
}
template<class V> void run() { reference_cases<V>(); exceptional_cases<V>(); }
}
int main() {
    std::fenv_t saved; std::fegetenv(&saved); reset_control_word();
    run<Vec4f>(); run<Vec2d>();
#if MAX_VECTOR_SIZE >= 256
    run<Vec8f>(); run<Vec4d>();
#endif
#if MAX_VECTOR_SIZE >= 512
    run<Vec16f>(); run<Vec8d>();
#endif
    std::fesetenv(&saved); std::printf("%d inverse/pi failures\n",failures); return failures!=0;
}
