# Sailfish AI Companion

A native AI companion for Sailfish OS. Two build targets from a single trunk.

*[Deutsche Fassung weiter unten](#deutsch)*

---

## English

| Target | Package | Store | Sandbox |
|---|---|---|---|
| Harbour | `harbour-nemoai` | Jolla Store | Sailjail, minimal permissions |
| Full | `sailfishai` | OpenRepos | unsandboxed |

Both packages can be installed side by side.

### Architecture in one sentence

The difference between the targets is the **tool manifest**, not the code.
`ToolRegistry` registers only what `Capabilities` reports at startup, and the
QML interface hides the rest.

The trunk is always Harbour-clean. The full target only ever adds, never
removes — so the store build cannot break because of it.

### Building for the device

Needs the Sailfish SDK (`sfdk`, from docs.sailfishos.org). It is available for
Linux, macOS and Windows and does the actual work inside a build engine VM, so
the host distribution is irrelevant.

```sh
sfdk config target=SailfishOS-5.0.0.x-aarch64

sfdk build                                    # Harbour (default)
sfdk build -- --define "fullaccess 1"         # full access
sfdk check                                    # Harbour validator
```

Run `sfdk check` before every store upload.

### Development environment without the SDK

`src/core/` is plain Qt by design — no Silica, no Sailfish APIs — so it builds
and unit-tests on any desktop, Linux or macOS.

With Nix, pinned through `flake.lock`:

```sh
nix develop -c scripts/run-tests.sh
```

Without flakes, resolving against the ambient channel instead:

```sh
nix-shell --run 'scripts/run-tests.sh'
```

Classically, with distribution packages:

| Distribution | Packages |
|---|---|
| Debian/Ubuntu | `build-essential qtbase5-dev qtbase5-dev-tools qt5-qmake libqt5sql5-sqlite` |
| Fedora | `gcc-c++ make qt5-qtbase-devel` |
| Arch | `base-devel qt5-base` |

```sh
scripts/run-tests.sh
```

Two traps outside Nix: Debian ships the SQLite driver as a separate package, and
without `libqt5sql5-sqlite` the store tests fail with `QSQLITE driver not
loaded`. Fedora installs the binary as `qmake-qt5`, so put `/usr/lib64/qt5/bin`
on `PATH`.

The script builds the entire core, not only what the tests cover, and then runs
the QtTest suite. It wipes `build/` when the toolchain changes — reusing object
files across two Qt installations produces misleading plugin errors.

Note this is Qt 5.15 while the device runs Qt 5.6. A green run is not proof that
the target builds, so keep to Qt 5.6 APIs in `src/core/`.

### Status

- [x] M1 Vertical slice (chat, streaming, history) — not yet run on a device
- [x] M2 Tool framework (registry, consent gate, function-calling roundtrip) —
      covered by desktop tests against a fake backend, not yet run against a
      real model or on a device
- [ ] M3 Harbour release
- [ ] M4 Full target / OpenRepos
- [ ] M5 Local inference
- [ ] M6 Voice

Full concept: [`docs/konzept-v2.md`](docs/konzept-v2.md)

---

## Deutsch

Nativer KI-Begleiter für Sailfish OS. Zwei Build-Targets aus einem Trunk.

| Target | Paket | Store | Sandbox |
|---|---|---|---|
| Harbour | `harbour-nemoai` | Jolla Store | Sailjail, minimale Berechtigungen |
| Full | `sailfishai` | OpenRepos | ohne Sandbox |

Beide Pakete lassen sich gleichzeitig installieren.

### Architektur in einem Satz

Der Unterschied zwischen den Targets ist das **Tool-Manifest**, nicht der Code.
`ToolRegistry` registriert beim Start nur, was `Capabilities` meldet; die
QML-Oberfläche blendet den Rest aus.

Der Trunk ist immer Harbour-konform. Das Full-Target fügt ausschliesslich hinzu
und entfernt nie etwas — der Store-Build kann dadurch nicht brechen.

### Für das Gerät bauen

Braucht das Sailfish SDK (`sfdk`, von docs.sailfishos.org). Es gibt es für
Linux, macOS und Windows; gebaut wird ohnehin in einer Build-Engine-VM, die
Host-Distribution spielt also keine Rolle.

```sh
sfdk config target=SailfishOS-5.0.0.x-aarch64

sfdk build                                    # Harbour (Standard)
sfdk build -- --define "fullaccess 1"         # Vollzugriff
sfdk check                                    # Harbour-Validator
```

`sfdk check` vor jedem Store-Upload ausführen.

### Entwicklungsumgebung ohne SDK

`src/core/` ist bewusst reines Qt — kein Silica, keine Sailfish-APIs — und baut
und testet deshalb auf jedem Desktop, Linux wie macOS.

Mit Nix, über `flake.lock` gepinnt:

```sh
nix develop -c scripts/run-tests.sh
```

Ohne Flakes, stattdessen gegen den vorhandenen Channel aufgelöst:

```sh
nix-shell --run 'scripts/run-tests.sh'
```

Klassisch, mit Distributionspaketen:

| Distribution | Pakete |
|---|---|
| Debian/Ubuntu | `build-essential qtbase5-dev qtbase5-dev-tools qt5-qmake libqt5sql5-sqlite` |
| Fedora | `gcc-c++ make qt5-qtbase-devel` |
| Arch | `base-devel qt5-base` |

```sh
scripts/run-tests.sh
```

Zwei Fallen ausserhalb von Nix: Debian liefert den SQLite-Treiber als eigenes
Paket aus — ohne `libqt5sql5-sqlite` scheitern die Store-Tests mit `QSQLITE
driver not loaded`. Fedora installiert die Binary als `qmake-qt5`, dort gehört
`/usr/lib64/qt5/bin` in den `PATH`.

Das Skript baut den gesamten Core, nicht nur das Getestete, und lässt danach die
QtTest-Suite laufen. Es räumt `build/` weg, sobald sich die Toolchain ändert —
Objektdateien aus zwei Qt-Installationen zu mischen führt zu irreführenden
Plugin-Fehlern.

Achtung: hier läuft Qt 5.15, auf dem Gerät Qt 5.6. Ein grüner Lauf beweist den
Target-Build nicht, in `src/core/` also bei Qt-5.6-APIs bleiben.

### Status

- [x] M1 Vertikaler Durchstich (Chat, Streaming, Verlauf) — noch nicht auf Gerät gelaufen
- [x] M2 Tool-Framework (Registry, Datenschleuse, Function-Calling-Roundtrip) —
      durch Desktop-Tests gegen ein Fake-Backend abgedeckt, noch nicht gegen ein
      echtes Modell und nicht auf dem Gerät gelaufen
- [ ] M3 Harbour-Release
- [ ] M4 Full-Target / OpenRepos
- [ ] M5 Lokale Inferenz
- [ ] M6 Sprachein- und -ausgabe

Ausführliches Konzept: [`docs/konzept-v2.md`](docs/konzept-v2.md)
