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

"""Check portable public files and documentation without compiling or timing."""
from pathlib import Path
import ast
import json
import os
import re

ROOT = Path(__file__).resolve().parents[1]
NATIVE = ['vcl_config.h', 'instrset_neon.h', 'instrset_helpers.h',
          'vectori128_neon.h', 'vectori128_neon_div.h', 'vectori128_neon_perm.h',
          'vectorf128_neon.h', 'vectorf128_neon_helpers.h']
TEST_SYSTEM_HEADERS = {
    'algorithm', 'array', 'cfenv', 'climits', 'cmath', 'cstddef', 'cstdint',
    'cstdio', 'cstdlib', 'cstring', 'initializer_list', 'limits', 'sys/mman.h',
    'type_traits', 'unistd.h', 'utility', 'vector',
}


def main():
    files = []
    for base, directories, names in os.walk(ROOT):
        directories[:] = [name for name in directories
                          if name not in {'.git', '.vscode', 'port-tmp', 'build', '__pycache__'}
                          and not name.startswith('build-')]
        files += [Path(base) / name for name in names
                  if name not in {'.DS_Store', 'CMakeUserPresets.json'}]
    links = 0
    for path in files:
        if path.suffix == '.py':
            ast.parse(path.read_bytes(), filename=str(path.relative_to(ROOT)))
        if path.suffix == '.md':
            for link in re.findall(r'\]\(([^)]+)\)', path.read_text()):
                if link.startswith(('https:', 'http:', '#', 'mailto:')):
                    continue
                target = link.split('#')[0]
                if target:
                    assert (path.parent / target).exists(), (str(path), link)
                    links += 1
        if path.suffix in {'.py', '.cpp', '.h', '.md', '.json', '.yml', '.sh', '.txt'}:
            data = path.read_bytes()
            assert not re.search(rb'/(?:Users|private/var)/|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----', data), str(path)
            assert not re.search(rb'(?m)^(<<<<<<< |=======\r?$|>>>>>>> )', data), str(path)
    for name in NATIVE:
        text = (ROOT / name).read_text()
        assert text.startswith('/*\n * Copyright 2026 DigitalExpressions Sweden')
        assert 'Licensed under the Apache License, Version 2.0' in text[:1000]
    for path in (ROOT / 'tests').iterdir():
        if path.suffix in {'.h', '.cpp', '.py'}:
            # Test includes must be public library/test files or explicitly
            # listed standard/system headers. Inspect Python string literals
            # used for generated compile probes too, without naming external
            # applications/SDKs.
            sources = [path.read_text()]
            if path.suffix == '.py':
                sources = [node.value for node in ast.walk(ast.parse(sources[0]))
                           if isinstance(node, ast.Constant) and isinstance(node.value, str)]
            for source in sources:
                for delimiter, header in re.findall(r'#\s*include\s*([<"])([^>"\r\n]+)[>"]', source):
                    if delimiter == '<':
                        assert header in TEST_SYSTEM_HEADERS, (str(path), header)
                        continue
                    candidates = [(ROOT / header).resolve(), (ROOT / 'tests' / header).resolve()]
                    assert any(candidate.parent in {ROOT, ROOT / 'tests'} and candidate.is_file()
                               for candidate in candidates), (str(path), header)
    print(json.dumps({'status': 'passed', 'public_files': len(files),
                      'local_documentation_links': links,
                      'native_headers_with_complete_notices': len(NATIVE)}, indent=2))


if __name__ == '__main__':
    main()
