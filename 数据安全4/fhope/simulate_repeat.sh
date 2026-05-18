#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

g++ -std=c++17 -Wall -Wextra -Wno-unused-parameter Node.cpp simulate_repeat.cpp -o simulate_repeat
./simulate_repeat "${1:-140}" "${2:-10}"
