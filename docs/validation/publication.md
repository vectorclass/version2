# Historical public-tree validation — 2026-11-14

This record describes the preceding port checkpoint before migration onto VCL
2.02.04. Its source fingerprints and results identify that historical tree. See
[fresh migration validation](migration.md) for the current VCL2 XA64 tree.

The prepared public tree was validated independently of the local development
workspace. It contains no development-history directory or application-specific
test, and requires no application, proprietary SDK, scene or license service.
The runtime source bodies are unchanged by publication preparation: eight added
headers receive the complete DigitalExpressions license notice; original VCL,
Agner Fog and sse2neon notices remain intact.

## Fresh regression results

Host: Apple M2, macOS 26.7 (25G229), Apple Clang 17.0.0
(`clang-1700.4.4.1`), CMake/CTest 4.4.4 and Python 3.9.6.
ARM binaries run natively; x86_64 binaries run through
translation on this ARM host. These are not physical Intel/AMD results.
Release uses the existing strict numerical options. Debug UBSan uses
`-fsanitize=undefined -fno-sanitize-recover=undefined`, with recovery disabled
and no sanitizer suppressions. Every registered test has a 60-second deadline.
Compiler flags were checked for every compiled regression target. Core and
dispatch targets were rebuilt and rerun after correcting their test-option
links during the harness split; unchanged case binaries were retained.

| Target/backend | Logical maximum | Mode | Passed/registered |
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
| x86 SSE42 (translated) | 512 | Release | 1348/1348 |
| x86 SSE42 (translated) | 512 | Debug, strict UBSan | 1348/1348 |

Total: **8,036 ARM cases and 2,776 translated x86 cases**, all passed,
without skips or UBSan diagnostics. Counts include the selected pinned upstream
matrix where enabled; SSE2 uses the standalone local suite. Compatibility/x86
configurations exercise all supported logical widths. Each Direct Neon maximum
uses its own build. Application integration cases are excluded from these totals.

The optional upstream checkout was supplied explicitly at revision
`a7293702385e5169a5f58b83177f2a43cd59139b`; it is separately licensed and is
not bundled in this source tree. [Test instructions](../../tests/README.md)
describe fetching it and running the same selected matrix.

## Additional checks

- All five ordinary numerical fixture generators passed regeneration in the
  matrix. The extended x86 reference generator independently reproduced 20,435
  entries with 120/180-digit agreement, without a libm/VCL expected-value oracle.
- Eight direct-include example builds passed: ordinary VCL on both ARM backends
  at native maxima 128/256/512 and translated SSE2, plus the optional external
  GfxMath example on compatibility, Direct Neon 512 and translated SSE2.
- The three generic benchmark programs passed correctness checks in Release
  and strict UBSan: six checks, with no new timing measurements.
- A default header-only configuration built no detector, tests or benchmark.
  A parent CMake project with testing enabled likewise built no implicit VCL
  tests. An explicit runtime-query consumer built and ran successfully; CPU
  tuning and sanitizer options were not exported to ordinary consumers.
- Selecting Direct Neon without its experimental opt-in failed as intended.
- Documentation links, public-file boundaries, Python syntax and complete port
  notices passed the repository check. Workflow YAML parsed successfully.

Separately, an unchanged external GfxMath checkout passed its three consumer
checks in Release and strict UBSan for both ARM backends (12 consumer tests).
That local compatibility evidence is excluded from the public regression suite
and totals; its consumer tests are not copied into this repository.

## Source checkpoint and historical measurements

The fingerprint of the root headers, runtime detector and dispatch examples is
`6d0e855b1931538c19164eb333c7b51e3be6ff79eb3ed4d206121808d5cbf1f9`.
The fingerprint of the selected tests, build files, examples and tools is
`cb9533add3665f92f133990998c710ff08409a12ac975e3532094c2f0f386a85`.
Each fingerprint hashes sorted repository-relative paths, a NUL, each file's
SHA256 hex digest and a newline, encoded as UTF-8. They identify this dated
validation checkpoint, rather than imposing a source-hash test on future changes.
The complete per-file manifest, exact commands, per-case outputs and binary
checksums are retained locally with the preparation checkpoint.

The [colored benchmark report](../benchmarks/STANDARD_MATH_RESULTS.md) retains
all 160 measurement rows and all 200 Markdown table lines from the earlier
measurement. CSV data is byte-identical; JSON numeric data is unchanged and
machine-specific metadata paths are redacted. Its historical source hashes
precede these notice/path edits, as explained in
[measurement notes](../benchmarks/measurement-notes.md). No existing experimental
archive, measurement, recovery bundle or historical audit baseline was replaced.

## Remaining coverage

Direct Neon remains experimental. Physical Intel/AMD, another ARM host,
Linux/GCC, other toolchains, FP16/unselected APIs and a functioning ASan host
remain separate coverage gates. This host's documented ASan startup hang was
not retried. ARM exception flags and masks remain unsupported. CI is prepared
for ARM and x86 regression profiles, but no hosted CI job has run as part of
this local preparation. The workflow pins
[checkout v7.0.1](https://github.com/actions/checkout/releases/tag/v7.0.1)
by its full commit and uses the ARM/Intel labels listed in the official
[runner images](https://github.com/actions/runner-images#available-images).
See [backend scope](../backends.md) and
[numerical contracts](../numerical-contracts.md).
