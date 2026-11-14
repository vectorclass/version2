# Provenance and third-party notices

- VCL2 originates with Agner Fog. Existing source copyright/license notices and
  the root [LICENSE](../LICENSE) and [changelog](../changelog.txt) are retained.
- The added native port sources carry DigitalExpressions Sweden's full
  Apache-2.0 notice. Extracted/adapted helpers retain their original Agner Fog
  attribution; this notice identifies the port-specific contributions.
- Earlier ARM compatibility work came through
  [yuygfgg/vectorclass_arm](https://github.com/yuygfgg/vectorclass_arm). This fork
  carries that compatibility work and the Direct Neon port onto official VCL
  2.02.04 while retaining the original source notices.
- The bundled [sse2neon.h](../sse2neon.h) retains its existing MIT notice. This
  preparation does not replace or silently update that dependency.
- The optional upstream testbench is a separate GPL-3.0 checkout at the pinned
  revision documented in [tests](../tests/README.md). Its generated copies and
  embedded matching fragments in the adapter/test configuration retain that
  provenance. It is not bundled with the public runtime headers.
- GfxMath is an optional external consumer used only by the explicitly enabled
  example. Its sources, tests and license are not copied into this library.

Upstream repositories: [VCL2](https://github.com/vectorclass/version2),
[testbench](https://github.com/vectorclass/testbench),
[sse2neon](https://github.com/DLTcollab/sse2neon).
