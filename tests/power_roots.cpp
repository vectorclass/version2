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
#include "power_roots_reference.h"
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
// Pow uses the upstream two-ULP baseline; roots use its five-ULP gate.
// Rational compositions have a separate bound for accumulated root/power error.
constexpr uint64_t budget(unsigned op) { return op==0?2:op<4?5:32; }
template<class V> V evaluate(unsigned op,V x,V y) {
    switch(op) {
    case 0:return pow(x,y); case 1:return cbrt(x);
    case 2:return reciprocal_cbrt(x); case 3:return square_cbrt(x);
    default:return rational_reference_operation(op,x);
    }
}
template<class V> void check_halves(unsigned op,V x,V y,V result) {
    if constexpr(sizeof(V)>16) {
        V combined(evaluate(op,x.get_low(),y.get_low()),evaluate(op,x.get_high(),y.get_high()));
        for(int i=0;i<V::size();++i) if(!(std::isnan(combined[i]) && std::isnan(result[i])) && bits(combined[i])!=bits(result[i])) {
            if(failures<30) std::fprintf(stderr,"half mismatch op=%u lanes=%d x=%a y=%a wide=%a half=%a wide_bits=%llx half_bits=%llx\n",op,V::size(),double(x[i]),double(y[i]),double(result[i]),double(combined[i]),(unsigned long long)bits(result[i]),(unsigned long long)bits(combined[i]));
            ++failures;
        }
    }
}
template<class V> void reference_cases() {
    using T=decltype(V()[0]); constexpr unsigned width=sizeof(T)*8;
    for(unsigned op=0;op<power_roots_operations;++op) {
        std::vector<PowerRootsReference> cases;
        for(auto c:power_roots_reference) if(c.width==width && c.op==op) cases.push_back(c);
        uint64_t worst=0;
        for(size_t offset=0;offset<cases.size();offset+=V::size()) {
            T input[V::size()],exponent[V::size()];
            for(int i=0;i<V::size();++i) {
                auto c=cases[(offset+i)%cases.size()];
                input[i]=from_bits<T>(c.x); exponent[i]=from_bits<T>(c.y);
            }
            V x,y; x.load(input); y.load(exponent); V z=evaluate(op,x,y); check_halves(op,x,y,z);
            for(int i=0;i<V::size();++i) {
                T expected=from_bits<T>(cases[(offset+i)%cases.size()].expected);
                uint64_t error=distance(z[i],expected); worst=std::max(worst,error);
                if(!std::isfinite(z[i]) || error>budget(op)) {
                    if(failures<30) std::fprintf(stderr,"op=%u f%u lanes=%d x=%a y=%a got=%a expected=%a ulp=%llu budget=%llu\n",op,width,V::size(),double(input[i]),double(exponent[i]),double(z[i]),double(expected),(unsigned long long)error,(unsigned long long)budget(op));
                    ++failures;
                }
                if(op==0) {
                    CHECK(bits(pow(V(input[i]),exponent[i])[0])==bits(z[i]));
                    if constexpr(sizeof(T)==4) CHECK(bits(pow(V(input[i]),double(exponent[i]))[0])==bits(z[i]));
                    else if(T(float(exponent[i]))==exponent[i]) CHECK(bits(pow(V(input[i]),float(exponent[i]))[0])==bits(z[i]));
                }
            }
        }
        std::printf("op=%u f%u lanes=%d Decimal_cases=%zu max_ulp=%llu budget=%llu\n",op,width,V::size(),cases.size(),(unsigned long long)worst,(unsigned long long)budget(op));
    }
}
template<class T> void check_special(T actual,T expected) {
    if(std::isnan(expected)) CHECK(std::isnan(actual));
    else { CHECK(actual==expected); if(actual==0 || std::isinf(expected)) CHECK(std::signbit(actual)==std::signbit(expected)); }
}
template<class V> void exceptional_cases() {
    using T=decltype(V()[0]); const T inf=std::numeric_limits<T>::infinity();
    const T normal=std::numeric_limits<T>::min(),tiny=std::numeric_limits<T>::denorm_min();
    const T qnan=from_bits<T>(sizeof(T)==4?0x7fc12345ull:0x7ff8123456789abcull);
    const T parity_limit=std::ldexp(T(1),std::numeric_limits<T>::digits);
    std::vector<T> bases={T(0),-T(0),tiny,-tiny,std::nextafter(normal,T(0)),
        -std::nextafter(normal,T(0)),normal,-normal,T(.5),T(-.5),T(1),T(-1),T(2),T(-2),inf,-inf,qnan,-qnan};
    std::vector<T> exponents={-inf,T(-4),T(-3),T(-1),T(-.5),-T(0),T(0),T(.5),T(1),T(2),T(3),T(4),inf,qnan,-qnan,
        parity_limit-1,parity_limit,std::nextafter(parity_limit,inf),-parity_limit+1,-parity_limit,
        std::numeric_limits<T>::max()};
    for(T boundary:{T(std::numeric_limits<T>::min_exponent-1),T(std::numeric_limits<T>::max_exponent)}) {
        exponents.push_back(boundary-T(1)); exponents.push_back(boundary);
        exponents.push_back(std::nextafter(boundary,-inf)); exponents.push_back(std::nextafter(boundary,inf));
    }
    for(T x:bases) for(T y:exponents) {
        T expected;
        // VCL always propagates NaNs, treats subnormal bases as zero,
        // rejects negative nonzero finite bases with fractional exponents,
        // and flushes subnormal results to signed zero.
        if(std::isnan(x)||std::isnan(y)) expected=qnan;
        else if(std::fpclassify(x)==FP_SUBNORMAL && x<0 && std::isfinite(y) && y!=std::round(y)) expected=qnan;
        else {
            T base=std::fpclassify(x)==FP_SUBNORMAL?std::copysign(T(0),x):x;
            expected=std::pow(base,y);
            if(std::fpclassify(expected)==FP_SUBNORMAL) expected=std::copysign(T(0),expected);
        }
        for(int lane=0;lane<V::size();++lane) {
            V a(T(2)),b(T(3)); a.insert(lane,x); b.insert(lane,y);
            V z=pow(a,b); check_halves(0,a,b,z);
            // Compare exceptional classes/signs exactly; finite pow results retain
            // the two-ULP gate (fractional exponents can produce irrationals).
            int before=failures;
            if(std::isnormal(expected)) CHECK(std::isfinite(z[lane]) && distance(z[lane],expected)<=2);
            else check_special(z[lane],expected);
            if(failures!=before && failures<30) std::fprintf(stderr,"pow special f%zu x=%a y=%a got=%a expected=%a\n",sizeof(T)*8,double(x),double(y),double(z[lane]),double(expected));
            for(int i=0;i<V::size();++i) if(i!=lane) CHECK(z[i]==T(8));
        }
    }
    for(T x:{T(0),-T(0),tiny,-tiny,normal,-normal,std::nextafter(normal,T(0)),-std::nextafter(normal,T(0)),inf,-inf,qnan,-qnan}) {
        for(unsigned op=1;op<4;++op) for(int lane=0;lane<V::size();++lane) {
            V a(T(8)); a.insert(lane,x); V z=evaluate(op,a,V(T(0))); check_halves(op,a,V(T(0)),z);
            T expected;
            if(std::isnan(x)) expected=qnan;
            else if(op==2) expected=std::copysign(std::isinf(x)?T(0):inf,x);
            else if(op==3) expected=std::isinf(x)?inf:T(0);
            else {
                expected=std::isinf(x)?x:T(0);
#ifdef SIGNED_ZERO
                expected=std::copysign(expected,x);
#endif
            }
            check_special(z[lane],expected);
            for(int i=0;i<V::size();++i) if(i!=lane) CHECK(distance(z[i],T(op==1?2:op==2?.5:4))<=5);
        }
    }
    // These composed reciprocals retain VCL's 0/0 and Inf/Inf behavior.
    for(T x:{T(0),inf}) {
        V a(x);
        CHECK(std::isnan(pow_ratio(a,-3,4)[0]));
        CHECK(std::isnan(pow_ratio(a,-5,6)[0]));
        CHECK(std::isnan(pow_ratio(a,-7,8)[0]));
    }
    // Specialized rational roots share the same special-value policies.
    for(T x:{T(0),-T(0),T(-8),T(8),inf,-inf,qnan}) {
        V a(x);
        auto same=[&](V first,V second) { for(int i=0;i<V::size();++i) check_special(first[i],second[i]); };
        same(pow_ratio(a,1,3),cbrt(a)); same(pow_ratio(a,2,3),square_cbrt(a));
        same(pow_ratio(a,-1,3),reciprocal_cbrt(a));
        same(pow_ratio(a,6,18),cbrt(a)); same(pow_ratio(a,2,-6),reciprocal_cbrt(a));
        CHECK(pow_ratio(a,0,1)[0]==T(1));
        if(x<0) { CHECK(std::isnan(pow_ratio(a,1,2)[0])); CHECK(std::isnan(pow_ratio(a,-1,4)[0])); CHECK(std::isnan(pow_ratio(a,5,6)[0])); CHECK(std::isnan(pow_ratio(a,3,8)[0])); }
    }
}
template<class V> void conversion_edges() {
    using T=decltype(V()[0]);
    if constexpr(sizeof(T)==8) {
        const T limit=9223372036854775808.0,inf=std::numeric_limits<T>::infinity();
        for(T x:{-limit,std::nextafter(-limit,-inf),std::nextafter(-limit,T(0)),
                 std::nextafter(limit,T(0)),limit,inf,-inf,std::numeric_limits<T>::quiet_NaN(),
                 T(-2.5),T(-1.75),T(1.75),T(2.5)}) for(int lane=0;lane<V::size();++lane) {
            V a(T(7)); a.insert(lane,x);
            auto truncated=truncatei(a),rounded=roundi(a);
            auto expected=[&](T v) -> int64_t { return v>=-limit && v<limit?int64_t(v):INT64_MIN; };
            CHECK(truncated[lane]==expected(x)); CHECK(rounded[lane]==expected(std::nearbyint(x)));
            for(int i=0;i<V::size();++i) if(i!=lane) { CHECK(truncated[i]==7); CHECK(rounded[i]==7); }
        }
    }
}
template<class V> void run() { reference_cases<V>(); exceptional_cases<V>(); conversion_edges<V>(); }
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
    std::fesetenv(&saved); std::printf("%d power/root failures\n",failures); return failures!=0;
}
