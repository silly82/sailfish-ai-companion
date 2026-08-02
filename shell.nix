# Desktop build environment for src/core/.
#
# The Sailfish SDK is only needed to package and to build anything touching
# Silica. Everything under src/core/ is plain Qt by design, so it can be built
# and unit-tested here:
#
#   nix develop -c scripts/run-tests.sh      # pinned via flake.lock
#   nix-shell --run 'scripts/run-tests.sh'   # fallback, uses the ambient channel
#
# flake.nix imports this file, so both entry points build the same shell — only
# the nixpkgs revision differs. Prefer the flake; the channel path exists for
# setups without flakes and can drift to a different Qt 5.15.x.
#
# CAVEAT: this is Qt 5.15, the device runs Qt 5.6. A green test run here does
# not prove the target builds. Stick to Qt 5.6 APIs in src/core/ — notably
# QString::SkipEmptyParts rather than Qt::SkipEmptyParts, and no QStringView.

{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  name = "sailfish-ai-companion-core";

  nativeBuildInputs = [
    pkgs.qt5.qmake
    pkgs.qt5.wrapQtAppsHook
    pkgs.gnumake
  ];

  buildInputs = [
    pkgs.qt5.qtbase
    pkgs.sqlite
  ];

  # qtbase ships the SQLite driver as a plugin; without this the desktop test
  # binary finds no QSQLITE driver because it is not wrapped by Nix.
  shellHook = ''
    export QT_PLUGIN_PATH="${pkgs.qt5.qtbase.bin}/${pkgs.qt5.qtbase.qtPluginPrefix}"
  '';
}
