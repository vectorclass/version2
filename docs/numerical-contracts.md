# Numerical contracts

Correctness tests retain the existing inputs, seeds, numerical gates and
special-value policies. These functions are bounded vector-math APIs rather
than full-range scalar-libm replacements. Use the documented FP environment;
validation uses nearest rounding and flush-to-zero disabled unless a test
explicitly exercises another supported mode. Explicit FMA helpers remain fused;
correctness/benchmark builds disable implicit contraction with `-ffp-contract=off`.

| Surface | Scope |
| --- | --- |
| Integer operations | Wrapping arithmetic, shifts, masks, conversions and supported divisor templates; invalid compile-time cases are rejected |
| Memory/movement | Partial-count policies, canaries, protected-page boundaries and compile-time index rejection |
| Exp/log | Conservative VCL exponent cutoffs and documented subnormal policies; independent Decimal fixtures |
| Powers/cube roots | Intermediate overflow/underflow limits and explicit exceptional-value tests |
| Native radian trig | Accuracy-gated within ±1e7f / ±1e15; beyond cutoffs including infinity, sin/tan return signed zero and cos returns one |
| Inverse/pi-scaled trig | Domain, quadrant, parity, pole and signed-zero checks at all selected widths |
| Sinh/cosh | Conservative overflow cutoffs at absolute input 89.0f / 709.7 |
| Reciprocal/rsqrt | Backend-specific estimates, precision and special-value/underflow behavior |

Legacy x86 RCPPS tiny estimates flush to correctly signed zero. The selected
test retains its prior 2^-11 relative tolerance outside tiny/boundary intervals;
the boundary accepts only the explicitly permitted zero or normal estimate.
ARM accuracy rules and rsqrt gates remain unchanged. Protected-page tests and
strict nonrecovering UBSan are independent safeguards; they do not establish
ASan coverage on the host where ASan startup hangs.

The implementation and executable tests are the precise contracts. See
[tests](../tests/README.md) and [migration validation](validation/migration.md)
for the selected matrix and its scope. No general emulation of ARM exception
flags/masks or Intel SVML is provided.
