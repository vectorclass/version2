# VCL2 XA64 — x86 and ARM64 Neon

This is DigitalExpressions’ fork of Agner Fog’s **Vector Class Library 2**,
with ARM64 support alongside the existing x86 backend. It provides the familiar
VCL C++ vector types and operators for application-independent SIMD code.

All credit for the original x86 Vector Class Library goes to **Agner Fog**.
We gratefully acknowledge the many hours of extraordinary work he has devoted
to its development. Without his work, this port would not exist.

ARM64 offers two backends: **sse2neon compatibility**, selected by default, and
**Direct Neon**, an explicitly enabled experimental implementation. Both execute
native ARM64 instructions. Logical 128/256/512-bit vectors use one/two/four
128-bit Neon registers. The existing x86 backend selects instructions according
to compiler target flags.

The tested native surface includes the portable integer/FP32/FP64 core, masks,
movement, conversions and the documented in-tree numerical functions.
[Backend support](docs/backends.md) and [numerical contracts](docs/numerical-contracts.md)
define the limits. Direct Neon remains experimental; physical x86 and additional
ARM hosts/toolchains are separate validation gates.

## Use this fork

The library is header-only for ordinary vector operations. Use **C++17 or newer**
and add this repository to your compiler’s include path. Compile
`instrset_detect.cpp` as well if you call its runtime feature-query functions.

```sh
git clone https://github.com/DigitalExpressions/vcl2-xa64.git
cd vcl2-xa64
```

### Direct VCL usage

The following example uses VCL directly, without `dex-gfxmath`. It includes
`vectorclass.h` and operates on VCL's `Vec4f` SIMD lanes; it does not use
GfxMath's spatial vector or matrix types.

A complete runnable version is in [examples/vector_arithmetic.cpp](examples/vector_arithmetic.cpp).
Save this as `example.cpp`:

```cpp
#include "vectorclass.h"
#include <cstdio>

int main() {
    float input[4] = {1.f, 2.f, 3.f, 4.f};
    float output[4];
    Vec4f x;
    x.load(input);
    Vec4f y = x * x + Vec4f(1.f);
    y.store(output);
    std::printf("%.0f %.0f %.0f %.0f\n",
                output[0], output[1], output[2], output[3]);
}
```

On Apple Silicon, use the default compatibility backend:

```sh
xcrun clang++ -std=c++17 -O3 -arch arm64 -mcpu=apple-m1 \
  -ffp-contract=off -I. example.cpp -o example
./example
# 2 5 10 17
```

To select the experimental direct backend:

```sh
xcrun clang++ -std=c++17 -O3 -arch arm64 -mcpu=apple-m1 \
  -ffp-contract=off -I. \
  -DVCL_BACKEND=VCL_BACKEND_NEON -DVCL_ENABLE_EXPERIMENTAL_NEON=1 \
  -DMAX_VECTOR_SIZE=128 example.cpp -o example-neon
./example-neon
```

Define these switches before any VCL header is included. Direct Neon also supports
`-DMAX_VECTOR_SIZE=256` and `512`. To explicitly select compatibility, use
`-DVCL_BACKEND=VCL_BACKEND_SSE2NEON`; its default maximum width is 512, with
128 and 256 also selectable. Its precise division, sqrt and min/max options
default to enabled.

On x86, the original backend is selected automatically from compiler target
macros. For example, on an x86 machine targeting AVX2/FMA:

```sh
clang++ -std=c++17 -O3 -mavx2 -mfma -I. example.cpp -o example
```

Use target flags appropriate to the deployment CPU. `INSTRSET` retains its x86
meaning; direct Neon uses the separate backend switch. On ARM,
`instrset_detect()` reports zero x86 capability and `has_neon()` reports Neon.

### Optional: using dex GfxMath

GfxMath adds spatial vectors and transformation matrices on top of VCL.
These examples use an external `dex-gfxmath` checkout with its VCL backend
integration applied; it is optional and is not a library or test dependency. Add both this repository and `../dex-gfxmath/Source` to
your include paths. GfxMath requires **C++20** and **`MAX_VECTOR_SIZE=512`**,
including on ARM64, because its public aliases include wider logical vectors.

The complete example is [examples/gfxmath_vectors.cpp](examples/gfxmath_vectors.cpp).
Include this setup once, then place the small examples below inside `main()`
or another function:

```cpp
#define GFXMATH_VEC3
#define GFXMATH_VEC4
#include "GfxMath.h"
```

**Vector arithmetic, lengths, dot and cross products:**

```cpp
const dex::vec3f position(1.f, 2.f, 3.f);
const dex::vec3f velocity(2.f, 0.f, 0.f);
const auto nextPosition = position + velocity * 0.5f; // (2, 2, 3)
const float speed = velocity.length();               // 2
const auto direction = velocity.normalized();
const float alignment = direction.dot(dex::vec3f(1.f, 0.f, 0.f));
const auto normal = dex::vec3f(1.f, 0.f, 0.f)
                        .cross(dex::vec3f(0.f, 1.f, 0.f)); // (0, 0, 1)
```

**Update four independent 3D positions with the same expression:**

```cpp
const dex::vec3f4 positions(dex::float4(1.f, 2.f, 3.f, 4.f),
                           dex::float4(0.f), dex::float4(0.f));
const dex::vec3f4 velocities(dex::float4(1.f),
                            dex::float4(2.f), dex::float4(0.f));
const auto advanced = positions + velocities * dex::float4(0.5f);
float resultX[4];
advanced.x.store(resultX); // 1.5, 2.5, 3.5, 4.5
```

`vec3f` represents one XYZ vector. `vec3f4` represents four independent XYZ
vectors: each of its `x`, `y` and `z` members holds four SIMD lanes.
`dex::float4` is GfxMath's alias for VCL's `Vec4f`.

**Transform a point and a direction:**

```cpp
const auto move = dex::Matrix4f::translation(dex::vec3f(10.f, 0.f, 0.f));
const auto movedPoint = move.transformPoint(dex::vec3f(1.f, 2.f, 3.f)); // (11, 2, 3)
const auto movedDirection = move.transformDir(dex::vec3f(1.f, 0.f, 0.f)); // (1, 0, 0)
```

Points include translation; directions ignore it. Use matrix factories or
`dex::Matrix4f(1)` for identity; default construction leaves entries uninitialized.

**Compose transforms and convert back to local coordinates:**

```cpp
const auto translation = dex::Matrix4f::translation(dex::vec3f(10.f, 0.f, 0.f));
const auto scale = dex::Matrix4f::scale(2.f);
const auto model = translation.multipliedWith(scale);
const auto worldPoint = model.transformPoint(dex::vec3f(1.f, 2.f, 3.f)); // (12, 4, 6)
const auto localPoint = model.inversed().transformPoint(worldPoint); // approx. (1, 2, 3)
```

Matrices store columns and act on column vectors: `A.multipliedWith(B)` applies
`B` first, then `A`. The inverse helper supports affine transforms with nonzero,
orthogonal scaled basis columns, as above; it is not a general inverse for
sheared or projective matrices. Use `vec3d` and `Matrix4d` for double precision.

**The application code is x86/ARM64 agnostic:** these expressions and type names
stay the same when compiled for either target. VCL selects the x86 backend on
x86 and sse2neon compatibility on ARM64 by default. Explicitly selecting Direct
Neon changes the SIMD implementation behind the GfxMath types; no architecture
branches or `_mm_*`/Neon intrinsics are needed in these snippets. Wider logical
types compose 128-bit registers on Neon. Recompile for each target and keep
backend/width settings consistent across translation units exchanging vectors.
This is source portability; floating-point results retain the documented
backend-specific numerical limits.

For a source file containing the setup and examples, build with the native
compiler for your target, from this repository's root:

```sh
clang++ -std=c++20 -O3 -ffp-contract=off -DMAX_VECTOR_SIZE=512 \
  -I. -I../dex-gfxmath/Source gfxmath_example.cpp -o gfxmath_example
```

On ARM64, append `-DVCL_BACKEND=3 -DVCL_ENABLE_EXPERIMENTAL_NEON=1` to opt into
Direct Neon. The direct backend remains experimental; the default ARM64
compatibility backend and the x86 backend use the same example source.


## Optional CMake integration

Direct header inclusion needs no build system. CMake 3.20+ exposes `vcl::vcl`
and the separately compiled `vcl::runtime` target for capability queries.
Tests, examples and benchmarks are opt-ins; a normal library configuration
does not build them. No CPU-specific compiler flags are exported to consumers.

```cmake
add_subdirectory(path/to/vcl2-xa64)
target_link_libraries(my_application PRIVATE vcl::vcl)
# Link vcl::runtime as well only when using runtime capability queries.
```

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DVCL_BUILD_TESTS=ON
cmake --build build -j 3
ctest --test-dir build --output-on-failure --timeout 60 -j 6
```

For Direct Neon add `-DVCL_BACKEND=3 -DVCL_ENABLE_EXPERIMENTAL_NEON=ON`
and select `-DVCL_NATIVE_MAX_VECTOR_SIZE=128`, `256` or `512`.
See [testing](tests/README.md) for strict UBSan and the optional pinned upstream
matrix, and [contributing](CONTRIBUTING.md) for regression expectations.

Use `-DVCL_BUILD_EXAMPLES=ON` to build the standalone `vcl_example`. The optional
GfxMath example has its own switch and checkout path:

```sh
cmake -S . -B build/gfx-example -DCMAKE_BUILD_TYPE=Release \
  -DVCL_BUILD_GFXMATH_EXAMPLE=ON -DVCL_GFXMATH_DIR=../dex-gfxmath
cmake --build build/gfx-example -j 3
./build/gfx-example/examples/gfxmath_example
```

For Direct Neon, also select its backend/opt-in and maximum width 512.

## Validation and performance

The [migration validation record](docs/validation/migration.md) distinguishes
fresh public-tree checks from retained earlier measurements. The public suite
tests VCL arithmetic, masks, conversions, memory boundaries, permutations,
wide composition, FP controls and bounded numerical functions. It requires no
application, proprietary SDK, scene data or license service.

[Colored standard vector/matrix results](docs/benchmarks/STANDARD_MATH_RESULTS.md)
compare genuine scalar FP32/FP64, sse2neon and Direct Neon. The published numbers
are retained measurements on Apple M2, not new timings or application-wide
speedup claims. See [benchmark sources and protocol](benchmarks/README.md).

## Repository and attribution

Upstream-compatible headers, runtime detector and dispatch examples stay at the
root. `tests/`, `benchmarks/`, `examples/`, `docs/` and `tools/` contain maintained
development assets. Temporary work, binaries and local evidence are ignored.

VCL retains its [Apache license](LICENSE), existing source notices and
[upstream changelog](changelog.txt). The bundled sse2neon header retains its MIT
notice. See [provenance and third-party notices](docs/third-party.md) and
[fork changes](docs/changes.md). [Upstream VCL](https://github.com/vectorclass/version2)
and its [manual](https://github.com/vectorclass/manual) remain the API references;
the ARM64 limits here supplement them.
