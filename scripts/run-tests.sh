#!/usr/bin/env bash
# Desktop-Testlauf für src/core/ — braucht kein Sailfish-SDK.
#
#   nix-shell --run 'scripts/run-tests.sh'
#
# Achtung: hier läuft Qt 5.15, auf dem Gerät Qt 5.6. Ein grüner Lauf beweisst
# nicht, dass das Target baut — dafür bleibt 'sfdk build' zuständig.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/build/tests"

mkdir -p "$build"
cd "$build"

qmake "$root/tests/tests.pro"
make -j"$(getconf _NPROCESSORS_ONLN)"

./core-tests
