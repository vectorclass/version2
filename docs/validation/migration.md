# VCL2 XA64 migration validation — 2026-11-14

The port was migrated onto official VCL **2.02.04**, revision
`667577495efaa36a10cbb5d6cb8c942c23913848`. The new tree retains upstream history, the current upstream
changelog, FP16 updates, instruction selection/detection through level 12,
mask extraction fixes, the namespace-qualified divisor overload and the added
wide lookup overload. Seven overlapping runtime files merged cleanly against
the common ancestor. Original VCL, DigitalExpressions and third-party notices
remain intact. The previous development clone was retained unchanged.

## Fresh selected regression matrix

Host: Apple M2, macOS 26.7 (25G229), Apple Clang 17.0.0
(`clang-1700.4.4.1`), CMake/CTest 4.4.4, Python 3.9.6.
All programs were rebuilt in fresh build directories against this repository.
ARM binaries run natively; x86_64 binaries run through translation. These are
not physical Intel/AMD results.

| Backend | Logical maximum | Mode | Passed/registered |
| --- | --- | --- | --- |
| ARM64 sse2neon | 512 | Release | 1350/1350 |
| ARM64 sse2neon | 512 | Debug, strict UBSan | 1350/1350 |
| ARM64 Direct Neon | 128 | Release | 486/486 |
| ARM64 Direct Neon | 128 | Debug, strict UBSan | 486/486 |
| ARM64 Direct Neon | 256 | Release | 890/890 |
| ARM64 Direct Neon | 256 | Debug, strict UBSan | 890/890 |
| ARM64 Direct Neon | 512 | Release | 1292/1292 |
| ARM64 Direct Neon | 512 | Debug, strict UBSan | 1292/1292 |
| x86 SSE2 (translated) | 512 | Release | 40/40 |
| x86 SSE2 (translated) | 512 | Debug, strict UBSan | 40/40 |
| x86 SSE4.2 (translated) | 512 | Release | 1348/1348 |
| x86 SSE4.2 (translated) | 512 | Debug, strict UBSan | 1348/1348 |

**8,036 ARM cases and 2,776 translated x86 cases passed**, with no skips or
UBSan diagnostics. Compiled regression targets use `-ffp-contract=off`; strict
UBSan targets use `-fsanitize=undefined -fno-sanitize-recover=undefined` with
recovery disabled. Compile flags and tested binary checksums were audited for
every compiled case. Every registered case retains its 60-second deadline.
Input generators, seeds, numerical acceptance gates and test bodies match the
preceding validated public suite.

The optional upstream testbench remains a separate GPL-3.0 checkout pinned at
`a7293702385e5169a5f58b83177f2a43cd59139b`. It was supplied explicitly and is
excluded from the public source tree. See [test commands](../../tests/README.md)
for standalone and pinned-matrix builds. The suite has no application, SDK,
scene or license-service dependency.

## Additional checks

- The three generic benchmark programs passed correctness checks in Release and
  strict UBSan: six checks. No timing measurements were collected.
- Eight direct-include example builds passed: ordinary VCL on ARM compatibility,
  native maxima 128/256/512 and translated SSE2; the optional external GfxMath
  example on compatibility, Direct Neon 512 and translated SSE2.
- All ordinary numerical fixtures regenerated in the matrix. The extended x86
  fixture independently reproduced 20,435 entries with 120/180-digit agreement.
- Default header-only builds and a parent project with testing enabled built no
  implicit VCL tests or runtime detector. An explicit runtime-query consumer
  built and ran. Consumer flags contained no exported CPU tuning or sanitizer.
  Direct Neon selection without its experimental opt-in failed as intended.
- An unchanged external GfxMath checkout passed its three consumer checks for
  both ARM backends in Release and strict UBSan: 12 checks. These are separate
  local compatibility evidence; the public library contains no consumer tests.
- Six compile-only x86 checks passed for instruction levels 8, 11 and 12, each
  in global and isolated namespaces. These checks did not execute AVX2/AVX-512
  instructions. Levels 8/11 also compiled the emulated FP16 header.
- Documentation links, complete native notices, repository boundaries, workflow
  YAML and fetch-script syntax passed their checks.

## Native x86 FP16 toolchain limit

The current Apple standard library lacks `<stdfloat>`, required by the upstream
native x86 FP16 header. A pristine copy of the same upstream revision fails on
that missing header even with C++23. Core instruction-level 12 compilation
passes, but native FP16 compilation/execution is outside this host's validated
coverage. No upstream FP16 implementation was changed to bypass this limit.

## Source and measurement checkpoints

The 90 non-Markdown public inputs have fingerprint
`c39dfadf29fed061c34f541c3c7004dd200d98f10b45600a1fd5d62faf87776c`.
It hashes sorted relative paths, a NUL, each file's SHA256 hex digest and a
newline, encoded as UTF-8. Complete source manifests, exact commands, per-case
outputs and binary checksums are retained with the local migration evidence.
This identifies a dated validation checkpoint; it is not a hash-based test
preventing future source changes.

The [colored standard benchmark report](../benchmarks/STANDARD_MATH_RESULTS.md)
retains all 200 Markdown table lines and every measurement. CSV, JSON results,
reading guide and historical code-generation data are byte-identical to the
preceding published copies. Those measurements predate this newer upstream
base. The [earlier validation record](publication.md) remains explicitly
historical; its fingerprints identify the preceding source tree.

Direct Neon remains experimental. Physical Intel/AMD, other ARM hosts,
Linux/GCC, other toolchains and unselected APIs remain separate runtime gates.
This host's documented ASan startup hang was not retried. Hosted CI is prepared
but has not run during the local migration. See [backend scope](../backends.md)
and [numerical contracts](../numerical-contracts.md).
