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

// Test-only independent x87 references; not part of the distributed VCL headers.
#ifndef VCL_TEST_UPSTREAM_X86_REFERENCE_H
#define VCL_TEST_UPSTREAM_X86_REFERENCE_H
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include "upstream_x86_reference_data.h"
namespace vcl_test_x86_reference {
template<class T> inline long double evaluate(T input,unsigned op) {
    static_assert(sizeof(long double)==16 && std::numeric_limits<long double>::digits==64 && std::numeric_limits<long double>::max_exponent==16384,
                  "This fixture requires the little-endian x86 extended format");
    static_assert(sizeof(T)==4 || sizeof(T)==8);
    uint64_t key=0;std::memcpy(&key,&input,sizeof(T));
    const uint64_t exponent=sizeof(T)==4?0x7f800000ull:0x7ff0000000000000ull;
    const uint64_t fraction=sizeof(T)==4?0x007fffffull:0x000fffffffffffffull;
    if((key&exponent)==exponent) {
        if(key&fraction)return static_cast<long double>(input);
        if(op==0 && input<0)return std::numeric_limits<long double>::quiet_NaN();
        return static_cast<long double>(input);
    }
    if(op==0 && input<=T(-1))return input==T(-1)?-std::numeric_limits<long double>::infinity():std::numeric_limits<long double>::quiet_NaN();
    const X86ReferenceRow* rows=nullptr;size_t count=0;
#define VCL_X86_ROWS(w,n) rows=x86_reference_##w##_##n;count=sizeof(x86_reference_##w##_##n)/sizeof(X86ReferenceRow)
    if constexpr(sizeof(T)==4) {if(op==0){VCL_X86_ROWS(32,0);}}
    else {
        switch(op){case 0:{VCL_X86_ROWS(64,0);break;}case 1:{VCL_X86_ROWS(64,1);break;}case 2:{VCL_X86_ROWS(64,2);break;}case 3:{VCL_X86_ROWS(64,3);break;}}
    }
#undef VCL_X86_ROWS
    size_t low=0,high=count;
    while(low<high){size_t mid=low+(high-low)/2;if(rows[mid].input<key)low=mid+1;else high=mid;}
    if(low==count || rows[low].input!=key) {
        std::fprintf(stderr,"Missing pinned x86 reference: width=%zu op=%u bits=%016llx; regenerate for changed inputs/seed\n",sizeof(T)*8,op,(unsigned long long)key);std::abort();
    }
    unsigned char bytes[16]={};std::memcpy(bytes,&rows[low].significand,8);std::memcpy(bytes+8,&rows[low].exponent,2);
    long double result;std::memcpy(&result,bytes,16);return result;
}
}
#endif
