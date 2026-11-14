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

# Benchmarks are explicit opt-ins and are never dependencies of vcl::vcl.
if(NOT VCL_TARGET_ARCH STREQUAL "arm64")
  message(FATAL_ERROR "The three-way benchmark compares both ARM backends; use an ARM64 build")
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  message(FATAL_ERROR "Benchmark compiler flags currently support Clang/GCC")
endif()
foreach(backend scalar compat neon)
  add_executable(standard_${backend} standard_math/main.cpp standard_math/kernels.cpp)
  target_compile_features(standard_${backend} PRIVATE cxx_std_20)
  target_compile_options(standard_${backend} PRIVATE -ffp-contract=off)
  if(APPLE)
    target_compile_options(standard_${backend} PRIVATE -mcpu=apple-m1)
  endif()
  if(backend STREQUAL "scalar")
    target_compile_definitions(standard_${backend} PRIVATE STANDARD_IMPL=0)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(standard_${backend} PRIVATE -fno-vectorize -fno-slp-vectorize)
    else()
      target_compile_options(standard_${backend} PRIVATE -fno-tree-vectorize -fno-tree-slp-vectorize)
    endif()
  else()
    target_include_directories(standard_${backend} PRIVATE "${VCL_SOURCE_DIR}")
    if(backend STREQUAL "compat")
      target_compile_definitions(standard_${backend} PRIVATE STANDARD_IMPL=2 VCL_BACKEND=2 MAX_VECTOR_SIZE=512)
    else()
      target_compile_definitions(standard_${backend} PRIVATE STANDARD_IMPL=3 VCL_BACKEND=3 MAX_VECTOR_SIZE=512 VCL_ENABLE_EXPERIMENTAL_NEON=1)
    endif()
  endif()
  if(VCL_SANITIZE)
    target_compile_options(standard_${backend} PRIVATE -fsanitize=undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    target_link_options(standard_${backend} PRIVATE -fsanitize=undefined)
  endif()
  add_test(NAME standard_${backend}_correctness COMMAND standard_${backend} --check)
  set_tests_properties(standard_${backend}_correctness PROPERTIES TIMEOUT 60)
endforeach()
