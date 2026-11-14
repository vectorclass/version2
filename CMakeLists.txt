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

cmake_minimum_required(VERSION 3.20)
project(VCL2XA64 LANGUAGES CXX)

set(VCL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(VCL_BACKEND "auto" CACHE STRING "auto, 1=x86, 2=sse2neon, 3=experimental Direct Neon")
set_property(CACHE VCL_BACKEND PROPERTY STRINGS auto 1 2 3)
set(VCL_NATIVE_MAX_VECTOR_SIZE "128" CACHE STRING "Direct Neon logical maximum: 128, 256 or 512")
option(VCL_ENABLE_EXPERIMENTAL_NEON "Explicitly enable the experimental native backend" OFF)
option(VCL_BUILD_TESTS "Build application-independent regression tests" OFF)
option(VCL_BUILD_EXAMPLES "Build standalone usage examples" OFF)
option(VCL_BUILD_BENCHMARKS "Build standard vector/matrix benchmark programs" OFF)
option(VCL_BUILD_GFXMATH_EXAMPLE "Build the optional external GfxMath example" OFF)
option(VCL_SANITIZE "Use strict nonrecovering UBSan for tests and benchmarks" OFF)
set(VCL_TESTBENCH_DIR "" CACHE PATH "Optional separately fetched pinned upstream testbench")
set(VCL_GFXMATH_DIR "" CACHE PATH "Optional external GfxMath checkout, for its example only")

set(VCL_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
set(VCL_CONFIGURATION_ARCH_FLAGS "")
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
  list(LENGTH CMAKE_OSX_ARCHITECTURES vcl_arch_count)
  if(vcl_arch_count GREATER 1 AND (VCL_BUILD_TESTS OR VCL_BUILD_BENCHMARKS))
    message(FATAL_ERROR "Use separate architecture builds for tests and benchmarks")
  endif()
  if(vcl_arch_count EQUAL 1)
    set(VCL_TARGET_ARCH "${CMAKE_OSX_ARCHITECTURES}")
    set(VCL_CONFIGURATION_ARCH_FLAGS -arch "${VCL_TARGET_ARCH}")
  endif()
endif()
if(VCL_TARGET_ARCH MATCHES "^(arm64|aarch64|ARM64)$")
  set(VCL_TARGET_ARCH arm64)
  set(vcl_default_backend 2)
else()
  set(vcl_default_backend 1)
endif()

add_library(vcl INTERFACE)
add_library(vcl::vcl ALIAS vcl)
target_include_directories(vcl INTERFACE "$<BUILD_INTERFACE:${VCL_SOURCE_DIR}>")
target_compile_features(vcl INTERFACE cxx_std_17)
if(VCL_BACKEND STREQUAL "auto")
  # Leave header selection automatic for each compiler target/slice.
  set(VCL_BACKEND "${vcl_default_backend}")
elseif(VCL_BACKEND MATCHES "^[123]$")
  target_compile_definitions(vcl INTERFACE VCL_BACKEND=${VCL_BACKEND})
else()
  message(FATAL_ERROR "VCL_BACKEND must be auto, 1, 2 or 3")
endif()
if(VCL_BACKEND STREQUAL "3")
  if(NOT VCL_ENABLE_EXPERIMENTAL_NEON)
    message(FATAL_ERROR "Direct Neon requires VCL_ENABLE_EXPERIMENTAL_NEON=ON")
  endif()
  if(NOT VCL_NATIVE_MAX_VECTOR_SIZE MATCHES "^(128|256|512)$")
    message(FATAL_ERROR "VCL_NATIVE_MAX_VECTOR_SIZE must be 128, 256 or 512")
  endif()
  target_compile_definitions(vcl INTERFACE VCL_ENABLE_EXPERIMENTAL_NEON=1 MAX_VECTOR_SIZE=${VCL_NATIVE_MAX_VECTOR_SIZE})
endif()

# Ordinary vector operations are header-only. Compile this target only when a
# consumer explicitly links runtime capability queries or requests the target.
add_library(vcl_detect STATIC EXCLUDE_FROM_ALL instrset_detect.cpp)
add_library(vcl::runtime ALIAS vcl_detect)
target_link_libraries(vcl_detect PUBLIC vcl)
if(VCL_SANITIZE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(vcl_detect PRIVATE -fsanitize=undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
  target_link_options(vcl_detect INTERFACE -fsanitize=undefined)
endif()

if(VCL_BUILD_TESTS OR VCL_BUILD_BENCHMARKS)
  include(CTest)
endif()
if(VCL_BUILD_TESTS)
  add_subdirectory(tests)
endif()
if(VCL_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
if(VCL_BUILD_EXAMPLES OR VCL_BUILD_GFXMATH_EXAMPLE)
  add_subdirectory(examples)
endif()
