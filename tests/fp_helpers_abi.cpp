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
Vec4f fp_helpers_abi(Vec4f a,Vec4f b,Vec4f c,Vec4fb mask) { return select(mask,mul_sub_x(a,b,c),a); }
Vec2d fp_helpers_abi(Vec2d a,Vec2d b,Vec2d c,Vec2db mask) { return select(mask,mul_sub_x(a,b,c),a); }
