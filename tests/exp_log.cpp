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

#include "vectormath_exp.h"
#include "exp_log_reference.h"
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
const char* names[]={"exp","expm1","exp2","exp10","log","log1p","log2","log10"};
// Keep the pinned upstream accuracy gates; the Decimal fixtures additionally
// avoid relying on Apple's long double, which has binary64 precision on ARM64.
constexpr uint64_t budgets[]={2,3,3,10,3,2,2,3};
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
    case 0:return exp(x); case 1:return expm1(x); case 2:return exp2(x); case 3:return exp10(x);
    case 4:return log(x); case 5:return log1p(x); case 6:return log2(x); default:return log10(x);
    }
}
template<class T> T cutoff(unsigned op) {
    if constexpr(sizeof(T)==4) return T(op<2?87.3f:op==2?126.f:37.9f);
    else return T(op<2?708.39:op==2?1022.:307.65);
}
template<class V> void check_halves(unsigned op,V input,V result) {
    if constexpr(sizeof(V)>16) {
        V combined(evaluate(op,input.get_low()),evaluate(op,input.get_high()));
        for(int i=0;i<V::size();++i) CHECK(bits(combined[i])==bits(result[i]));
    }
}
template<class V> void reference_cases() {
    using T=decltype(V()[0]); constexpr unsigned width=sizeof(T)*8;
    for(unsigned op=0;op<8;++op) {
        std::vector<ExpLogReference> cases;
        for(auto c:exp_log_reference) if(c.width==width && c.op==op) cases.push_back(c);
        uint64_t worst=0;
        for(size_t offset=0;offset<cases.size();offset+=V::size()) {
            T input[V::size()];
            for(int i=0;i<V::size();++i) input[i]=from_bits<T>(cases[(offset+i)%cases.size()].input);
            V x; x.load(input); V y=evaluate(op,x); check_halves(op,x,y);
            for(int i=0;i<V::size();++i) {
                T expected=from_bits<T>(cases[(offset+i)%cases.size()].expected);
                uint64_t error=distance(y[i],expected); worst=std::max(worst,error);
                if(!std::isfinite(y[i]) || error>budgets[op]) {
                    if(failures<30) std::fprintf(stderr,"%s f%u lanes=%d x=%a got=%a expected=%a ulp=%llu budget=%llu\n",names[op],width,V::size(),double(input[i]),double(y[i]),double(expected),(unsigned long long)error,(unsigned long long)budgets[op]);
                    ++failures;
                }
            }
        }
        std::printf("%s f%u lanes=%d Decimal_cases=%zu max_ulp=%llu budget=%llu\n",names[op],width,V::size(),cases.size(),(unsigned long long)worst,(unsigned long long)budgets[op]);
    }
}
template<class V> void exceptional_cases() {
    using T=decltype(V()[0]); const T inf=std::numeric_limits<T>::infinity();
    const T normal=std::numeric_limits<T>::min(),tiny=std::numeric_limits<T>::denorm_min();
    const T qnan=from_bits<T>(sizeof(T)==4?0x7fc12345ull:0x7ff8123456789abcull);
    std::vector<T> cases={T(0),-T(0),normal,-normal,tiny,-tiny,std::nextafter(normal,T(0)),
        -std::nextafter(normal,T(0)),inf,-inf,qnan,-qnan,T(1),T(-1),T(-2),T(2),
        std::nextafter(T(-1),T(0)),std::nextafter(T(-1),-inf)};
    for(unsigned op=0;op<8;++op) {
        auto inputs=cases;
        if(op<4) {
            T limit=cutoff<T>(op);
            for(T boundary:{-limit,limit}) for(T x:{std::nextafter(boundary,-inf),boundary,std::nextafter(boundary,inf)}) inputs.push_back(x);
        }
        // Rotate every special value through every lane, with ordinary lanes
        // alongside it to exercise horizontal special-case slow paths.
        for(T x:inputs) for(int lane=0;lane<V::size();++lane) {
            V input(T(1)); input.insert(lane,x); V output=evaluate(op,input); T y=output[lane];
            check_halves(op,input,output);
            for(int i=0;i<V::size();++i) if(i!=lane) CHECK(bits(output[i])==bits(evaluate(op,V(T(1)))[i]));
            if(std::isnan(x)) { CHECK(std::isnan(y)); continue; }
            if(op<4) {
                T limit=cutoff<T>(op);
                if(x>=limit) CHECK(y==inf);
                else if(x<=-limit) CHECK(y==(op==1?T(-1):T(0)));
                else if(x==0) {
                    CHECK(y==(op==1?T(0):T(1)));
#ifdef SIGNED_ZERO
                    if(op==1) CHECK(std::signbit(y)==std::signbit(x));
#endif
                } else CHECK(std::isfinite(y));
            } else if(op==5) {
                if(x==T(-1)) CHECK(y==-inf);
                else if(x<T(-1)) CHECK(std::isnan(y));
                else if(x==inf) CHECK(y==inf);
                else if(x==0) {
                    CHECK(y==0);
#ifdef SIGNED_ZERO
                    CHECK(std::signbit(y)==std::signbit(x));
#endif
                } else CHECK(std::isfinite(y));
            } else {
                // Existing broad-mask VCL contract: either sign of a subnormal
                // is treated as zero. Negative normals produce domain NaNs.
                if(x==0 || std::fpclassify(x)==FP_SUBNORMAL) CHECK(y==-inf);
                else if(x<0) CHECK(std::isnan(y));
                else if(x==inf) CHECK(y==inf);
                else if(x==1) CHECK(y==0);
                else CHECK(std::isfinite(y));
            }
        }
    }
}
template<class V> void helpers() {
    using T=decltype(V()[0]);
    // fraction_2 is an internal helper for normal, nonzero inputs. Public
    // functions handle zero/subnormal/Inf/NaN through their special-case paths.
    for(T x:{T(1),T(-1),T(1.5),T(100),std::numeric_limits<T>::min(),std::numeric_limits<T>::max()}) {
        int exponent; T expected=std::frexp(std::abs(x),&exponent);
        auto y=fraction_2(V(x)); for(int i=0;i<V::size();++i) CHECK(y[i]==expected);
    }
    for(int n:{-100,-17,-1,0,1,17,100}) { auto y=vm_pow2n(V(T(n))); for(int i=0;i<V::size();++i) CHECK(y[i]==std::ldexp(T(1),n)); }
}
template<class V> void run() { reference_cases<V>(); exceptional_cases<V>(); helpers<V>(); }
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
    std::fesetenv(&saved); std::printf("%d exp/log failures\n",failures); return failures!=0;
}
