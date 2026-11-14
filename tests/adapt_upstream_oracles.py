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

"""Adapt scalar references in generated GPL upstream copies, leaving vector code intact.

The source checkout stays untouched. Matching original snippets exactly makes
an upstream revision change fail visibly instead of silently changing coverage.
"""
import argparse
import hashlib
from pathlib import Path

INTEGER_CASES = {1, 2, 3, 8, 14, 15, 16, 100, 400}


def guarded(original, repaired):
    return '#if defined(VCL_UPSTREAM_SAFE_INTEGER_REFERENCE)\n' + repaired + '\n#else\n' + original + '\n#endif'


def adapt(source, kind, header):
    changes = []

    def replace(old, new):
        nonlocal source
        if source.count(old) != 1:
            raise ValueError(f'Expected exactly one original {kind} scalar reference: {old[:90]!r}')
        source = source.replace(old, new, 1)
        changes.append((old, new))

    if kind == 'general':
        anchor = '#include <cmath>'
        replace(anchor, anchor + '\n#if defined(VCL_UPSTREAM_SAFE_INTEGER_REFERENCE)\n#include "' + str(header) + '"\n#endif')
        for symbol, function in [('+', 'add'), ('-', 'subtract'), ('*', 'multiply')]:
            old = f'RT referenceFunction(ST a, ST b) {{ return a {symbol} b; }}'
            new = f'RT referenceFunction(ST a, ST b) {{ return vcl_test_oracle::{function}(a,b); }}'
            replace(old, guarded(old, new))
        old = 'RT referenceFunction(ST a, ST b) { return -b; }'
        replace(old, guarded(old, 'RT referenceFunction(ST a, ST b) { return vcl_test_oracle::negate(b); }'))
        for symbol, function in [('+', 'add'), ('-', 'subtract'), ('*', 'multiply')]:
            old = f'RT referenceFunction(ST f, ST a, ST b) {{ return f != 0 ? a{symbol}b : a; }}'
            new = f'RT referenceFunction(ST f, ST a, ST b) {{ return f != 0 ? vcl_test_oracle::{function}(a,b) : a; }}'
            replace(old, guarded(old, new))
        old = '''RT referenceFunction(ST a, ST b) {
    int c = b0 & (sizeof(ST) * 8 - 1);
    return a << c;
}'''
        replace(old, guarded(old, old.replace('a << c', 'vcl_test_oracle::shift_left(a,unsigned(c))')))
        old = '''RT referenceFunction(vtype const& a, vtype const& b) {
    ST sum = 0;
    for (int i = 0; i < b.size(); i++) sum += b[i];
    return sum;
}'''
        replace(old, guarded(old, old.replace('sum += b[i]', 'sum = vcl_test_oracle::add(sum,b[i])')))
    elif kind == 'exp_log':
        old = 'long double referenceFunction(ST a) {\n#define powLL pow_accurate'
        new = '''long double referenceFunction(ST a) {
    if (std::isnan(a)) return static_cast<long double>(a);
    if (std::isinf(a)) return a > 0 ? static_cast<long double>(a) : 0.L;
#define powLL pow_accurate'''
        replace(old, new)
    elif kind == 'trig':
        old = 'long double referenceFunction(ST a) {\n    if (abs(a) >= FLT_MAX) return bit_castf(0x7FFF0000); // NAN\n    if (abs(a) > trig_input_limit && !is_inf(vtype(a))[0]) return 0.;'
        new = old.replace('long double referenceFunction(ST a) {\n', 'long double referenceFunction(ST a) {\n    if (std::isnan(a)) return static_cast<long double>(a);\n', 1)
        replace(old, new)
    elif kind != 'math':
        raise ValueError(kind)
    if kind in {'exp_log', 'trig', 'math'}:
        # Only the Apple x86 selected fixture uses these references. ARM and
        # every other target retain their original active scalar expressions.
        condition = 'defined(__APPLE__) && defined(__x86_64__)'
        anchor = '#include <cmath>'
        reference_header = Path(header).with_name('upstream_x86_reference.h')
        replace(anchor, anchor + '\n#if ' + condition + ' && (testcase == 105 || (testcase >= 200 && testcase <= 204))\n#include "' + str(reference_header) + '"\n#endif')
        original = 'long double referenceFunction(ST a) { return log1pl(a); }'
        repaired = '''long double referenceFunction(ST a) {
#if ''' + condition + '''
    return vcl_test_x86_reference::evaluate(a,0);
#else
    return log1pl(a);
#endif
}'''
        replace(original, repaired)
        for expression,op,expected in [('return sinl(a);',1,2),('return cosl(a);',2,2),('return std::tan((long double)a);',3,1)]:
            assert source.count(expression)==expected,(kind,expression)
            replacement='''
#if ''' + condition + '''
    if constexpr (sizeof(ST)==8) return vcl_test_x86_reference::evaluate(a,''' + str(op) + ''');
#endif
    '''+expression
            source=source.replace(expression,replacement)
            changes.extend([(expression,replacement)]*expected)
    return source, changes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kind', choices=['general', 'exp_log', 'trig', 'math'], required=True)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--header', type=Path, required=True)
    args = parser.parse_args()
    source = args.input.read_text()
    result, changes = adapt(source, args.kind, args.header.resolve())
    if not args.output.exists() or args.output.read_text() != result:
        args.output.write_text(result)
    print(f'{args.kind}: {len(changes)} scalar-only adaptations; sha256 {hashlib.sha256(result.encode()).hexdigest()}')


if __name__ == '__main__':
    main()
