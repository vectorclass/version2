# Contributing

Preserve VCL's public names, lane ordering, supported masks/conversions, root
include layout and explicit Direct Neon opt-in. Keep x86 changes limited and
reviewable. Preserve copyright and third-party notices. All source files added
for this port use the complete DigitalExpressions Sweden Apache-2.0 notice;
derived files must retain their original attribution too.

Run [the regression suite](tests/README.md) for both ARM backends, native widths
128/256/512, Release and strict UBSan where supported. Exercise selected x86
targets separately. Add a regression that demonstrates a changed contract or
bug; use independent expected values, meaningful edge cases and appropriate
memory/FP checks. Do not suppress failures, relax tolerances or substitute the
implementation itself as its own oracle. Reproduce fixtures with their checked-in
generators. Keep tests application independent.

Build examples and benchmark correctness checks before evaluating performance.
Timing runs use sequential Release processes and retain flags, all slower rows
and variability. Source restructuring or copyright-only edits do not establish
new timings. Preserve recorded benchmark values and presentation.

Use fresh build directories. Document compiler, target, backend, width and any
coverage gaps. CI workflows are intended checks; a platform is validated only
after its jobs actually run successfully. Physical x86, additional ARM hosts,
unselected APIs and unavailable sanitizer support remain explicit until tested.
