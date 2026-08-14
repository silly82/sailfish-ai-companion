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
ohne RPM-Assets ist unvollständig — Pflicht sind vier Dateien, gebaut über
`sfdk build` (Harbour) und `sfdk build -- --define "fullaccess 1"` (Full),
je einmal pro Plattform (`aarch64`, `i486`):

- `harbour-nemoai-X.Y.Z-1.aarch64.rpm`
- `harbour-nemoai-X.Y.Z-1.i486.rpm`
- `sailfishai-X.Y.Z-1.aarch64.rpm`
- `sailfishai-X.Y.Z-1.i486.rpm`

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
`sailfishsecretsd` laufen — AIClient/OpenRouterBackend lesen den Key mitten im
Request-Aufbau, ein asynchroner Umbau hätte sich durch mehrere Schichten
gezogen für einen Vorgang, der nur beim Start und beim Speichern/Löschen des
Keys überhaupt passiert. Ein kleiner Prozess-Cache (`QHash`, Datei-lokal)
erspart wiederholte D-Bus-Requests für denselben Provider.

`core/keystore.cpp` (die 0600-Datei-Variante) bleibt bestehen, ist aber jetzt
nur noch im `fullaccess`-Block der `.pro`-Datei verdrahtet — sie ist dort
weiterhin die dokumentierte Fallback-Implementierung (Full läuft unsandboxed,
das Dateisystem ist sowieso frei zugänglich) und wird ausserdem von den
Desktop-Tests gebaut, die kein `Sailfish.Secrets` zur Verfügung haben. Damit
kein `#ifdef` quer durch `keystore.cpp`, sondern zwei Implementierungen
derselben Klasse, über `CONFIG(harbour)`/`CONFIG(fullaccess)` ausgewählt —
Architekturentscheidung 1.

`rpm/harbour-nemoai.spec`: `BuildRequires: pkgconfig(sailfishsecrets)` ergänzt;
dabei auch `Requires: nemo-qml-plugin-contextkit-qt5` entfernt — die Zeile war
seit dem sysfs-Umbau für Akku/Netz (0.5.1/0.6.0) bereits Leiche, nichts im Code
importiert das QML-Modul mehr. `rpm/sailfishai.spec` (Full) bleibt unverändert,
da Full weiter die Datei-Variante nutzt und `sailfishsecrets` nicht braucht.

Noch offen für M3: `harbour-rpmvalidator` gegen den neuen Build laufen lassen
(`sfdk check`, hier ohne SDK nicht geprüft), Icons, Übersetzungen (de/en) unter
`translations/` anlegen, Store-Assets.

Version auf 0.7.0 (Minor, neues Feature: Secrets-Backend für Harbour).

Detailkonzept: `docs/konzept-v2.md`
