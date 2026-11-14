# Backend support

| Backend | Header selection | Status |
| --- | --- | --- |
| x86 | Automatic on x86, or `VCL_BACKEND=1` | Existing VCL implementation; ISA selected by compiler flags |
| ARM64 sse2neon | Automatic on ARM64, or `VCL_BACKEND=2` | Compatibility implementation using native Neon |
| ARM64 Direct Neon | `VCL_BACKEND=3` and `VCL_ENABLE_EXPERIMENTAL_NEON` | Experimental native implementation |

C++17 is required for the library. Define backend/width options before the first
VCL include. `MAX_VECTOR_SIZE` accepts 128, 256 or 512; header inclusion defaults
to 512. CMake's explicit Direct Neon maximum defaults to 128 and is controlled
with `VCL_NATIVE_MAX_VECTOR_SIZE`. All modules exchanging VCL objects must agree
on backend, width, namespace and other ABI-affecting settings.

The portable core includes signed/unsigned 8/16/32/64-bit integers, FP32/FP64,
broad masks, construction, memory operations, classification, conversions,
division and movement. Wider native vectors compose 128-bit registers. Direct
Neon exposes `native()` and `registertype`; raw SSE interoperability uses x86
or sse2neon. FP16 and Intel SVML are outside Direct Neon support.

On ARM64, legacy x86 capability queries report no x86 hardware features and
`has_neon()` reports Neon. ARM FP control maps rounding and coupled FTZ/DAZ;
exception flags and masks are not emulated. The header does not change the
caller's FP environment merely by being included.

Validation currently uses Apple M2/Apple Clang, including translated x86_64
execution. Physical x86, other ARM hosts, Linux/GCC, additional toolchains and
unselected APIs remain separately unvalidated. CI is configured to collect
additional evidence; unrun CI jobs are not claimed as passes.

Native x86 FP16 in upstream VCL 2.02.04 requires the standard-library
`<stdfloat>` header, which is unavailable on the current validation host even
in C++23 mode. Core instruction-level 12 syntax checks pass; native FP16 runtime
coverage remains a separate toolchain/hardware gate. See
[migration validation](validation/migration.md).
