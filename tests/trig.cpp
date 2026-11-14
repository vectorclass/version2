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
#include "trig_reference.h"
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
template<class V> V evaluate(unsigned op,V x) {
    switch(op) { case 0:return sin(x); case 1:return cos(x); case 2:return tan(x);
    case 3:{ V c; return sincos(&c,x); } default:{ V c; sincos(&c,x); return c; } }
}
template<class T> T input_limit(unsigned op) {
#if VCL_BACKEND == VCL_BACKEND_NEON || INSTRSET >= 8
    return sizeof(T)==4?T(1.e7):T(1.e15);
#else
    return sizeof(T)==4?T(1.e5):op==2?T(1.e15):T(1.e13);
#endif
}
template<class V> void check_halves(unsigned op,V x,V result) {
    if constexpr(sizeof(V)>16) {
        V combined(evaluate(op,x.get_low()),evaluate(op,x.get_high()));
        for(int i=0;i<V::size();++i) CHECK((std::isnan(combined[i]) && std::isnan(result[i])) || bits(combined[i])==bits(result[i]));
    }
}
template<class V> void reference_cases() {
    using T=decltype(V()[0]); constexpr unsigned width=sizeof(T)*8;
    for(unsigned op=0;op<5;++op) {
        std::vector<TrigReference> cases;
        for(auto c:trig_reference) if(c.width==width && c.op==(op<3?op:op-3)) {
#if VCL_BACKEND != VCL_BACKEND_NEON && INSTRSET < 8
            // Existing non-FMA range reduction is only accuracy-gated over
            // the upstream tested range; its larger cutoff is not a bound.
            if(std::abs(from_bits<T>(c.input))>T(sizeof(T)==4?1.e4:1.e8)) continue;
#endif
            cases.push_back(c);
        }
        double worst=0; uint64_t worst_ulp=0;
        for(size_t offset=0;offset<cases.size();offset+=V::size()) {
            T input[V::size()];
            for(int i=0;i<V::size();++i) input[i]=from_bits<T>(cases[(offset+i)%cases.size()].input);
            V x; x.load(input); V y=evaluate(op,x); check_halves(op,x,y);
            for(int i=0;i<V::size();++i) {
                T expected=from_bits<T>(cases[(offset+i)%cases.size()].expected);
                // Match the upstream near-zero error floor: .01 for FP32,
                // 1 for FP64 when |result| < .01. Else measure in result ULPs.
                T magnitude=std::abs(expected);
                if(magnitude<T(.01)) magnitude=sizeof(T)==4?T(.01):T(1);
                T spacing=std::nextafter(magnitude,std::numeric_limits<T>::infinity())-magnitude;
                double error=std::abs(double(y[i])-double(expected))/double(spacing);
                worst=std::max(worst,error); worst_ulp=std::max(worst_ulp,distance(y[i],expected));
                double gate=op==0?3:4;
#if VCL_BACKEND != VCL_BACKEND_NEON && INSTRSET < 8
                // Existing FP32 non-FMA reduction loses precision immediately
                // beside 3*pi/2. Keep that legacy path characterized separately.
                if(op==2 && sizeof(T)==4) gate=16;
#endif
                if(!std::isfinite(y[i]) || error>gate) {
                    if(failures<30) std::fprintf(stderr,"trig op=%u f%u lanes=%d x=%a got=%a expected=%a normalized_error=%g gate=%g\n",op,width,V::size(),double(input[i]),double(y[i]),double(expected),error,gate);
                    ++failures;
                }
                if(op>=3) CHECK(bits(y[i])==bits(evaluate(op-3,x)[i]));
            }
        }
        std::printf("trig op=%u f%u lanes=%d Decimal_cases=%zu max_normalized_error=%g max_raw_ulp=%llu\n",op,width,V::size(),cases.size(),worst,(unsigned long long)worst_ulp);
    }
}
template<class V> void exceptional_cases() {
    using T=decltype(V()[0]); const T inf=std::numeric_limits<T>::infinity();
    const T tiny=std::numeric_limits<T>::denorm_min(),normal=std::numeric_limits<T>::min();
    const T nan=from_bits<T>(sizeof(T)==4?0x7fc12345ull:0x7ff8123456789abcull);
    for(unsigned op=0;op<5;++op) {
        T limit=input_limit<T>(op);
        std::vector<T> inputs={T(0),-T(0),tiny,-tiny,normal,-normal,inf,-inf,nan,-nan,
            std::numeric_limits<T>::max(),-std::numeric_limits<T>::max()};
        for(T boundary:{-limit,limit}) for(T x:{std::nextafter(boundary,-inf),boundary,std::nextafter(boundary,inf)}) inputs.push_back(x);
        for(T x:inputs) for(int lane=0;lane<V::size();++lane) {
            V a(T(.25)); a.insert(lane,x); V result=evaluate(op,a); T y=result[lane];
            check_halves(op,a,result);
            for(int i=0;i<V::size();++i) if(i!=lane) CHECK(bits(result[i])==bits(evaluate(op,V(T(.25)))[i]));
            if(std::isnan(x)) CHECK(std::isnan(y));
            else if(std::abs(x)>limit || std::abs(x)<=normal) {
                bool cosine=op==1||op==4;
                T expected=cosine?T(1):std::abs(x)>limit?std::copysign(T(0),x):x;
                CHECK(bits(y)==bits(expected));
            } else CHECK(std::isfinite(y));
            if(op>=3 && !std::isnan(x)) CHECK(bits(y)==bits(evaluate(op-3,a)[lane]));
        }
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
    std::fesetenv(&saved); std::printf("%d trig failures\n",failures); return failures!=0;
}
