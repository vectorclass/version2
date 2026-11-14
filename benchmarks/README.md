# Standard vector/matrix benchmarks

The standalone C++20 harness compares genuine scalar float/double arithmetic,
sse2neon and Direct Neon for identical vector/matrix operations. It is independent
of application or consumer libraries. Scalar auto-vectorization is disabled;
the SIMD programs use the same operation definitions and validation gates.

On ARM64, build explicitly and check correctness first:

```sh
cmake -S . -B build/bench -DCMAKE_BUILD_TYPE=Release -DVCL_BUILD_BENCHMARKS=ON
cmake --build build/bench -j 3
ctest --test-dir build/bench --output-on-failure --timeout 60
```

The three binaries are `build/bench/benchmarks/standard_scalar`,
`standard_compat` and `standard_neon`. `--check` performs numerical correctness
only. To measure, run each separately with `--count 128` or `--count 4096` and
optionally `--dump <new-directory>`; stdout contains JSON records. Record all
three invocations per backend, nine trials per operation and comparable load.
The program calibrates its repeat count. Retain input hashes, raw rows, dump
files, compiler/flags and variability; do not mix controls from different runs.

`-DVCL_SANITIZE=ON` enables strict UBSan for correctness; do not collect timings
from sanitizer builds. The comparison requires ARM64 because both ARM backends
are measured; the numerical source remains generic.

[Colored results](../docs/benchmarks/STANDARD_MATH_RESULTS.md) are historical
measurements, preserved without new timings. [Measurement notes](../docs/benchmarks/measurement-notes.md)
explain the public metadata, protocol and source checkpoints.
`python3 -B tools/format_benchmarks.py` preserves the established color/label
policy in the standard report. It does not perform a measurement or reconstruct
lost raw trials. Raw evidence from a new run must be retained separately.
