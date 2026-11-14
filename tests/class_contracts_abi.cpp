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
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
void class_abi_raw(Vec128b& a,Vec128b const& b) { a=b; }
Vec4i& class_abi_signed(Vec4i& a) { a+=Vec4i(1); return a; }
Vec4ui class_abi_unsigned(Vec4ui a) { a+=Vec4ui(1); return a; }
Vec4ib class_abi_mask(Vec4ib a) { return !a; }
