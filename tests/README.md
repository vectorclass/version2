# Application-independent regression tests

The suite detects changes in VCL behavior: arithmetic and masks, integer
division, conversions, permutations, gather/scatter, protected-page memory
boundaries, namespace/ABI contracts, 256/512 composition, FP environment and
bounded numerical functions. Independent scalar/Decimal references provide
expected values. No application-specific test or proprietary dependency is part
of this suite. POSIX protected-page tests currently target macOS/Linux; other
test toolchains remain unvalidated.

Run from the repository root with CMake 3.20+, Python 3.9+ and Clang/GCC:

```sh
cmake -S . -B build/compat-release -DCMAKE_BUILD_TYPE=Release -DVCL_BUILD_TESTS=ON -DVCL_BACKEND=2
cmake --build build/compat-release -j 3
ctest --test-dir build/compat-release --output-on-failure --timeout 60 -j 6

cmake -S . -B build/neon512-release -DCMAKE_BUILD_TYPE=Release -DVCL_BUILD_TESTS=ON \
  -DVCL_BACKEND=3 -DVCL_ENABLE_EXPERIMENTAL_NEON=ON -DVCL_NATIVE_MAX_VECTOR_SIZE=512
cmake --build build/neon512-release -j 3
ctest --test-dir build/neon512-release --output-on-failure --timeout 60 -j 6
```

Use separate Direct Neon builds for widths 128 and 256. Compatibility and x86
local builds exercise all configured widths. Explicit ARM backends require an
ARM64 compiler target. For strict UBSan use a separate Debug directory with
`-DVCL_SANITIZE=ON` and run with
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. Sanitizer recovery is disabled.
ASan hangs during startup on the current validation host and is not retried.

## Optional pinned upstream matrix

```sh
sh tools/fetch_testbench.sh build/_deps/testbench
cmake -S . -B build/upstream-neon512 -DCMAKE_BUILD_TYPE=Release -DVCL_BUILD_TESTS=ON \
  -DVCL_BACKEND=3 -DVCL_ENABLE_EXPERIMENTAL_NEON=ON -DVCL_NATIVE_MAX_VECTOR_SIZE=512 \
  -DVCL_TESTBENCH_DIR="$PWD/build/_deps/testbench"
cmake --build build/upstream-neon512 -j 3
ctest --test-dir build/upstream-neon512 --output-on-failure --timeout 60 -j 6
```

The revision is `a7293702385e5169a5f58b83177f2a43cd59139b`; CMake checks its Git
revision and source hashes. The separately fetched testbench retains its GPL-3.0
license. Generated copies retain that provenance; expected-value adapters repair
defined-domain scalar references without modifying the original checkout,
vector functions, input generation or numerical tolerances. Embedded matching
fragments in the adapter/configuration originate in that testbench. The selected
matrix is intentionally smaller than the entire upstream suite.

The upstream matrix targets SSE4.2 for x86. Use a separately configured local
SSE2 build for minimum-ISA coverage. Physical Intel/AMD, other ARM platforms,
FP16 and unavailable x86 ISAs remain separate coverage gates.

## Independent fixtures

`generate_*_reference.py --check` regenerates the committed fixtures and checks
agreement at two Decimal precisions. The extended x86 fixture generator uses
exact pinned seed-1 keys in its header when no capture directory is supplied.
Changed keys require explicit regeneration. Unknown selected x86 reference
inputs fail closed. `configuration.py` includes negative compilation controls
and header/backend isolation checks. Test cases and original 60-second deadlines
are preserved. See [numerical contracts](../docs/numerical-contracts.md).
