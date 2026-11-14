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

#include "common.h"
#include <algorithm>
#include <cmath>
#include <type_traits>
#if STANDARD_IMPL != 0
#include "vectorclass.h"
#endif

namespace standard_bench {
namespace {
template<class V> inline V root(V v) { using std::sqrt; return sqrt(v); }
template<class V> inline V nonzero(V v) {
    if constexpr (std::is_arithmetic_v<V>) return v==V(0)?V(1):v;
#if STANDARD_IMPL != 0
    else return select(v==V(0),V(1),v);
#endif
}

// Ordinary algebra shared by scalar objects and batched lanes. Matrix storage
// is column-major. Dot grouping matches the packed four-component reduction.
template<Op op,class V> inline void algebra(const V* a,const V* b,V* out) {
    if constexpr (op==Op::add || op==Op::scale_add) {
        for(int c=0;c<4;++c) {
            if constexpr (op==Op::add) out[c]=a[c]+b[c];
            else out[c]=a[c]*V(0.625)+b[c];
        }
    } else if constexpr (op==Op::dot || op==Op::length || op==Op::normalize) {
        V sum;
        if constexpr (op==Op::dot) sum=(a[0]*b[0]+a[1]*b[1])+(a[2]*b[2]+a[3]*b[3]);
        else sum=(a[0]*a[0]+a[1]*a[1])+(a[2]*a[2]+a[3]*a[3]);
        if constexpr (op==Op::dot) out[0]=sum;
        else if constexpr (op==Op::length) out[0]=root(sum);
        else {
            const auto inverse=V(1)/nonzero(root(sum));
            for(int c=0;c<4;++c) out[c]=a[c]*inverse;
        }
    } else if constexpr (op==Op::cross) {
        out[0]=a[1]*b[2]-a[2]*b[1]; out[1]=a[2]*b[0]-a[0]*b[2];
        out[2]=a[0]*b[1]-a[1]*b[0]; out[3]=V(0);
    } else if constexpr (op==Op::matvec || op==Op::matmul) {
        for(int c=0;c<(op==Op::matvec?1:4);++c) for(int r=0;r<4;++r)
            out[c*4+r]=((a[r]*b[c*4]+a[4+r]*b[c*4+1])+a[8+r]*b[c*4+2])+a[12+r]*b[c*4+3];
    } else if constexpr (op==Op::transpose) {
        for(int c=0;c<4;++c) for(int r=0;r<4;++r) out[c*4+r]=a[r*4+c];
    } else if constexpr (op==Op::inverse_trs) {
        V scale[3];
        for(int c=0;c<3;++c) scale[c]=V(1)/nonzero((a[4*c]*a[4*c]+a[4*c+1]*a[4*c+1])+a[4*c+2]*a[4*c+2]);
        for(int c=0;c<3;++c) {
            for(int r=0;r<3;++r) out[c*4+r]=a[r*4+c]*scale[r];
            out[c*4+3]=V(0);
        }
        for(int r=0;r<3;++r) out[12+r]=V(0)-((out[r]*a[12]+out[4+r]*a[13])+out[8+r]*a[14]);
        out[15]=V(1);
    }
}

template<Op op,class T> __attribute__((noinline)) void object_scalar(const T* __restrict a,const T* __restrict b,T* __restrict out,size_t count,size_t) {
    for(size_t i=0;i<count;++i) algebra<op>(a+i*inputs_a(op),b+i*inputs_b(op),out+i*outputs(op));
}
template<class V,class T> inline V read(const T* p,int active) {
    if constexpr (std::is_arithmetic_v<V>) return *p;
    else { V v; if(active==V::size()) v.load(p); else v.load_partial(active,p); return v; }
}
template<class V,class T> inline void write(T* p,int active,V v) {
    if constexpr (std::is_arithmetic_v<V>) *p=v;
    else { if(active==V::size()) v.store(p); else v.store_partial(active,p); }
}
template<class V> constexpr int lanes() {
    if constexpr (std::is_arithmetic_v<V>) return 1; else return V::size();
}
template<Op op,class T,class V> __attribute__((noinline)) void batch(const T* __restrict a,const T* __restrict b,T* __restrict out,size_t count,size_t pitch) {
    for(size_t i=0;i<count;i+=lanes<V>()) {
        const int active=int(std::min(size_t(lanes<V>()),count-i));
        V av[16],bv[16],result[16];
        for(int c=0;c<inputs_a(op);++c) av[c]=read<V>(a+c*pitch+i,active);
        for(int c=0;c<inputs_b(op);++c) bv[c]=read<V>(b+c*pitch+i,active);
        // The guarded zero denominator keeps inactive inverse lanes finite.
        // No inactive result is stored.
        algebra<op>(av,bv,result);
        for(int c=0;c<outputs(op);++c) write(out+c*pitch+i,active,result[c]);
    }
}

#if STANDARD_IMPL != 0
template<class T> using Four=std::conditional_t<sizeof(T)==4,Vec4f,Vec4d>;
template<class V> inline void transpose(V& a,V& b,V& c,V& d) {
    const auto lo0=blend4<0,1,4,5>(a,b), hi0=blend4<2,3,6,7>(a,b);
    const auto lo1=blend4<0,1,4,5>(c,d), hi1=blend4<2,3,6,7>(c,d);
    a=blend4<0,2,4,6>(lo0,lo1); b=blend4<1,3,5,7>(lo0,lo1);
    c=blend4<0,2,4,6>(hi0,hi1); d=blend4<1,3,5,7>(hi0,hi1);
}
template<class V> inline V product(const V* m,V v) {
    return ((m[0]*permute4<0,0,0,0>(v)+m[1]*permute4<1,1,1,1>(v))
           +m[2]*permute4<2,2,2,2>(v))+m[3]*permute4<3,3,3,3>(v);
}
template<Op op,class T> __attribute__((noinline)) void object_simd(const T* __restrict input_a,const T* __restrict input_b,T* __restrict output,size_t count,size_t) {
    using V=Four<T>;
    for(size_t i=0;i<count;++i) {
        const T* a=input_a+i*inputs_a(op); const T* b=input_b+i*inputs_b(op); T* out=output+i*outputs(op);
        V av; av.load(a);
        if constexpr (op<=Op::cross) {
            V bv; bv.load(b);
            if constexpr (op==Op::add) (av+bv).store(out);
            else if constexpr (op==Op::scale_add) (av*V(0.625)+bv).store(out);
            else if constexpr (op==Op::dot) *out=horizontal_add(av*bv);
            else {
                const auto c=permute4<1,2,0,3>(av)*permute4<2,0,1,3>(bv)-permute4<2,0,1,3>(av)*permute4<1,2,0,3>(bv);
                blend4<0,1,2,7>(c,V(0)).store(out);
            }
        } else if constexpr (op==Op::length || op==Op::normalize) {
            const T length=std::sqrt(horizontal_add(av*av));
            if constexpr (op==Op::length) *out=length;
            else (av*V(T(1)/(length==T(0)?T(1):length))).store(out);
        } else {
            V m[4]; for(int c=0;c<4;++c) m[c].load(a+4*c);
            if constexpr (op==Op::matvec || op==Op::matmul) {
                for(int c=0;c<(op==Op::matvec?1:4);++c) {
                    V v; v.load(b+4*c); product(m,v).store(out+4*c);
                }
            } else if constexpr (op==Op::transpose) {
                transpose(m[0],m[1],m[2],m[3]);
                for(int c=0;c<4;++c) m[c].store(out+4*c);
            } else {
                const auto translation=m[3]; m[3]=V(0);
                transpose(m[0],m[1],m[2],m[3]);
                const auto sizes=(m[0]*m[0]+m[1]*m[1])+m[2]*m[2];
                const auto inverse=V(1)/select(sizes==V(0),V(1),sizes);
                for(int c=0;c<3;++c) m[c]*=inverse;
                m[3]=V(0,0,0,1)-((m[0]*permute4<0,0,0,0>(translation)+m[1]*permute4<1,1,1,1>(translation))+m[2]*permute4<2,2,2,2>(translation));
                for(int c=0;c<4;++c) m[c].store(out+4*c);
            }
        }
    }
}
#endif

template<Op op,class T> Kernel<T> choose(Layout layout,int bits) {
#if STANDARD_IMPL == 0
    if(bits!=0) return nullptr;
    return layout==Layout::object?object_scalar<op,T>:batch<op,T,T>;
#else
    if(layout==Layout::object) return bits==int(sizeof(T)*32)?object_simd<op,T>:nullptr;
    if constexpr (sizeof(T)==4) {
        if(bits==128) return batch<op,T,Vec4f>; if(bits==256) return batch<op,T,Vec8f>; if(bits==512) return batch<op,T,Vec16f>;
    } else {
        if(bits==128) return batch<op,T,Vec2d>; if(bits==256) return batch<op,T,Vec4d>; if(bits==512) return batch<op,T,Vec8d>;
    }
    return nullptr;
#endif
}
template<class T> Kernel<T> dispatch(Op op,Layout layout,int bits) {
    switch(op) {
    case Op::add:return choose<Op::add,T>(layout,bits);
    case Op::scale_add:return choose<Op::scale_add,T>(layout,bits);
    case Op::dot:return choose<Op::dot,T>(layout,bits);
    case Op::cross:return choose<Op::cross,T>(layout,bits);
    case Op::length:return choose<Op::length,T>(layout,bits);
    case Op::normalize:return choose<Op::normalize,T>(layout,bits);
    case Op::matvec:return choose<Op::matvec,T>(layout,bits);
    case Op::matmul:return choose<Op::matmul,T>(layout,bits);
    case Op::transpose:return choose<Op::transpose,T>(layout,bits);
    case Op::inverse_trs:return choose<Op::inverse_trs,T>(layout,bits);
    }
    return nullptr;
}
}
Kernel<float> kernel_float(Op op,Layout layout,int bits) { return dispatch<float>(op,layout,bits); }
Kernel<double> kernel_double(Op op,Layout layout,int bits) { return dispatch<double>(op,layout,bits); }
}
