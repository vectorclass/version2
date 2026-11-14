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

"""Color benchmark numbers with inline spans in Markdown comparison tables.

Run from any directory after publishing reports. Historical BEFORE comparisons retain their classifications; frozen evidence,
measurement files and benchmark producers are preserved.
"""
from collections import Counter
from decimal import Decimal
from html import unescape
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
BENCHMARKS = ROOT / 'docs/benchmarks'
PALETTE = {'green': '#1a7f37', 'red': '#cf222e', 'blue': '#0969da'}
BACKEND_COLORS = {'Direct Neon': '#6f42c1', 'sse2neon': '#bc4c00'}
THROUGHPUT_NEUTRAL = '#ffffff'
LEGACY = {'🟢': 'green', '🔴': 'red', '🔵': 'blue'}
LEGEND = '''<!-- benchmark-comparison-colors -->
**Direct Neon vs sse2neon:** <span style="color: #1a7f37;">faster</span>,
<span style="color: #cf222e;">slower</span>,
<span style="color: #0969da;">approximately equal</span>.
Blue covers a throughput ratio of **0.95–1.05× inclusive** (±5%). Colors use
the displayed comparison ratio, or compatibility time / Neon time when no
comparison ratio is shown. In before/after tables, both native times compare
against the same sse2neon control.
<!-- /benchmark-comparison-colors -->

'''


def plain(cell):
    cell = re.sub(r'^[🟢🔴🔵] ', '', cell.strip())
    match = re.fullmatch(r'<span style="color: #[0-9a-fA-F]{6};">(.*?)</span>', cell, flags=re.S)
    return match.group(1) if match else cell


def colored(cell, color):
    return f'<span style="color: {PALETTE[color]};">{plain(cell)}</span>'


def number(cell):
    match = re.search(r'[-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?', plain(cell).replace(',', ''))
    return Decimal(match.group()) if match else None


def marker(ratio):
    return 'green' if ratio > Decimal('1.05') else 'red' if ratio < Decimal('.95') else 'blue'


def throughput_color(ratio):
    """White at parity; saturate at a 1.5x throughput lead for either backend."""
    ratio = Decimal(ratio)
    if not ratio.is_finite() or ratio <= 0:
        raise ValueError('Throughput ratio must be finite and positive')
    if ratio >= 1:
        target = BACKEND_COLORS['Direct Neon']
        lead = ratio - 1
    else:
        target = BACKEND_COLORS['sse2neon']
        lead = 1 / ratio - 1
    amount = min(lead / Decimal('.5'), Decimal(1))
    neutral = THROUGHPUT_NEUTRAL
    channels = [round(Decimal(int(neutral[i:i+2], 16)) * (1 - amount) +
                      Decimal(int(target[i:i+2], 16)) * amount)
                for i in (1, 3, 5)]
    return '#' + ''.join(f'{value:02x}' for value in channels)


def annotate_table(lines):
    cells = [[cell.strip() for cell in line.strip().strip('|').split('|')] for line in lines]
    header = [cell.lower() for cell in cells[0]]
    counts = Counter()

    def mark(row, index, ratio):
        color = marker(ratio)
        row[index] = colored(row[index], color)
        counts[color] += 1

    if 'backend' in header and 'median' in header:
        backend, median = header.index('backend'), header.index('median')
        identities = [i for i, name in enumerate(header) if name in ('kernel', 'invocation', 'fp', 'bits')]
        controls = {}
        for row in cells[2:]:
            if 'sse2neon' in row[backend].lower() or 'compatibility' in row[backend].lower():
                controls[tuple(row[i] for i in identities)] = number(row[median])
        for row in cells[2:]:
            name = row[backend].lower()
            if 'sse2neon' in name or 'compatibility' in name:
                continue
            control = controls.get(tuple(row[i] for i in identities))
            time = number(row[median])
            if ('neon' in name or 'native' in name) and control is not None and time and time > 0:
                mark(row, median, control / time)
    else:
        compatibility = next((i for i, name in enumerate(header)
                              if name.startswith(('sse2neon', 'compatibility')) and ' / ' not in name), None)
        native = [i for i, name in enumerate(header)
                  if name.startswith(('direct neon', 'neon ', 'native ', 'after ', 'before '))
                  and any(unit in name for unit in ('ns', 'µs'))]
        ratio_column = next((i for i, name in enumerate(header)
                             if name in ('sse2neon / neon', 'ratio', 'throughput ratio', 'measured throughput gain')), None)
        if compatibility is not None and native:
            for row in cells[2:]:
                control = number(row[compatibility])
                if control is None:
                    continue
                for index in native:
                    time = number(row[index])
                    if time and time > 0:
                        # A/B improvement ratios compare different native versions;
                        # only a compatibility/native ratio can color this comparison.
                        ratio = number(row[ratio_column]) if ratio_column is not None else None
                        if ratio is None:
                            ratio = control / time
                        mark(row, index, ratio)
                if ratio_column is not None:
                    ratio = number(row[ratio_column])
                    if ratio is None:
                        time = number(row[native[-1]])
                        ratio = control / time if time and time > 0 else None
                    if ratio is not None:
                        mark(row, ratio_column, ratio)
    if not counts:
        return lines, counts
    return ['| ' + ' | '.join(row) + ' |' for row in cells], counts


def annotate(text):
    text = re.sub(r'<!-- benchmark-comparison-colors -->\n.*?<!-- /benchmark-comparison-colors -->\n\n', '', text, flags=re.S)
    lines = text.splitlines(keepends=True)
    result, counts = [], Counter()
    first_table = None
    index = 0
    while index < len(lines):
        if lines[index].startswith('|'):
            end = index
            while end < len(lines) and lines[end].startswith('|'):
                end += 1
            table, found = annotate_table([line.rstrip('\r\n') for line in lines[index:end]])
            if found and first_table is None:
                first_table = len(result)
            result.extend(line + '\n' for line in table)
            counts.update(found)
            index = end
        else:
            result.append(lines[index])
            index += 1
    if first_table is not None:
        result.insert(first_table, LEGEND)
    return ''.join(result), counts


def annotate_standard_math(text):
    """Pair the two SIMD competitors explicitly; keep scalar as a reference."""
    def undecorate_labels(value):
        for name, color in BACKEND_COLORS.items():
            label = r'Direct(?:&nbsp;|\s)+Neon' if name == 'Direct Neon' else name
            value = re.sub(r'<span style="color: ' + color + r';">(' + label + r')</span>', r'\1', value)
        return value.replace('Direct&nbsp;Neon', 'Direct Neon')

    text = undecorate_labels(text)
    guide_start = '<!-- standard-math-reading-guide -->'
    guide_end = '<!-- /standard-math-reading-guide -->'
    text = re.sub(re.escape(guide_start) + r'\n.*?' + re.escape(guide_end) + r'\n\n', '', text, flags=re.S)
    text = re.sub(r'<!-- benchmark-comparison-colors -->\n.*?<!-- /benchmark-comparison-colors -->\n\n', '', text, flags=re.S)
    text = re.sub(r'<!-- standard-math-table-notes -->\n.*?<!-- /standard-math-table-notes -->\n\n', '', text, flags=re.S)
    pair_note = '''<!-- standard-math-table-notes -->
- **sse2neon:** SSE operations translated to Neon SIMD.
- **Direct Neon:** native Neon SIMD.
- **(nsec/op):** nanoseconds per complete vector/matrix operation; lower is better.
- **Scalar:** float/double arithmetic without SIMD; auto-vectorization disabled.
<!-- /standard-math-table-notes -->'''
    text = re.sub(r'^\*\*Compared pair: [^\n]*\n\n', '', text, flags=re.M)

    def unstyle(cell):
        cell = cell.strip()
        if cell.startswith('**') and cell.endswith('**'):
            cell = cell[2:-2]
        cell = re.sub(r'<br\s*/?>', ' ', cell, flags=re.I)
        return unescape(re.sub(r'</?span\b[^>]*>', '', cell)).replace('\xa0', ' ')

    old_header = ['operation', 'fp', 'scalar ns/object', 'sse2neon ns/object',
                  'direct neon ns/object', 'scalar / neon', 'sse2neon / neon', 'backend ranges separate']
    previous_header = ['operation', 'fp', 'a: sse2neon ns/object', 'b: direct neon ns/object',
                       'b / a throughput', 'b vs a result', 'scalar (nsec/op)',
                       'scalar / b', 'consistent timing']
    new_header = ['Operation', 'FP', '**sse2neon<br>(nsec/op)**', '**Direct Neon<br>(nsec/op)**',
                  '**Direct Neon /<br>sse2neon**', '**Winning<br>backend**', '**Throughput<br>gain&nbsp;(%)**', '**Scalar<br>(nsec/op)**',
                  'Scalar /<br>Direct Neon', 'Consistent<br>Timing']
    normalized_new = [unstyle(cell).lower() for cell in new_header]
    named_header = normalized_new.copy()
    del named_header[6]
    lead_header = named_header.copy()
    lead_header[5] = 'winner (% higher throughput)'
    counts = Counter()
    lines = text.splitlines(keepends=True)
    result = []
    index = 0
    while index < len(lines):
        if lines[index].startswith('|'):
            end = index
            while end < len(lines) and lines[end].startswith('|'):
                end += 1
            table = [[cell.strip() for cell in line.strip().strip('|').split('|')]
                     for line in lines[index:end]]
            header = [unstyle(cell).lower() for cell in table[0]]
            if len(header) in (9, 10) and header[-1] in ('median ranges separate', 'a/b median ranges separate'):
                header[-1] = 'consistent timing'
            if len(header) > 4 and header[4] == 'direct neon / sse2neon throughput':
                header[4] = 'direct neon / sse2neon'
            if len(header) > 5 and header[5] in ('winner', 'winner backend'):
                header[5] = 'winning backend'
            if len(header) in (9, 10):
                for position in (2, 3, 7 if len(header) == 10 else 6):
                    if header[position].endswith(' nsec/op'):
                        header[position] = header[position][:-len(' nsec/op')] + ' (nsec/op)'
                for position, backend in [(2, 'sse2neon'), (3, 'direct neon')]:
                    if header[position] == backend + ' ns/object':
                        header[position] = backend + ' (nsec/op)'
                scalar_position = 7 if len(header) == 10 else 6
                if header[scalar_position] in ('scalar reference ns/object', 'scalar ns/object', 'no simd ns/object', 'no simd (nsec/op)'):
                    header[scalar_position] = 'scalar (nsec/op)'
                ratio_position = scalar_position + 1
                if header[ratio_position] == 'scalar(simd) / direct neon':
                    header[ratio_position] = 'scalar / direct neon'
            if len(header) == 10 and header[6] == 'throughput':
                header[6] = 'throughput gain (%)'
            if header in (old_header, previous_header, named_header, lead_header, normalized_new):
                rows = [new_header, ['---', '---:', '---:', '---:', '---:', '---', '---:', '---:', '---:', '---']]
                for row in table[2:]:
                    row = [unstyle(cell) for cell in row]
                    if header == old_header:
                        operation, fp, scalar, compatibility, native, scalar_ratio, ratio, separate = row
                    elif header == normalized_new:
                        operation, fp, compatibility, native, ratio, _, _, scalar, scalar_ratio, separate = row
                    else:
                        operation, fp, compatibility, native, ratio, _, scalar, scalar_ratio, separate = row
                    color = marker(number(ratio))
                    compatibility_color = {'green': 'red', 'blue': 'blue', 'red': 'green'}[color]
                    verdict = {'green': 'Direct Neon', 'blue': 'Within&nbsp;5%', 'red': 'sse2neon'}[color]
                    lead_text = '—'
                    if color != 'blue':
                        value = number(ratio)
                        lead = (value - 1) if color == 'green' else (1 / value - 1)
                        lead_text = f'+{lead * 100:.1f}%'
                    rows.append([operation, fp, '**' + colored(compatibility, compatibility_color) + '**',
                                 '**' + colored(native, color) + '**',
                                 '**<span style="color: ' + throughput_color(number(ratio)) + ';">' + ratio + '</span>**', verdict,
                                 lead_text, scalar, scalar_ratio, separate])
                    counts[color] += 1
                    counts[compatibility_color] += 1
                result.append(pair_note + '\n\n')
                result.extend('| ' + ' | '.join(row) + ' |\n' for row in rows)
            else:
                result.extend(lines[index:end])
            index = end
        else:
            result.append(lines[index])
            index += 1
    guide = undecorate_labels((BENCHMARKS / 'standard_math_reading_guide.md').read_text().rstrip())
    insertion = guide_start + '\n' + guide + '\n' + guide_end + '\n\n'
    text = ''.join(result)
    position = text.find('Units are **ns/vector')
    if position < 0:
        position = text.find('## ')
    if position < 0:
        raise ValueError('Cannot locate the standard-math report body')
    text = text[:position] + insertion + text[position:]
    text = re.sub(r'^(## [\d,]+ objects — )object_aos$',
                  r'\1Array of Structures (AoS): SIMD across one object’s components', text, flags=re.M)
    text = re.sub(r'^(## [\d,]+ objects — )batch_soa( / logical \d+ bits)$',
                  r'\1Structure of Arrays (SoA): SIMD across objects\2', text, flags=re.M)
    text = re.sub(r'\bsse2neon\b',
                  lambda _: '<span style="color: ' + BACKEND_COLORS['sse2neon'] + ';">sse2neon</span>', text, flags=re.I)
    text = re.sub(r'\bdirect\s+neon\b',
                  lambda _: '<span style="color: ' + BACKEND_COLORS['Direct Neon'] + ';">Direct&nbsp;Neon</span>', text, flags=re.I)
    return text, counts


def main():
    path = BENCHMARKS / 'STANDARD_MATH_RESULTS.md'
    before = path.read_text()
    after, counts = annotate_standard_math(before)
    after = after.rstrip("\n") + "\n"
    if after != before:
        path.write_text(after)
    print(dict(counts))

if __name__ == '__main__':
    main()
