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

#pragma once
#include "common.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace standard_bench {
inline size_t index(Layout layout,int component,size_t item,int components,size_t pitch) {
    return layout==Layout::object?item*components+component:component*pitch+item;
}
struct Random {
    uint64_t state=0x874af31b01753ull;
    double next() { state^=state<<13; state^=state>>7; state^=state<<17; return double(state>>11)*0x1p-53*2-1; }
};
template<class T> struct Data {
    Op op; Layout layout; size_t count,pitch;
    std::vector<T> a,b,out;
    static constexpr T guard=T(-12345.25);
    Data(Op operation,Layout storage,size_t n):op(operation),layout(storage),count(n),pitch(layout==Layout::batch?n+17:n),
        a(pitch*inputs_a(op)+2,guard),b(pitch*inputs_b(op)+2,guard),out(pitch*outputs(op)+2,guard) {
        Random random;
        for(size_t i=0;i<n;++i) {
            for(int c=0;c<inputs_a(op);++c) at(a,c,i,inputs_a(op))=T(random.next()*2);
            for(int c=0;c<inputs_b(op);++c) at(b,c,i,inputs_b(op))=T(random.next()*2);
            if(op==Op::normalize && i%31==0) for(int c=0;c<4;++c) at(a,c,i,4)=T(0);
            if(op==Op::inverse_trs) {
                // Rotation with nonuniform scale and translation; no shear.
                const double x=random.next(),y=random.next(),z=random.next();
                const double cx=std::cos(x),sx=std::sin(x),cy=std::cos(y),sy=std::sin(y),cz=std::cos(z),sz=std::sin(z);
                const double rotation[3][3]={{cz*cy,cz*sy*sx-sz*cx,cz*sy*cx+sz*sx},
                                             {sz*cy,sz*sy*sx+cz*cx,sz*sy*cx-cz*sx},
                                             {-sy,cy*sx,cy*cx}};
                for(int c=0;c<3;++c) {
                    const double scale=1.25+random.next()*.75;
                    for(int r=0;r<3;++r) at(a,c*4+r,i,16)=T(rotation[r][c]*scale);
                    at(a,c*4+3,i,16)=T(0);
                }
                at(a,15,i,16)=T(1);
            }
        }
    }
    T& at(std::vector<T>& values,int c,size_t i,int components) { return values[1+index(layout,c,i,components,pitch)]; }
    T get(const std::vector<T>& values,int c,size_t i,int components) const { return values[1+index(layout,c,i,components,pitch)]; }
    bool guards() const {
        auto guarded=[&](const std::vector<T>& values,int components) {
            if(values.front()!=guard || values.back()!=guard) return false;
            if(layout==Layout::batch) for(int c=0;c<components;++c)
                for(size_t i=count;i<pitch;++i) if(values[1+c*pitch+i]!=guard) return false;
            return true;
        };
        return guarded(a,inputs_a(op)) && guarded(b,inputs_b(op)) && guarded(out,outputs(op));
    }
    uint64_t hash(bool output=false) const {
        uint64_t value=14695981039346656037ull;
        auto add=[&](T x) {
            for(auto byte:std::bit_cast<std::array<unsigned char,sizeof(T)>>(x)) { value^=byte; value*=1099511628211ull; }
        };
        for(size_t i=0;i<count;++i) {
            if(output) { for(int c=0;c<outputs(op);++c) add(get(out,c,i,outputs(op))); }
            else {
                for(int c=0;c<inputs_a(op);++c) add(get(a,c,i,inputs_a(op)));
                for(int c=0;c<inputs_b(op);++c) add(get(b,c,i,inputs_b(op)));
            }
        }
        return value;
    }
};
struct Expected { std::array<long double,16> value{},scale{}; };
// Independent scalar reference: ordinary coordinate loops, hypot, and pivoted
// Gauss-Jordan inversion rather than the timed transpose/scale inverse formula.
template<class T> Expected reference(const Data<T>& data,size_t item) {
    Expected e; e.scale.fill(1);
    long double a[16]{},b[16]{};
    for(int c=0;c<inputs_a(data.op);++c) a[c]=data.get(data.a,c,item,inputs_a(data.op));
    for(int c=0;c<inputs_b(data.op);++c) b[c]=data.get(data.b,c,item,inputs_b(data.op));
    switch(data.op) {
    case Op::add: case Op::scale_add:
        for(int c=0;c<4;++c) e.value[c]=a[c]*(data.op==Op::add?1.L:.625L)+b[c];
        break;
    case Op::dot:
        for(int c=0;c<4;++c) { e.value[0]+=a[c]*b[c]; e.scale[0]+=std::abs(a[c]*b[c]); }
        break;
    case Op::cross:
        for(int c=0;c<3;++c) e.value[c]=a[(c+1)%3]*b[(c+2)%3]-a[(c+2)%3]*b[(c+1)%3];
        break;
    case Op::length: case Op::normalize: {
        const auto length=std::hypot(std::hypot(a[0],a[1]),std::hypot(a[2],a[3]));
        if(data.op==Op::length) e.value[0]=length;
        else for(int c=0;c<4;++c) e.value[c]=length==0?0:a[c]/length;
        break;
    }
    case Op::matvec: case Op::matmul:
        for(int c=0;c<(data.op==Op::matvec?1:4);++c) for(int r=0;r<4;++r)
            for(int k=0;k<4;++k) { e.value[c*4+r]+=a[k*4+r]*b[c*4+k]; e.scale[c*4+r]+=std::abs(a[k*4+r]*b[c*4+k]); }
        break;
    case Op::transpose:
        for(int c=0;c<4;++c) for(int r=0;r<4;++r) e.value[c*4+r]=a[r*4+c];
        break;
    case Op::inverse_trs: {
        long double m[4][8]{};
        for(int r=0;r<4;++r) for(int c=0;c<4;++c) { m[r][c]=a[c*4+r]; m[r][4+c]=(r==c); }
        for(int c=0;c<4;++c) {
            int pivot=c;
            for(int r=c+1;r<4;++r) if(std::abs(m[r][c])>std::abs(m[pivot][c])) pivot=r;
            if(pivot!=c) for(int k=0;k<8;++k) std::swap(m[c][k],m[pivot][k]);
            const auto inverse=1.L/m[c][c];
            for(int k=0;k<8;++k) m[c][k]*=inverse;
            for(int r=0;r<4;++r) if(r!=c) {
                const auto factor=m[r][c];
                for(int k=0;k<8;++k) m[r][k]-=factor*m[c][k];
            }
        }
        for(int c=0;c<4;++c) for(int r=0;r<4;++r) e.value[c*4+r]=m[r][4+c];
        break;
    }
    }
    for(int c=0;c<outputs(data.op);++c) e.scale[c]=std::max(e.scale[c],std::abs(e.value[c]));
    return e;
}
struct Check { size_t fields=0; long double max_absolute=0,max_epsilon=0; };
template<class T> Check validate(const Data<T>& data) {
    if(!data.guards()) throw std::runtime_error("output canary changed");
    Check checked;
    for(size_t i=0;i<data.count;++i) {
        const auto expected=reference(data,i);
        for(int c=0;c<outputs(data.op);++c) {
            const auto actual=data.get(data.out,c,i,outputs(data.op));
            const auto absolute=std::abs(static_cast<long double>(actual)-expected.value[c]);
            const auto error=absolute/(std::numeric_limits<T>::epsilon()*expected.scale[c]);
            if(!std::isfinite(actual) || error>64) throw std::runtime_error(std::string(name(data.op))+" reference mismatch at "+std::to_string(i)+":"+std::to_string(c));
            checked.max_absolute=std::max(checked.max_absolute,absolute); checked.max_epsilon=std::max(checked.max_epsilon,error); ++checked.fields;
        }
    }
    return checked;
}
}
