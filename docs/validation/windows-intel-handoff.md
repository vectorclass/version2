# Native Windows Intel validation: agent handoff

Prepared 2026-11-15. The user wants physical Intel testing on their Windows
machine, with **native Windows testing taking priority**. Execute this handoff
when working on that machine. These instructions are a plan, not Windows test
results. Hosted ARM CI and the earlier translated x86 results are separate
evidence. Do not substitute WSL results for native Windows results.

## Starting point and scope

Use `DigitalExpressions/vcl2-xa64`, branch `neon-port`. The port checkpoint is
`4d220c79da62e96acd1022a56dc24e8ee73d634d`, based on upstream VCL 2.02.04 at
`667577495efaa36a10cbb5d6cb8c942c23913848`. The latest branch also contains this
handoff and CI preparation; record the actual full HEAD before testing.
Read any local `AGENTS.md`, [migration validation](migration.md),
[test instructions](../../tests/README.md), and
[numerical contracts](../numerical-contracts.md) first.

Work only in this library checkout. Preserve other repositories and unrelated
changes. Keep consumer/application tests, SDKs, scene data, local machine paths
and credentials out of the public library. Keep evidence under ignored `build/`;
publish only a portable validation summary after reviewing it. Do not rewrite
the dated migration evidence, relax numerical gates, remove test cases or
increase their 60-second deadlines. No timing collection is required.

For a fresh checkout, run in PowerShell:

```powershell
git clone --branch neon-port https://github.com/DigitalExpressions/vcl2-xa64.git
Set-Location vcl2-xa64
git status --short --branch
git rev-parse HEAD
git merge-base --is-ancestor 4d220c79da62e96acd1022a56dc24e8ee73d634d HEAD
if ($LASTEXITCODE -ne 0) { throw 'Unexpected port ancestry' }
git switch -c windows-intel-validation
```

If the checkout already exists, inspect its branch, HEAD and worktree before
fetching or switching. Do not reset, clean or overwrite it. The separate branch
keeps Windows harness changes reviewable. Do not push it or merge it without
approval in the Windows session.

## Inventory and immediate native smoke test

Identify Windows version, physical CPU vendor/model, x64 process architecture,
installed Visual Studio Build Tools/MSVC, Windows SDK, LLVM/Clang, Python,
CMake and Ninja. Use an **x64 Native Tools developer PowerShell** when invoking
MSVC. Check existing tools before installing anything. Use CMake 3.20+ and
Python 3.9+. Record versions and all command exit codes.

```powershell
Get-CimInstance Win32_Processor | Select-Object Manufacturer, Name, Architecture
[System.Runtime.InteropServices.RuntimeInformation]::OSDescription
[System.Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture
Get-Command cl, clang++, clang-cl, cmake, ctest, ninja, python -ErrorAction SilentlyContinue
cmake --version
python --version
```

If the hardware is AMD or the process is ARM/emulated, label it accurately;
do not report it as physical Intel coverage. Establish the CPU/OS-supported ISA
with the existing runtime detector before executing higher-ISA binaries.

The ordinary header-only library and direct example can be tried before any
harness changes. In a fresh build directory (choose a new suffix if it exists):

```powershell
$smokeBuild = 'build/windows-msvc-smoke-release'
cmake -S . -B $smokeBuild -G Ninja -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release -DVCL_BACKEND=1 -DVCL_BUILD_EXAMPLES=ON
if ($LASTEXITCODE -ne 0) { throw 'Smoke configure failed' }
cmake --build $smokeBuild --verbose
if ($LASTEXITCODE -ne 0) { throw 'Smoke build failed' }
& "$smokeBuild/examples/vcl_example.exe"
if ($LASTEXITCODE -ne 0) { throw 'Smoke execution failed' }
python -B tools/check_repository.py
if ($LASTEXITCODE -ne 0) { throw 'Repository check failed' }
```

Expected example output: `2 5 10 17`. Also check Debug, namespace isolation,
logical widths 128/256/512, and an explicit consumer of `vcl::runtime`.
Smoke success alone does not validate the regression suite or every API.

## Required harness adaptation before the full native suite

The current harness rejects MSVC and uses GNU-style compiler options. Several
tests include POSIX memory APIs unconditionally. Do not present the existing
POSIX commands as ready-to-run native Windows instructions.

1. Add a small test-only protected-memory abstraction using `GetSystemInfo`,
   `VirtualAlloc`, `VirtualProtect` and `VirtualFree` on Windows, retaining
   the POSIX implementation elsewhere. Apply it to `core.cpp`,
   `movement_slice.cpp`, `wide_composition.cpp`, and `fp_partial.cpp`.
   Preserve inaccessible guard pages, allocation/error checks, exact buffer
   boundaries, canaries, every partial count and cleanup. Use actual page size;
   distinguish it from allocation granularity. Use `NOMINMAX` and
   `WIN32_LEAN_AND_MEAN` locally so Windows macros do not pollute VCL headers.
2. Adapt `tests/CMakeLists.txt` to each compiler **driver**, distinguishing
   MSVC/clang-cl from GNU-style clang++. Preserve no implicit FP contraction,
   rounding-mode behavior, NaNs and signed zero. For MSVC, assess `/fp:strict`
   and confirm actual compiler command lines. Keep the classification test's
   deliberate finite-math stress as a separate target; document any compiler
   equivalent precisely. Do not apply broad fast-math flags to the suite.
3. Adapt `tests/configuration.py` compilation probes for native Windows:
   compiler options, include paths, temporary source/object paths, syntax-only
   behavior, namespaces and intentional failures. Carry the selected target/ISA
   options into probes. Preserve every negative control; unsupported options
   must not make an expected failure pass accidentally. Keep its overall
   CTest deadline and positive compilation controls.
4. The optional upstream matrix currently adds `-msse4.2`. Adapt its x86 ISA
   selection for the chosen Windows compiler without allowing unrelated higher
   instructions into the SSE2 baseline. Check existing `_MSC_VER` paths in the
   pinned sources and preserve upstream provenance, adapters and source hashes.
5. Inspect FP environment and ABI tests on native x64 Windows. Investigate
   failures against pristine upstream at the pinned VCL revision using the
   same toolchain/flags. Keep unsigned wrapping and 32-bit Windows `long` versus
   64-bit pointers in mind. Do not weaken references to match the port.
6. Probe sanitizer support with a small intentional-UB executable before
   claiming UBSan coverage. MSVC Debug is not UBSan. Only enable
   `VCL_SANITIZE=ON` when the selected native compiler/runtime demonstrably
   supports strict nonrecovering UBSan. A clear unsupported result is a coverage
   gap, never a sanitizer pass. ASan is a separate optional gate on Windows;
   do not retry the documented startup hang on the original Apple host.

Refer to Microsoft's [virtual-memory API documentation](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc)
and [floating-point option documentation](https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior),
and LLVM's [UBSan documentation](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html).
Validate the selected installed toolchain; documentation alone is not evidence
that its sanitizer runtime or compiler options work.

## Native execution matrix after adaptation

Use separate fresh build directories for compiler, ISA and mode. Start with
MSVC x64 SSE2 Release and Debug, then native Windows Clang if available.
The local suite exercises logical widths 128/256/512 and namespaces. The
historical SSE2 registration count was 40 per profile including six Python
checks; reconcile the current inventory rather than claiming that number
without enumerating it.

After the MSVC adaptation, the intended baseline command shape is:

```powershell
$testBuild = 'build/windows-msvc-sse2-release'
cmake -S . -B $testBuild -G Ninja -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release -DVCL_BACKEND=1 -DVCL_BUILD_TESTS=ON -DVCL_BUILD_EXAMPLES=ON -DVCL_SANITIZE=OFF
if ($LASTEXITCODE -ne 0) { throw 'Regression configure failed' }
cmake --build $testBuild --parallel 3 --verbose
if ($LASTEXITCODE -ne 0) { throw 'Regression build failed' }
ctest --test-dir $testBuild --show-only=json-v1
if ($LASTEXITCODE -ne 0) { throw 'Inventory failed' }
ctest --test-dir $testBuild --output-on-failure --no-tests=error --timeout 60 --parallel 3 --output-junit results.xml
if ($LASTEXITCODE -ne 0) { throw 'Regression tests failed' }
& "$testBuild/examples/vcl_example.exe"
if ($LASTEXITCODE -ne 0) { throw 'Example failed' }
```

Repeat with a fresh Debug directory and `-DCMAKE_BUILD_TYPE=Debug`. Capture
configure/build output, inventory JSON and test logs to separate files as well
as the console, preserving exit codes. Keep Python fixture regeneration enabled.

Then fetch the optional GPL-3.0 testbench into its own ignored checkout using
Git (no shell-script dependency is required on Windows):

```powershell
git clone --no-checkout https://github.com/vectorclass/testbench.git build/_deps/testbench
if ($LASTEXITCODE -ne 0) { throw 'Testbench clone failed' }
git -C build/_deps/testbench checkout --detach a7293702385e5169a5f58b83177f2a43cd59139b
if ($LASTEXITCODE -ne 0) { throw 'Testbench checkout failed' }
```

Inspect an existing destination instead of overwriting it. Configure separate
SSE4.2 Release/Debug builds with `VCL_TESTBENCH_DIR` pointing to that checkout,
after checking physical CPU support and implementing the compiler-specific
selection above. Historical selected SSE4.2 coverage was 1,348 tests per profile;
retain and reconcile the actual registration list. Run strict UBSan profiles
only where the native support probe passes. For supported Clang, retain
`-fno-sanitize-recover=undefined` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

AVX2, AVX-512 and native FP16 are additional gates. Compile probes do not
establish execution coverage. Verify CPU **and OS state support** before each
execution; do not use `-march=native` for a baseline test. Native FP16 also
requires a working `<stdfloat>` implementation; do not bypass that upstream
requirement. Direct Neon and the three-way ARM benchmark are not Windows x64
targets. WSL testing can be added later, clearly labeled Linux/WSL evidence.

## Evidence and completion

Record the initial and final source commits, any uncommitted patch fingerprint,
Windows/CPU/toolchain versions, ISA detection, exact commands, compile/link
flags, registered/pass/fail/skip counts, timeouts, JUnit/log paths and tested
binary SHA256 hashes. Keep raw machine-specific evidence under ignored `build/`.
Write a fresh portable Windows validation record only for tests actually run;
list unavailable sanitizer/ISA/toolchain gates explicitly.

Review all harness changes for preserved coverage, then rerun the ARM profiles
affected by the shared POSIX abstraction or CMake/Python changes before merging.
Keep historical migration manifests unchanged. Finish by reporting native
Windows results and outstanding gates, with a reviewable patch/commit. Leave
publication and merging for explicit approval in that session.

Suggested instruction to the Windows agent:

> Read `docs/validation/windows-intel-handoff.md`, inspect this checkout and
> machine, then implement the native Windows harness adaptations and execute
> the supported Intel matrix. Prioritize MSVC/native Windows; preserve all
> numerical gates and guard-page coverage. Keep raw evidence in `build/` and
> prepare a portable results record. Do not push or merge without my approval.
