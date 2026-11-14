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
#include <algorithm>
#include <array>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>
#include <sys/mman.h>
#include <unistd.h>
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
#include "wide_types.h"
#define PROBE(V,M) V wide_abi(V,V,M);
PROBE(WideC,WideCMask) PROBE(WideUC,WideUCMask) PROBE(WideS,WideSMask) PROBE(WideUS,WideUSMask)
PROBE(WideI,WideIMask) PROBE(WideUI,WideUIMask) PROBE(WideQ,WideQMask) PROBE(WideUQ,WideUQMask)
PROBE(WideF,WideFMask) PROBE(WideD,WideDMask)
namespace {
int failures;
#define CHECK(...) do { if(!(__VA_ARGS__)) { if(failures<25) std::fprintf(stderr,"line %d: %s\n",__LINE__,#__VA_ARGS__); ++failures; } } while(0)
template<class V> using Scalar=decltype(V()[0]);
template<class T,class U> T bits(U a) { static_assert(sizeof(T)==sizeof(U)); T b; std::memcpy(&b,&a,sizeof(b)); return b; }
uint64_t state=0x123456789abcdef0ull;
uint64_t random_bits() { state^=state<<13; state^=state>>7; state^=state<<17; return state; }
template<class V,size_t... I> V reverse(V a,std::index_sequence<I...>) {
    constexpr int n=V::size();
#if MAX_VECTOR_SIZE >= 512
    if constexpr(n==64) return permute64<(n-1-int(I))...>(a);
    else
#endif
    if constexpr(n==32) return permute32<(n-1-int(I))...>(a);
    else if constexpr(n==16) return permute16<(n-1-int(I))...>(a);
    else if constexpr(n==8) return permute8<(n-1-int(I))...>(a);
    else return permute4<(n-1-int(I))...>(a);
}
template<int i,int n> constexpr int blend_index() { return i%5==0?-1:(i+n/2)%n+(i%2?n:0); }
template<class V,size_t... I> V mixing(V a,V b,std::index_sequence<I...>) {
    constexpr int n=V::size();
#if MAX_VECTOR_SIZE >= 512
    if constexpr(n==64) return blend64<blend_index<I,n>()...>(a,b);
    else
#endif
    if constexpr(n==32) return blend32<blend_index<I,n>()...>(a,b);
    else if constexpr(n==16) return blend16<blend_index<I,n>()...>(a,b);
    else if constexpr(n==8) return blend8<blend_index<I,n>()...>(a,b);
    else return blend4<blend_index<I,n>()...>(a,b);
}
template<class V,size_t... I> V four_sources(V a,V b,std::index_sequence<I...>) {
    constexpr int n=V::size();
#if MAX_VECTOR_SIZE >= 512
    if constexpr(n==64) return blend64<((I%4)*(n/2)+(I/4)%(n/2))...>(a,b);
    else
#endif
    if constexpr(n==32) return blend32<((I%4)*(n/2)+(I/4)%(n/2))...>(a,b);
    else if constexpr(n==16) return blend16<((I%4)*(n/2)+(I/4)%(n/2))...>(a,b);
    else if constexpr(n==8) return blend8<((I%4)*(n/2)+(I/4)%(n/2))...>(a,b);
    else return blend4<((I%4)*(n/2)+(I/4)%(n/2))...>(a,b);
}
template<class V,size_t... I> V lane_constructor(std::index_sequence<I...>) { return V(Scalar<V>(I+1)...); }
template<class V,class M> void structure_and_memory() {
    using T=Scalar<V>; using H=decltype(V().get_low()); constexpr int n=V::size(),h=n/2;
    static_assert(sizeof(V)==wide_bits/8 && alignof(V)>=16 && std::is_trivially_copyable<V>::value);
    static_assert(std::is_same<decltype(V().get_high()),H>::value);
    T input[n]; for(int i=0;i<n;++i) input[i]=T(i+1);
    V lanes=lane_constructor<V>(std::make_index_sequence<n>{});
    V a; a.load(input); V assembled(a.get_low(),a.get_high()); auto extended=extend_z(a.get_high());
    auto joined=concatenate2(a.get_high(),a.get_low());
    auto reversed=reverse(a,std::make_index_sequence<n>{}); auto blended=mixing(a,V(T(99)),std::make_index_sequence<n>{});
    auto all_sources=four_sources(a,V(T(99)),std::make_index_sequence<n>{});
    for(int i=0;i<n;++i) {
        const int source=(i%4)*h+(i/4)%h; CHECK(all_sources[i]==(source<n?input[source]:T(99)));
        CHECK(lanes[i]==input[i]); CHECK(assembled[i]==input[i]); CHECK(extended[i]==(i<h?input[i+h]:T(0)));
        CHECK(joined[i]==input[(i+h)%n]); CHECK(reversed[i]==input[n-1-i]);
        int j=i%5==0?-1:(i+h)%n+(i%2?n:0); CHECK(blended[i]==(j<0?T(0):j<n?input[j]:T(99)));
        V inserted=a; inserted.insert(i,T(77)); CHECK(inserted[i]==T(77));
    }
    for(int i=0;i<=n;++i) { V cut=a; cut.cutoff(i); for(int j=0;j<n;++j) CHECK(cut[j]==(j<i?input[j]:T(0))); }
    M alternating; alternating.load_bits(decltype(to_bits(M()))(0xaaaaaaaaaaaaaaaaull)); auto abi=wide_abi(V(T(1)),V(T(2)),alternating);
    for(int i=0;i<n;++i) CHECK(abi[i]==(i%2?T(3):T(1)));
    const long page=sysconf(_SC_PAGESIZE); void* mem=mmap(nullptr,2*page,PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE,-1,0);
    CHECK(mem!=MAP_FAILED); if(mem==MAP_FAILED) return;
    CHECK(mprotect(static_cast<char*>(mem)+page,page,PROT_NONE)==0);
    for(int count=0;count<=n;++count) {
        T* p=reinterpret_cast<T*>(static_cast<char*>(mem)+page)-count;
        for(int i=0;i<count;++i) p[i]=input[i];
        V loaded; loaded.load_partial(count,p);
        for(int i=0;i<n;++i) CHECK(loaded[i]==(i<count?input[i]:T(0)));
        a.store_partial(count,p); for(int i=0;i<count;++i) CHECK(p[i]==input[i]);
    }
    CHECK(munmap(mem,2*page)==0);
    alignas(64) T aligned[n]; a.store_a(aligned); V load; load.load_a(aligned); load.store_nt(aligned);
    for(int i=0;i<n;++i) CHECK(aligned[i]==input[i]);
}
template<class M> void masks() {
    constexpr int n=M::size(); const uint64_t all=(~uint64_t(0)) >> (64-n);
#if VCL_ARCH_ARM64 || INSTRSET < 10
    static_assert(sizeof(M)==wide_bits/8);
#endif
    for(int bit=0;bit<n;++bit) {
        M one; one.load_bits(decltype(to_bits(M()))(uint64_t(1)<<bit));
        CHECK(to_bits(one)==(uint64_t(1)<<bit)); CHECK(horizontal_count(one)==1);
        CHECK(horizontal_find_first(one)==bit);
    }
    CHECK(horizontal_and(M(true))); CHECK(!horizontal_or(M(false)));
    for(unsigned i=0;i<65536;++i) {
        const uint64_t pattern=(uint64_t(i)|(uint64_t(i^0x5a5a)<<16)|(uint64_t(i^0xa5a5)<<32)|(uint64_t(i^0xffff)<<48))&all;
        M a; a.load_bits(decltype(to_bits(M()))(pattern)); CHECK(to_bits(a)==pattern); CHECK(horizontal_count(a)==__builtin_popcountll(pattern));
        CHECK(horizontal_find_first(a)==(pattern?__builtin_ctzll(pattern):-1)); CHECK(to_bits(!a)==(all^pattern));
        M combined(a.get_low(),a.get_high()); CHECK(to_bits(combined)==pattern); CHECK(horizontal_and(a==combined));
        a.insert(n/2,true); CHECK(to_bits(a)==(pattern|(uint64_t(1)<<(n/2))));
    }
}
template<class V> void integer_arithmetic() {
    using T=Scalar<V>; using U=typename std::make_unsigned<T>::type; constexpr int n=V::size();
    // Native x86 256/512-bit qword classes retain their inherited signed result.
#if VCL_ARCH_ARM64 || INSTRSET < 8
    static_assert(std::is_same<decltype(~V()),V>::value);
#elif INSTRSET < 9
    if constexpr(wide_bits==512) static_assert(std::is_same<decltype(~V()),V>::value);
#endif
    for(int trial=0;trial<300;++trial) {
        T x[n],y[n]; for(int i=0;i<n;++i) { x[i]=bits<T>(U(random_bits())); y[i]=bits<T>(U(random_bits())); }
        V a,b; a.load(x); b.load(y); auto sum=a+b,diff=a-b,prod=a*b,low=min(a,b),high=max(a,b);
        auto inverted=~a; auto masked=a & V(~b);
        for(int i=0;i<n;++i) { CHECK(U(inverted[i])==U(~U(x[i]))); CHECK(U(masked[i])==U(U(x[i])&~U(y[i]))); }
        U total=0; for(int i=0;i<n;++i) total=U(uint64_t(total)+uint64_t(U(x[i])));
        // Some legacy small-integer reductions expose a signed or wider scalar.
        CHECK(U(horizontal_add(a))==total);
        if constexpr(sizeof(T)<8) {
            auto lo=extend_low(a),hi=extend_high(a); auto back=compress(lo,hi);
            for(int i=0;i<n;++i) CHECK(back[i]==x[i]);
            auto one=compress(extend(a.get_high()));
            for(int i=0;i<n/2;++i) CHECK(one[i]==x[i+n/2]);
        }
        for(int i=0;i<n;++i) {
            CHECK(sum[i]==bits<T>(U(uint64_t(U(x[i]))+uint64_t(U(y[i])))));
            CHECK(diff[i]==bits<T>(U(uint64_t(U(x[i]))-uint64_t(U(y[i])))));
            CHECK(prod[i]==bits<T>(U(uint64_t(U(x[i]))*uint64_t(U(y[i])))));
            CHECK(low[i]==std::min(x[i],y[i]) && high[i]==std::max(x[i],y[i]));
        }
    }
    if constexpr(sizeof(T)<8) {
        auto runtime=V(T(100))/T(13); auto constant=V(T(100))/const_int(13);
        for(int i=0;i<n;++i) CHECK(runtime[i]==T(7) && constant[i]==T(7));
        using L=decltype(extend_low(V())); using LT=Scalar<L>;
        LT values[n]; for(int i=0;i<n;++i) values[i]=i%3==0?std::numeric_limits<LT>::max():i%3==1?std::numeric_limits<LT>::lowest():LT(i);
        L lo,hi; lo.load(values); hi.load(values+n/2); auto saturated=compress_saturated(lo,hi);
        for(int i=0;i<n;++i) CHECK(saturated[i]==T(std::min<LT>(std::numeric_limits<T>::max(),std::max<LT>(std::numeric_limits<T>::lowest(),values[i]))));
        auto larger=extend_low(V(T(37))); auto larger_high=extend_high(V(T(37)));
        auto packed=compress(larger,larger_high); auto sat=compress_saturated(larger,larger_high);
        for(int i=0;i<n;++i) CHECK(packed[i]==T(37) && sat[i]==T(37));
    }
}
template<class V,size_t... I> V repeated_gather(Scalar<V> const* p,std::index_sequence<I...>) {
    if constexpr(std::is_same<V,Vec8i>::value) return gather8i<(int(I)*0)...>(p);
    else if constexpr(std::is_same<V,Vec4q>::value) return gather4q<(int(I)*0)...>(p);
    else if constexpr(std::is_same<V,Vec8f>::value) return gather8f<(int(I)*0)...>(p);
    else if constexpr(std::is_same<V,Vec4d>::value) return gather4d<(int(I)*0)...>(p);
#if MAX_VECTOR_SIZE >= 512
    else if constexpr(std::is_same<V,Vec16i>::value) return gather16i<(int(I)*0)...>(p);
    else if constexpr(std::is_same<V,Vec8q>::value) return gather8q<(int(I)*0)...>(p);
    else if constexpr(std::is_same<V,Vec16f>::value) return gather16f<(int(I)*0)...>(p);
    else if constexpr(std::is_same<V,Vec8d>::value) return gather8d<(int(I)*0)...>(p);
#endif
    else return V(*p);
}
template<int N,class V,class I> void table_boundary() {
    using T=Scalar<V>; using IT=Scalar<I>; constexpr int count=N>0?N:0;
    const long page=sysconf(_SC_PAGESIZE); void* mem=mmap(nullptr,2*page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    CHECK(mem!=MAP_FAILED); if(mem==MAP_FAILED) return; CHECK(mprotect(static_cast<char*>(mem)+page,page,PROT_NONE)==0);
    T* table=reinterpret_cast<T*>(static_cast<char*>(mem)+page)-count;
    for(int i=0;i<count;++i) table[i]=T(i+3);
    IT ix[I::size()]; for(int i=0;i<I::size();++i) { if constexpr(N>0) ix[i]=IT(i%N); else ix[i]=0; }
    I index; index.load(ix); V result=lookup<N>(index,table);
    for(int i=0;i<V::size();++i) CHECK(result[i]==(N>0?T(int(ix[i])+3):T(0)));
    if constexpr(N==1) CHECK(horizontal_and(repeated_gather<V>(table,std::make_index_sequence<V::size()>{})==V(3)));
    munmap(mem,2*page);
}
template<class V,class I> void tables() { table_boundary<-1,V,I>(); table_boundary<0,V,I>(); table_boundary<1,V,I>(); table_boundary<3,V,I>(); table_boundary<V::size()-1,V,I>(); table_boundary<V::size()+1,V,I>(); }
template<unsigned B> void byte_shift() {
    constexpr int n=WideC::size(); int8_t input[n]; for(int i=0;i<n;++i) input[i]=int8_t(i+1);
    WideC a; a.load(input); auto up=shift_bytes_up<B>(a),down=shift_bytes_down<B>(a);
    for(int i=0;i<n;++i) { CHECK(up[i]==(i>=int(B)?input[i-B]:0)); CHECK(down[i]==(i+B<n?input[i+B]:0)); }
}
template<class V> void wide_fp() {
    using T=Scalar<V>; constexpr int n=V::size();
    T input[n]; for(int i=0;i<n;++i) input[i]=T(i+1);
    V a; a.load(input); auto power=pow_const(a,3),poly=polynomial_3(a,T(1),T(2),T(3),T(4));
    auto fused=mul_add(a,V(T(2)),V(T(1))); auto raw=reinterpret_i(a);
    V back; if constexpr(sizeof(T)==4) back=reinterpret_f(raw); else back=reinterpret_d(raw);
    for(int i=0;i<n;++i) {
        T x=input[i]; CHECK(power[i]==x*x*x); CHECK(poly[i]==1+2*x+3*x*x+4*x*x*x);
        CHECK(fused[i]==2*x+1); CHECK(back[i]==x);
    }
    CHECK(horizontal_add(a)==n*(n+1)/2); CHECK(horizontal_min(a)==1); CHECK(horizontal_max(a)==n);
    CHECK(horizontal_and(is_nan(nan_vec<V>(0x123)))); CHECK(horizontal_and(is_inf(infinite_vec<V>())));
    CHECK(horizontal_and(sign_bit(-V(T(0))))); CHECK(!horizontal_or(sign_bit(V(T(0)))));
    if constexpr(sizeof(T)==4) {
        auto lo=extend_low(a),hi=extend_high(a); auto packed=compress(lo,hi);
        for(int i=0;i<n;++i) CHECK(packed[i]==input[i]);
        auto d=to_double(a.get_high()); auto f=to_float(d);
        for(int i=0;i<n/2;++i) CHECK(d[i]==input[i+n/2] && f[i]==input[i+n/2]);
        auto integer=roundi(a); auto roundtrip=to_float(integer);
        for(int i=0;i<n;++i) CHECK(roundtrip[i]==input[i]);
    } else {
        const int expected[4][4]={{-2,-2,2,2},{-3,-2,1,2},{-2,-1,2,3},{-2,-1,1,2}};
        for(int i=0;i<n;++i) input[i]=T(i%4==0?-2.5:i%4==1?-1.5:i%4==2?1.5:2.5);
        a.load(input);
        for(unsigned mode=0;mode<4;++mode) {
            set_control_word(0x1f80u|(mode<<13)); auto r=round_to_int32(a);
            for(int i=0;i<n;++i) CHECK(r[i]==expected[mode][i%4]);
        }
        reset_control_word();
    }
}
template<class V,class I> void scatter_bounds() {
    using T=Scalar<V>; using IT=Scalar<I>; constexpr int n=V::size();
    const long page=sysconf(_SC_PAGESIZE); void* mem=mmap(nullptr,2*page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    CHECK(mem!=MAP_FAILED); if(mem==MAP_FAILED) return; CHECK(mprotect(static_cast<char*>(mem)+page,page,PROT_NONE)==0);
    T* out=reinterpret_cast<T*>(static_cast<char*>(mem)+page)-n; IT ix[n]; T data[n];
    for(int i=0;i<n;++i) { out[i]=T(0); ix[i]=IT(i); data[i]=T(i+1); }
    ix[0]=IT(-1); ix[n-1]=IT(n); I indexes; indexes.load(ix); V values; values.load(data);
    scatter(indexes,n,values,out);
    CHECK(out[0]==T(0) && out[n-1]==T(0)); for(int i=1;i<n-1;++i) CHECK(out[i]==data[i]);
    munmap(mem,2*page);
}
void conversions_and_fp() {
    Vec4f f(1,2,3,4); auto d=to_double(f); auto again=to_float(d); for(int i=0;i<4;++i) CHECK(d[i]==i+1 && again[i]==i+1);
    Vec8f input(1,2,3,4,5,6,7,8); auto a=extend_low(input),b=extend_high(input); auto c=compress(a,b);
    for(int i=0;i<8;++i) CHECK(c[i]==input[i]); CHECK(horizontal_add(input)==36);
    auto powers=pow_const(input,3); auto poly=polynomial_3(input,1.f,2.f,3.f,4.f);
    for(int i=0;i<8;++i) { float x=input[i]; CHECK(powers[i]==x*x*x); CHECK(poly[i]==1+2*x+3*x*x+4*x*x*x); }
    CHECK(horizontal_and(is_nan(nan_vec<Vec8f>(0x123)))); CHECK(horizontal_and(is_inf(infinite_vec<Vec4d>())));
    Vec8fb mask(true,false,true,false,false,true,false,true); Vec8ib im;
#if VCL_ARCH_ARM64 || INSTRSET < 7
    im=mask;
#else
    im.load_bits(to_bits(mask));
#endif
     CHECK(to_bits(Vec8fb(im))==to_bits(mask));
    Vec4db dm(true,false,false,true); Vec4qb qi;
#if VCL_ARCH_ARM64 || INSTRSET < 7
    qi=dm;
#else
    qi.load_bits(to_bits(dm));
#endif
     CHECK(to_bits(Vec4db(qi))==9);
#if VCL_BACKEND == VCL_BACKEND_NEON || INSTRSET < 7
    Vec8f selected(selectf(mask,Vec8f(1),Vec8f(2))); for(int i=0;i<8;++i) CHECK(selected[i]==(mask[i]?1:2));
    Vec4d chosen(selectd(dm,Vec4d(1),Vec4d(2))); for(int i=0;i<4;++i) CHECK(chosen[i]==(dm[i]?1:2));
#endif
    auto raw=reinterpret_i(input); Vec8f cast=reinterpret_f(raw); for(int i=0;i<8;++i) CHECK(cast[i]==input[i]);
    const int expected[4][4]={{-2,-2,2,2},{-3,-2,1,2},{-2,-1,2,3},{-2,-1,1,2}};
    for(unsigned mode=0;mode<4;++mode) { set_control_word(0x1f80u|(mode<<13)); auto rounded=round_to_int32(Vec4d(-2.5,-1.5,1.5,2.5)); for(int i=0;i<4;++i) CHECK(rounded[i]==expected[mode][i]); }
    reset_control_word();
    for(uint32_t rawbits:{0u,0x80000000u,0x7f800000u,0x7fc12345u,1u}) {
        float v=bits<float>(rawbits); Vec8f all(v); auto neg=-all;
        for(int i=0;i<8;++i) CHECK(bits<uint32_t>(neg[i])==(rawbits^0x80000000u));
    }
}

void indexed_memory() {
    int32_t i[]={11}; int64_t q[]={19}; float f[]={7}; double d[]={13};
    CHECK(horizontal_and(gather8i<0,0,0,0,0,0,0,0>(i)==Vec8i(11)));
    CHECK(horizontal_and(gather4q<0,0,0,0>(q)==Vec4q(19)));
    CHECK(horizontal_and(gather8f<0,0,0,0,0,0,0,0>(f)==Vec8f(7)));
    CHECK(horizontal_and(gather4d<0,0,0,0>(d)==Vec4d(13)));
    int32_t out[12]={}; scatter(Vec8i(0,1,2,3,4,5,6,7),8,Vec8i(1,2,3,4,5,6,7,8),out);
    for(int j=0;j<8;++j) CHECK(out[j]==j+1);
}
}
int main() {
    std::fenv_t saved; std::fegetenv(&saved); reset_control_word();
#define RUN(V,M) structure_and_memory<V,M>(); masks<M>(); integer_arithmetic<V>();
    RUN(WideC,WideCMask) RUN(WideUC,WideUCMask) RUN(WideS,WideSMask) RUN(WideUS,WideUSMask)
    RUN(WideI,WideIMask) RUN(WideUI,WideUIMask) RUN(WideQ,WideQMask) RUN(WideUQ,WideUQMask)
    structure_and_memory<WideF,WideFMask>(); structure_and_memory<WideD,WideDMask>(); masks<WideFMask>(); masks<WideDMask>();
    #ifndef VCL_TEST_WIDE512
    tables<WideC,WideC>(); tables<WideS,WideS>();
#endif
     tables<WideI,WideI>(); tables<WideQ,WideQ>(); tables<WideF,WideI>(); tables<WideD,WideQ>();
    byte_shift<0>(); byte_shift<1>(); byte_shift<15>(); byte_shift<16>(); byte_shift<17>();
    byte_shift<31>(); byte_shift<32>(); byte_shift<33>(); byte_shift<63>(); byte_shift<64>(); byte_shift<65>();
    wide_fp<WideF>(); wide_fp<WideD>();
    scatter_bounds<WideI,WideI>(); scatter_bounds<WideQ,WideQ>(); scatter_bounds<WideF,WideI>(); scatter_bounds<WideD,WideQ>();
    conversions_and_fp(); indexed_memory(); std::fesetenv(&saved); std::printf("%d %d-bit composition failures\n",failures,wide_bits); return failures!=0;
}
