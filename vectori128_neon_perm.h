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

// Experimental native Neon data movement. Copyright 2026. Apache-2.0.
// VCL API and contracts follow vectori128.h / vectorf128.h:
// (c) Copyright 2012-2023 Agner Fog. Apache License version 2.0 or later.
#ifndef VECTORI128_NEON_PERM_H
#define VECTORI128_NEON_PERM_H
#include "vectori128_neon.h"
#include <array>
#include <cstddef>
#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif
namespace neon_detail {
template<int Sources,int... I> constexpr std::array<uint8_t,16> permutation_bytes() {
    constexpr int count=sizeof...(I),width=16/count;
    static_assert(count==2 || count==4 || count==8 || count==16,"Unsupported lane count");
    static_assert((((I>=0 && I<Sources*count) || I==-1 || I==V_DC)&&...),"Index out of range in permute/blend function");
    constexpr int lanes[]={I...}; std::array<uint8_t,16> result{};
    for(int lane=0;lane<count;++lane) for(int byte=0;byte<width;++byte)
        result[lane*width+byte]=lanes[lane]<0?uint8_t(255):uint8_t(lanes[lane]*width+byte);
    return result;
}
template<int I> inline uint64x1_t lane64(uint64x2_t a,uint64x2_t b) {
    if constexpr(I<0) return vdup_n_u64(0);
    else if constexpr(I==0) return vget_low_u64(a);
    else if constexpr(I==1) return vget_high_u64(a);
    else if constexpr(I==2) return vget_low_u64(b);
    else return vget_high_u64(b);
}
template<int... I,class V> inline V permute(V a) {
    static_assert(sizeof...(I)==V::size(),"Wrong permutation index count");
    constexpr auto bytes=permutation_bytes<1,I...>();
    constexpr int lane_indices[]={I...};
    constexpr int lane=lane_indices[0];
    if constexpr(lane>=0 && lane<V::size() && ((I==lane)&&...)) {
        // Preserve lane bits, including FP NaN payloads. A typed lane duplicate
        // lets the compiler fold broadcasts into arithmetic lane operands.
        using R=typename V::register_type;
        if constexpr(V::size()==16) return V(bit_cast<R>(vdupq_laneq_u8(bit_cast<uint8x16_t>(a.native()),lane)));
        else if constexpr(V::size()==8) return V(bit_cast<R>(vdupq_laneq_u16(bit_cast<uint16x8_t>(a.native()),lane)));
        else if constexpr(V::size()==4) return V(bit_cast<R>(vdupq_laneq_u32(bit_cast<uint32x4_t>(a.native()),lane)));
        else return V(bit_cast<R>(vdupq_laneq_u64(bit_cast<uint64x2_t>(a.native()),lane)));
    } else if constexpr(V::size()==2) {
        // Typed lane operations expose identities, swaps and zeroing to the
        // compiler without byte-table indexes. No FP arithmetic touches bits.
        auto source=bit_cast<uint64x2_t>(a.native());
        using R=typename V::register_type;
        if constexpr(lane_indices[0]<0 && lane_indices[1]<0) return V(bit_cast<R>(vdupq_n_u64(0)));
        else if constexpr(lane_indices[1]<0) {
            if constexpr(lane_indices[0]==0) return V(bit_cast<R>(vsetq_lane_u64(0,source,1)));
            else return V(bit_cast<R>(vzip2q_u64(source,vdupq_n_u64(0))));
        } else if constexpr(lane_indices[0]<0) {
            if constexpr(lane_indices[1]==1) return V(bit_cast<R>(vsetq_lane_u64(0,source,0)));
            else return V(bit_cast<R>(vzip1q_u64(vdupq_n_u64(0),source)));
        } else return V(bit_cast<R>(vcombine_u64(
            lane64<lane_indices[0]>(source,source),lane64<lane_indices[1]>(source,source))));
    } else {
        return V(bit_cast<typename V::register_type>(vqtbl1q_u8(bit_cast<uint8x16_t>(a.native()),vld1q_u8(bytes.data()))));
    }
}
template<int... I,class V> inline V blend(V a,V b) {
    static_assert(sizeof...(I)==V::size(),"Wrong blend index count");
    constexpr auto bytes=permutation_bytes<2,I...>();
    // Composed vectors use blends even when every result comes from one half.
    // Route those cases through permute so broadcasts retain their lane form.
    if constexpr(((I<V::size())&&...)) {
        return permute<I...>(a);
    } else if constexpr(((I<0 || I>=V::size())&&...)) {
        return permute<(I<0?I:I-V::size())...>(b);
    } else if constexpr(V::size()==2) {
        constexpr int lane_indices[]={I...};
        auto first=bit_cast<uint64x2_t>(a.native()),second=bit_cast<uint64x2_t>(b.native());
        return V(bit_cast<typename V::register_type>(vcombine_u64(
            lane64<lane_indices[0]>(first,second),lane64<lane_indices[1]>(first,second))));
    } else {
        uint8x16x2_t table={{bit_cast<uint8x16_t>(a.native()),bit_cast<uint8x16_t>(b.native())}};
        return V(bit_cast<typename V::register_type>(vqtbl2q_u8(table,vld1q_u8(bytes.data()))));
    }
}
// The low lane bits select a register entry. Out-of-range register indexes are
// implementation dependent in VCL; native Neon deliberately wraps by table size.
// Narrow within the lane before expanding to byte indexes, so high index bits
// cannot bleed into neighbouring lanes or overflow scalar signed arithmetic.
template<int Registers,class V,class Index> inline V register_lookup(Index index,std::array<V,Registers> const& tables) {
    static_assert(Registers==1 || Registers==2 || Registers==4);
    static_assert(Index::size()==V::size());
    constexpr int width=16/V::size(),entries=Registers*V::size();
    constexpr auto repeat=[] {
        std::array<uint8_t,16> bytes{};
        for(int lane=0;lane<V::size();++lane) for(int byte=0;byte<width;++byte) bytes[lane*width+byte]=uint8_t(lane*width);
        return bytes;
    }();
    constexpr auto offsets=[] {
        std::array<uint8_t,16> bytes{};
        for(int i=0;i<16;++i) bytes[i]=uint8_t(i%width);
        return bytes;
    }();
    auto lanes=bit_cast<uint8x16_t>(index.native());
    if constexpr(width>1) lanes=vqtbl1q_u8(lanes,vld1q_u8(repeat.data()));
    lanes=vandq_u8(lanes,vdupq_n_u8(entries-1));
    const auto lookup=vmlaq_u8(vld1q_u8(offsets.data()),lanes,vdupq_n_u8(width));
    if constexpr(Registers==1) return V(bit_cast<typename V::register_type>(vqtbl1q_u8(bit_cast<uint8x16_t>(tables[0].native()),lookup)));
    else if constexpr(Registers==2) {
        uint8x16x2_t table={{bit_cast<uint8x16_t>(tables[0].native()),bit_cast<uint8x16_t>(tables[1].native())}};
        return V(bit_cast<typename V::register_type>(vqtbl2q_u8(table,lookup)));
    }
    else {
        uint8x16x4_t table={{bit_cast<uint8x16_t>(tables[0].native()),bit_cast<uint8x16_t>(tables[1].native()),
                           bit_cast<uint8x16_t>(tables[2].native()),bit_cast<uint8x16_t>(tables[3].native())}};
        return V(bit_cast<typename V::register_type>(vqtbl4q_u8(table,lookup)));
    }
}
template<int N,class V,class Index> inline V memory_lookup(Index index,void const* table) {
    static_assert(Index::size()==V::size());
    if constexpr(N<=0) return V(0);
    else {
        using Scalar=decltype(V()[0]);
        using U=typename std::make_unsigned<decltype(index[0])>::type;
        Scalar output[V::size()];
        for(int lane=0;lane<V::size();++lane) {
            uint64_t selected=U(index[lane]);
            if constexpr(N!=INT_MAX) {
                if constexpr((N&(N-1))==0) selected&=uint64_t(N-1);
                else if(selected>=uint64_t(N)) selected=uint64_t(N-1);
            }
            // Touch only the selected element, including for very small tables.
            std::memcpy(output+lane,static_cast<uint8_t const*>(table)+selected*sizeof(Scalar),sizeof(Scalar));
        }
        return V().load(output);
    }
}
template<class V,int... I> inline V gather(void const* table) {
    static_assert(sizeof...(I)==V::size());
    static_assert(((I>=0)&&...),"Negative index in gather function");
    using Scalar=decltype(V()[0]);
    constexpr int lane_indexes[]={I...}; Scalar output[V::size()];
    for(int lane=0;lane<V::size();++lane)
        std::memcpy(output+lane,static_cast<uint8_t const*>(table)+size_t(lane_indexes[lane])*sizeof(Scalar),sizeof(Scalar));
    return V().load(output);
}
template<int... I,class V> inline void scatter_fixed(V data,void* destination) {
    static_assert(sizeof...(I)==V::size(),"Wrong scatter index count");
    using Scalar=decltype(data[0]);
    constexpr int lane_indexes[]={I...}; Scalar values[V::size()]; data.store(values);
    // Ascending lane order preserves the ARM compatibility path's last-lane
    // wins behavior for duplicate destinations. Any negative constant skips.
    for(int lane=0;lane<V::size();++lane) if(lane_indexes[lane]>=0)
        std::memcpy(static_cast<uint8_t*>(destination)+size_t(lane_indexes[lane])*sizeof(Scalar),values+lane,sizeof(Scalar));
}
template<class Index,class V> inline void scatter_variable(Index index,uint32_t limit,V data,void* destination) {
    static_assert(Index::size()==V::size());
    using Scalar=decltype(data[0]);
    using U=typename std::make_unsigned<decltype(index[0])>::type;
    Scalar values[V::size()]; data.store(values);
    for(int lane=0;lane<V::size();++lane) {
        const uint64_t selected=U(index[lane]);
        if(selected<uint64_t(limit))
            std::memcpy(static_cast<uint8_t*>(destination)+size_t(selected)*sizeof(Scalar),values+lane,sizeof(Scalar));
    }
}
}
template<int... I> inline Vec16c permute16(Vec16c a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec16c blend16(Vec16c a,Vec16c b) { return neon_detail::blend<I...>(a,b); }
template<int... I> inline Vec16uc permute16(Vec16uc a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec16uc blend16(Vec16uc a,Vec16uc b) { return neon_detail::blend<I...>(a,b); }
template<int... I> inline Vec8s permute8(Vec8s a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec8s blend8(Vec8s a,Vec8s b) { return neon_detail::blend<I...>(a,b); }
template<int... I> inline Vec8us permute8(Vec8us a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec8us blend8(Vec8us a,Vec8us b) { return neon_detail::blend<I...>(a,b); }
template<int... I> inline Vec4i permute4(Vec4i a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec4i blend4(Vec4i a,Vec4i b) { return neon_detail::blend<I...>(a,b); }
template<int... I> inline Vec4ui permute4(Vec4ui a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec4ui blend4(Vec4ui a,Vec4ui b) { return neon_detail::blend<I...>(a,b); }
template<int... I> inline Vec2q permute2(Vec2q a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec2q blend2(Vec2q a,Vec2q b) { return neon_detail::blend<I...>(a,b); }
template<int... I> inline Vec2uq permute2(Vec2uq a) { return neon_detail::permute<I...>(a); }
template<int... I> inline Vec2uq blend2(Vec2uq a,Vec2uq b) { return neon_detail::blend<I...>(a,b); }
inline Vec16c lookup16(Vec16c index,Vec16c t0) { return neon_detail::register_lookup<1,Vec16c>(index,{{t0}}); }
inline Vec16c lookup32(Vec16c index,Vec16c t0,Vec16c t1) { return neon_detail::register_lookup<2,Vec16c>(index,{{t0,t1}}); }
template<int n> inline Vec16c lookup(Vec16c index,void const* table) { return neon_detail::memory_lookup<n,Vec16c>(index,table); }
inline Vec8s lookup8(Vec8s index,Vec8s t0) { return neon_detail::register_lookup<1,Vec8s>(index,{{t0}}); }
inline Vec8s lookup16(Vec8s index,Vec8s t0,Vec8s t1) { return neon_detail::register_lookup<2,Vec8s>(index,{{t0,t1}}); }
template<int n> inline Vec8s lookup(Vec8s index,void const* table) { return neon_detail::memory_lookup<n,Vec8s>(index,table); }
inline Vec4i lookup4(Vec4i index,Vec4i t0) { return neon_detail::register_lookup<1,Vec4i>(index,{{t0}}); }
inline Vec4i lookup8(Vec4i index,Vec4i t0,Vec4i t1) { return neon_detail::register_lookup<2,Vec4i>(index,{{t0,t1}}); }
inline Vec4i lookup16(Vec4i index,Vec4i t0,Vec4i t1,Vec4i t2,Vec4i t3) { return neon_detail::register_lookup<4,Vec4i>(index,{{t0,t1,t2,t3}}); }
template<int n> inline Vec4i lookup(Vec4i index,void const* table) { return neon_detail::memory_lookup<n,Vec4i>(index,table); }
inline Vec2q lookup2(Vec2q index,Vec2q t0) { return neon_detail::register_lookup<1,Vec2q>(index,{{t0}}); }
template<int n> inline Vec2q lookup(Vec2q index,void const* table) { return neon_detail::memory_lookup<n,Vec2q>(index,table); }
template<unsigned int b> inline Vec16c shift_bytes_up(Vec16c a) {
    if constexpr(b>=16) return Vec16c(0);
    else if constexpr(b==0) return a;
    else return Vec16c(vextq_s8(vdupq_n_s8(0),a.native(),16-b));
}
template<unsigned int b> inline Vec16c shift_bytes_down(Vec16c a) {
    if constexpr(b>=16) return Vec16c(0);
    else return Vec16c(vextq_s8(a.native(),vdupq_n_s8(0),b));
}
template<int i0,int i1,int i2,int i3> inline Vec4i gather4i(void const* table) { return neon_detail::gather<Vec4i,i0,i1,i2,i3>(table); }
template<int i0,int i1> inline Vec2q gather2q(void const* table) { return neon_detail::gather<Vec2q,i0,i1>(table); }
template<int i0,int i1,int i2,int i3> inline void scatter(Vec4i data,void* destination) { neon_detail::scatter_fixed<i0,i1,i2,i3>(data,destination); }
template<int i0,int i1> inline void scatter(Vec2q data,void* destination) { neon_detail::scatter_fixed<i0,i1>(data,destination); }
inline void scatter(Vec4i index,uint32_t limit,Vec4i data,void* destination) { neon_detail::scatter_variable(index,limit,data,destination); }
inline void scatter(Vec2q index,uint32_t limit,Vec2q data,void* destination) { neon_detail::scatter_variable(index,limit,data,destination); }
#ifdef VCL_NAMESPACE
}
#endif
#endif
