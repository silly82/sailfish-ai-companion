#!/bin/sh
# Legt die Projektstruktur an. Idempotent — überschreibt keine Dateien.
set -e
for d in src/core src/platform/sandboxed src/platform/full \
         qml/pages qml/components qml/cover rpm translations \
         icons/86x86 icons/108x108 icons/128x128 icons/172x172 \
         docs models scripts; do
    mkdir -p "$d"
done
[ -d .git ] || git init -q
echo "Struktur bereit. Nächster Schritt: sfdk config target=... && sfdk build"
