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

### UI fixes and default model

The conversation list preview could overlap the next row when the last
message spanned multiple lines — `ConversationStore` now collapses it to a
single line (`.simplified()`), with `maximumLineCount: 1` in `MainPage.qml`
as a second safeguard. The Settings page also gained a "Default model" entry
that shows the currently selected model and opens the model picker — the
choice already persisted across restarts, it just had no way to be reached
before starting a chat.

### API key storage (Sailfish.Secrets)

The Harbour build now stores the API key via `Sailfish.Secrets` 1.0 instead
of the earlier 0600-permission file. The full-access build keeps the
file-based fallback unchanged (it's unsandboxed anyway, and it's what the
desktop test suite links against, since there's no `Sailfish.Secrets` there).

Two bugs surfaced only once this shipped to real hardware — neither the
desktop tests nor the SDK emulator caught them:

- **Wrong SDK build target for `aarch64`.** RPMs built against the SDK's
  newest target (`SailfishOS-5.1.0.11`) require a glibc newer than what's on
  the actual device (`SailfishOS-5.0.0.x`), so installation failed outright
  with a full page of "failed dependencies". Fixed by building `aarch64`
  against `SailfishOS-5.0.0.62-aarch64` specifically — see the version note
  in the build command below.
- **Wrong Secrets plugin for a standalone secret.** The code used
  `SecretManager::DefaultEncryptedStoragePluginName` (SQLCipher) as the
  storage plugin without ever setting an encryption plugin — that
  combination is meant for `CreateCollectionRequest`, not a standalone
  `StoreSecretRequest`, and failed on-device with "no such plugin exists".
  Fixed to follow Jolla's documented pattern instead:
  `DefaultStoragePluginName` plus a separately set
  `DefaultEncryptionPluginName`.

Both fixes shipped in v0.7.1 and are confirmed on real Jolla hardware: key
storage and a full chat round-trip both work end to end.

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

Use `SailfishOS-5.0.0.x`, not the SDK's newest installed target — a newer
target links against a newer glibc than what ships on-device and the
resulting package fails to install (see
[API key storage](#api-key-storage-sailfishsecrets) above). Building for
`i486`/`armv7hl` needs the matching `SailfishOS-5.0.0.x-i486`/`-armv7hl`
target installed; if only a newer target is available for those
architectures, treat those two builds as unverified until a matching
`5.0.0.x` target exists (there's no `i486`/`armv7hl` Jolla hardware to test
against here either way).

Run `sfdk check` before every store upload — **from a checkout exactly on
the release tag.** A dirty git-suffix version (e.g. `0.7.1+main.<timestamp>.
<sha>`, added automatically whenever `HEAD` is ahead of the tag) fails the
mandatory "RPM file name" check outright, not just a warning. If your
working tree has moved past the tag, build from a separate checkout instead,
e.g. `git worktree add ../check-v0.7.1 v0.7.1`.

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
      (OpenRouter) without errors; re-confirmed after the M3 Secrets
      migration below, key storage and a full chat round-trip both work
- [x] M2 Tool framework (registry, consent gate, function-calling roundtrip) —
      covered by desktop tests against a fake backend, and now also verified
      against a real model on-device (see M1)
- [x] M3 Harbour release — API key now stored via `Sailfish.Secrets` (see
      [API key storage](#api-key-storage-sailfishsecrets) above), confirmed
      on real hardware after fixing two on-device-only bugs.
      `harbour-rpmvalidator` (`sfdk check`) passes clean against a build
      from the exact release tag (a dirty git-suffix version fails it — see
      the build note above). German translation shipped under
      `translations/harbour-nemoai-de.ts` (UI source strings are English
      as of 0.8.0). Icons are no longer placeholders either — Harbour keeps
      the robot glyph, `sailfishai` adds a red skull badge so the two
      variants are distinguishable at a glance. Store submission material
      lives in `store/`: listing text (English and German), a bilingual
      privacy policy ([`docs/privacy.md`](docs/privacy.md)), a cover image,
      and screenshots. Screenshots are real Jolla Phone 2026 captures —
      device locale was German at capture time, so the listing carries both
      English and German Details to match — upscaled from the device's
      native 1032×2272 to 1080×2378 to clear the Store form's stated
      1080×1920 minimum, replacing the earlier 336×798 emulator
      placeholders. RPMs (`harbour-nemoai-0.8.0-1.{aarch64,armv7hl,i486}.rpm`)
      built and **submitted to Jolla Store QA on 2026-08-14 — awaiting
      approval.** M3 is otherwise complete
- [x] M4 Full target / OpenRepos — calendar/SMS/`run_command` tools
      implemented (see [Full-access tools](#full-access-tools) above);
      installs and launches cleanly, verified on the SDK emulator including
      real navigation (conversation list, pulley menu, Settings, Tools &
      Permissions) driven by a small `uinput`-based touch injector — no
      VNC/RDP setup needed, see CLAUDE.md for how. The one flow still not
      click-tested is an actual message send with tool consent, since that
      needs a real API key against a live cloud model, not available in
      this environment. A real-device self-test also surfaced three open
      gaps in the full-access tools — calendar access failing with
      `query_failed`, `find_contact` still unimplemented, and `run_command`
      timing out on interactive programs like `top` — tracked with a fix
      plan in [`docs/m4-follow-up-tools.md`](docs/m4-follow-up-tools.md)
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

### UI-Fixes und Standardmodell

Die Konversationslisten-Vorschau konnte die nächste Zeile überlappen, wenn
die letzte Nachricht mehrzeilig war — `ConversationStore` fasst sie jetzt zu
einer Zeile zusammen (`.simplified()`), mit `maximumLineCount: 1` in
`MainPage.qml` als zweiter Absicherung. Die Einstellungsseite hat ausserdem
einen neuen Eintrag „Standardmodell" bekommen, der das aktuell gewählte
Modell zeigt und die Modellauswahl öffnet — die Wahl war schon vorher über
Neustarts hinweg persistent, war aber ohne einen offenen Chat nicht
erreichbar.

### API-Key-Ablage (Sailfish.Secrets)

Der Harbour-Build speichert den API-Key jetzt über `Sailfish.Secrets` 1.0
statt über die frühere Datei mit 0600-Rechten. Der Vollzugriffs-Build behält
die dateibasierte Variante unverändert bei (läuft ohnehin unsandboxed, und
die Desktop-Testsuite linkt dagegen, da es dort kein `Sailfish.Secrets`
gibt).

Zwei Bugs sind erst beim echten Geräte-Rollout aufgefallen — weder die
Desktop-Tests noch der SDK-Emulator haben sie gezeigt:

- **Falsches SDK-Build-Target für `aarch64`.** Gegen das neueste SDK-Target
  (`SailfishOS-5.1.0.11`) gebaute RPMs verlangen ein neueres glibc, als auf
  dem echten Gerät (`SailfishOS-5.0.0.x`) läuft — die Installation scheiterte
  komplett mit einer ganzen Seite „Fehlgeschlagene Abhängigkeiten". Fix:
  `aarch64` gezielt gegen `SailfishOS-5.0.0.62-aarch64` bauen — siehe der
  Versionshinweis beim Build-Befehl unten.
- **Falsches Secrets-Plugin für ein Standalone-Secret.** Der Code nutzte
  `SecretManager::DefaultEncryptedStoragePluginName` (SQLCipher) als
  Storage-Plugin, ohne je ein Encryption-Plugin zu setzen — diese
  Kombination ist für `CreateCollectionRequest` gedacht, nicht für ein
  Standalone-`StoreSecretRequest`, und scheiterte auf dem Gerät mit „no such
  plugin exists". Fix: stattdessen Jollas dokumentiertes Muster —
  `DefaultStoragePluginName` plus separat gesetztes
  `DefaultEncryptionPluginName`.

Beide Fixes sind in v0.7.1 enthalten und auf echter Jolla-Hardware bestätigt:
Key-Speicherung und ein voller Chat-Roundtrip funktionieren durchgängig.

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

`SailfishOS-5.0.0.x` verwenden, nicht das neueste installierte SDK-Target —
ein neueres Target linkt gegen ein neueres glibc, als auf dem Gerät läuft,
und das fertige Paket lässt sich dann nicht installieren (siehe
[API-Key-Ablage](#api-key-ablage-sailfishsecrets) oben). Für `i486`/`armv7hl`
wird das passende `SailfishOS-5.0.0.x-i486`/`-armv7hl`-Target gebraucht;
ist dafür nur ein neueres Target verfügbar, gelten diese beiden Builds als
ungeprüft, bis ein passendes `5.0.0.x`-Target existiert (für diese
Architekturen gibt es hier ohnehin keine Jolla-Hardware zum Testen).

`sfdk check` vor jedem Store-Upload ausführen — **aus einem Checkout genau
auf dem Release-Tag.** Ein Dirty-Git-Versions-Suffix (z. B.
`0.7.1+main.<timestamp>.<sha>`, automatisch dabei, sobald `HEAD` dem Tag
voraus ist) lässt den Pflichtcheck „RPM file name" komplett durchfallen,
nicht nur eine Warnung. Steht der Arbeitsbaum weiter als der Tag, stattdessen
aus einem separaten Checkout bauen, z. B. `git worktree add
../check-v0.7.1 v0.7.1`.

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
      durchgelaufen; nach der M3-Secrets-Umstellung unten erneut bestätigt —
      Key-Speicherung und ein voller Chat-Roundtrip funktionieren beide
- [x] M2 Tool-Framework (Registry, Datenschleuse, Function-Calling-Roundtrip) —
      durch Desktop-Tests gegen ein Fake-Backend abgedeckt, und jetzt auch
      gegen ein echtes Modell auf dem Gerät verifiziert (siehe M1)
- [x] M3 Harbour-Release — API-Key wird jetzt über `Sailfish.Secrets`
      gespeichert (siehe [API-Key-Ablage](#api-key-ablage-sailfishsecrets)
      oben), auf echter Hardware bestätigt nach dem Fix zweier Bugs, die nur
      auf dem Gerät auffielen. `harbour-rpmvalidator` (`sfdk check`) läuft
      grün gegen einen Build exakt vom Release-Tag (ein Dirty-Git-Suffix in
      der Version lässt ihn durchfallen — siehe Build-Hinweis oben). Deutsche
      Übersetzung liegt unter `translations/harbour-nemoai-de.ts` (UI-
      Quelltexte sind seit 0.8.0 Englisch). Auch die Icons sind keine
      Platzhalter mehr — Harbour behält den Robo-Kopf, `sailfishai` bekommt
      ein rotes Totenkopf-Badge zur Unterscheidung auf einen Blick.
      Einreichungsmaterial liegt unter `store/`: Listing-Text (Englisch und
      Deutsch), eine zweisprachige Datenschutzerklärung
      ([`docs/privacy.md`](docs/privacy.md)), ein Cover-Bild und
      Screenshots. Die Screenshots sind echte Aufnahmen vom Jolla Phone
      2026 — Geräte-Systemsprache war zum Aufnahmezeitpunkt Deutsch,
      deshalb trägt das Listing passend dazu sowohl englische als auch
      deutsche Details — von der geräteeigenen Auflösung 1032×2272 auf
      1080×2378 hochskaliert, um das im Formular geforderte Minimum von
      1080×1920 zu erfüllen; ersetzen die vorherigen Emulator-Platzhalter
      bei 336×798. RPMs (`harbour-nemoai-0.8.0-1.{aarch64,armv7hl,i486}.rpm`)
      gebaut und am 14.08.2026 **bei der Jolla-Store-QA eingereicht — wartet
      auf Freigabe.** Damit ist M3 sonst abgeschlossen
- [x] M4 Full-Target / OpenRepos — Kalender-/SMS-/`run_command`-Tools
      implementiert (siehe [Tools im Vollzugriffs-Build](#tools-im-vollzugriffs-build)
      oben); installiert und startet sauber, auf dem SDK-Emulator verifiziert
      inklusive echter Navigation (Konversationsliste, Pulley-Menü,
      Einstellungen, Tools & Freigaben) über einen kleinen
      `uinput`-basierten Touch-Injektor — kein VNC/RDP-Setup nötig, siehe
      CLAUDE.md für den Weg dahin. Einzig eine echte Nachricht mit
      Tool-Consent ist noch nicht durchgeklickt, da das einen echten
      API-Key gegen ein laufendes Cloud-Modell braucht, den es in dieser
      Umgebung nicht gibt. Ein Selbsttest auf echter Hardware hat zudem drei
      offene Lücken in den Full-Access-Tools aufgedeckt — Kalenderzugriff
      scheitert mit `query_failed`, `find_contact` ist noch nicht
      implementiert, und `run_command` läuft bei interaktiven Programmen wie
      `top` in einen Timeout — mit Fix-Plan festgehalten in
      [`docs/m4-follow-up-tools.md`](docs/m4-follow-up-tools.md)
- [ ] M5 Lokale Inferenz
- [ ] M6 Sprachein- und -ausgabe

Ausführliches Konzept: [`docs/konzept-v2.md`](docs/konzept-v2.md)

### Lizenz

MIT — siehe [`LICENSE`](LICENSE). Kopieren, ändern und weitergeben ist frei;
einzige Bedingung ist, dass der Copyright-Vermerk mitgeht.
