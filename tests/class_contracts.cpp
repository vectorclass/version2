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
#include "vectormath_common.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
void class_abi_raw(Vec128b&,Vec128b const&);
Vec4i& class_abi_signed(Vec4i&);
Vec4ui class_abi_unsigned(Vec4ui);
Vec4ib class_abi_mask(Vec4ib);
namespace {
int failures;
#define CHECK(...) do { if(!(__VA_ARGS__)) { if(failures<20) std::fprintf(stderr,"line %d: %s\n",__LINE__,#__VA_ARGS__); ++failures; } } while(0)
#define SAME(A,B) static_assert(std::is_same<A,B>::value,#A " must be " #B)
template<class V> using Scalar=decltype(std::declval<V>()[0]);
template<class T,class U> T bit_cast(U a) { static_assert(sizeof(T)==sizeof(U)); T b; std::memcpy(&b,&a,sizeof(b)); return b; }
template<class V> void layout() {
    static_assert(sizeof(V)==16 && alignof(V)==16);
    static_assert(std::is_trivially_copyable<V>::value);
    static_assert(std::is_standard_layout<V>::value);
}
template<class S,class U,class M> void integers() {
    using T=Scalar<S>; using UT=Scalar<U>; constexpr int lanes=S::size();
    layout<S>(); layout<U>(); static_assert(std::is_base_of<Vec128b,S>::value && std::is_base_of<S,U>::value);
    static_assert(std::is_convertible<U,S>::value && !std::is_convertible<S,U>::value);
    SAME(decltype(std::declval<S&>().load(nullptr)),S&); SAME(decltype(std::declval<U&>().load(nullptr)),U&);
    SAME(decltype(std::declval<U&>().load_partial(0,nullptr)),S&); SAME(decltype(std::declval<U&>().cutoff(0)),S&);
    SAME(decltype(std::declval<U&>().insert(0,UT(1))),const U); SAME(decltype(std::declval<S&>().insert(0,T(1))),const S);
    SAME(decltype(++std::declval<U&>()),S&); SAME(decltype(std::declval<U&>()++),S);
    SAME(decltype(--std::declval<U&>()),S&); SAME(decltype(std::declval<U&>()--),S);
    SAME(decltype(std::declval<U&>()+=U()),S&); SAME(decltype(std::declval<U&>()-=S()),S&);
    SAME(decltype(std::declval<U&>()*=U()),S&); SAME(decltype(std::declval<U&>()&=U()),S&);
    SAME(decltype(std::declval<U&>()|=U()),S&); SAME(decltype(std::declval<U&>()^=U()),S&);
    SAME(decltype(std::declval<U&>()<<=1),S&); SAME(decltype(std::declval<U&>()>>=1),U&);
    SAME(decltype(-U()),S); SAME(decltype(abs(U())),S); SAME(decltype(abs_saturated(U())),S);
    if constexpr(sizeof(T)!=2) { SAME(decltype(!S()),M); }
    SAME(decltype(andnot(S(),S())),Vec128b);
    using Sum=typename std::conditional<sizeof(T)==1,int32_t,T>::type;
    using USum=typename std::conditional<(sizeof(UT)<=2),uint32_t,UT>::type;
    SAME(decltype(horizontal_add(S())),Sum); SAME(decltype(horizontal_add(U())),USum);
    if constexpr(sizeof(T)<8) {
        using Wide=typename std::conditional<(sizeof(T)<4),int32_t,int64_t>::type;
        using UWide=typename std::make_unsigned<Wide>::type;
        SAME(decltype(horizontal_add_x(S())),Wide); SAME(decltype(horizontal_add_x(U())),UWide);
        CHECK(add_saturated(S(std::numeric_limits<T>::max()),1)[0]==std::numeric_limits<T>::max());
        CHECK(sub_saturated(U(0),1)[0]==0);
    }
    CHECK(if_add(M(true),S(7),3)[0]==10); CHECK(if_sub(M(false),U(7),3)[0]==7); CHECK(if_mul(M(true),U(7),3)[0]==21);
    CHECK(abs(U(std::numeric_limits<UT>::max()))[0]==T(1));
    CHECK(abs_saturated(U(UT(1)<<(sizeof(T)*8-1)))[0]==std::numeric_limits<T>::max());
    U u(UT(5)); S& signed_base=u; Vec128b& raw_base=u; CHECK(static_cast<void*>(&u)==static_cast<void*>(&signed_base));
    signed_base+=S(2); CHECK(u[0]==7); raw_base=Vec128b(S(-1)); CHECK(u[0]==std::numeric_limits<UT>::max());
    auto before=u++; CHECK(before[0]==T(-1) && u[0]==0); (--u)+=S(2); CHECK(u[0]==1);
    alignas(16) UT storage[lanes+2]; for(auto& x:storage) x=UT(9);
    u.load_partial(lanes-1,storage); CHECK(u[lanes-1]==0); CHECK(&u.cutoff(lanes-2)==&signed_base); CHECK(u[lanes-2]==0);
    u=U(UT(13)); u.store_partial(lanes-1,storage+1); CHECK(storage[0]==9 && storage[lanes]==9);
    S s(-1); U assigned; assigned=s; CHECK(assigned[0]==std::numeric_limits<UT>::max());
#if VCL_ARCH_ARM64 || INSTRSET < 10
    layout<M>(); static_assert(std::is_base_of<S,M>::value);
    SAME(decltype(std::declval<M&>().load(nullptr)),S&); SAME(decltype(std::declval<M&>().load_a(nullptr)),S&);
    SAME(decltype(std::declval<M&>().load_partial(0,nullptr)),S&); SAME(decltype(std::declval<M&>().cutoff(0)),S&);
    SAME(decltype(M()+M()),S); SAME(decltype(M()*M()),S); SAME(decltype(M()<<1),S); SAME(decltype(++std::declval<M&>()),S&);
    M m(true); S& numeric=m; Vec128b& raw=m; CHECK(static_cast<void*>(&m)==static_cast<void*>(&raw));
    CHECK((m+m)[0]==T(-2)); numeric+=S(1); CHECK(numeric[0]==0); m=true;
    auto& loaded=m.load_partial(lanes-1,&s); CHECK(&loaded==&numeric); CHECK(m[0] && !m[lanes-1]);
    m=s; CHECK(horizontal_and(m)); raw=Vec128b(S(0)); CHECK(!horizontal_or(m)); m=Vec128b(s); CHECK(horizontal_and(m));
    m.cutoff(lanes-1); CHECK(!m[lanes-1]);
#endif
}
template<class V> void partial_boundaries() {
    using T=Scalar<V>; constexpr int n=V::size(); alignas(16) T input[n],out[n+2];
    for(int i=0;i<n;++i) input[i]=T(i+1);
    for(int count=0;count<=n;++count) {
        V v(T(-1)); v.load_partial(count,count?input:nullptr);
        for(int i=0;i<n;++i) CHECK(v[i]==(i<count?input[i]:T(0)));
        for(auto& x:out) x=T(99); v.store_partial(count,count?out+1:nullptr);
        CHECK(out[0]==99 && out[n+1]==99); for(int i=0;i<count;++i) CHECK(out[i+1]==input[i]);
    }
#if VCL_ARCH_ARM64 || INSTRSET < 10
    // Match the explicit SSE2/SSE4 fallback behavior outside the ordinary range.
    for(int count: {-1,n+1}) {
        V v(T(7)); v.load_partial(count,input);
        if constexpr(sizeof(T)<=2) CHECK(v[0]==(count<0?T(0):input[0]));
        else if constexpr(std::is_same<T,double>::value) CHECK(v[0]==0);
        else CHECK(v[0]==7);
        for(auto& x:out) x=T(99); v=V(T(7)); v.store_partial(count,out+1);
        const bool stored=sizeof(T)<8 || count>0;
        CHECK(out[0]==99 && out[n+1]==99); for(int i=0;i<n;++i) CHECK(out[i+1]==(stored?T(7):T(99)));
    }
#endif
}
void utilities() {
    constexpr int indexes[]={-1,V_DC,1,0}; static_assert(min_index(indexes)==V_DC && max_index(indexes)==1);
    constexpr auto zeros=zero_mask(indexes); static_assert(zeros==12);
    constexpr auto broad=zero_mask_broad<Vec4i>(indexes); static_assert(broad.a[0]==0 && broad.a[2]==-1);
    constexpr auto mask=make_broad_mask<Vec4i>(5); static_assert(mask.a[0]==-1 && mask.a[1]==0 && mask.a[2]==-1);
    SAME(decltype(get_inttype<Vec16c>()),int8_t); SAME(decltype(get_inttype<Vec2d>()),int64_t);
    constexpr int identity[]={0,1,2,3}; static_assert(!(perm_flags<Vec4i>(identity)&perm_perm));
    constexpr int outside[]={0,1,2,4}; static_assert(perm_flags<Vec4i>(outside)&perm_outofrange);
    constexpr int blend[]={0,5,-1,V_DC}; static_assert((blend_flags<Vec4i>(blend)&(blend_a|blend_b))==(blend_a|blend_b));
    constexpr auto low=largeblock_perm(identity); static_assert(low.a[0]==0 && low.a[1]==1);
    constexpr auto bytes=pshufb_mask<Vec4i>(identity); static_assert(bytes.a[0]==0 && bytes.a[15]==15);
    Vec4ui values=constant4ui<0,0x80000000u,0xffffffffu,0x12345678u>();
    CHECK(values[0]==0 && values[1]==0x80000000u && values[2]==UINT32_MAX && values[3]==0x12345678u);
    Vec2q same=constant4ui<0,0x80000000u,0xffffffffu,0x12345678u>(); CHECK(uint64_t(same[0])==0x8000000000000000ull);
    CHECK(bit_scan_reverse_const(0)==-1 && bit_scan_reverse_const(UINT64_MAX)==63);
}
}
int main() {
    integers<Vec16c,Vec16uc,Vec16cb>(); integers<Vec8s,Vec8us,Vec8sb>(); integers<Vec4i,Vec4ui,Vec4ib>(); integers<Vec2q,Vec2uq,Vec2qb>();
    partial_boundaries<Vec16c>(); partial_boundaries<Vec16uc>(); partial_boundaries<Vec8s>(); partial_boundaries<Vec8us>();
    partial_boundaries<Vec4i>(); partial_boundaries<Vec4ui>(); partial_boundaries<Vec2q>(); partial_boundaries<Vec2uq>(); partial_boundaries<Vec4f>(); partial_boundaries<Vec2d>();
    utilities(); Vec4i signed_value(7); CHECK(&class_abi_signed(signed_value)==&signed_value && signed_value[0]==8);
    Vec4ui unsigned_value(7); CHECK(class_abi_unsigned(unsigned_value)[0]==8);
    CHECK(!horizontal_or(class_abi_mask(Vec4ib(true)))); Vec128b& base=signed_value; class_abi_raw(base,Vec4i(-1)); CHECK(signed_value[0]==-1);
    std::printf("%d class/utility contract failures\n",failures); return failures!=0;
}
