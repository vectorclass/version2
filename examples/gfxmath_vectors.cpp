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

#include <cstdio>
#define GFXMATH_VEC3
#define GFXMATH_VEC4
#include "GfxMath.h"
int main() {
const dex::vec3f position(1.f, 2.f, 3.f);
const dex::vec3f velocity(2.f, 0.f, 0.f);
const auto nextPosition = position + velocity * 0.5f; // (2, 2, 3)
const float speed = velocity.length();               // 2
const auto direction = velocity.normalized();
const float alignment = direction.dot(dex::vec3f(1.f, 0.f, 0.f));
const auto normal = dex::vec3f(1.f, 0.f, 0.f)
                        .cross(dex::vec3f(0.f, 1.f, 0.f)); // (0, 0, 1)
const dex::vec3f4 positions(dex::float4(1.f, 2.f, 3.f, 4.f),
                           dex::float4(0.f), dex::float4(0.f));
const dex::vec3f4 velocities(dex::float4(1.f),
                            dex::float4(2.f), dex::float4(0.f));
const auto advanced = positions + velocities * dex::float4(0.5f);
float resultX[4];
advanced.x.store(resultX); // 1.5, 2.5, 3.5, 4.5
const auto move = dex::Matrix4f::translation(dex::vec3f(10.f, 0.f, 0.f));
const auto movedPoint = move.transformPoint(dex::vec3f(1.f, 2.f, 3.f)); // (11, 2, 3)
const auto movedDirection = move.transformDir(dex::vec3f(1.f, 0.f, 0.f)); // (1, 0, 0)
const auto translation = dex::Matrix4f::translation(dex::vec3f(10.f, 0.f, 0.f));
const auto scale = dex::Matrix4f::scale(2.f);
const auto model = translation.multipliedWith(scale);
const auto worldPoint = model.transformPoint(dex::vec3f(1.f, 2.f, 3.f)); // (12, 4, 6)
const auto localPoint = model.inversed().transformPoint(worldPoint); // approx. (1, 2, 3)
    std::printf("world: %.0f %.0f %.0f\n", worldPoint.x, worldPoint.y, worldPoint.z);
}
