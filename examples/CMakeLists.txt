# Copyright 2026 DigitalExpressions Sweden
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(VCL_BUILD_EXAMPLES)
  add_executable(vcl_example vector_arithmetic.cpp)
  target_link_libraries(vcl_example PRIVATE vcl::vcl)
endif()
if(VCL_BUILD_GFXMATH_EXAMPLE)
  if(NOT EXISTS "${VCL_GFXMATH_DIR}/Source/GfxMath.h")
    message(FATAL_ERROR "Set VCL_GFXMATH_DIR to the external checkout")
  endif()
  if(VCL_BACKEND STREQUAL "3" AND VCL_NATIVE_MAX_VECTOR_SIZE LESS 512)
    message(FATAL_ERROR "The GfxMath example requires VCL_NATIVE_MAX_VECTOR_SIZE=512")
  endif()
  add_executable(gfxmath_example gfxmath_vectors.cpp)
  target_compile_features(gfxmath_example PRIVATE cxx_std_20)
  target_link_libraries(gfxmath_example PRIVATE vcl::vcl)
  target_include_directories(gfxmath_example PRIVATE "${VCL_GFXMATH_DIR}/Source")
endif()
