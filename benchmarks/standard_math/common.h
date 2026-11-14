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
#include <array>
#include <cstddef>
#include <string_view>

namespace standard_bench {
enum class Op { add, scale_add, dot, cross, length, normalize, matvec, matmul, transpose, inverse_trs };
constexpr std::array operations{Op::add,Op::scale_add,Op::dot,Op::cross,Op::length,Op::normalize,
                               Op::matvec,Op::matmul,Op::transpose,Op::inverse_trs};
constexpr std::string_view name(Op op) {
    constexpr std::array names{"vec4_add","vec4_scale_add","vec4_dot","vec3_cross","vec4_length",
                              "vec4_normalize","mat4_vector","mat4_multiply","mat4_transpose","mat4_inverse_trs"};
    return names[int(op)];
}
enum class Layout { object, batch };
constexpr const char* name(Layout layout) { return layout==Layout::object?"object_aos":"batch_soa"; }
constexpr int inputs_a(Op op) { return op>=Op::matvec?16:4; }
constexpr int inputs_b(Op op) {
    return op==Op::matmul?16:(op<=Op::cross || op==Op::matvec?4:0);
}
constexpr int outputs(Op op) {
    return op>=Op::matmul?16:(op==Op::dot || op==Op::length?1:4);
}
template<class T> using Kernel=void(*)(const T*,const T*,T*,size_t,size_t);
Kernel<float> kernel_float(Op,Layout,int bits);
Kernel<double> kernel_double(Op,Layout,int bits);
template<class T> Kernel<T> kernel(Op op,Layout layout,int bits) {
    if constexpr (sizeof(T)==4) return kernel_float(op,layout,bits);
    else return kernel_double(op,layout,bits);
}
}
