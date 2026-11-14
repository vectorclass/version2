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
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <sys/mman.h>
#include <unistd.h>
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
namespace {
int failures; uint64_t checks;
#define CHECK(x) do { ++checks; if(!(x)) { if(failures<16) std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
template<class V> using Scalar=decltype(V()[0]);
struct Guarded {
    size_t page=size_t(sysconf(_SC_PAGESIZE));
    unsigned char* memory=static_cast<unsigned char*>(mmap(nullptr,3*page,PROT_NONE,MAP_ANON|MAP_PRIVATE,-1,0));
    Guarded() { if(memory==MAP_FAILED || mprotect(memory+page,page,PROT_READ|PROT_WRITE)!=0) std::abort(); }
    ~Guarded() { munmap(memory,3*page); }
    unsigned char* begin() { return memory+page; }
    unsigned char* end() { return memory+2*page; }
};
template<class T> T payload(uint64_t bits) { T value; std::memcpy(&value,&bits,sizeof(T)); return value; }
template<class V> std::array<Scalar<V>,V::size()> values(int rotation) {
    using T=Scalar<V>;
    constexpr uint64_t fp32[]={0,0x80000000,1,0x80000001,0x7f800000,0xff800000,
        0x7fc12345,0xffc54321,0x7f800001,0x007fffff,0x3f800000,0xbf800000};
    constexpr uint64_t fp64[]={0,0x8000000000000000ull,1,0x8000000000000001ull,
        0x7ff0000000000000ull,0xfff0000000000000ull,0x7ff8123456789abcull,
        0xfff8abcdef123456ull,0x7ff0000000000001ull,0x000fffffffffffffull,
        0x3ff0000000000000ull,0xbff0000000000000ull};
    std::array<T,V::size()> data;
    for(int i=0;i<V::size();++i) data[i]=payload<T>((sizeof(T)==4?fp32:fp64)[(rotation+i)%12]);
    return data;
}
template<class V> void load_check(int n,const unsigned char* source,const Scalar<V>* expected) {
    using T=Scalar<V>; V value(T(7)); value.load_partial(n,reinterpret_cast<const T*>(source));
    T actual[V::size()];value.store(actual);
    for(int lane=0;lane<V::size();++lane) {
        const T wanted=lane<n?expected[lane]:T(0);
        CHECK(std::memcmp(actual+lane,&wanted,sizeof(T))==0);
    }
}
template<class V> void run() {
    using T=Scalar<V>;Guarded guarded;
    for(int rotation=0;rotation<12;++rotation) {
        const auto input=values<V>(rotation); V value;value.load(input.data());
        for(int n=0;n<=V::size();++n) {
            const size_t bytes=size_t(n)*sizeof(T);
            // Exact byte bounds at both protected-page edges, including n=0
            // pointing at inaccessible memory. No inactive address may be read.
            for(auto p:{guarded.begin(),guarded.end()-bytes}) {
                if(bytes) std::memcpy(p,input.data(),bytes);
                load_check<V>(n,p,input.data());
                value.store_partial(n,reinterpret_cast<T*>(p));
                if(bytes) CHECK(std::memcmp(p,input.data(),bytes)==0);
            }
            // Exercise every byte alignment, while checking all store canaries.
            for(int offset=0;offset<16;++offset) {
                unsigned char area[192];std::memset(area,0xa5,sizeof(area));
                auto p=area+32+offset;
                if(bytes) std::memcpy(p,input.data(),bytes);
                load_check<V>(n,p,input.data());
                std::memset(area,0xa5,sizeof(area));
                value.store_partial(n,reinterpret_cast<T*>(p));
                for(size_t i=0;i<sizeof(area);++i) {
                    const auto expected=(area+i>=p && area+i<p+bytes)?reinterpret_cast<const unsigned char*>(input.data())[area+i-p]:0xa5;
                    CHECK(area[i]==expected);
                }
            }
        }
    }
    if constexpr(sizeof(V)==16) {
        V zero(T(7));zero.load_partial(0,nullptr);T result[V::size()];zero.store(result);
        for(T lane:result) { const T expected=T(0);CHECK(std::memcmp(&lane,&expected,sizeof(T))==0); }
        zero.store_partial(0,nullptr);
#if VCL_BACKEND == VCL_BACKEND_NEON
        // Preserve the native API's pre-existing count policies. FP32 invalid
        // loads leave the vector unchanged; FP64 invalid loads zero it. FP32
        // invalid stores write a full vector; FP64 clamps to [0,2].
        auto input=values<V>(8);V value;value.load(input.data());
        for(int n:{INT_MIN,-1,V::size()+1,INT_MAX}) {
            V loaded=value;loaded.load_partial(n,reinterpret_cast<T*>(guarded.end()));loaded.store(result);
            for(int i=0;i<V::size();++i) {
                const T expected=sizeof(T)==4?input[i]:T(0);
                CHECK(std::memcmp(result+i,&expected,sizeof(T))==0);
            }
            constexpr size_t width=sizeof(T)*V::size();unsigned char area[width+32];std::memset(area,0xa5,sizeof(area));
            const int count=sizeof(T)==4 || n>0?V::size():0;
            value.store_partial(n,reinterpret_cast<T*>(area+16));
            for(size_t i=0;i<sizeof(area);++i) {
                const auto expected=i>=16 && i<16+size_t(count)*sizeof(T)?reinterpret_cast<const unsigned char*>(input.data())[i-16]:0xa5;
                CHECK(area[i]==expected);
            }
            if constexpr(sizeof(T)==8) if(n<0) value.store_partial(n,reinterpret_cast<T*>(guarded.end()));
        }
#endif
    }
}
#if VCL_BACKEND == VCL_BACKEND_NEON && MAX_VECTOR_SIZE >= 512
void wide_counts() {
    Guarded guarded;
    for(int rotation=0;rotation<12;++rotation) {
        const auto input=values<Vec16f>(rotation);Vec16f value;value.load(input.data());
        // Keep the composed API's existing invalid-count behavior: invalid
        // loads zero all lanes when negative, or retain only the first eight
        // loaded lanes when oversized. Invalid stores write four/eight lanes.
        for(int n:{0,INT_MIN,-1,17,INT_MAX}) {
            auto source=guarded.end()-8*sizeof(float);
            std::memcpy(source,input.data(),8*sizeof(float));
            Vec16f loaded(7.f);
            loaded.load_partial(n,n==0?nullptr:reinterpret_cast<const float*>(n<0?guarded.end():source));
            float actual[16];loaded.store(actual);
            const int load_count=n>16?8:0;
            for(int i=0;i<16;++i) {
                const float expected=i<load_count?input[i]:0.f;
                CHECK(std::memcmp(actual+i,&expected,sizeof(float))==0);
            }
            const int store_count=n==0?0:(n<0?4:8);
            unsigned char area[96];std::memset(area,0xa5,sizeof(area));
            value.store_partial(n,reinterpret_cast<float*>(area+16));
            for(size_t i=0;i<sizeof(area);++i) {
                const auto expected=i>=16 && i<16+size_t(store_count)*sizeof(float)?reinterpret_cast<const unsigned char*>(input.data())[i-16]:0xa5;
                CHECK(area[i]==expected);
            }
            auto edge=guarded.end()-size_t(store_count)*sizeof(float);
            value.store_partial(n,reinterpret_cast<float*>(edge));
            if(store_count) CHECK(std::memcmp(edge,input.data(),size_t(store_count)*sizeof(float))==0);
            if(n==0) value.store_partial(0,nullptr);
        }
    }
}
#endif
}
int main() {
    reset_control_word();run<Vec4f>();run<Vec2d>();
#if MAX_VECTOR_SIZE >= 256
    run<Vec8f>();run<Vec4d>();
#endif
#if MAX_VECTOR_SIZE >= 512
    run<Vec16f>();run<Vec8d>();
#endif
#if VCL_BACKEND == VCL_BACKEND_NEON && MAX_VECTOR_SIZE >= 512
    wide_counts();
#endif
    std::printf("%llu partial-memory checks; %d failures\n",(unsigned long long)checks,failures);return failures!=0;
}
