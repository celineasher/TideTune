#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
if command -v clang++ >/dev/null 2>&1; then compiler=clang++;
elif command -v g++ >/dev/null 2>&1; then compiler=g++;
else echo 'C++ compiler not found.' >&2; exit 1; fi
mkdir -p build
"$compiler" -std=c++17 -Wall -Wextra -Werror -O2 Tests/dsp_test.cpp -o build/tidetune_dsp_tests
./build/tidetune_dsp_tests
"$compiler" -std=c++17 -Wall -Wextra -Werror -O2 Tests/transition_test.cpp -o build/tidetune_transition_tests
./build/tidetune_transition_tests
