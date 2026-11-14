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

// Separate translation unit to exercise by-value vector/divisor calls.
#include "vectorclass.h"
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
Vec4i slice_divide_abi(Vec4i a,Divisor_i d) { return a/d; }
Vec4ui slice_divide_abi(Vec4ui a,Divisor_ui d) { return a/d; }
Vec8s slice_divide_abi(Vec8s a,Divisor_s d) { return a/d; }
Vec8us slice_divide_abi(Vec8us a,Divisor_us d) { return a/d; }
Vec16c slice_divide_abi(Vec16c a,Divisor_s d) { return a/d; }
Vec16uc slice_divide_abi(Vec16uc a,Divisor_us d) { return a/d; }
