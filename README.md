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

### First-run fixes

This commit fixes several issues that affected the first launch on the Sailfish emulator:

- the main QML file is now loaded correctly for both targets
- the app organization name matches the Sailjail permission whitelist
- the key storage context object no longer collides with QtQuick's built-in `Keys`
- the settings page now shows key-save errors instead of clearing the field silently
- placeholder app icons are included so packaging succeeds

### Battery and network status

`get_battery_status` and `get_network_status` were `not_implemented` stubs in
both providers until now — there's no C++ ContextKit package in the SDK
sysroot (only its QML module), so both read `/sys/class/power_supply`
directly (percentage, charging state, and a plausibility-capped time
estimate) and use `QNetworkInterface` for connection type/IP. Identical
implementation in both targets, verified on real device hardware (Jolla
Phone 2026) through a headless harness that calls the same
`ToolRegistry::invoke()` path a real tool call takes.

### Full-access tools

Beyond Harbour's tool set, the full-access build (`sailfishai`) adds three
more tools, all **disabled by default** and requiring a per-call
confirmation once enabled, showing the exact call before anything runs:

- `get_upcoming_events` — calendar, via `mkcal-qt5`
- `read_recent_messages` — recent SMS conversations, via `libcommhistory-qt5`
- `run_command` — runs a program with arguments via `QProcess` and returns
  its exit code, stdout and stderr

⚠️ **`run_command` lets the model execute arbitrary programs with the app's
OS privileges.** There is no allowlist or sandboxing beyond the consent
dialog and the default-off toggle — leave it disabled unless you
specifically want an AI that can run commands on your phone.

The full-access package additionally requires `libcommhistory-qt5`,
`mkcal-qt5`, and `kf5-calendarcore` at runtime. These normally ship with the
native Messages and Calendar apps; if your device is missing any of them,
install first with `pkcon install <name>`.

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

- [x] M1 Vertical slice (chat, streaming, history) — **confirmed on real
      Jolla hardware**: a test API key plus a device-storage query
      round-tripped through a tool call against a real cloud model
      (OpenRouter) without errors
- [x] M2 Tool framework (registry, consent gate, function-calling roundtrip) —
      covered by desktop tests against a fake backend, and now also verified
      against a real model on-device (see M1)
- [ ] M3 Harbour release — pending: move the API key from a 0600 file to
      `Sailfish.Secrets`
- [x] M4 Full target / OpenRepos — calendar/SMS/`run_command` tools
      implemented (see [Full-access tools](#full-access-tools) above);
      installs and launches cleanly, verified on the SDK emulator, but the
      full in-app flow (granting tool consent, sending a message) is not
      yet click-tested — only physical devices or a VNC/RDP-capable setup
      can drive touch input into the emulator
- [ ] M5 Local inference
- [ ] M6 Voice

Full concept: [`docs/konzept-v2.md`](docs/konzept-v2.md)

### License

MIT — see [`LICENSE`](LICENSE). Copy, modify and redistribute freely; the only
condition is that the copyright notice travels with it.

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

### First-Run-Fixes

Dieser Commit behebt mehrere Probleme beim ersten Start im Sailfish-Emulator:

- die Haupt-QML-Datei wird für beide Targets nun korrekt geladen
- der Organisationsname der App passt jetzt zur Sailjail-Whitelist
- das Kontextobjekt für den Schlüsselspeicher kollidiert nicht mehr mit dem eingebauten QtQuick-Property `Keys`
- die Einstellungsseite zeigt Fehler beim Speichern des Schlüssels an, statt das Feld stillschweigend zu leeren
- Platzhalter-Icons sind enthalten, damit das Packaging erfolgreich durchläuft

### Akku- und Netzwerkstatus

`get_battery_status` und `get_network_status` waren bisher in beiden
Providern reine `not_implemented`-Stubs — im SDK-Sysroot gibt es kein
C++-ContextKit-Paket, nur dessen QML-Modul. Beide lesen jetzt direkt
`/sys/class/power_supply` (Prozent, Ladezustand, plausibilitätsgekappte
Restzeit) und nutzen `QNetworkInterface` für Verbindungstyp/IP. Identische
Implementierung in beiden Targets, verifiziert auf echter Gerätehardware
(Jolla Phone 2026) über ein headless Testharness, das denselben
`ToolRegistry::invoke()`-Pfad aufruft wie ein echter Tool-Call.

### Tools im Vollzugriffs-Build

Zusätzlich zum Harbour-Tool-Set bringt der Vollzugriffs-Build (`sailfishai`)
drei weitere Tools mit, alle **standardmässig deaktiviert** und nach dem
Einschalten mit Bestätigung pro Aufruf, die den genauen Aufruf zeigt, bevor
etwas passiert:

- `get_upcoming_events` — Kalender, über `mkcal-qt5`
- `read_recent_messages` — jüngste SMS-Konversationen, über
  `libcommhistory-qt5`
- `run_command` — führt ein Programm mit Argumenten über `QProcess` aus und
  liefert Exit-Code, Stdout und Stderr zurück

⚠️ **`run_command` erlaubt dem Modell, beliebige Programme mit den Rechten
der App auszuführen.** Es gibt ausser dem Bestätigungsdialog und dem
standardmässig ausgeschalteten Schalter keine Absicherung — lass es
deaktiviert, wenn du keine KI willst, die Befehle auf deinem Handy ausführen
kann.

Das Vollzugriffs-Paket braucht zur Laufzeit zusätzlich `libcommhistory-qt5`,
`mkcal-qt5` und `kf5-calendarcore`. Die kommen normalerweise mit den
nativen Nachrichten- und Kalender-Apps mit; falls dem Gerät eine davon
fehlt, vorher mit `pkcon install <name>` installieren.

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

- [x] M1 Vertikaler Durchstich (Chat, Streaming, Verlauf) — **auf echter
      Jolla-Hardware bestätigt**: Test-API-Key eingegeben, Speicherabfrage
      per Tool-Call gegen ein echtes Cloud-Modell (OpenRouter) fehlerfrei
      durchgelaufen
- [x] M2 Tool-Framework (Registry, Datenschleuse, Function-Calling-Roundtrip) —
      durch Desktop-Tests gegen ein Fake-Backend abgedeckt, und jetzt auch
      gegen ein echtes Modell auf dem Gerät verifiziert (siehe M1)
- [ ] M3 Harbour-Release — offen: den API-Key von einer 0600-Datei auf
      `Sailfish.Secrets` umstellen
- [x] M4 Full-Target / OpenRepos — Kalender-/SMS-/`run_command`-Tools
      implementiert (siehe [Tools im Vollzugriffs-Build](#tools-im-vollzugriffs-build)
      oben); installiert und startet sauber, auf dem SDK-Emulator verifiziert,
      der volle In-App-Ablauf (Tool-Consent erteilen, Nachricht senden) aber
      noch nicht durchgeklickt — nur echte Geräte oder ein
      VNC/RDP-fähiges Setup können Touch-Eingaben in den Emulator steuern
- [ ] M5 Lokale Inferenz
- [ ] M6 Sprachein- und -ausgabe

Ausführliches Konzept: [`docs/konzept-v2.md`](docs/konzept-v2.md)

### Lizenz

MIT — siehe [`LICENSE`](LICENSE). Kopieren, ändern und weitergeben ist frei;
einzige Bedingung ist, dass der Copyright-Vermerk mitgeht.
