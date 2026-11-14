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
#if VCL_BACKEND != VCL_BACKEND_NEON
#include "vectormath_exp.h"
#include "vectormath_trig.h"
#include "vectormath_hyp.h"
#endif
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>
#include <sys/mman.h>
#include <unistd.h>
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
static int failures;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
template<class V, class T> void integers() {
    constexpr int n=V::size(); T a[n],b[n],out[n];
    for(int i=0;i<n;++i) { a[i]=T(i*13+1); b[i]=T(i*3+2); }
    V x,y; x.load(a); y.load(b); (x+y).store(out);
    for(int i=0;i<n;++i) CHECK(out[i]==T(a[i]+b[i]));
    (x-y).store(out); for(int i=0;i<n;++i) CHECK(out[i]==T(a[i]-b[i]));
    (x^y).store(out); for(int i=0;i<n;++i) CHECK(out[i]==T(a[i]^b[i]));
    (x<<1).store(out); for(int i=0;i<n;++i) CHECK(out[i]==T(typename std::make_unsigned<T>::type(a[i])<<1));
    auto m=x<y; select(m,x,y).store(out);
    for(int i=0;i<n;++i) CHECK(out[i]==(a[i]<b[i]?a[i]:b[i]));
    CHECK(horizontal_and(x==x)); CHECK(!horizontal_or(x!=x));
}
template<class V,class T> void floats() {
    constexpr int n=V::size(); T a[n],out[n]; for(int i=0;i<n;++i) a[i]=T(i+1)/T(n);
    V x; x.load(a); ((x+V(T(1)))*V(T(2))).store(out);
    for(int i=0;i<n;++i) CHECK(out[i]==(a[i]+T(1))*T(2));
    sqrt(x*x).store(out); for(int i=0;i<n;++i) CHECK(std::abs(out[i]-a[i])<T(1e-6));
#if VCL_BACKEND != VCL_BACKEND_NEON
    exp(x).store(out); for(int i=0;i<n;++i) CHECK(std::abs(out[i]-std::exp(a[i]))<T(2e-6));
    sin(x).store(out); for(int i=0;i<n;++i) CHECK(std::abs(out[i]-std::sin(a[i]))<T(2e-6));
    tanh(x).store(out); for(int i=0;i<n;++i) CHECK(std::abs(out[i]-std::tanh(a[i]))<T(2e-6));
#endif
    V q(std::numeric_limits<T>::quiet_NaN()), zero(T(0)), negzero(T(-0.0));
    CHECK(horizontal_and(is_nan(q))); CHECK(horizontal_and(is_inf(V(std::numeric_limits<T>::infinity()))));
    CHECK(horizontal_and(is_finite(x))); CHECK(!horizontal_or(is_finite(q)));
    min(q,x).store(out); for(int i=0;i<n;++i) CHECK(out[i]==a[i]);
    max(zero,negzero).store(out); for(T f:out) CHECK(std::signbit(f));
    const long page=sysconf(_SC_PAGESIZE);
    void* mem=mmap(nullptr,2*page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    CHECK(mem!=MAP_FAILED); if(mem!=MAP_FAILED) {
      CHECK(mprotect(static_cast<char*>(mem)+page,page,PROT_NONE)==0);
      for(int count=0;count<=n;++count) {
        T* p=reinterpret_cast<T*>(static_cast<char*>(mem)+page)-count;
        for(int i=0;i<count;++i) p[i]=T(i+1);
        V part; part.load_partial(count,p); part.store(out);
        for(int i=0;i<n;++i) CHECK(out[i]==(i<count?T(i+1):T(0)));
        x.store_partial(count,p); for(int i=0;i<count;++i) CHECK(p[i]==a[i]);
      }
      munmap(mem,2*page);
    }
}
int main() {
    integers<Vec16c,int8_t>(); integers<Vec16uc,uint8_t>(); integers<Vec8s,int16_t>(); integers<Vec8us,uint16_t>();
    integers<Vec4i,int32_t>(); integers<Vec4ui,uint32_t>(); integers<Vec2q,int64_t>(); integers<Vec2uq,uint64_t>();
    floats<Vec4f,float>(); floats<Vec2d,double>();
#if MAX_VECTOR_SIZE >= 256
    integers<Vec8i,int32_t>(); integers<Vec4q,int64_t>(); floats<Vec8f,float>(); floats<Vec4d,double>();
#endif
#if MAX_VECTOR_SIZE >= 512
    integers<Vec16i,int32_t>(); integers<Vec8q,int64_t>(); floats<Vec16f,float>(); floats<Vec8d,double>();
#endif
    for(unsigned i=0;i<32;++i) { CHECK(bit_scan_reverse(uint32_t(1)<<i)==i); CHECK(bit_scan_forward(uint32_t(1)<<i)==i); }
    for(unsigned i=0;i<64;++i) CHECK(bit_scan_reverse(uint64_t(1)<<i)==i);
    auto saved=get_control_word();
    for(unsigned rounding=0;rounding<4;++rounding) { set_control_word((saved&~0x6000u)|(rounding<<13)); CHECK((get_control_word()&0x6000)==(rounding<<13)); }
    set_control_word(saved);
#if VCL_ARCH_ARM64
    CHECK(has_neon());
    CHECK(instrset_detect()==0); CHECK(!hasFMA3()); CHECK(!hasF16C()); CHECK(!hasAVX512FP16());
    int words[4]; cpuid(words,1); for(int w:words) CHECK(w==0);
#endif
    std::printf("%d failures (width %d)\n",failures,MAX_VECTOR_SIZE); return failures!=0;
}
