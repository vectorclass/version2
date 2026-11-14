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

// Native reduction differential checks. Copyright 2026. Apache-2.0.
#include "vectorclass.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#pragma STDC FENV_ACCESS ON
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
namespace {
uint64_t checks;int failures;
uint64_t read_fpcr() { uint64_t x;asm volatile("mrs %0, fpcr":"=r"(x));return x; }
uint64_t read_fpsr() { uint64_t x;asm volatile("mrs %0, fpsr":"=r"(x));return x; }
void write_fpcr(uint64_t x) { asm volatile("msr fpcr, %0\n\tisb"::"r"(x):"memory"); }
void write_fpsr(uint64_t x) { asm volatile("msr fpsr, %0"::"r"(x):"memory"); }
uint32_t bits(float x) { uint32_t u;std::memcpy(&u,&x,4);return u; }
float from_bits(uint32_t u) { float x;std::memcpy(&x,&u,4);return x; }
void check(bool ok,const char* what) { ++checks;if(!ok) { if(failures<12) std::fprintf(stderr,"%s\n",what);++failures; } }
// Frozen pre-change instruction sequence, kept separate from the candidate.
// Composed vectors preserve their existing low+high reduction tree.
template<class V> __attribute__((noinline)) float previous(V a) {
    if constexpr(V::size()==4) {
        const auto x=a.native();
        const auto pairs=vadd_f32(vget_low_f32(x),vget_high_f32(x));
        return vget_lane_f32(vpadd_f32(pairs,pairs),0);
    } else return previous(a.get_low()+a.get_high());
}
template<class V> __attribute__((noinline)) float current(V a) { return horizontal_add(a); }
template<class V> void compare(const std::array<float,V::size()>& input) {
    V a;a.load(input.data());
    write_fpsr(0);const float expected=previous(a);const uint64_t previous_flags=read_fpsr();
    write_fpsr(0);const float actual=current(a);const uint64_t current_flags=read_fpsr();
    check(bits(expected)==bits(actual),"reduction payload mismatch");
    check(previous_flags==current_flags,"reduction exception flags mismatch");
}
uint32_t next(uint32_t& seed) { seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed; }
template<class V> void random_cases() {
    uint32_t seed=0x6c8e9cf5u+V::size();
    for(int i=0;i<32768;++i) {
        std::array<float,V::size()> input;
        for(auto& x:input)x=from_bits(next(seed));
        compare<V>(input);
    }
}
void exhaustive_native() {
    constexpr uint32_t payload[]={0,0x80000000,1,0x80000001,0x007fffff,0x807fffff,
        0x00800000,0x3f800000,0xbf800000,0x7f7fffff,0xff7fffff,0x7f800000,
        0xff800000,0x7fc12345,0xffc54321,0x7f800001};
    for(uint32_t i=0;i<65536;++i) {
        std::array<float,4> input;
        for(int lane=0;lane<4;++lane)input[lane]=from_bits(payload[(i>>(4*lane))&15]);
        compare<Vec4f>(input);
    }
}
}
int main() {
    const uint64_t saved_fpcr=read_fpcr(),saved_fpsr=read_fpsr();
    // Four rounding modes, flush-to-zero on/off, default-NaN on/off.
    constexpr uint64_t fields=(3ull<<22)|(1ull<<24)|(1ull<<25);
    constexpr uint64_t traps=0x1full<<8;
    for(unsigned mode=0;mode<16;++mode) {
        const auto fpcr=(saved_fpcr&~(fields|traps))|(uint64_t(mode&3)<<22)|
            (uint64_t((mode>>2)&1)<<24)|(uint64_t((mode>>3)&1)<<25);
        write_fpcr(fpcr);check((read_fpcr()&fields)==(fpcr&fields),"FPCR mode mismatch");
        exhaustive_native();random_cases<Vec4f>();
#if MAX_VECTOR_SIZE >= 256
        random_cases<Vec8f>();
#endif
#if MAX_VECTOR_SIZE >= 512
        random_cases<Vec16f>();
#endif
    }
    write_fpcr(saved_fpcr&~(fields|traps));
    // This cancellation witness distinguishes the required grouping from
    // adjacent-pair reduction: (large-large)+(1+1)=2, versus zero.
    check(bits(current(Vec4f(0x1p100f,1.f,-0x1p100f,1.f)))==bits(2.f),"summation grouping changed");
    check(bits(current(Vec4f(-0.f,-0.f,-0.f,-0.f)))==bits(-0.f),"negative zero lost");
    write_fpcr(saved_fpcr);write_fpsr(saved_fpsr);
    std::printf("%llu reduction checks across 16 FPCR modes; %d failures\n",(unsigned long long)checks,failures);
    return failures!=0;
}
