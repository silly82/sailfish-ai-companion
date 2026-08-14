# Kontext für Claude Code

Sailfish OS AI Companion. Native Qt/QML-App, zwei Build-Targets aus einem Trunk.

## Sprache

Code, Kommentare, Commit-Messages und Bezeichner auf Englisch. Nutzerseitige
Dokumentation zweisprachig: Englisch zuerst, danach Schweizer Hochdeutsch
(Standardsprache, kein Dialekt, durchgehend «ss» statt «ß»).
Übersetzungen sollen unter `translations/` (de/en) liegen — das Verzeichnis
existiert noch nicht, die Strings sind aber durchgehend in `qsTr()` gefasst.
Anlegen gehört zu M3.

## Nicht verhandelbare Architekturentscheidungen

1. **Zwei Build-Targets, kein Git-Fork.** Ein Trunk, zwei RPM-Specs.
   Der Trunk ist IMMER Harbour-clean. `fullaccess` fügt nur hinzu, entfernt nie.
   Wenn eine Änderung den Harbour-Build brechen würde, gehört sie hinter
   `Capabilities` — nicht in ein `#ifdef` quer durch den Code.

2. **Der Unterschied ist das Tool-Manifest, nicht der Code.**
   `ToolRegistry::buildManifest()` registriert nur, was `Capabilities` meldet.
   Harbour ~8 Tools, Full ~25. UI, Chat-Logik und Backend sind identisch.
   QML fragt den `Caps`-Singleton ab — **keine #ifdefs in QML**.

3. **llama.cpp wird NICHT in die App gelinkt.** Es läuft als separates Paket
   (`sailfishai-llama`) mit `llama-server` auf `127.0.0.1:8080` und einer
   OpenAI-kompatiblen API. Damit ist lokale Inference nur ein anderer
   Base-URL-Wert — `ILlmBackend` hat zwei Implementierungen, sonst ändert sich
   nichts.

4. **Modelle nie hardcoden.** Liste zur Laufzeit von `/models`, cachen,
   nach tool_use-Fähigkeit + Kontext + Preis filtern.

5. **ConsentGate ist Pflichtdurchgang.** Alles ab `Sensitivity::Personal`
   braucht Bestätigung und läuft durch die Redaktionsschicht, bevor es an ein
   Cloud-Modell geht. Der API-Key landet nie in QSettings und nie im Log.

## Harbour-Regeln (geprüft gegen docs.sailfishos.org)

Erlaubt und genutzt:
- `Sailfish.Secrets 1.0` + Permission `Secrets` — API-Key-Ablage
- `org.nemomobile.contacts 1.0` + Permission `Contacts` — read-only
- `org.kde.bluezqt 1.0` + Permission `Bluetooth`
- Akku/Netz: `/sys/class/power_supply` + `QNetworkInterface` statt ContextKit
  — im SDK-Sysroot gibt es nur das QML-Modul `org.freedesktop.contextkit 1.0`,
  keine C++-API; sysfs ist world-readable, keine Extra-Permission nötig
- `Sailfish.Telephony 1.0` ⚠️ Umfang unklar, keine Backwards-Compat-Garantie
- `Nemo.Notifications 1.0` — nur SENDEN
- `Nemo.KeepAlive 1.2` — Request zu Ende bringen, solange App läuft
- Qt5Sql + `libsqlite3.so.0`

Nicht erlaubt, gehört ins Full-Target:
- Notifications LESEN (auch mit Vollzugriff fragil — eigenes Spike-Ticket)
- SMS/Call-Log (`libcommhistory`), Kalender (`libmkcal`)
- Prozess-Spawn, systemd-User-Units, freies Dateisystem

Vor jedem Store-Upload: `sfdk check`. Die Wahrheit sind die Validator-Configs
in `sailfishos/sdk-harbour-rpmvalidator`, nicht die Doku-Seite.

## Zielhardware

| | Jolla C2 | Jolla Phone 2026 |
|---|---|---|
| SoC | Unisoc T606, 2×A75+6×A55 @1.6 | MediaTek, A78-Klasse |
| RAM | 8 GB LPDDR4X | 12 GB LPDDR5 |
| Lokales Modell | Qwen3-1.7B Q4_K_M, ~4-6 tok/s | Qwen3-4B Q4_K_M, ~6-10 tok/s |

**Prefill ist der Flaschenhals, nicht die Generierung.** 8 Tool-Schemas ≈ 1500
Token; auf dem C2 sind das ~60 s bis zum ersten Zeichen. Deshalb lokal:
`--prompt-cache`, `maxTools() = 4`, `maxContextTokens() = 4096`.
Threads nur auf die grossen Kerne — mehr Kerne bringen kaum Durchsatz, weil
bandbreitenlimitiert, aber viel Hitze.

## Bauen

```sh
sfdk config target=SailfishOS-5.0.0.x-aarch64
sfdk build                                    # Harbour (Default)
sfdk build -- --define "fullaccess 1"         # Full-Access
sfdk check                                    # Harbour-Validator
```

Ohne SDK: `src/core/` ist frei von Silica und Sailfish-APIs und baut auf dem
Desktop. `nix develop -c scripts/run-tests.sh` baut den gesamten Core und laesst
die QtTest-Suite laufen — dort faellt auf, was den Target-Build brechen wuerde.
`flake.nix` und `shell.nix` teilen sich dieselbe Shell-Definition; der klassische
Weg ueber Distributionspakete steht in der README. Aber: Desktop ist Qt 5.15, das
Geraet Qt 5.6. Ein gruener Lauf ersetzt `sfdk build` nicht, deshalb in
`src/core/` bei Qt-5.6-APIs bleiben.

## Versionierung & Releases

Semantic Versioning, `Version:` in beiden Specs (`rpm/sailfishai.spec`,
`rpm/harbour-nemoai.spec`) muss immer synchron sein — ein Trunk, eine Version
für beide Targets.

- **Bugfix** → Patch-Stelle erhöhen (`x.x.1`), z. B. 0.6.5 → 0.6.6.
- **Feature** → Minor-Stelle erhöhen (`x.1.x`), Patch zurück auf 0.
  Z. B. 0.6.6 → 0.7.0.

Jede Versionserhöhung bekommt ein GitHub-Release (Tag `vX.Y.Z`). Ein Release
ohne RPM-Assets ist unvollständig — Pflicht sind sechs Dateien, gebaut über
`sfdk build` (Harbour) und `sfdk build -- --define "fullaccess 1"` (Full),
je einmal pro Plattform (`aarch64`, `armv7hl`, `i486`):

- `harbour-nemoai-X.Y.Z-1.aarch64.rpm`
- `harbour-nemoai-X.Y.Z-1.armv7hl.rpm`
- `harbour-nemoai-X.Y.Z-1.i486.rpm`
- `sailfishai-X.Y.Z-1.aarch64.rpm`
- `sailfishai-X.Y.Z-1.armv7hl.rpm`
- `sailfishai-X.Y.Z-1.i486.rpm`

**`armv7hl` und `i486` werden nur gebaut, nicht getestet** — kein Gerät für
diese Architekturen vorhanden (s. Zielhardware, beide Geräte sind `aarch64`).
Sollte jemand mit `armv7hl`/`i486`-Hardware (z. B. Jolla 1, Jolla C, ältere
Tablets) einen Bug melden, gilt das nicht automatisch für `aarch64` und
umgekehrt.

Tag, GitHub-Release, alle sechs RPM-Builds und der Asset-Upload gehören zum
Versionsbump dazu — passiert automatisch, ohne dass extra danach gefragt
werden muss. `~/SailfishOS/bin/sfdk` (nicht auf `$PATH`) hat eine laufende
Build-Engine.

Stolpersteine, auf die man beim Bauen aller sechs Kombinationen trifft:

- **SDK-Target-Version muss zur Sailfish-OS-Version auf dem Gerät passen —
  nicht die neueste nehmen.** `aarch64` **immer** gegen
  `SailfishOS-5.0.0.62-aarch64` bauen, nicht gegen das neuere
  `SailfishOS-5.1.0.11-aarch64` (`sfdk tools list` zeigt beide; 5.1.0.11 ist
  nur `latest`, nicht das, was auf Jolla C2 / Jolla Phone 2026 läuft). Ein
  gegen 5.1.0.11 gebautes `aarch64`-RPM verlangt `libc.so.6(GLIBC_2.34)` —
  zu neu fürs Gerät, `pkcon`/`zypper` bricht mit einer langen Liste
  „Fehlgeschlagene Abhängigkeiten" ab (alle Qt5-Libs, libc, libstdc++,
  libgcc_s gleichzeitig — daran erkennt man das Symptom sofort). Gegen
  `5.0.0.62-aarch64` gebaut verlangt dieselbe Binary nur `GLIBC_2.17`.
  Für `armv7hl`/`i486` gibt es lokal **kein** `5.0.0.x`-Target (nur
  `5.0.0.62-aarch64` ist installiert) — die beiden werden zwangsläufig gegen
  `5.1.0.11` gebaut und tragen deshalb potenziell dasselbe Risiko auf echter
  (älterer) `armv7hl`/`i486`-Hardware. Ohne Testgerät für diese Architekturen
  unentdeckt, bis ein zusätzliches `5.0.0.x`-SDK-Target für sie installiert
  wird (braucht Netzwerkzugriff auf Jollas Repos, hier nicht geprüft).
- **Config-Scope überlebt keinen neuen Shell-Aufruf.** `sfdk config target=…`
  (Session-Scope) gilt nur innerhalb desselben Shell-Prozesses — jeder
  eigenständige Tool-Aufruf startet effektiv neu und fällt sonst still auf
  die zuletzt global gesetzte Target/Spec-Kombination zurück. Stattdessen pro
  Build-Befehl explizit auf Command-Scope setzen:
  `sfdk -c target=SailfishOS-5.0.0.62-aarch64 -c specfile=rpm/harbour-nemoai.spec build`.
- **Build passiert in-place, nicht in einem Shadow-Build-Verzeichnis** (kein
  `<project-dir-or-file>`-Argument an `build` übergeben). `.o`/`moc_*`-Dateien
  vom letzten Build bleiben im Arbeitsbaum liegen und werden beim nächsten
  Architektur-/Target-Wechsel mit eingelinkt — Ergebnis ein
  ABI-Mismatch-Linkerfehler (`undefined reference to operator delete(void*,
  unsigned int)`/`QArrayData::deallocate(...)`, weil 32-Bit- und
  64-Bit-Objektdateien gemischt werden). Vor jedem Build mit anderem
  Target/Spec aufräumen: `rm -f Makefile *.o moc_*.cpp moc_*.o
  harbour-nemoai sailfishai documentation.list` (nicht `git clean -xdf` ohne
  Weiteres — das reisst auch `RPMS/` mit bereits fertig gebauten Paketen mit).
- **Versionsstring im RPM trägt einen Git-Suffix**
  (`X.Y.Z+main.<timestamp>.<sha>`), sobald `HEAD` nicht exakt auf dem Tag
  `vX.Y.Z` steht (z. B. weil noch ein Doku-Commit nach dem Tag folgte). Für
  reine GitHub-Release-Assets ist das nur kosmetisch — vor dem Upload auf den
  sauberen Namen `<paket>-X.Y.Z-1.<arch>.rpm` umkopieren reicht. **Für
  `sfdk check` reicht das Umbenennen nicht**: der Harbour-Validator liest die
  echte RPM-Metadata, nicht den Dateinamen, und der Suffix lässt den
  Pflichtcheck „RPM file name" mit `ERROR: rpm version must contain only
  digits (0-9) and periods (.)` durchfallen (Validierung schlägt komplett
  fehl, nicht nur eine Warnung). Für einen validator-grünen Build also aus
  einem Checkout **exakt auf dem Release-Tag** bauen — notfalls über ein
  temporäres `git worktree add <pfad> vX.Y.Z` **unterhalb des
  SDK-Workspace-Roots** (`sfdk config` zeigt ihn; `sfdk build`/`check`
  verweigern den Dienst ausserhalb davon, z. B. unter `/tmp`).

Ablauf kurz: alle sechs bauen → sauber umbenennen → `gh release upload
vX.Y.Z <dateien>` → `gh release edit vX.Y.Z --draft=false`. Vor einem
Store-Upload zusätzlich `sfdk check` gegen einen Tag-exakten Harbour-Build
laufen lassen (s. o.).

## Nächster Schritt

`get_battery_status`/`get_network_status` waren in beiden Providern
(`SandboxedProvider` UND `FullProvider`) reine `not_implemented`-Stubs.
Beide jetzt identisch implementiert (sysfs + `QNetworkInterface`, s.o.) —
kein Codepfad-Unterschied zwischen Harbour und Full, wie
Architekturentscheidung 2 es verlangt. Auf echter Hardware (Jolla Phone
2026, aarch64) headless verifiziert: ein Testharness linkt `Capabilities`
+ `ConsentGate` + `ToolRegistry` + den jeweiligen Provider direkt (ohne
QML/AIClient/Netzwerk) und ruft `ToolRegistry::invoke()` — denselben Pfad,
den ein echter Tool-Call durchläuft. Dabei zwei Gerätebugs gefangen, die
der Desktop-Build (Qt 5.15) nicht zeigt:
- `QNetworkInterface::type()`/`Loopback` gibt es erst ab Qt 5.11; das
  Gerät läuft Qt 5.6 → über `IsLoopBack`-Flag gelöst.
- Der MTK-Fuel-Gauge-Treiber liefert `time_to_full_now` vor eingeschwungener
  Kalibrierung gelegentlich einen unplausiblen Rohwert (beobachtet: 940h) →
  auf ein 24h-Fenster gekappt statt durchgereicht.

Konversationslisten-Vorschau überlappte sich: `ConversationStore::conversations()`
gab den rohen `content` der letzten Nachricht ungefiltert als `preview`
zurück. Bei mehrzeiligen Nachrichten sprengte das die feste Zeilenhöhe von
`MainPage.qml`s `ListItem` und überlappte mit der nächsten Zeile. Fix an der
Quelle (`.simplified()` fasst Zeilenumbrüche/Whitespace-Läufe zu je einem
Leerzeichen zusammen) plus `maximumLineCount: 1` auf dem Preview-`Label` als
zweite Absicherung. Verifiziert über dasselbe Headless-Harness-Muster: Store
direkt verlinkt, mehrzeilige Testnachricht eingefügt, `preview` enthält
nachweislich kein `\n` mehr.

Settings-Seite hatte keinen Weg, ein Modell zu wählen, bevor man den ersten
Chat öffnet — `AI.model` war zwar schon über `QSettings` persistent (jede
Auswahl in `ModelPage.qml` überlebt einen Neustart), aber nur aus einem
laufenden Chat heraus erreichbar. Jetzt zusätzlich ein `ValueButton` in
`SettingsPage.qml`, der das aktuelle Modell zeigt und `ModelPage.qml` öffnet.
Kein neuer C++-Code nötig — Persistenz existierte schon in `AIClient::setModel()`.

M3 begonnen: Harbour-`KeyStore` läuft jetzt über `Sailfish.Secrets` statt über
die 0600-Datei. Neue `src/platform/sandboxed/keystore_secrets.cpp` implementiert
dieselbe `KeyStore`-Klasse (`core/keystore.h`) über `StoreSecretRequest`/
`StoredSecretRequest`/`DeleteSecretRequest`; `waitForFinished()` hält die
öffentliche API synchron, obwohl die Requests intern über D-Bus zum
`sailfishsecretsd` laufen. `core/keystore.cpp` (die 0600-Datei-Variante)
bleibt bestehen, ist aber jetzt nur noch im `fullaccess`-Block der `.pro`-Datei
verdrahtet — Full läuft unsandboxed und wird ausserdem von den Desktop-Tests
gebaut, die kein `Sailfish.Secrets` zur Verfügung haben. Kein `#ifdef` quer
durch `keystore.cpp`, sondern zwei Implementierungen derselben Klasse, über
`CONFIG(harbour)`/`CONFIG(fullaccess)` ausgewählt — Architekturentscheidung 1.

0.7.0 hatte zwei Bugs, beide erst beim echten Geräte-Install/-Betrieb
aufgefallen (Desktop-Tests und Emulator zeigen sie nicht):

1. **`aarch64`-RPMs gegen das falsche SDK-Target gebaut.** Gegen das lokal
   neueste `SailfishOS-5.1.0.11` statt gegen das dokumentierte
   `SailfishOS-5.0.0.x`, das zur echten Geräte-OS-Version passt. Ergebnis:
   `libc.so.6(GLIBC_2.34)` als Abhängigkeit — zu neu fürs Gerät
   (`GLIBC_2.17`), `pkcon`/`zypper` bricht mit „Fehlgeschlagene
   Abhängigkeiten" für praktisch jede gelinkte Lib gleichzeitig ab. Fix:
   `aarch64` konsequent gegen `SailfishOS-5.0.0.62-aarch64` bauen (Details
   und Stolperstein in „Versionierung & Releases" oben).
2. **Falsches Secrets-Plugin-Muster für Standalone-Secrets.**
   `keystore_secrets.cpp` nutzte `SecretManager::DefaultEncryptedStoragePluginName`
   (sqlcipher) als Storage-Plugin der `Secret::Identifier`, ohne je ein
   Encryption-Plugin zu setzen. Laut Jollas eigener Doku (lokal im SDK als
   `sailfish-secrets260616.qch` gebündelt, „Which Plugin Should My
   Application Use?") ist das nicht das dokumentierte Muster für
   `StoreSecretRequest::StandaloneDeviceLockSecret` — der offizielle Weg ist
   `SecretManager::DefaultStoragePluginName` als Storage-Plugin plus
   separates `setEncryptionPluginName(SecretManager::DefaultEncryptionPluginName)`.
   `DefaultEncryptedStoragePluginName` ist für `CreateCollectionRequest`
   gedacht, wo dieselbe Plugin-Id beide Rollen übernimmt. Auf dem Gerät
   scheiterte der alte Code mit „no such plugin exists"
   (`org.sailfishos.secrets.plugin.encryptedstorage.sqlcipher`) beim
   Key-Speichern — der Datenblob wird mit dem Fix trotzdem verschlüsselt,
   das ist ja der Zweck des Encryption-Plugins.

`harbour-rpmvalidator` (`sfdk check`) lief gegen einen `aarch64`-Build exakt
auf dem `v0.7.1`-Tag: **„Validation succeeded"**, nur eine nicht blockierende
Warnung (`file is not stripped!` — rpmlint, kein Store-Blocker; die
`brp-strip`-Skripte laufen laut Build-Log, das Binary bleibt trotzdem
ungestrippt, noch nicht weiter untersucht). Dabei fiel ein zunächst harter
Fehler auf: gegen den `HEAD`-Stand (zwei Commits nach dem Tag) gebaut, schlug
der Pflichtcheck „RPM file name" fehl, weil die RPM-Metadata den
Dirty-Git-Versions-Suffix trägt (`0.7.1+main.<timestamp>.<sha>` statt
`0.7.1`) — Harbour erlaubt in der Version nur Ziffern und Punkte. Für den
validator-tauglichen Build stattdessen aus einem `git worktree add
<pfad-unter-dem-sdk-workspace> vX.Y.Z`-Checkout gebaut (Details im
Stolperstein oben unter „Versionierung & Releases").

Noch offen für M3: Icons (aktuell Platzhalter, s. u.), Übersetzungen (de/en)
unter `translations/` anlegen, Store-Assets/Screenshots.

Version auf 0.7.1 (Patch — beides Bugfixes an 0.7.0, kein neues Feature).

Detailkonzept: `docs/konzept-v2.md`
