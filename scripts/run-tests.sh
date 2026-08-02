#!/usr/bin/env bash
# Desktop-Testlauf für src/core/ — braucht kein Sailfish-SDK.
#
#   nix develop -c scripts/run-tests.sh
#   nix-shell --run 'scripts/run-tests.sh'
#
# Achtung: hier läuft Qt 5.15, auf dem Gerät Qt 5.6. Ein grüner Lauf beweisst
# nicht, dass das Target baut — dafür bleibt 'sfdk build' zuständig.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/build/tests"

# Ein Build-Verzeichnis aus einem anderen Qt weiterzuverwenden endet in
# "QSQLITE driver not loaded" plus "two sets of Qt binaries": make sieht
# unveraenderte Quellen, behaelt das alt gelinkte Binary, und das trifft dann
# auf QT_PLUGIN_PATH des neuen Qt. Tritt beim Wechsel Rechner/Kanal/Distro auf.
if ! toolchain="$(command -v qmake)"; then
    echo "qmake not found — see the development environment section in README.md." >&2
    exit 1
fi

if [ -f "$build/.toolchain" ] && [ "$(cat "$build/.toolchain")" != "$toolchain" ]; then
    echo "Toolchain changed, rebuilding from scratch."
    rm -rf "$build"
fi

mkdir -p "$build"
printf '%s\n' "$toolchain" > "$build/.toolchain"
cd "$build"

qmake "$root/tests/tests.pro"
make -j"$(getconf _NPROCESSORS_ONLN)"

./core-tests
