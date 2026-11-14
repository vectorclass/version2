# Copyright 2026 DigitalExpressions Sweden
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Independent Decimal power/root fixtures, with exact binary inputs.

Run --check to check the committed artifact and 120/160 digit agreement.
Roots use a bounded range because the VCL Newton iteration forms x^(-4/3).
Rational powers use moderate inputs so intermediate integer powers stay finite.
"""
import argparse
import decimal
import math
import pathlib
import random
import struct

D = decimal.Decimal
# Exercise every specialized denominator/remainder, integer powers, reduction,
# negative denominators, and odd/even generic fallbacks.
RATIOS = [(0, 1), (3, 1), (-3, 1), (1, 2), (-1, 2), (3, 2), (-3, 2),
          (-5, 3), (-4, 3), (-2, 3), (-1, 3), (1, 3), (2, 3), (4, 3), (5, 3),
          (-7, 4), (-5, 4), (-3, 4), (-1, 4), (1, 4), (3, 4), (5, 4), (7, 4),
          (-11, 6), (-7, 6), (-5, 6), (-1, 6), (1, 6), (5, 6), (7, 6), (11, 6),
          (-15, 8), (-13, 8), (-11, 8), (-9, 8), (-7, 8), (-5, 8), (-3, 8), (-1, 8),
          (1, 8), (3, 8), (5, 8), (7, 8), (9, 8), (11, 8), (13, 8), (15, 8),
          (4, 7), (3, 7), (-3, 7), (1, 10), (6, 18), (10, 30), (2, -6)]


def generate(precision):
    rows = []
    with decimal.localcontext() as ctx:
        ctx.prec = precision
        for width, code, intcode, root_lo, root_hi, emin, emax in [
                (32, 'f', 'I', -90, 96, -126, 127),
                (64, 'd', 'Q', -750, 664, -1022, 1023)]:
            def bits(x):
                return struct.unpack('>' + intcode, struct.pack('>' + code, x))[0]

            def value(i):
                return struct.unpack('>' + code, struct.pack('>' + intcode, i))[0]

            def quantize(x):
                return value(bits(float(x)))

            def closest(y):
                center = bits(quantize(y))
                candidates = [i for i in (center-1, center, center+1)
                              if 0 <= i < 1 << width and math.isfinite(value(i))]
                return min(candidates, key=lambda i: (abs(D.from_float(value(i))-y), i & 1))

            def neighbors(x):
                b = bits(x)
                return [value(i) for i in (b-1, b, b+1)]

            def add(op, x, y, exponent):
                dx = D.from_float(x)
                # Exact integer powers avoid transcendental noise at tie cases.
                result = abs(dx) ** int(exponent) if exponent == int(exponent) else (abs(dx).ln()*exponent).exp()
                if x < 0:
                    if op == 0: negative = int(y) & 1
                    elif op < 4: negative = op != 3
                    else:
                        a, b = RATIOS[op-4]
                        g = math.gcd(a, b)
                        negative = (a//g) & 1
                    if negative: result = -result
                # Numerical fixtures require normal finite output; underflow
                # and overflow policy is tested separately in C++.
                if D(2)**emin <= abs(result) <= D(2)**emax:
                    rows.append((width, op, bits(x), bits(y), closest(result)))

            rng = random.Random(0xC0BE + width)
            pairs = []
            for e in [emin, emin+1, -80, -10, -1, 0, 1, 10, 80, emax-1]:
                for x in neighbors(math.ldexp(1., e)):
                    if x >= math.ldexp(1., emin):
                        for y in [-3., -1., -.5, .125, .5, 1., 2., 3.]: pairs.append((x, y))
            for x in neighbors(1.) + neighbors(.5) + neighbors(2.):
                for y in [-1000000., -1000., -17., -.5, .5, 17., 1000., 1000000.]: pairs.append((x, y))
            # Near-one bases with exponents at the integer-parity boundary
            # stress the compensated logarithm/FMA residual in real pow.
            for x in neighbors(1.):
                limit = 2.**(24 if width == 32 else 53)
                for y in [limit/2, limit-1, limit, -limit/2, -limit+1, -limit]: pairs.append((x, y))
            for _ in range(300):
                x = quantize(math.ldexp(rng.uniform(1., 2.), rng.randrange(emin, emax)))
                y = quantize(rng.uniform(-2., 2.))
                pairs.append((x, y))
            for x in [.125, .5, 1., 2., 3., 10.]:
                for y in [-19., -4., -3., -1., 0., 1., 2., 3., 18.]: pairs.append((-x, y))
            for x, y in pairs: add(0, x, y, D.from_float(y))

            roots = [1., 2., 8., 27., 64., 125., quantize(1.e29 if width == 32 else 1.e200)]
            for e in [root_lo, root_lo+1, -20, -2, -1, 0, 1, 2, 20, root_hi-1]:
                roots += neighbors(math.ldexp(1., e))
            roots += [quantize(math.ldexp(rng.uniform(1., 2.), rng.randrange(root_lo, root_hi))) for _ in range(140)]
            for op, exponent in [(1, D(1)/3), (2, -D(1)/3), (3, D(2)/3)]:
                for x in roots:
                    for sign in [-1, 1]: add(op, x*sign, 0., exponent)
            rational_inputs = [.125, .25, .5, 1., 2., 3., 8., 27.]
            rational_inputs += neighbors(1.) + neighbors(8.)
            rational_inputs += [quantize(2.**rng.uniform(-6., 6.)) for _ in range(24)]
            for i, (a, b) in enumerate(RATIOS):
                for x in rational_inputs:
                    add(i+4, x, 0., D(a)/b)
                    if (b//math.gcd(a, b)) & 1: add(i+4, -x, 0., D(a)/b)
    return sorted(set(rows))


def render(rows):
    text = '// Generated by generate_power_roots_reference.py; do not hand-edit.\n'
    text += '#ifndef VCL_TEST_POWER_ROOTS_REFERENCE_H\n#define VCL_TEST_POWER_ROOTS_REFERENCE_H\n#include <cstdint>\n'
    text += 'struct PowerRootsReference { unsigned width, op; uint64_t x, y, expected; };\n'
    text += 'static constexpr PowerRootsReference power_roots_reference[] = {\n'
    text += ''.join(f'    {{{w},{op},0x{x:016x}ull,0x{y:016x}ull,0x{z:016x}ull}},\n' for w, op, x, y, z in rows)
    text += '};\n'
    text += 'template<class V> V rational_reference_operation(unsigned op, V x) {\n    switch(op) {\n'
    for i, (a, b) in enumerate(RATIOS): text += f'    case {i+4}: return pow_ratio(x,{a},{b});\n'
    text += '    default: return x;\n    }\n}\n'
    text += f'static constexpr unsigned power_roots_operations = {len(RATIOS)+4};\n#endif\n'
    return '/*\n * Copyright 2026 DigitalExpressions Sweden\n *\n * Licensed under the Apache License, Version 2.0 (the "License");\n * you may not use this file except in compliance with the License.\n * You may obtain a copy of the License at\n *\n *     https://www.apache.org/licenses/LICENSE-2.0\n *\n * Unless required by applicable law or agreed to in writing, software\n * distributed under the License is distributed on an "AS IS" BASIS,\n * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n * See the License for the specific language governing permissions and\n * limitations under the License.\n */\n\n'+text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--output', type=pathlib.Path, default=pathlib.Path(__file__).with_name('power_roots_reference.h'))
    args = parser.parse_args()
    rows = generate(120)
    if args.check:
        assert rows == generate(160), 'reference rounding changed at higher precision'
        assert args.output.read_text() == render(rows), 'committed fixture differs'
    else: args.output.write_text(render(rows))
    print(f'{len(rows)} Decimal power/root cases; precision=120' + ('; 160-digit agreement checked' if args.check else ''))


if __name__ == '__main__':
    main()
