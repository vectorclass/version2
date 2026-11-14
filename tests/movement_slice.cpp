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
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
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
namespace {
int failures=0;
#define CHECK(x) do { if(!(x)) { if(failures<20) std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
uint64_t seed=0x12345678abcdef01ull;
uint64_t random_bits() { seed^=seed<<13; seed^=seed>>7; seed^=seed<<17; return seed; }
template<class T> T payload(uint64_t bits) { T value; std::memcpy(&value,&bits,sizeof(T)); return value; }
template<class T> bool same_bits(T a,T b) { return std::memcmp(&a,&b,sizeof(T))==0; }
template<class V> using Scalar=decltype(V()[0]);
template<class V> V random_vector() {
    Scalar<V> values[V::size()];
    for(auto& x:values) {
        uint64_t bits=random_bits();
        if constexpr(std::is_floating_point<Scalar<V>>::value) {
            if constexpr(sizeof(Scalar<V>)==4) {
                constexpr uint32_t special[]={0,0x80000000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f800001u,1,0x007fffffu};
                if(bits&8) bits=special[bits&7];
            }
            else {
                constexpr uint64_t special[]={0,0x8000000000000000ull,0x7ff0000000000000ull,0xfff0000000000000ull,0x7ff8123456789abcull,0x7ff0000000000001ull,1,0x000fffffffffffffull};
                if(bits&8) bits=special[bits&7];
            }
        }
        x=payload<Scalar<V>>(bits);
    }
    return V().load(values);
}
template<int Mode,int Lane,int N,bool Blend> constexpr int lane_index() {
    constexpr int range=Blend?2*N:N;
    if constexpr(Mode==0) return Lane;
    if constexpr(Mode==1) return range-1-Lane;
    if constexpr(Mode==2) return range-1;
    if constexpr(Mode==3) return (Lane+1)%range;
    if constexpr(Mode==4) return (Lane+N/2)%range;
    if constexpr(Mode==5) return Lane%2?-1:Lane;
    if constexpr(Mode==6) return Lane%2?V_DC:Lane;
    if constexpr(Mode==7) return -1;
    if constexpr(Mode==8) return V_DC;
    if constexpr(Mode==9) return (2*Lane)%range;
    if constexpr(Mode==10 && Blend) return Lane%2?N+Lane/2:Lane/2;
    if constexpr(Mode==11 && Blend) return N+Lane;
    if constexpr(Mode==12 && Blend) return Lane%2?N+Lane:Lane;
    if constexpr(Mode>=64) {
        constexpr uint32_t bits=uint32_t(Mode*1140071481u+Lane*2654435761u) ^ uint32_t(Mode<<(Lane%12));
        constexpr int i=int(bits%(range+2)); return i==range?-1:i==range+1?V_DC:i;
    }
    constexpr int i=(Mode+Lane)%(range+2); return i==range?-1:i==range+1?V_DC:i;
}
template<class V,int... I> V do_permute(V a) {
    if constexpr(V::size()==2) return permute2<I...>(a);
    if constexpr(V::size()==4) return permute4<I...>(a);
    if constexpr(V::size()==8) return permute8<I...>(a);
    if constexpr(V::size()==16) return permute16<I...>(a);
}
template<class V,int... I> V do_blend(V a,V b) {
    if constexpr(V::size()==2) return blend2<I...>(a,b);
    if constexpr(V::size()==4) return blend4<I...>(a,b);
    if constexpr(V::size()==8) return blend8<I...>(a,b);
    if constexpr(V::size()==16) return blend16<I...>(a,b);
}
template<int Mode,class V,size_t... Lane> void permutation_pattern(std::index_sequence<Lane...>) {
    constexpr int n=V::size();
    constexpr int perm[]={lane_index<Mode,int(Lane),n,false>()...},blend[]={lane_index<Mode,int(Lane),n,true>()...};
    for(int trial=0;trial<3;++trial) {
        V a=random_vector<V>(),b=random_vector<V>();
        auto p=do_permute<V,lane_index<Mode,int(Lane),n,false>()...>(a);
        auto q=do_blend<V,lane_index<Mode,int(Lane),n,true>()...>(a,b);
        for(int i=0;i<n;++i) {
            if(perm[i]!=V_DC) CHECK(same_bits(p[i],perm[i]<0?Scalar<V>(0):a[perm[i]]));
            if(blend[i]!=V_DC) CHECK(same_bits(q[i],blend[i]<0?Scalar<V>(0):blend[i]<n?a[blend[i]]:b[blend[i]-n]));
        }
    }
}
template<class V,size_t... Mode> void permutations(std::index_sequence<Mode...>) {
    (permutation_pattern<int(Mode),V>(std::make_index_sequence<V::size()>{}),...);
}
template<class V,int Source,size_t... Dest> void broadcast_lane(std::index_sequence<Dest...>) {
    using S=Scalar<V>;
    constexpr uint64_t sign=uint64_t(1)<<(sizeof(S)*8-1);
    std::array<uint64_t,12> patterns={0,sign,1,sign|1,~uint64_t(0),0x123456789abcdef0ull,
        0x807f01ff807f01ffull,0x55aa55aa55aa55aaull,17,sign|17,123,456};
    if constexpr(std::is_floating_point<S>::value && sizeof(S)==4)
        patterns={0,sign,1,sign|1,0x7f800000u,0xff800000u,0x7fc12345u,0xffc54321u,0x7f800001u,0x007fffffu,0x3f800000u,0xbf800000u};
    if constexpr(std::is_floating_point<S>::value && sizeof(S)==8)
        patterns={0,sign,1,sign|1,0x7ff0000000000000ull,0xfff0000000000000ull,0x7ff8123456789abcull,
            0xfff8abcdef123456ull,0x7ff0000000000001ull,0x000fffffffffffffull,0x3ff0000000000000ull,0xbff0000000000000ull};
    for(size_t rotation=0;rotation<patterns.size();++rotation) {
        S input[V::size()],second[V::size()],output[V::size()];
        for(int i=0;i<V::size();++i) {
            input[i]=payload<S>(patterns[(rotation+i)%patterns.size()]);
            second[i]=payload<S>(patterns[(rotation+i+V::size())%patterns.size()]);
        }
        const auto a=V().load(input),b=V().load(second);
        const auto broadcast=do_permute<V,(int(Dest)*0+Source)...>(a);
        broadcast.store(output);
        for(int i=0;i<V::size();++i) CHECK(same_bits(output[i],input[Source]));
        do_blend<V,(int(Dest)*0+Source)...>(a,b).store(output);
        for(int i=0;i<V::size();++i) CHECK(same_bits(output[i],input[Source]));
        do_blend<V,(int(Dest)*0+V::size()+Source)...>(a,b).store(output);
        for(int i=0;i<V::size();++i) CHECK(same_bits(output[i],second[Source]));
    }
}
template<class V,size_t... Source> void broadcasts(std::index_sequence<Source...>) {
    (broadcast_lane<V,int(Source)>(std::make_index_sequence<V::size()>{}),...);
}
template<class V> void broadcasts() { broadcasts<V>(std::make_index_sequence<V::size()>{}); }
template<class V,bool Blend,size_t Code> void two_lane_pattern() {
    static_assert(V::size()==2);
    constexpr int candidates[]={-1,V_DC,0,1,2,3};
    constexpr int range=Blend?6:4;
    constexpr int first=candidates[Code%range],second=candidates[Code/range];
    constexpr int indexes[]={first,second};
    constexpr uint64_t patterns[]={0,0x8000000000000000ull,1,0x8000000000000001ull,
        0x7ff0000000000000ull,0xfff0000000000000ull,0x7ff8123456789abcull,
        0xfff8abcdef123456ull,0x7ff0000000000001ull,0x000fffffffffffffull,
        0x123456789abcdef0ull,0xffffffffffffffffull};
    for(int rotation=0;rotation<12;++rotation) {
        Scalar<V> a[2],b[2],output[2];
        for(int lane=0;lane<2;++lane) {
            a[lane]=payload<Scalar<V>>(patterns[(rotation+lane)%12]);
            b[lane]=payload<Scalar<V>>(patterns[(rotation+lane+2)%12]);
        }
        const auto av=V().load(a),bv=V().load(b);
        if constexpr(Blend) do_blend<V,first,second>(av,bv).store(output);
        else do_permute<V,first,second>(av).store(output);
        for(int lane=0;lane<2;++lane) if(indexes[lane]!=V_DC) {
            const int index=indexes[lane];
            CHECK(same_bits(output[lane],index<0?Scalar<V>(0):index<2?a[index]:b[index-2]));
        }
    }
}
template<class V,bool Blend,size_t... Code> void two_lane_patterns(std::index_sequence<Code...>) {
    (two_lane_pattern<V,Blend,Code>(),...);
}
template<class V> void two_lane_patterns() {
    two_lane_patterns<V,false>(std::make_index_sequence<16>{});
    two_lane_patterns<V,true>(std::make_index_sequence<36>{});
}
template<int Registers,class V,class Index> V lookup_register(Index index,std::array<V,Registers> const& table) {
    if constexpr(Registers*V::size()==2) return lookup2(index,table[0]);
    if constexpr(Registers*V::size()==4 && Registers==1) return lookup4(index,table[0]);
    if constexpr(Registers*V::size()==4 && Registers==2) return lookup4(index,table[0],table[1]);
    if constexpr(Registers*V::size()==8 && Registers==1) return lookup8(index,table[0]);
    if constexpr(Registers*V::size()==8 && Registers==2) return lookup8(index,table[0],table[1]);
    if constexpr(Registers*V::size()==16 && Registers==1) return lookup16(index,table[0]);
    if constexpr(Registers*V::size()==16 && Registers==2) return lookup16(index,table[0],table[1]);
    if constexpr(Registers==4) return lookup16(index,table[0],table[1],table[2],table[3]);
    if constexpr(Registers*V::size()==32) return lookup32(index,table[0],table[1]);
}
template<int Registers,class V,class Index> void register_lookups() {
    constexpr int n=V::size(),entries=Registers*n;
    using I=Scalar<Index>; using U=typename std::make_unsigned<I>::type;
    for(int trial=0;trial<16;++trial) {
        std::array<V,Registers> table; for(auto& v:table) v=random_vector<V>();
        for(int start=0;start<entries;++start) {
            I indexes[n]; for(int i=0;i<n;++i) indexes[i]=I((start+i)%entries);
            Index ix; ix.load(indexes); auto result=lookup_register<Registers>(ix,table);
            for(int i=0;i<n;++i) CHECK(same_bits(result[i],table[indexes[i]/n][indexes[i]%n]));
        }
#if VCL_BACKEND == VCL_BACKEND_NEON
        I indexes[n]; for(int i=0;i<n;++i) indexes[i]=payload<I>(random_bits());
        Index ix; ix.load(indexes); auto result=lookup_register<Registers>(ix,table);
        for(int i=0;i<n;++i) { unsigned selected=U(indexes[i])&(entries-1); CHECK(same_bits(result[i],table[selected/n][selected%n])); }
#endif
    }
}
struct Guarded {
    size_t page=size_t(sysconf(_SC_PAGESIZE));
    size_t capacity;
    void* memory;
    Guarded(size_t minimum=16384):capacity(((std::max(page,minimum)+page-1)/page)*page),memory(mmap(nullptr,capacity+2*page,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0)) { if(memory==MAP_FAILED) { std::perror("mmap"); std::abort(); } if(mprotect(static_cast<char*>(memory)+page,capacity,PROT_READ|PROT_WRITE)!=0) std::abort(); }
    ~Guarded() { munmap(memory,capacity+2*page); }
    template<class T> T* tail(size_t count) { CHECK(count*sizeof(T)<=capacity); return reinterpret_cast<T*>(static_cast<char*>(memory)+page+capacity)-count; }
};
template<int N,class V,class Index> void memory_lookups() {
    using T=Scalar<V>; using I=Scalar<Index>; using U=typename std::make_unsigned<I>::type;
    if constexpr(N<=0) { V zero=lookup<N>(Index(I(0)),static_cast<T const*>(nullptr)); for(int i=0;i<V::size();++i) CHECK(same_bits(zero[i],T(0))); }
    else {
        constexpr int size=N==INT_MAX?128:N;
        Guarded page(size*sizeof(T));
        T* table=page.tail<T>(size); for(int i=0;i<size;++i) table[i]=payload<T>(random_bits());
        for(int trial=0;trial<64;++trial) {
            I indexes[Index::size()];
            for(int i=0;i<Index::size();++i) {
                uint64_t selected=random_bits()%std::min<uint64_t>(size,uint64_t(std::numeric_limits<U>::max())+uint64_t(sizeof(U)<8));
                if constexpr(sizeof(U)==8) selected=random_bits()%size;
                indexes[i]=payload<I>(selected);
            }
            Index index; index.load(indexes); auto r=lookup<N>(index,table);
            for(int i=0;i<V::size();++i) CHECK(same_bits(r[i],table[U(indexes[i])]));
        }
        // Larger array lookups explicitly define modulo/clamping in VCL.
        // Small-array out-of-range results remain backend-dependent.
        if constexpr(N!=INT_MAX && (VCL_BACKEND==VCL_BACKEND_NEON || N>(V::size()==2 && std::is_integral<T>::value?0:2*V::size()))) {
            I indexes[Index::size()]; for(int i=0;i<Index::size();++i) indexes[i]=payload<I>(random_bits());
            Index index; index.load(indexes); auto r=lookup<N>(index,table);
            for(int i=0;i<V::size();++i) {
                uint64_t selected=U(indexes[i]);
                if constexpr((N&(N-1))==0) selected&=N-1; else selected=std::min<uint64_t>(selected,N-1);
                CHECK(same_bits(r[i],table[selected]));
            }
        }
    }
}
template<class V,class Index> void memory_tables() {
    memory_lookups<INT_MIN,V,Index>(); memory_lookups<0,V,Index>(); memory_lookups<-1,V,Index>(); memory_lookups<1,V,Index>(); memory_lookups<2,V,Index>();
    memory_lookups<3,V,Index>(); memory_lookups<4,V,Index>(); memory_lookups<5,V,Index>(); memory_lookups<7,V,Index>();
    memory_lookups<8,V,Index>(); memory_lookups<9,V,Index>(); memory_lookups<15,V,Index>(); memory_lookups<16,V,Index>();
    memory_lookups<17,V,Index>(); memory_lookups<31,V,Index>(); memory_lookups<32,V,Index>(); memory_lookups<33,V,Index>();
    memory_lookups<64,V,Index>(); memory_lookups<65,V,Index>(); memory_lookups<255,V,Index>(); memory_lookups<256,V,Index>();
    memory_lookups<257,V,Index>(); memory_lookups<300,V,Index>(); memory_lookups<1024,V,Index>(); memory_lookups<INT_MAX,V,Index>();
}
template<int... I,class V> void fixed_scatter(V data,void* destination) {
    if constexpr(std::is_same<V,Vec4f>::value) scatter<I...>(data,static_cast<float*>(destination));
    else if constexpr(std::is_same<V,Vec2d>::value) scatter<I...>(data,static_cast<double*>(destination));
    else scatter<I...>(data,destination);
}
template<class V,int... I> V fixed_gather(void const* table) {
    if constexpr(std::is_same<V,Vec4i>::value) return gather4i<I...>(table);
    if constexpr(std::is_same<V,Vec2q>::value) return gather2q<I...>(table);
    if constexpr(std::is_same<V,Vec4f>::value) return gather4f<I...>(table);
    if constexpr(std::is_same<V,Vec2d>::value) return gather2d<I...>(table);
}
template<class V,int... I> void gather_case() {
    constexpr int indexes[]={I...}; constexpr int count=std::max({I...})+1;
    Guarded page; auto* table=page.tail<Scalar<V>>(count);
    for(int i=0;i<count;++i) table[i]=payload<Scalar<V>>(random_bits());
    auto result=fixed_gather<V,I...>(table);
    for(int lane=0;lane<V::size();++lane) CHECK(same_bits(result[lane],table[indexes[lane]]));
}
template<class V,int... I> void scatter_case() {
    constexpr int indexes[]={I...}; constexpr int count=16; using T=Scalar<V>;
    Guarded page; T* destination=page.tail<T>(count); T expected[count];
    for(int i=0;i<count;++i) expected[i]=destination[i]=payload<T>(0xa5a5a5a5a5a5a5a5ull);
    V data=random_vector<V>(); fixed_scatter<I...>(data,destination);
    for(int lane=0;lane<V::size();++lane) if(indexes[lane]>=0) expected[indexes[lane]]=data[lane];
    CHECK(std::memcmp(destination,expected,sizeof(expected))==0);
}
template<class V,class Index> void indexed_memory() {
    using T=Scalar<V>; using I=Scalar<Index>; using U=typename std::make_unsigned<I>::type;
    for(uint32_t limit: {0u,1u,2u,7u,16u,32u}) {
        Guarded page; T* destination=page.tail<T>(limit); T expected[32];
        for(int trial=0;trial<64;++trial) {
            I indexes[Index::size()];
            for(int i=0;i<Index::size();++i) indexes[i]=payload<I>(trial<8?uint64_t(trial==0?-1:trial==1?0:trial==2?limit:trial==3?limit+1:trial==4?INT32_MIN:1):random_bits()%48);
            if constexpr(sizeof(I)==8) { if(trial==5) for(auto& ix:indexes) ix=I(1ull<<32); if(trial==6) for(auto& ix:indexes) ix=std::numeric_limits<I>::min(); if(trial==7) for(auto& ix:indexes) ix=I(-1); }
            for(uint32_t i=0;i<limit;++i) expected[i]=destination[i]=payload<T>(0xaaaaaaaa55555555ull);
            V data=random_vector<V>(); Index index; index.load(indexes); scatter(index,limit,data,destination);
            for(int lane=0;lane<V::size();++lane) if(uint64_t(U(indexes[lane]))<limit) expected[U(indexes[lane])]=data[lane];
            CHECK(std::memcmp(destination,expected,limit*sizeof(T))==0);
        }
    }
    scatter(Index(I(-1)),0,random_vector<V>(),static_cast<T*>(nullptr));
    if constexpr(V::size()==4) {
        fixed_scatter<-1,-2,-3,-4>(random_vector<V>(),nullptr);
        gather_case<V,0,0,0,0>(); gather_case<V,2,1,0,2>(); gather_case<V,0,2,4,7>(); gather_case<V,100,0,127,64>();
        scatter_case<V,15,0,1,2>(); scatter_case<V,3,-1,V_DC,1>(); scatter_case<V,1,1,1,1>();
    }
    else {
        fixed_scatter<-1,-2>(random_vector<V>(),nullptr);
        gather_case<V,0,0>(); gather_case<V,1,0>(); gather_case<V,127,3>();
        scatter_case<V,15,0>(); scatter_case<V,-1,1>(); scatter_case<V,1,1>();
    }
}
template<unsigned B> void byte_shift() {
    auto a=random_vector<Vec16c>(); auto up=shift_bytes_up<B>(a),down=shift_bytes_down<B>(a);
    for(unsigned i=0;i<16;++i) { CHECK(up[i]==(i>=B?a[i-B]:0)); CHECK(down[i]==(i+B<16?a[i+B]:0)); }
}
}
int main() {
    broadcasts<Vec16c>(); broadcasts<Vec16uc>(); broadcasts<Vec8s>(); broadcasts<Vec8us>();
    broadcasts<Vec4i>(); broadcasts<Vec4ui>(); broadcasts<Vec2q>(); broadcasts<Vec2uq>();
    broadcasts<Vec4f>(); broadcasts<Vec2d>();
    two_lane_patterns<Vec2q>(); two_lane_patterns<Vec2uq>(); two_lane_patterns<Vec2d>();
#if MAX_VECTOR_SIZE >= 256
    broadcasts<Vec4d>();
#endif
#if MAX_VECTOR_SIZE >= 512
    broadcasts<Vec8d>();
#endif
    permutations<Vec16c>(std::make_index_sequence<96>{}); permutations<Vec16uc>(std::make_index_sequence<96>{});
    permutations<Vec8s>(std::make_index_sequence<96>{}); permutations<Vec8us>(std::make_index_sequence<96>{});
    permutations<Vec4i>(std::make_index_sequence<96>{}); permutations<Vec4ui>(std::make_index_sequence<96>{});
    permutations<Vec2q>(std::make_index_sequence<96>{}); permutations<Vec2uq>(std::make_index_sequence<96>{});
    permutations<Vec4f>(std::make_index_sequence<96>{}); permutations<Vec2d>(std::make_index_sequence<96>{});
    register_lookups<1,Vec16c,Vec16c>(); register_lookups<2,Vec16c,Vec16c>();
    register_lookups<1,Vec8s,Vec8s>(); register_lookups<2,Vec8s,Vec8s>();
    register_lookups<1,Vec4i,Vec4i>(); register_lookups<2,Vec4i,Vec4i>(); register_lookups<4,Vec4i,Vec4i>();
    register_lookups<1,Vec2q,Vec2q>(); register_lookups<1,Vec4f,Vec4i>(); register_lookups<2,Vec4f,Vec4i>();
    register_lookups<1,Vec2d,Vec2q>(); register_lookups<2,Vec2d,Vec2q>();
    memory_tables<Vec16c,Vec16c>(); memory_tables<Vec8s,Vec8s>(); memory_tables<Vec4i,Vec4i>(); memory_tables<Vec2q,Vec2q>();
    memory_tables<Vec4f,Vec4i>(); memory_tables<Vec2d,Vec2q>();
    memory_lookups<65537,Vec8s,Vec8s>();
    indexed_memory<Vec4i,Vec4i>(); indexed_memory<Vec2q,Vec2q>(); indexed_memory<Vec4f,Vec4i>(); indexed_memory<Vec2d,Vec2q>();
    byte_shift<0>(); byte_shift<1>(); byte_shift<7>(); byte_shift<8>(); byte_shift<15>(); byte_shift<16>(); byte_shift<17>(); byte_shift<255>();
    Vec4f f; float bits4[4]={payload<float>(0x80000000u),payload<float>(0x7fc12345u),1.f,-2.f}; f.load(bits4);
    auto changed=change_sign<1,0,1,0>(f); float out4[4]; changed.store(out4);
    for(int i=0;i<4;++i) { uint32_t before; std::memcpy(&before,bits4+i,4); uint32_t after; std::memcpy(&after,out4+i,4); CHECK(after==(before^(i%2?0u:0x80000000u))); }
    auto d=change_sign<1,0>(Vec2d(-0.0,payload<double>(0x7ff8123456789abcull))); CHECK(same_bits(d[0],0.0)); CHECK(same_bits(d[1],payload<double>(0x7ff8123456789abcull)));
    static_assert(std::is_same<decltype(lookup4(Vec4ui(),Vec4ui())),Vec4i>::value);
    static_assert(std::is_same<decltype(blend4<0,4,1,5>(Vec4ui(),Vec4i())),Vec4i>::value);
    auto mixed=blend4<0,4,1,5>(Vec4ui(UINT32_MAX),Vec4i(42)); CHECK(mixed[0]==-1 && mixed[1]==42 && mixed[2]==-1 && mixed[3]==42);
    std::printf("%d movement slice failures\n",failures); return failures!=0;
}
