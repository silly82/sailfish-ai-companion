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

### Building

```sh
sfdk config target=SailfishOS-5.0.0.x-aarch64

sfdk build                                    # Harbour (default)
sfdk build -- --define "fullaccess 1"         # full access
sfdk check                                    # Harbour validator
```

Run `sfdk check` before every store upload.

Without the SDK: `src/core/` is plain Qt by design and builds on the desktop.

```sh
nix-shell --run 'scripts/run-tests.sh'
```

This builds the whole core and runs the QtTest suite. Note it uses Qt 5.15
while the device runs Qt 5.6 — a green run is not proof that the target builds.

### Status

- [x] M1 Vertical slice (chat, streaming, history) — not yet run on a device
- [ ] M2 Tool framework
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

### Bauen

```sh
sfdk config target=SailfishOS-5.0.0.x-aarch64

sfdk build                                    # Harbour (Standard)
sfdk build -- --define "fullaccess 1"         # Vollzugriff
sfdk check                                    # Harbour-Validator
```

`sfdk check` vor jedem Store-Upload ausführen.

Ohne SDK: `src/core/` ist bewusst reines Qt und baut auf dem Desktop.

```sh
nix-shell --run 'scripts/run-tests.sh'
```

Das baut den gesamten Core und führt die QtTest-Suite aus. Achtung: hier läuft
Qt 5.15, auf dem Gerät Qt 5.6 — ein grüner Lauf beweist den Target-Build nicht.

### Status

- [x] M1 Vertikaler Durchstich (Chat, Streaming, Verlauf) — noch nicht auf Gerät gelaufen
- [ ] M2 Tool-Framework
- [ ] M3 Harbour-Release
- [ ] M4 Full-Target / OpenRepos
- [ ] M5 Lokale Inferenz
- [ ] M6 Sprachein- und -ausgabe

Ausführliches Konzept: [`docs/konzept-v2.md`](docs/konzept-v2.md)
