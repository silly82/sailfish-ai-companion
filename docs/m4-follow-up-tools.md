# M4 Nacharbeiten — offene Tool-Lücken (Kalender, Kontakte, `run_command`)

> Ausgangspunkt: Ein Schnittstellen-Check-Screenshot vom Testgerät
> (2026-08-14, siehe `Screenshot_20260814_130139_001.png`) zeigte drei
> Auffälligkeiten in den Full-Access-Tools, die hier aufgearbeitet werden.

## Ausgangslage

Auf dem Jolla-Testgerät wurde ein Chat-Screenshot analysiert, in dem die App
einen Selbsttest aller Tool-Schnittstellen durchgeführt hat. Drei Punkte sind
dabei aufgefallen:

1. `get_upcoming_events` → Fehler `query_failed` (Kalenderdatenbank nicht
   erreichbar/gesperrt)
2. `find_contact` → `not_implemented`
3. `run_command` mit `top` → Timeout (15s), weil `top` interaktiv ist

Der Code dazu liegt in `src/core/toolregistry.cpp` (Tool-Definitionen),
`src/platform/isystemprovider.h` (Interface) sowie den beiden Implementierungen
`src/platform/full/fullprovider.cpp` (OpenRepos, unsandboxed) und
`src/platform/sandboxed/sandboxedprovider.cpp` (Harbour/Jolla Store).

**Wichtiger Kontext-Fund:** Das zum Zeitpunkt der Analyse installierte Paket
auf dem Gerät war `harbour-nemoai` (bestätigt per `rpm -qR` — keine
`libmkcal`/`libcommhistory`-Abhängigkeiten, also der Harbour-Build). In diesem
Build sind `calendar()` und `automation()` in `src/core/capabilities.cpp` fest
auf `false` gesetzt (`#ifdef SFAI_HARBOUR`), wodurch `get_upcoming_events` und
`run_command` dort **gar nicht erst als Tool registriert** werden können. Die
persistierte Conversation-History auf dem Gerät
(`~/.local/share/ch.silly/harbour-nemoai/history.db`) bestätigt das: Der
letzte gespeicherte Selbsttest (07:53 UTC) hat nur `get_battery_status`,
`get_network_status`, `get_storage_status`, `get_datetime` aufgerufen — keine
Spur von `get_upcoming_events`/`run_command`. Die im Screenshot gezeigte
Session (13:00 Uhr) ist in keiner der beiden DB-Dateien auf dem Gerät
vorhanden.

Das deutet stark darauf hin, dass beim Screenshot ein **abweichend deployter
Full-Access-Build** (z. B. Qt-Creator-„Deploy to device" mit
`CONFIG+=fullaccess`, das direkt das Binary unter dem Pfad
`/usr/bin/harbour-nemoai` überschreibt, ohne das RPM-Tracking zu berühren) auf
dem Gerät lief — nicht das tatsächlich installierte Harbour-Paket. Das ist an
sich unkritisch (normaler Dev-Workflow), aber **relevant, weil das Projekt am
2026-08-14 bei der Jolla-Store-QA eingereicht wurde**: Ein liegengebliebener
Full-Access-Testbuild unter dem Harbour-Namen auf dem Testgerät kann künftige
Tests/Screenshots verfälschen. Deshalb als erster Schritt eine Verifikation,
kein Code-Fix.

## Betroffene Build-Targets

Wichtig für die Umsetzung: Nur `find_contact` betrifft beide Targets. Die
anderen beiden Punkte lassen sich am eingereichten Harbour-Paket gar nicht
reproduzieren, weil die Tools dort erst gar nicht registriert werden
(`Capabilities::calendar()`/`automation()` sind unter `SFAI_HARBOUR` fest auf
`false` gesetzt, siehe `src/core/capabilities.cpp:6-14`).

| Punkt | Betroffenes Target | Build-Befehl zum Testen |
|---|---|---|
| 1. Kalender (`query_failed`) | **Nur Full-Access** (`sailfishai`, OpenRepos) | `sfdk build -- --define "fullaccess 1"` |
| 2. `find_contact` | **Beide** (`harbour-nemoai` und `sailfishai`) | `sfdk build` (Harbour) und wie oben |
| 3. `run_command`/`top` | **Nur Full-Access** (`sailfishai`, OpenRepos) | `sfdk build -- --define "fullaccess 1"` |

Das Harbour-Paket (`harbour-nemoai`), das bei der Jolla-Store-QA eingereicht
wurde, ist von Punkt 1 und 3 also überhaupt nicht betroffen — dort existieren
`get_upcoming_events` und `run_command` als Tools schlicht nicht. Sie sind nur
relevant für die separate `sailfishai`-Full-Access-Variante für OpenRepos.

## Empfohlenes Vorgehen

### 0. Sauberen Zustand auf dem Testgerät sicherstellen (keine Code-Änderung)
Vor den folgenden Fixes: `harbour-nemoai` auf dem Gerät deinstallieren und aus
dem sauberen `rpm/harbour-nemoai.spec`-Build neu installieren (oder zumindest
`rpm -qi`/Binary-Timestamp gegen den zuletzt deployten Build abgleichen), damit
alle weiteren Tests garantiert gegen den echten Harbour-Sandboxed-Build laufen
und nicht gegen einen liegengebliebenen Full-Access-Deploy.

### 1. Kalenderzugriff (`query_failed`) — Diagnose vor Fix [nur Full-Access]
Betroffen: `FullProvider::upcomingEvents()` in
`src/platform/full/fullprovider.cpp:162-190`.

Der Code schluckt aktuell den eigentlichen Fehler: `storage->open()` bzw.
`storage->load(start, end)` liefern nur `bool`, der konkrete Grund (DB
gesperrt, Schema fehlt, falscher User, mkcal-Backend nicht initialisiert)
geht verloren. Vor einem funktionalen Fix:

- `qCWarning`-Logging ergänzen, das den tatsächlichen mKCal/SQLite-Fehler
  protokolliert (mKCal::ExtendedStorage bietet Fehler-Signale/-Methoden, die
  am realen SDK-Sysroot geprüft werden müssen).
- Mit diesem Logging auf dem `sailfishai`-Full-Access-Build (nicht dem
  Harbour-Build!) reproduzieren, da `upcomingEvents()` nur dort kompiliert
  wird.
- Naheliegendste Ursache basierend auf dem Code: `mKCal::SqliteStorage` wird
  bei jedem Tool-Call neu erzeugt und geöffnet/geschlossen — parallele
  Zugriffe (z. B. durch `jolla-calendar`/`msyncd`, die die DB offen halten)
  können zu Lock-Konflikten führen. Ggf. Retry mit kurzem Backoff oder Prüfen,
  ob `storage->open()` idempotent mit einer länger lebenden Storage-Instanz
  aufgerufen werden sollte statt pro Aufruf neu.

### 2. `find_contact` implementieren [beide Targets]
Betroffen: `ISystemProvider::findContact` (Interface bereits vorhanden),
Stubs in `fullprovider.cpp:122-123` und `sandboxedprovider.cpp:121-128`
(dort bereits mit TODO „M2: org.nemomobile.contacts 1.0, read-only" annotiert).

- Für beide Provider dieselbe Backend-Wahl: **QtContacts**
  (`QContactManager` mit dem `org.nemomobile.contacts.sqlite`-Backend), da
  das die von `docs.sailfishos.org` dokumentierte Harbour-konforme C++-API
  ist (das QML-Pendant `Sailfish.Contacts`/`org.nemomobile.contacts` nutzt
  intern dasselbe Backend). Kein D-Bus nötig.
- `.pro`: `QT += contacts` in beiden Targets ergänzen (gemeinsame Zeile, kein
  fullaccess/harbour-Split nötig, da das Backend in beiden Builds gleich
  zugänglich ist).
- Implementierung: `QContactManager` mit Name-Filter (`QContactName`/
  `QContactDisplayLabel`) nach `query` durchsuchen, Telefonnummern
  (`QContactPhoneNumber`) und Adressen (`QContactAddress`) aus den Treffern
  extrahieren, als `QVariantMap`/`QVariantList` zurückgeben — analog zum
  bestehenden Rückgabemuster von `recentMessages()`/`upcomingEvents()`.
- Consent/Redaktion ist bereits vorhanden: `find_contact` ist als
  `ConsentGate::Personal` mit `enabled=false` registriert
  (`toolregistry.cpp:108-127`), `ToolRegistry::invoke()` redigiert das
  Ergebnis automatisch (`toolregistry.cpp:239-240`) — hier ist nichts zu
  ändern.
- Harbour-Manifest/Permissions prüfen: `harbour-nemoai.desktop`/`rpm/harbour-nemoai.spec`
  ggf. um die `Contacts`-Permission ergänzen, falls Sailjail den Zugriff sonst
  blockt (analog zum bestehenden `Requires: sailfishsecretsdaemon` für den
  Secrets-Zugriff).

### 3. `run_command`-Timeout bei interaktiven Programmen (`top`) [nur Full-Access]
Betroffen: `FullProvider::runCommand()` in `fullprovider.cpp:192-215` und die
Tool-Beschreibung in `toolregistry.cpp:172-198`.

Das ist kein Bug im engeren Sinn: `QProcess::start()` stellt bewusst kein PTY
bereit (siehe Kommentar „Keine Shell dazwischen"), und `busybox top` auf dem
Gerät läuft ohne `-n COUNT` in einer Endlosschleife mit periodischem Redraw —
das Verhalten ist erwartbar und der 15s-Timeout fängt es sauber ab (kein Hang,
sauberer `timedOut: true`-Response). Fix ist daher eine gezielte, kleine
Anpassung statt eines Verhaltens-Fixes:

- Tool-Beschreibung in `toolregistry.cpp:172-176` präzisieren, z. B.: „Nur für
  nicht-interaktive Einzelaufrufe geeignet; es wird kein Terminal
  bereitgestellt. Interaktive/laufend aktualisierende Programme (z. B. `top`
  ohne `-n`) laufen in einen Timeout." Das gibt dem Modell die Information,
  um z. B. automatisch `top -n 1` statt `top` zu wählen, ohne Code-Änderung
  an der Ausführung selbst.
- Keine Allow-/Blocklist einführen — das widerspräche explizit dem
  bestehenden Sicherheitskommentar im Code („Schutz ist ausschliesslich
  ConsentGate::Critical + Default-aus").

## Kritische Dateien
- `src/platform/full/fullprovider.cpp` — Kalender-Logging, `findContact`-Impl
- `src/platform/sandboxed/sandboxedprovider.cpp` — `findContact`-Impl
- `src/core/toolregistry.cpp` — Tool-Beschreibung `run_command` anpassen
- `sailfish-ai-companion.pro` — `QT += contacts`
- `rpm/harbour-nemoai.spec` / `harbour-nemoai.desktop` — ggf. Contacts-Permission

## Verifikation
- `nix-shell --run 'scripts/run-tests.sh'` für die plattformunabhängigen
  `src/core/`-Tests (unverändert von diesen Fixes betroffen, aber als
  Regressionscheck sinnvoll).
- Sauberer Full-Access-Build (`sfdk build -- --define "fullaccess 1"`) auf
  dem Testgerät, `get_upcoming_events` erneut über den Chat auslösen, Log
  auf den konkreten mKCal-Fehler prüfen.
- Sauberer Harbour-Build, `find_contact` mit einem bekannten Testkontakt
  aufrufen, Konsens-Dialog + redigierte Ausgabe im Chat verifizieren.
- `run_command` mit `top -n 1` und mit `top` (ohne Args) gegentesten: Ersteres
  liefert ein Ergebnis, Letzteres sauber `timedOut: true` — beides jetzt
  durch die Tool-Beschreibung für das Modell nachvollziehbar erwartbar.
