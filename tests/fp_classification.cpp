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

// Classification must inspect bits even with -ffinite-math-only.
#include "vectorclass.h"
#include <cstdio>
#include <cstring>
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
template<class V,class T,class U> int check(U const* patterns,unsigned count,U exponent,U fraction,U sign) {
    int failures=0;
    for(unsigned i=0;i<count;++i) {
        volatile U input=patterns[i]; U bits=input; T scalar; std::memcpy(&scalar,&bits,sizeof(scalar)); V a(scalar);
        const bool finite=(bits&exponent)!=exponent,zero_exp=(bits&exponent)==0;
        failures+=horizontal_and(is_finite(a))!=finite;
        failures+=horizontal_and(is_inf(a))!=((bits&~sign)==exponent);
        failures+=horizontal_and(is_nan(a))!=(!finite && (bits&fraction)!=0);
        failures+=horizontal_and(is_subnormal(a))!=(zero_exp && (bits&fraction)!=0);
        failures+=horizontal_and(is_zero_or_subnormal(a))!=zero_exp;
        failures+=horizontal_and(sign_bit(a))!=bool(bits&sign);
    }
    return failures;
}
int main() {
    const uint32_t f[]={0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x3f800000,0x7f800000,0xff800000,0x7fc12345,0xff800001};
    const uint64_t d[]={0,0x8000000000000000ull,1,0x8000000000000001ull,0x000fffffffffffffull,0x0010000000000000ull,0x3ff0000000000000ull,0x7ff0000000000000ull,0xfff0000000000000ull,0x7ff8123456789abcull,0xfff0000000000001ull};
    int failures=check<Vec4f,float,uint32_t>(f,11,0x7f800000,0x007fffff,0x80000000)+check<Vec2d,double,uint64_t>(d,11,0x7ff0000000000000ull,0x000fffffffffffffull,0x8000000000000000ull);
    std::printf("%d finite-math classification failures\n",failures); return failures!=0;
}
