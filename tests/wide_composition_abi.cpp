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
#include "wide_types.h"
#define PROBE(V,M) V wide_abi(V a,V b,M m) { return select(m,a+b,a); }
PROBE(WideC,WideCMask) PROBE(WideUC,WideUCMask) PROBE(WideS,WideSMask) PROBE(WideUS,WideUSMask)
PROBE(WideI,WideIMask) PROBE(WideUI,WideUIMask) PROBE(WideQ,WideQMask) PROBE(WideUQ,WideUQMask)
PROBE(WideF,WideFMask) PROBE(WideD,WideDMask)
