# Regression contracts supporting the benchmark

The public regression suite checks complete vector/mask types, boundary-safe
partial memory, lane permutations and wide composition; native partial-memory
and reduction variants exercise alignments, exceptional payloads, signed zeros
and FP control modes. Namespace and ABI translation-unit variants are retained.
Independent numerical fixtures and defined-domain scalar references verify the
selected vector math without using vector results as expected values.

The standard benchmark adds its own independent scalar reference checks,
canaries and output comparisons across scalar/sse2neon/Direct Neon. Those
correctness tests are distinct from timing measurements. The historical
[code-generation record](../benchmarks/standard_math_validation.json) describes
the measured source checkpoint; [migration validation](migration.md) records
the fresh application-independent tree. See [tests](../../tests/README.md) for
commands and [numerical contracts](../numerical-contracts.md) for limits.
