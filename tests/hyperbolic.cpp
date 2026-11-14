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

#include "vectormath_hyp.h"
#include "hyperbolic_reference.h"
#include <algorithm>
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
    switch(op) {
    case 0:return sinh(x); case 1:return cosh(x); case 2:return tanh(x);
    case 3:return asinh(x); case 4:return acosh(x); default:return atanh(x);
    }
}
template<class V> void check_halves(unsigned op,V a,V result) {
    if constexpr(sizeof(V)>16) {
        V combined(evaluate(op,a.get_low()),evaluate(op,a.get_high()));
        for(int i=0;i<V::size();++i) CHECK((std::isnan(combined[i]) && std::isnan(result[i])) || bits(combined[i])==bits(result[i]));
    }
}
template<class T> bool matches(T actual,T expected,uint64_t budget=3) {
    if(std::isnan(expected)) return std::isnan(actual);
    if(std::isinf(expected)) return actual==expected;
    return std::isfinite(actual) && distance(actual,expected)<=budget;
}
template<class V> void reference_cases() {
    using T=decltype(V()[0]); constexpr unsigned width=sizeof(T)*8;
    for(unsigned op=0;op<6;++op) {
        std::vector<HyperbolicReference> cases;
        for(auto c:hyperbolic_reference) if(c.width==width && c.op==op) cases.push_back(c);
        uint64_t worst=0;
        for(size_t offset=0;offset<cases.size();offset+=V::size()) {
            T input[V::size()];
            for(int i=0;i<V::size();++i) input[i]=from_bits<T>(cases[(offset+i)%cases.size()].input);
            V a; a.load(input); V result=evaluate(op,a); check_halves(op,a,result);
            for(int i=0;i<V::size();++i) {
                T expected=from_bits<T>(cases[(offset+i)%cases.size()].expected);
                worst=std::max(worst,distance(result[i],expected));
                if(!matches(result[i],expected)) {
                    if(failures<30) std::fprintf(stderr,"hyperbolic op=%u f%u lanes=%d x=%a got=%a expected=%a ulp=%llu\n",op,width,V::size(),double(input[i]),double(result[i]),double(expected),(unsigned long long)distance(result[i],expected));
                    ++failures;
                }
            }
        }
        std::printf("hyperbolic op=%u f%u lanes=%d Decimal_cases=%zu max_ulp=%llu gate=3\n",op,width,V::size(),cases.size(),(unsigned long long)worst);
    }
}
template<class T> T scalar(unsigned op,T x) {
    switch(op) {
    case 0:return std::sinh(x); case 1:return std::cosh(x); case 2:return std::tanh(x);
    case 3:return std::asinh(x); case 4:return std::acosh(x); default:return std::atanh(x);
    }
}
template<class V> void exceptional_cases() {
    using T=decltype(V()[0]); const T inf=std::numeric_limits<T>::infinity();
    const T normal=std::numeric_limits<T>::min(),tiny=std::numeric_limits<T>::denorm_min();
    const T nan=from_bits<T>(sizeof(T)==4?0x7fc12345ull:0x7ff8123456789abcull);
    const T cutoff=sizeof(T)==4?T(89):T(709.7);
    std::vector<T> values={T(0),-T(0),tiny,-tiny,normal,-normal,T(-1),T(-.5),T(.5),T(1),T(-2),T(2),inf,-inf,nan,-nan,
        std::nextafter(T(1),inf),std::nextafter(T(1),T(0)),std::numeric_limits<T>::max(),-std::numeric_limits<T>::max(),
        cutoff,-cutoff,std::nextafter(cutoff,inf),-std::nextafter(cutoff,inf)};
    for(unsigned op=0;op<6;++op) for(T x:values) {
        // Uniform vectors force all-small/all-large and huge-only paths.
        V uniform=evaluate(op,V(x)); check_halves(op,V(x),uniform);
        for(int lane=0;lane<V::size();++lane) {
            T ordinary=op==4?T(1.25):T(.25);
            V a(ordinary); a.insert(lane,x); V result=evaluate(op,a); T y=result[lane];
            check_halves(op,a,result);
            CHECK((std::isnan(y)&&std::isnan(uniform[lane])) || bits(y)==bits(uniform[lane]));
            for(int i=0;i<V::size();++i) if(i!=lane) CHECK(bits(result[i])==bits(evaluate(op,V(ordinary))[i]));
            T expected=scalar(op,x);
            // VCL deliberately overflows sinh/cosh at its exp helper's
            // conservative cutoffs, before the scalar libm limit.
            if(op<2 && std::abs(x)>=cutoff) expected=op==0?std::copysign(inf,x):inf;
            if(!matches(y,expected)) {
                if(failures<30) std::fprintf(stderr,"special op=%u f%zu x=%a got=%a expected=%a\n",op,sizeof(T)*8,double(x),double(y),double(expected));
                ++failures;
            }
            if(x==0 && op!=1 && op!=4) CHECK(std::signbit(y)==std::signbit(x));
            if(op==4 && x==1) CHECK(y==0 && !std::signbit(y));
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
    std::fesetenv(&saved); std::printf("%d hyperbolic failures\n",failures); return failures!=0;
}
