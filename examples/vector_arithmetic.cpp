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
#include <cstdio>

int main() {
    float input[4] = {1.f, 2.f, 3.f, 4.f}, output[4];
    Vec4f x;
    x.load(input);
    (x * x + Vec4f(1.f)).store(output);
    std::printf("%.0f %.0f %.0f %.0f\n", output[0], output[1], output[2], output[3]);
    return output[0] == 2.f && output[1] == 5.f && output[2] == 10.f && output[3] == 17.f ? 0 : 1;
}
