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

"""Generate exp/log fixtures using Python Decimal, without a binary libm oracle.

Inputs and outputs are IEEE bit patterns. Decimal evaluates at 120 digits; the
closest of the binary candidate and its two neighbors is selected, ties to even.
For |x| < 2^-80, expm1/log1p round to x in both tested binary formats.
Run from any directory; the default output is beside this script.
"""
import argparse
import decimal
import math
import pathlib
import random
import struct

D = decimal.Decimal
OPS = ('exp', 'expm1', 'exp2', 'exp10', 'log', 'log1p', 'log2', 'log10')


def generate(precision):
    rows = []
    with decimal.localcontext() as ctx:
        ctx.prec = precision
        ln2, ln10 = D(2).ln(), D(10).ln()
        for width, code, intcode, emin, emax in [(32, 'f', 'I', -126, 127), (64, 'd', 'Q', -1022, 1023)]:
            def bits(x):
                return struct.unpack('>' + intcode, struct.pack('>' + code, x))[0]

            def value(i):
                return struct.unpack('>' + code, struct.pack('>' + intcode, i))[0]

            def quantize(x):
                return value(bits(float(x)))

            def closest(y):
                center = bits(quantize(y))
                candidates = [i for i in (center - 1, center, center + 1) if 0 <= i < 1 << width and math.isfinite(value(i))]
                return min(candidates, key=lambda i: (abs(D.from_float(value(i)) - y), i & 1))

            def neighbors(x):
                b = bits(quantize(x))
                return [value(i) for i in range(max(0, b - 2), min(1 << width, b + 3)) if math.isfinite(value(i))]

            smallest = math.ldexp(1., emin)
            largest = value(0x7f7fffff if width == 32 else 0x7fefffffffffffff)
            rng = random.Random(0xE7106 + width)
            for op, name in enumerate(OPS):
                samples = [0., -0., smallest, -smallest, value(1), -value(1)]
                if op < 4:
                    limit = quantize(([87.3, 87.3, 126., 37.9] if width == 32 else [708.39, 708.39, 1022., 307.65])[op])
                    step = ln2 if op < 2 else D(1) if op == 2 else ln2 / ln10
                    count = int(D.from_float(limit) / step)
                    for k in [-count+1, -count//2, -17, -2, -1, 0, 1, 2, 17, count//2, count-1]:
                        for offset in [D(0), D('0.5')]:
                            samples += neighbors((D(k)+offset)*step)
                    samples += neighbors(limit) + neighbors(-limit)
                    samples += [quantize(rng.uniform(-limit, limit)) for _ in range(160)]
                    samples += [math.ldexp(1., -i) * sign for i in [10, 24, 53, 80, 100] for sign in [-1, 1]]
                    samples = [x for x in samples if abs(x) < limit]
                else:
                    for e in [emin, emin+1, -20, -2, -1, 0, 1, 2, 20, emax-1]:
                        samples += neighbors(math.ldexp(1., e)) + neighbors(D(2).sqrt() * D(2)**e)
                    samples += neighbors(1.) + neighbors(largest)
                    low = bits(smallest)
                    high = bits(largest)
                    samples += [value(rng.randrange(low, high)) for _ in range(180)]
                    if name == 'log1p':
                        samples += neighbors(-1.) + neighbors(-0.5) + neighbors(0.5)
                        samples += [math.ldexp(1., -i) * sign for i in [10, 24, 53, 80, 100] for sign in [-1, 1]]
                        samples += [quantize(rng.uniform(-1., 1.)) for _ in range(80)]
                        samples = [x for x in samples if x > -1.]
                    else:
                        samples = [x for x in samples if x >= smallest]
                # Stable sorted bits keep the committed artifact reproducible.
                for xb in sorted({bits(x) for x in samples}):
                    x = D.from_float(value(xb))
                    if name == 'exp': y = x.exp()
                    elif name == 'expm1': y = x if abs(x) < D(2)**-80 else x.exp()-1
                    elif name == 'exp2': y = (x*ln2).exp()
                    elif name == 'exp10': y = (x*ln10).exp()
                    elif name == 'log': y = x.ln()
                    elif name == 'log1p': y = x if abs(x) < D(2)**-80 else (1+x).ln()
                    elif name == 'log2': y = x.ln()/ln2
                    else: y = x.ln()/ln10
                    rows.append((width, op, xb, closest(y)))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='verify the committed fixture and 160-digit agreement')
    parser.add_argument('--output', type=pathlib.Path, default=pathlib.Path(__file__).with_name('exp_log_reference.h'))
    args = parser.parse_args()
    rows = generate(120)
    if args.check:
        assert rows == generate(160), 'reference rounding changed at higher precision'
    text = '// Generated by generate_exp_log_reference.py; do not hand-edit.\n'
    text += '#ifndef VCL_TEST_EXP_LOG_REFERENCE_H\n#define VCL_TEST_EXP_LOG_REFERENCE_H\n#include <cstdint>\n'
    text += 'struct ExpLogReference { unsigned width, op; uint64_t input, expected; };\n'
    text += 'static constexpr ExpLogReference exp_log_reference[] = {\n'
    text += ''.join(f'    {{{w},{op},0x{x:016x}ull,0x{y:016x}ull}},\n' for w, op, x, y in rows)
    text += '};\n#endif\n'
    text = '/*\n * Copyright 2026 DigitalExpressions Sweden\n *\n * Licensed under the Apache License, Version 2.0 (the "License");\n * you may not use this file except in compliance with the License.\n * You may obtain a copy of the License at\n *\n *     https://www.apache.org/licenses/LICENSE-2.0\n *\n * Unless required by applicable law or agreed to in writing, software\n * distributed under the License is distributed on an "AS IS" BASIS,\n * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n * See the License for the specific language governing permissions and\n * limitations under the License.\n */\n\n' + text
    if args.check:
        assert args.output.read_text() == text, 'committed fixture differs'
    else:
        args.output.write_text(text)
    print(f'{len(rows)} Decimal reference cases; precision=120' + ('; 160-digit agreement checked' if args.check else ''))


if __name__ == '__main__':
    main()
