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

#ifndef VCL_TEST_WIDE_TYPES_H
#define VCL_TEST_WIDE_TYPES_H
#include "vectorclass.h"
#ifdef VCL_NAMESPACE
using namespace VCL_NAMESPACE;
#endif
#ifdef VCL_TEST_WIDE512
using WideC=Vec64c; using WideCMask=Vec64cb;
using WideUC=Vec64uc; using WideUCMask=Vec64cb;
using WideS=Vec32s; using WideSMask=Vec32sb;
using WideUS=Vec32us; using WideUSMask=Vec32sb;
using WideI=Vec16i; using WideIMask=Vec16ib;
using WideUI=Vec16ui; using WideUIMask=Vec16ib;
using WideQ=Vec8q; using WideQMask=Vec8qb;
using WideUQ=Vec8uq; using WideUQMask=Vec8qb;
using WideF=Vec16f; using WideFMask=Vec16fb;
using WideD=Vec8d; using WideDMask=Vec8db;
constexpr int wide_bits=512;
#else
using WideC=Vec32c; using WideCMask=Vec32cb;
using WideUC=Vec32uc; using WideUCMask=Vec32cb;
using WideS=Vec16s; using WideSMask=Vec16sb;
using WideUS=Vec16us; using WideUSMask=Vec16sb;
using WideI=Vec8i; using WideIMask=Vec8ib;
using WideUI=Vec8ui; using WideUIMask=Vec8ib;
using WideQ=Vec4q; using WideQMask=Vec4qb;
using WideUQ=Vec4uq; using WideUQMask=Vec4qb;
using WideF=Vec8f; using WideFMask=Vec8fb;
using WideD=Vec4d; using WideDMask=Vec4db;
constexpr int wide_bits=256;
#endif
#endif
