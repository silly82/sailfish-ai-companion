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

Version auf 0.6.0 (Minor, da Full-Access-Verhalten sich ändert).

**Nächste Ziele:**
1. Konversationslisten-Vorschau überlappt sich — jede Konversation in der
   Liste (`MainPage.qml`, vermutlich derselbe Delegate-Bereich wie der
   bereits gefixte Ghost-Text-Bug) auf eine Zeile begrenzen.
2. Setting für ein Default-Modell ergänzen, statt bei jedem Start/Neuer
   Konversation manuell wählen zu müssen.

Detailkonzept: `docs/konzept-v2.md`
