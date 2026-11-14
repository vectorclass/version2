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

"""Independent inverse/pi-scaled trig fixtures using Decimal at 120/160 digits.

Pi comes from Machin's formula. Atan uses half-angle reduction and a convergent
series; asin/acos use square-root identities. Pi-scaled arguments are reduced
with exact integer ratios before multiplication by pi, even for binary64 maxima.
"""
import argparse
import decimal
import math
import pathlib
import random
import struct
from fractions import Fraction
from generate_trig_reference import arctan_inverse, sincos

D = decimal.Decimal


def atan(x, pi):
    if x.is_zero() or abs(x) < D(2)**-80: return x
    if x < 0: return -atan(-x, pi)
    if x > 1: return pi/2-atan(1/x, pi)
    for _ in range(3): x = x/(1+(1+x*x).sqrt())
    term = total = x
    k = 1
    while True:
        term *= -x*x
        updated = total+term/(2*k+1)
        if updated == total: return total*8
        total = updated
        k += 1


def atan2(a, b, pi):
    if b == 0: return pi/2 if a > 0 else -pi/2
    result = atan(a/b, pi)
    if b < 0: result += pi if a >= 0 else -pi
    return result


def generate(precision):
    rows = []
    with decimal.localcontext() as ctx:
        ctx.prec = precision
        pi = 16*arctan_inverse(5)-4*arctan_inverse(239)
        for width,code,intcode,emin,emax,digits in [(32,'f','I',-126,127,24),(64,'d','Q',-1022,1023,53)]:
            def bits(x): return struct.unpack('>'+intcode,struct.pack('>'+code,x))[0]
            def value(i): return struct.unpack('>'+code,struct.pack('>'+intcode,i))[0]
            def quantize(x): return value(bits(float(x)))
            def neighbors(x):
                b = bits(quantize(x))
                return [value(i) for i in range(max(0,b-2),min(1<<width,b+3)) if math.isfinite(value(i))]
            def closest(y):
                if y.is_infinite() or y.is_zero(): return bits(float(y))
                center = bits(quantize(y))
                candidates = [i for i in [center-1,center,center+1] if 0 <= i < 1<<width and math.isfinite(value(i))]
                return min(candidates,key=lambda i:(abs(D.from_float(value(i))-y),i&1))
            def add(op,a,b,y):
                if op == 3 and b > 0 and abs(y) < D(2)**-80:
                    # Atan is strictly smaller in magnitude than its ratio.
                    # Resolve subnormal midpoint ties with the exact rational
                    # ratio: at subnormal midpoints the cubic correction can be
                    # far below the Decimal evaluation precision.
                    ratio = Fraction.from_float(a)/Fraction.from_float(b)
                    center = bits(quantize(float(ratio)))
                    candidates = [i for i in [center-1,center,center+1] if 0 <= i < 1<<width and math.isfinite(value(i))]
                    expected = min(candidates,key=lambda i:(abs(Fraction.from_float(value(i))-ratio),abs(Fraction.from_float(value(i)))))
                else: expected = closest(y)
                rows.append((width,op,bits(a),bits(b),expected))
            rng = random.Random(0xA71A2+width)
            tiny,normal = value(1),math.ldexp(1.,emin)
            maximum = value(0x7f7fffff if width == 32 else 0x7fefffffffffffff)
            small = [0.,tiny,normal]+[math.ldexp(1.,-e) for e in [10,24,53,80,100]]
            unit = small[:]
            for x in [.5,.625,1.]: unit += neighbors(x)
            # Exact inputs from the pinned compatibility asin cancellation
            # failures, plus neighbors, are independent fixture regressions.
            for x in ['0x1.1d88p-1','0x1.02cb4ep-1','0x1.005748p-1']:
                unit += neighbors(float.fromhex(x))
            unit += [quantize(rng.random()) for _ in range(160)]
            for a in sorted({x*sign for x in unit if 0 <= x <= 1 for sign in [-1,1]}):
                da = D.from_float(a)
                asin = (pi/2 if a == 1 else -pi/2 if a == -1 else
                        atan(da/((1-da)*(1+da)).sqrt(),pi))
                # This identity remains accurate arbitrarily close to +1.
                acos = pi if a == -1 else 2*atan(((1-da)/(1+da)).sqrt(),pi)
                add(0,a,0.,asin); add(1,a,0.,acos)
            xs = small+[maximum]
            for x in [D(2).sqrt()-1,D('.66'),D(2).sqrt()+1,1.]: xs += neighbors(x)
            xs += [value(rng.randrange(bits(normal),bits(maximum))) for _ in range(160)]
            for a in sorted({x*sign for x in xs for sign in [-1,1]}): add(2,a,0.,atan(D.from_float(a),pi))
            pairs = [(a,b) for a in [tiny,normal,.5,1.,2.,maximum] for b in [tiny,normal,.5,1.,2.,maximum]]
            for _ in range(180):
                pairs.append((value(rng.randrange(bits(normal),bits(maximum))),value(rng.randrange(bits(normal),bits(maximum)))))
            pairs += [(a,b) for a in neighbors(1.) for b in neighbors(1.)]
            for a,b in pairs:
                for sa in [-1,1]:
                    for sb in [-1,1]: add(3,a*sa,b*sb,atan2(D.from_float(a*sa),D.from_float(b*sb),pi))
            xs = small+[maximum,maximum/2]
            for x in [.25,.5,.75,1.,1.5,2.,3.,10.,2.**(digits-2),2.**(digits-1),2.**digits,2.**(emax-1)]: xs += neighbors(x)
            xs += [quantize(rng.uniform(0.,1000.)) for _ in range(140)]
            xs += [value(rng.randrange(bits(normal),bits(maximum))) for _ in range(80)]
            for a in sorted({x*sign for x in xs for sign in [-1,1]}):
                n,d = a.as_integer_ratio()
                remainder = n % (2*d)
                if remainder > d: remainder -= 2*d
                r = D(remainder)/D(d)
                s,c = sincos(r*pi,pi)
                t = s/c if c else D('Infinity').copy_sign(s)
                add(4,a,0.,s); add(5,a,0.,c); add(6,a,0.,t)
    return sorted(set(rows))


def render(rows):
    text = '// Generated by generate_inverse_pi_reference.py; do not hand-edit.\n'
    text += '#ifndef VCL_TEST_INVERSE_PI_REFERENCE_H\n#define VCL_TEST_INVERSE_PI_REFERENCE_H\n#include <cstdint>\n'
    text += 'struct InversePiReference { unsigned width, op; uint64_t a, b, expected; };\n'
    text += 'static constexpr InversePiReference inverse_pi_reference[] = {\n'
    text += ''.join(f'    {{{w},{op},0x{a:016x}ull,0x{b:016x}ull,0x{y:016x}ull}},\n' for w,op,a,b,y in rows)
    return '/*\n * Copyright 2026 DigitalExpressions Sweden\n *\n * Licensed under the Apache License, Version 2.0 (the "License");\n * you may not use this file except in compliance with the License.\n * You may obtain a copy of the License at\n *\n *     https://www.apache.org/licenses/LICENSE-2.0\n *\n * Unless required by applicable law or agreed to in writing, software\n * distributed under the License is distributed on an "AS IS" BASIS,\n * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n * See the License for the specific language governing permissions and\n * limitations under the License.\n */\n\n'+text+'};\n#endif\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    parser.add_argument('--output',type=pathlib.Path,default=pathlib.Path(__file__).with_name('inverse_pi_reference.h'))
    args = parser.parse_args(); rows = generate(120)
    if args.check:
        assert rows == generate(160),'reference rounding changed at higher precision'
        assert args.output.read_text() == render(rows),'committed fixture differs'
    else: args.output.write_text(render(rows))
    print(f'{len(rows)} Decimal inverse/pi cases; precision=120'+('; 160-digit agreement checked' if args.check else ''))


if __name__ == '__main__': main()
