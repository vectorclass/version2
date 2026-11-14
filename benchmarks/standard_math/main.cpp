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

#include "reference.h"
#include <atomic>
#include <cfenv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace standard_bench;
namespace {
using Clock=std::chrono::steady_clock;
struct Environment {
    std::fenv_t saved;
    uint64_t fpcr=0,fpsr=0;
    Environment() {
        std::fegetenv(&saved);
        asm volatile("mrs %0, fpcr":"=r"(fpcr));
        asm volatile("mrs %0, fpsr":"=r"(fpsr));
        uint64_t zero=0; asm volatile("msr fpcr, %0"::"r"(zero):"memory");
        std::feclearexcept(FE_ALL_EXCEPT);
    }
    ~Environment() {
        std::fesetenv(&saved);
        asm volatile("msr fpcr, %0"::"r"(fpcr):"memory");
        asm volatile("msr fpsr, %0"::"r"(fpsr):"memory");
    }
};
struct Totals { size_t shapes=0,fields=0; long double max_epsilon=0; } totals;

template<class T> void measure(Op op,Layout layout,int bits,size_t count,bool check_only,const std::filesystem::path& dumps) {
    Data<T> data(op,layout,count);
    const auto input_hash=data.hash();
    auto execute=kernel<T>(op,layout,bits);
    if(!execute) throw std::runtime_error("unsupported shape");
    execute(data.a.data()+1,data.b.data()+1,data.out.data()+1,count,data.pitch);
    const auto checked=validate(data);
    const auto expected_hash=data.hash(true);
    ++totals.shapes; totals.fields+=checked.fields; totals.max_epsilon=std::max(totals.max_epsilon,checked.max_epsilon);
    std::array<double,9> samples{}; int repeats=0;
    if(!check_only) {
        auto trial=[&](int iterations) {
            const auto start=Clock::now();
            for(int r=0;r<iterations;++r) {
                execute(data.a.data()+1,data.b.data()+1,data.out.data()+1,count,data.pitch);
                // Opaque separately compiled kernel + no LTO + memory barrier
                // prevent repeated stores/math being hoisted or deleted.
                std::atomic_signal_fence(std::memory_order_seq_cst);
            }
            return std::chrono::duration<double,std::nano>(Clock::now()-start).count();
        };
        // A scheduler pause during a single calibration sample must not shrink
        // the measured batches. Use the shortest of three calibration samples.
        auto calibration=[&](int iterations) { return std::min({trial(iterations),trial(iterations),trial(iterations)}); };
        repeats=1; double elapsed=calibration(repeats);
        while(elapsed<1e6 && repeats<(1<<18)) { repeats*=2; elapsed=calibration(repeats); }
        repeats=std::clamp(int(std::ceil(repeats*1e7/std::max(1.,elapsed))),1,1<<20);
        const auto warm=trial(repeats);
        if(warm<5e6) {
            repeats=std::clamp(int(std::ceil(repeats*1e7/std::max(1.,warm))),1,1<<20);
            trial(repeats);
        }
        for(double& sample:samples) sample=trial(repeats);
        const auto after=validate(data);
        if(after.fields!=checked.fields || data.hash(true)!=expected_hash) throw std::runtime_error("timed output changed");
    }
    if(!data.guards() || data.hash()!=input_hash) throw std::runtime_error("input/canary changed");
    std::string dump;
    if(!dumps.empty()) {
        dump=std::string(name(op))+"_"+name(layout)+"_f"+std::to_string(sizeof(T)*8)+"_w"+std::to_string(bits)+"_n"+std::to_string(count)+".bin";
        std::ofstream stream(dumps/dump,std::ios::binary);
        // Canonical AoS output order makes every layout directly comparable.
        for(size_t i=0;i<count;++i) for(int c=0;c<outputs(op);++c) {
            const auto value=data.get(data.out,c,i,outputs(op));
            stream.write(reinterpret_cast<const char*>(&value),sizeof(T));
        }
        if(!stream) throw std::runtime_error("cannot write output");
    }
    uint64_t fpcr; asm volatile("mrs %0, fpcr":"=r"(fpcr));
    if(fpcr!=0) throw std::runtime_error("FPCR changed");
    if(!check_only || !dumps.empty()) {
        std::printf("{\"backend\":%d,\"operation\":\"%s\",\"layout\":\"%s\",\"precision\":%zu,\"bits\":%d,\"count\":%zu,\"repeats\":%d,\"trial_batch_ns\":[",STANDARD_IMPL,name(op).data(),name(layout),sizeof(T)*8,bits,count,repeats);
        for(size_t i=0;i<samples.size();++i) std::printf("%s%.17g",i?",":"",samples[i]);
        std::printf("],\"fields_checked\":%zu,\"max_absolute\":%.17g,\"max_epsilon\":%.17g,\"input_hash\":\"%016llx\",\"output_hash\":\"%016llx\",\"fpcr\":%llu,\"dump\":\"%s\"}\n",
            checked.fields,double(checked.max_absolute),double(checked.max_epsilon),(unsigned long long)input_hash,(unsigned long long)expected_hash,(unsigned long long)fpcr,dump.c_str());
        std::fflush(stdout);
    }
}
template<class T> void precision(size_t count,bool check,const std::filesystem::path& dumps) {
    for(Op op:operations) {
        if constexpr (STANDARD_IMPL==0) {
            measure<T>(op,Layout::object,0,count,check,dumps);
            measure<T>(op,Layout::batch,0,count,check,dumps);
        } else {
            measure<T>(op,Layout::object,sizeof(T)*32,count,check,dumps);
            for(int bits:{128,256,512}) measure<T>(op,Layout::batch,bits,count,check,dumps);
        }
    }
}
}
int main(int argc,char** argv) {
    try {
        bool check=false; size_t count=4096;
        std::filesystem::path dumps;
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--check") check=true;
            else if(arg=="--count" && i+1<argc) {
                const std::string value=argv[++i]; size_t end=0;
                count=std::stoull(value,&end);
                if(end!=value.size() || count<1 || count>1048576) throw std::runtime_error("invalid count");
            } else if(arg=="--dump" && i+1<argc) dumps=argv[++i];
            else throw std::runtime_error("usage: standard_* [--check] [--count 1..1048576] [--dump directory]");
        }
        if(!dumps.empty()) {
            if(std::filesystem::exists(dumps) && !std::filesystem::is_empty(dumps)) throw std::runtime_error("dump directory must be empty");
            std::filesystem::create_directories(dumps);
        }
        Environment environment;
        if(check) for(size_t n:{size_t(1),size_t(3),size_t(7),size_t(17),size_t(129),size_t(257)}) {
            precision<float>(n,true,dumps); precision<double>(n,true,dumps);
        } else { precision<float>(count,false,dumps); precision<double>(count,false,dumps); }
        std::fprintf(stderr,"backend=%d shapes=%zu fields=%zu max_epsilon=%.9g passed\n",STANDARD_IMPL,totals.shapes,totals.fields,double(totals.max_epsilon));
        return 0;
    } catch(const std::exception& error) { std::fprintf(stderr,"%s\n",error.what()); return 1; }
}
