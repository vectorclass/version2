/****************************  vector.h   **********************************
* Author:        Agner Fog
* Date created:  2024-01-24
* Last modified: 2024-01-24
* Version:       2.02.01
* Project:       vector class library
* Description: Template wrappers around the Vec<N><T> types.
*
* This makes it possible to write code like
* ```
* #define VCL_NAMESPACE vcl
* constexpr size_t vcl_elements = 8;
* template<typename T>
* using vcl_T = vcl::Vec<vcl_elements, T>;
* using vcl_intv = vcl_T<int>;
* using vcl_floatv = vcl_T<float>;
* ```
*
* For instructions, see vcl_manual.pdf
*
* © Copyright 2012-2023 Agner Fog.
* Apache License version 2.0 or later.
******************************************************************************/
#pragma once

#include <stdint.h> // int32_t, etc.

#include "vectorclass.h"

#ifndef VECTORCLASS_NAMESPACE_details
    #ifdef VCL_NAMESPACE
        #define VECTORCLASS_NAMESPACE_details details // e.g., vcl::details
    #else
        #define VECTORCLASS_NAMESPACE_details vcl_details // ::vcl_details
    #endif
#endif // VCL_NAMESPACE_details

#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif

    namespace VECTORCLASS_NAMESPACE_details
    {
        // Vec_N_T<8, int32_t> == Vec8i
        // https://github.com/vectorclass/manual/raw/master/vcl_manual.pdf
        template <size_t elements_per_vector, typename T> struct Vec_N_T;

        // Table 2.1 from https://github.com/vectorclass/manual/raw/master/vcl_manual.pdf
        // 128 Total bits
        template<> struct Vec_N_T<16, int8_t> final { using type = Vec16c; };
        template<> struct Vec_N_T<16, uint8_t> final { using type = Vec16uc; };
        template<> struct Vec_N_T<8, int16_t> final { using type = Vec8s; };
        template<> struct Vec_N_T<8, uint16_t> final { using type = Vec8us; };
        template<> struct Vec_N_T<4, int32_t> final { using type = Vec4i; };
        template<> struct Vec_N_T<4, uint32_t> final { using type = Vec4ui; };
        template<> struct Vec_N_T<2, int64_t> final { using type = Vec2q; };
        template<> struct Vec_N_T<2, uint64_t> final { using type = Vec2uq; };
        // 256 Total bits
        template<> struct Vec_N_T<32, int8_t> final { using type = Vec32c; };
        template<> struct Vec_N_T<32, uint8_t> final { using type = Vec32uc; };
        template<> struct Vec_N_T<16, int16_t> final { using type = Vec16s; };
        template<> struct Vec_N_T<16, uint16_t> final { using type = Vec16us; };
        template<> struct Vec_N_T<8, int32_t> final { using type = Vec8i; };
        template<> struct Vec_N_T<8, uint32_t> final { using type = Vec8ui; };
        template<> struct Vec_N_T<4, int64_t> final { using type = Vec4q; };
        template<> struct Vec_N_T<4, uint64_t> final { using type = Vec4uq; };
        // 512 Total bits
        template<> struct Vec_N_T<64, int8_t> final { using type = Vec64c; };
        template<> struct Vec_N_T<64, uint8_t> final { using type = Vec64uc; };
        template<> struct Vec_N_T<32, int16_t> final { using type = Vec32s; };
        template<> struct Vec_N_T<32, uint16_t> final { using type = Vec32us; };
        template<> struct Vec_N_T<16, int32_t> final { using type = Vec16i; };
        template<> struct Vec_N_T<16, uint32_t> final { using type = Vec16ui; };
        template<> struct Vec_N_T<8, int64_t> final { using type = Vec8q; };
        template<> struct Vec_N_T<8, uint64_t> final { using type = Vec8uq; };

        // Table 2.2 from https://github.com/vectorclass/manual/raw/master/vcl_manual.pdf
        // 128 Total bits
        template<> struct Vec_N_T<4, float> final { using type = Vec4f; };
        template<> struct Vec_N_T<2, double> final { using type = Vec2d; };
        // 256 Total bits
        template<> struct Vec_N_T<8, float> final { using type = Vec8f; };
        template<> struct Vec_N_T<4, double> final { using type = Vec4d; };
        // 512 Total bits
        template<> struct Vec_N_T<16, float> final { using type = Vec16f; };
        template<> struct Vec_N_T<8, double> final { using type = Vec8d; };

        // Table 2.3 from https://github.com/vectorclass/manual/raw/master/vcl_manual.pdf
        template <size_t elements_per_vector, typename For_use_with> struct Vec_N_b;
        template<> struct Vec_N_b<16, int8_t> final { using type = Vec16cb; };
        template<> struct Vec_N_b<16, uint8_t> final { using type = Vec16cb; };
        template<> struct Vec_N_b<8, int16_t> final { using type = Vec8sb; };
        template<> struct Vec_N_b<8, uint16_t> final { using type = Vec8sb; };
        template<> struct Vec_N_b<4, int32_t> final { using type = Vec4ib; };
        template<> struct Vec_N_b<4, uint32_t> final { using type = Vec4ib; };
        template<> struct Vec_N_b<2, int64_t> final { using type = Vec2qb; };
        template<> struct Vec_N_b<2, uint64_t> final { using type = Vec2qb; };
        template<> struct Vec_N_b<32, int8_t> final { using type = Vec32cb; };
        template<> struct Vec_N_b<32, uint8_t> final { using type = Vec32cb; };
        template<> struct Vec_N_b<16, int16_t> final { using type = Vec16sb; };
        template<> struct Vec_N_b<16, uint16_t> final { using type = Vec16sb; };
        template<> struct Vec_N_b<8, int32_t> final { using type = Vec8ib; };
        template<> struct Vec_N_b<8, uint32_t> final { using type = Vec8ib; };
        template<> struct Vec_N_b<4, int64_t> final { using type = Vec4qb; };
        template<> struct Vec_N_b<4, uint64_t> final { using type = Vec4qb; };
        template<> struct Vec_N_b<64, int8_t> final { using type = Vec64cb; };
        template<> struct Vec_N_b<64, uint8_t> final { using type = Vec64cb; };
        template<> struct Vec_N_b<32, int16_t> final { using type = Vec32sb; };
        template<> struct Vec_N_b<32, uint16_t> final { using type = Vec32sb; };
        template<> struct Vec_N_b<16, int32_t> final { using type = Vec16ib; };
        template<> struct Vec_N_b<16, uint32_t> final { using type = Vec16ib; };
        template<> struct Vec_N_b<8, int64_t> final { using type = Vec8qb; };
        template<> struct Vec_N_b<8, uint64_t> final { using type = Vec8qb; };
        template<> struct Vec_N_b<4, float> final { using type = Vec4fb; };
        template<> struct Vec_N_b<2, double> final { using type = Vec2db; };
        template<> struct Vec_N_b<8, float> final { using type = Vec8fb; };
        template<> struct Vec_N_b<4, double> final { using type = Vec4db; };
        template<> struct Vec_N_b<16, float> final { using type = Vec16fb; };
        template<> struct Vec_N_b<8, double> final { using type = Vec8db; };
                
    } // namespace VECTORCLASS_NAMESPACE_details

    // Vec<8, int32_t> == Vec8i
    // https://github.com/vectorclass/manual/raw/master/vcl_manual.pdf
    template <size_t elements_per_vector, typename T>
    using Vec = VECTORCLASS_NAMESPACE_details::Vec_N_T<elements_per_vector, T>::type;  // not for `bool`

    // Template wrapper, specify size for a fixed type; e.g., Vec_i<8> == Vec8i; not for `bool`.
    template<size_t elements_per_vector> using Vec_c = Vec<elements_per_vector, int8_t>;
    template<size_t elements_per_vector> using Vec_uc = Vec<elements_per_vector, uint8_t>;
    template<size_t elements_per_vector> using Vec_s = Vec<elements_per_vector, int16_t>;
    template<size_t elements_per_vector> using Vec_us = Vec<elements_per_vector, uint16_t>;
    template<size_t elements_per_vector> using Vec_i = Vec<elements_per_vector, int32_t>;
    template<size_t elements_per_vector> using Vec_ui = Vec<elements_per_vector, uint32_t>;
    template<size_t elements_per_vector> using Vec_q = Vec<elements_per_vector, int64_t>;
    template<size_t elements_per_vector> using Vec_uq = Vec<elements_per_vector, uint64_t>;
    template<size_t elements_per_vector> using Vec_f = Vec<elements_per_vector, float>;
    template<size_t elements_per_vector> using Vec_d = Vec<elements_per_vector, double>;
    //template<size_t elements_per_vector> using Vec_b = Vec_N_b<elements_per_vector, bool>;

    // Template wrapper, specify type for a fixed size; e.g., Vec8<int32_t> == Vec8i; not for `bool`.
    template <typename T> using Vec64 = Vec<64, T>;
    template <typename T> using Vec32 = Vec<32, T>;
    template <typename T> using Vec16 = Vec<16, T>;
    template <typename T> using Vec8 = Vec<8, T>;
    template <typename T> using Vec4 = Vec<4, T>;
    template <typename T> using Vec2 = Vec<2, T>;

#ifdef VCL_NAMESPACE
}
#endif