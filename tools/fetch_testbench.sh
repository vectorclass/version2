#!/bin/sh
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

# GPL-3.0 upstream tests remain in their own checkout; VCL is not relicensed.
set -eu
destination=${1:-build/_deps/testbench}
revision=a7293702385e5169a5f58b83177f2a43cd59139b
if [ -e "$destination" ]; then
    echo "Destination already exists: $destination" >&2
    exit 1
fi
git clone --no-checkout https://github.com/vectorclass/testbench.git "$destination"
git -C "$destination" checkout --detach "$revision"
