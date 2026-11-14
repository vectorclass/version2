# Fork changes

This fork retains VCL's root header layout and x86 implementation, adds the
selectable ARM64 compatibility/native backends, and documents the experimental
native support boundary. New helpers separate target selection, shared
compile-time algorithms, native integer/FP storage, division and movement.
Wider types compose the selected smaller vector representation.

Earlier validated fixes include small-table memory bounds, integer edge cases,
SSE2 multiplication/large-double rounding, dynamic rounding and finite-math
classification, and bounded native numerical adaptations. Test-only independent
references address original upstream scalar-oracle issues without relaxing the
vector-function acceptance gates.

The public-tree preparation adds complete DigitalExpressions notices to added
files while retaining Agner Fog and third-party attribution. It adds optional
CMake integration, maintained application-independent tests, examples and
benchmark sources, and excludes local development history and build evidence.
The upstream [changelog](../changelog.txt) remains preserved. The original local
experimental source snapshot remains historical and is not overwritten by this
preparation. No source commit or public release is created by preparing this tree.

The VCL2 XA64 migration starts from official VCL 2.02.04 at
`667577495efaa36a10cbb5d6cb8c942c23913848`. It preserves the newer upstream
FP16 updates, x86 instruction detection through INSTRSET 12, mask extraction,
namespace-qualified divisor overload and wide lookup additions. The earlier
colored benchmark measurements remain historical; see
[migration validation](validation/migration.md) for fresh correctness results.
