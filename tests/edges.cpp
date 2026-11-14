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
#include <cfenv>
#include <algorithm>
#include <initializer_list>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
static int failures=0;
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
template<class T,class U> T bits(U x) { static_assert(sizeof(T)==sizeof(U)); T y; std::memcpy(&y,&x,sizeof(y)); return y; }
static uint64_t state=0x12345678abcdef01ull;
static uint64_t random_bits() { state^=state<<13; state^=state>>7; state^=state<<17; return state; }
template<class V,class T> void integer_edges() {
    using U=typename std::make_unsigned<T>::type;
    constexpr int n=V::size(),w=sizeof(T)*8;
    static_assert(sizeof(V)==16 && alignof(V)==16);
    T a[n],b[n],out[n];
    for(int trial=0;trial<256;++trial) {
      for(int i=0;i<n;++i) { a[i]=bits<T>(U(random_bits())); b[i]=bits<T>(U(random_bits())); }
      if(trial==0) for(int i=0;i<n;++i) { a[i]=std::numeric_limits<T>::min(); b[i]=1; }
      if(trial==1) for(int i=0;i<n;++i) { a[i]=std::numeric_limits<T>::max(); b[i]=1; }
      V x,y; x.load(a); y.load(b);
      (x+y).store(out); for(int i=0;i<n;++i) CHECK(out[i]==bits<T>(U(uint64_t(U(a[i]))+U(b[i]))));
      (x-y).store(out); for(int i=0;i<n;++i) CHECK(out[i]==bits<T>(U(uint64_t(U(a[i]))-U(b[i]))));
      (x*y).store(out); for(int i=0;i<n;++i) CHECK(out[i]==bits<T>(U(uint64_t(U(a[i]))*U(b[i]))));
      for(int shift: {0,1,w-1,w,w+1,255}) {
        (x<<shift).store(out); for(int i=0;i<n;++i) CHECK(out[i]==bits<T>(shift>=w?U(0):U(uint64_t(U(a[i]))<<shift)));
        (x>>shift).store(out);
        for(int i=0;i<n;++i) {
          U expected;
          if(shift>=w) expected=std::is_signed<T>::value && a[i]<0?U(~U(0)):U(0);
          else { expected=U(a[i])>>shift; if(std::is_signed<T>::value && a[i]<0 && shift>0) expected|=U(uint64_t(~uint64_t(0))<<(w-shift)); }
          CHECK(out[i]==bits<T>(expected));
        }
      }
      auto m=x<y; uint64_t expected=0; for(int i=0;i<n;++i) expected|=uint64_t(a[i]<b[i])<<i;
      CHECK(to_bits(m)==expected); CHECK(horizontal_count(m)==__builtin_popcountll(expected));
      CHECK(horizontal_find_first(m)==(expected?__builtin_ctzll(expected):-1));
      decltype(m) from_bits; from_bits.load_bits(expected); CHECK(to_bits(from_bits)==expected);
      select(m,x,y).store(out); for(int i=0;i<n;++i) CHECK(out[i]==(a[i]<b[i]?a[i]:b[i]));
      if constexpr(sizeof(T)<=2) {
        add_saturated(x,y).store(out); for(int i=0;i<n;++i) { int64_t sum=int64_t(a[i])+int64_t(b[i]); sum=std::max(sum,int64_t(std::numeric_limits<T>::min())); sum=std::min(sum,int64_t(std::numeric_limits<T>::max())); CHECK(out[i]==T(sum)); }
        sub_saturated(x,y).store(out); for(int i=0;i<n;++i) { int64_t sum=int64_t(a[i])-int64_t(b[i]); sum=std::max(sum,int64_t(std::numeric_limits<T>::min())); sum=std::min(sum,int64_t(std::numeric_limits<T>::max())); CHECK(out[i]==T(sum)); }
      }
    }
}
template<class V,class T,class U> void fp_edges() {
    static_assert(sizeof(V)==16 && alignof(V)==16);
    constexpr U sign=U(1)<<(sizeof(T)*8-1);
    constexpr U infinity=sizeof(T)==4?U(0x7f800000u):U(0x7ff0000000000000ull);
    U cases[]={0,sign,1,sign|1,infinity,infinity|sign,infinity|U(1),infinity|U(1234),infinity-U(1)};
    T out[V::size()];
    for(U pattern:cases) {
      T x=bits<T>(pattern); V a(x),b(T(1));
      const bool nan=(pattern&~sign)>infinity,inf=(pattern&~sign)==infinity;
      CHECK(horizontal_and(is_nan(a))==nan); CHECK(horizontal_or(is_nan(a))==nan);
      CHECK(horizontal_and(is_inf(a))==inf); CHECK(horizontal_and(is_finite(a))==(!nan&&!inf));
      CHECK(horizontal_and(is_subnormal(a))==(pattern==1||pattern==(sign|1)));
      min(a,b).store(out); for(T f:out) CHECK(f==(x<T(1)?x:T(1)));
      max(a,b).store(out); for(T f:out) CHECK(f==(x>T(1)?x:T(1)));
      if(nan) { min(b,a).store(out); for(T f:out) CHECK(std::isnan(f)); }
    }
    for(T value:{T(-2.5),T(-1.5),T(-0.5),T(0.5),T(1.5),T(2.5)}) {
      round(V(value)).store(out); T expect=std::nearbyint(value); for(T f:out) CHECK(f==expect);
    }
#if VCL_BACKEND == VCL_BACKEND_NEON
    for(int trial=0;trial<200;++trial) {
      T a=T(int(random_bits()%20000)-10000)/T(127),b=T(int(random_bits()%20000)-10000)/T(127),c=T(int(random_bits()%20000)-10000)/T(127);
      mul_add(V(a),V(b),V(c)).store(out); for(T f:out) CHECK(f==std::fma(a,b,c));
      mul_sub(V(a),V(b),V(c)).store(out); for(T f:out) CHECK(f==std::fma(a,b,-c));
      nmul_add(V(a),V(b),V(c)).store(out); for(T f:out) CHECK(f==std::fma(-a,b,c));
    }
#endif
}
__attribute__((noinline)) Vec4i current_round(Vec4f x) { return roundi(x); }
int main() {
    std::fenv_t saved_env; std::fegetenv(&saved_env); reset_control_word();
    integer_edges<Vec16c,int8_t>(); integer_edges<Vec16uc,uint8_t>(); integer_edges<Vec8s,int16_t>(); integer_edges<Vec8us,uint16_t>();
    integer_edges<Vec4i,int32_t>(); integer_edges<Vec4ui,uint32_t>(); integer_edges<Vec2q,int64_t>(); integer_edges<Vec2uq,uint64_t>();
    fp_edges<Vec4f,float,uint32_t>(); fp_edges<Vec2d,double,uint64_t>();
    Vec4i p=permute4<3,-1,0,2>(Vec4i(1,2,3,4)); CHECK(p[0]==4 && p[1]==0 && p[2]==1 && p[3]==3);
    Vec4f pf=permute4<2,0,-1,1>(Vec4f(1,2,3,4)); CHECK(pf[0]==3 && pf[1]==1 && pf[2]==0 && pf[3]==2);
    Vec2d pd=permute2<1,0>(Vec2d(1,2)); CHECK(pd[0]==2 && pd[1]==1);
    for(float f:{-2147483648.f,2147483648.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),1.75f,-1.75f}) {
      int32_t expected=(!std::isfinite(f)||f>=2147483648.f||f< -2147483648.f)?INT32_MIN:int32_t(f);
      CHECK(horizontal_and(truncatei(Vec4f(f))==Vec4i(expected)));
    }
    static_assert(std::is_same<decltype(-Vec4ui()),Vec4i>::value);
    Vec4ib bm(false); bm=true; CHECK(horizontal_and(bm)); bm=false; CHECK(!horizontal_or(bm));
    CHECK(horizontal_add(Vec4f(1.e20f,1.f,-1.e20f,1.f))==2.f);
    auto converted=to_float(Vec4i(INT32_MIN,-123,0,INT32_MAX));
    for(int i=0;i<4;++i) CHECK(converted[i]==float(Vec4i(INT32_MIN,-123,0,INT32_MAX)[i]));
    auto unsigned_converted=to_float(Vec4ui(0,1,0x80000000u,UINT32_MAX));
    for(int i=0;i<4;++i) CHECK(unsigned_converted[i]==float(Vec4ui(0,1,0x80000000u,UINT32_MAX)[i]));
    auto doubles=to_double(Vec2q(INT64_MIN,INT64_MAX)); CHECK(doubles[0]==double(INT64_MIN)); CHECK(doubles[1]==double(INT64_MAX));
    auto ext=extend_high(Vec4f(1,2,3,4)); CHECK(ext[0]==3 && ext[1]==4);
    auto packed=compress(Vec2d(1,2),Vec2d(3,4)); for(int i=0;i<4;++i) CHECK(packed[i]==float(i+1));
#if VCL_BACKEND == VCL_BACKEND_NEON
    for(double value:{-9223372036854775808.0,9223372036854775808.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),-1.75,1.75}) {
      int64_t expected=(!std::isfinite(value)||value>=9223372036854775808.0||value< -9223372036854775808.0)?INT64_MIN:int64_t(value);
      CHECK(horizontal_and(truncatei(Vec2d(value))==Vec2q(expected)));
    }
    auto reinterpreted=reinterpret_f(reinterpret_i(Vec4f(-0.f,1.f,2.f,3.f))); CHECK(std::signbit(reinterpreted[0])); CHECK(reinterpreted[3]==3.f);
#endif
    float ties[]={-2.5f,-1.5f,1.5f,2.5f};
    int32_t expected[4][4]={{-2,-2,2,2},{-3,-2,1,2},{-2,-1,2,3},{-2,-1,1,2}};
    for(unsigned mode=0;mode<4;++mode) {
      set_control_word(0x1f80u|(mode<<13)); Vec4f x; x.load(ties); auto r=current_round(x); for(int i=0;i<4;++i) CHECK(r[i]==expected[mode][i]);
    }
    set_control_word(0x1f80u); CHECK((get_control_word()&0x8040)==0);
    #if VCL_ARCH_ARM64
    set_control_word(0x1f80u|0x8000); CHECK((get_control_word()&0x8040)==0x8040);
    set_control_word(0x1f80u|0x40); CHECK((get_control_word()&0x8040)==0x8040);
    #endif
    std::fesetenv(&saved_env);
    std::printf("%d edge failures\n",failures); return failures!=0;
}
