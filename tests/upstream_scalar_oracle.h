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

// Scalar-only modular references for generated upstream tests. Apache-2.0.
#ifndef VCL_TEST_UPSTREAM_SCALAR_ORACLE_H
#define VCL_TEST_UPSTREAM_SCALAR_ORACLE_H
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace vcl_test_oracle {
template<class T> using unsigned_type = typename std::make_unsigned<T>::type;

// The tested ARM/x86 integer formats have no padding and use two's complement.
// Copy the reduced unsigned bit pattern instead of converting an out-of-range
// unsigned value to a signed value or performing overflowing signed arithmetic.
template<class T> inline T from_bits(unsigned_type<T> bits) {
    static_assert(std::is_integral<T>::value && sizeof(T) <= sizeof(uint64_t));
    static_assert(std::numeric_limits<T>::digits == int(sizeof(T)*8) - int(std::is_signed<T>::value));
    T result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
template<class T> inline uint64_t bits(T value) {
    return static_cast<uint64_t>(static_cast<unsigned_type<T>>(value));
}
template<class T> inline T add(T a, T b) {
    return from_bits<T>(static_cast<unsigned_type<T>>(bits(a) + bits(b)));
}
template<class T> inline T subtract(T a, T b) {
    return from_bits<T>(static_cast<unsigned_type<T>>(bits(a) - bits(b)));
}
template<class T> inline T multiply(T a, T b) {
    return from_bits<T>(static_cast<unsigned_type<T>>(bits(a) * bits(b)));
}
template<class T> inline T negate(T a) {
    return from_bits<T>(static_cast<unsigned_type<T>>(uint64_t(0) - bits(a)));
}
// Callers retain the upstream count mask: 0 <= count < sizeof(T)*8.
template<class T> inline T shift_left(T a, unsigned count) {
    return from_bits<T>(static_cast<unsigned_type<T>>(bits(a) << count));
}
}
#endif
