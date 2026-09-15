# Offene To-dos pro Target — Schnittstellen-Audit (Harbour vs. Full)

> Ausgangspunkt: Frage „sind alle Harbour-möglichen Schnittstellen bedacht?“
> Ergebnis: nein — und zwei Punkte sind heute nicht nur ungenutzt, sondern
> regelwidrig bzw. falsch diagnostiziert. Dieses Dokument ist die
> To-do-Liste daraus, getrennt nach Target, weil sich beide Targets
> unterschiedlich verhalten und unterschiedlich viel dürfen.

## Prüfgrundlage (Stand 2026-09-15)

Nicht die Doku-Seite, sondern die Configs, die `sfdk check` wirklich liest:

- `sailfishos/sdk-harbour-rpmvalidator`: `allowed_libraries.conf`,
  `allowed_permissions.conf`, `allowed_qmlimports.conf`,
  `allowed_requires.conf`, `allowed_sailjailkeys.conf`,
  `disallowed_orgnames.conf`, `disallowed_qmlimport_patterns.conf`,
  `rpmvalidation.sh` (Prüflogik: `isLibraryAllowed()`, `validatelibraries()`,
  `validateicon()`)
- `sailfishos/sailjail-permissions`: `config/50-default-profile.conf`,
  `permissions/*.permission`
- `sailfishos/sailjail`: `daemon/permissions.h` (`PERMISSION_PRIVILEGED`),
  `daemon/appinfo.c` (Beispiel-Permission-Liste)

## Blocker im Harbour-Target (heute, nicht optional)

| # | Befund | Beleg | Betroffene Dateien |
|---|---|---|---|
| H1 | `libQt5Contacts.so.5` ist **nicht** in `allowed_libraries.conf` → `Cannot link to shared library` (ERROR). Der Harbour-Build linkt es trotzdem, weil `QT += contacts` **außerhalb** des `fullaccess`-Blocks steht und `sandboxedprovider.cpp` `QContactManager` benutzt | `allowed_libraries.conf` enthält kein `libQt5Contacts`; `rpmvalidation.sh:631-648` | `sailfish-ai-companion.pro`, `rpm/harbour-nemoai.spec`, `src/platform/sandboxed/sandboxedprovider.cpp` |
| H2 | Kontakte sind in Harbour nur über die **QML**-Schiene erlaubt: `Sailfish.Contacts 1.0` bzw. `org.nemomobile.contacts 1.0` (+ `Requires: qml(Sailfish.Contacts)` / `qml(org.nemomobile.contacts)`) — C++/QtContacts ist es nicht | `allowed_qmlimports.conf:29,116`, `allowed_requires.conf:45,103` | `src/core/capabilities.cpp`, `qml/`, `.pro` |
| H3 | Permission `Bluetooth` deklariert, aber `bluetoothDevices()` ist Stub und **kein Tool** registriert → ungenutzte Permission, QA fragt danach | `allowed_permissions.conf` (Bluetooth erlaubt, aber Begründungspflicht) | `harbour-nemoai.desktop`, `src/core/toolregistry.cpp` |
| H4 | `Requires: nemo-qml-plugin-notifications-qt5` ohne eine einzige Notification im Code | `allowed_requires.conf:90` | `rpm/harbour-nemoai.spec` |
| H5 | Secrets-Requires unvollständig: es fehlen `sailfishsecretsdaemon-cryptoplugins-default` und `sailfishsecretsdaemon-secretsplugins-default` — genau der Fall, der als „no such plugin exists“ auftrat | `allowed_requires.conf:56-58` | `rpm/harbour-nemoai.spec` |

`sfdk check` konnte in dieser Umgebung nicht laufen — H1 ist aus dem
Allowlist-Inhalt plus der Prüflogik abgeleitet, nicht aus einem echten
Validatorlauf. Vor dem nächsten Store-Upload gegen einen Tag-Build nachziehen.

## To-dos — Target `harbour-nemoai` (Jolla Store)

- [ ] **H1a** `QT += contacts` aus der gemeinsamen Zeile in den
      `fullaccess`-Block verschieben; `BuildRequires: pkgconfig(Qt5Contacts)`
      aus `rpm/harbour-nemoai.spec` entfernen.
- [ ] **H1b** `QContactManager`/`QContactFetchRequest`/`QContactDisplayLabel`/
      `QContactPhoneNumber`/`QContactAddress` aus
      `src/platform/sandboxed/sandboxedprovider.cpp` entfernen.
- [ ] **H2** `find_contact` im Harbour-Target über QML neu aufsetzen
      (`Sailfish.Contacts 1.0` oder `org.nemomobile.contacts 1.0`,
      read-only). Bis die QML→`ToolRegistry`-Brücke steht:
      `Capabilities::contacts()` im `SFAI_HARBOUR`-Zweig auf `false`, damit
      das Tool gar nicht erst im Manifest steht (statt es wie in 0.9.2 nur
      „unavailable“ auszugrauen).
- [ ] **H3** `Bluetooth` aus `Permissions=` streichen **oder** das Tool
      implementieren (`Sailfish.Bluetooth 1.0` bzw. `org.kde.bluezqt 1.0`
      sind beide erlaubt) und die Permission begründen.
- [ ] **H4** `Requires: nemo-qml-plugin-notifications-qt5` streichen **oder**
      „Antwort fertig“-Notification mit `Nemo.Notifications 1.0` (erlaubt)
      tatsächlich umsetzen.
- [ ] **H5** Secrets-Requires ergänzen:
      `Requires: sailfishsecretsdaemon-cryptoplugins-default`,
      `Requires: sailfishsecretsdaemon-secretsplugins-default`.
- [ ] **H7** Contacts-Laufzeitproblem neu diagnostizieren, siehe
      „Falsche Schlüsse“ unten — die 0.9.2-Begründung ist widerlegt.
- [ ] **H8** Backlog erlaubte, ungenutzte Schnittstellen (Abschnitt weiter
      unten) — jede einzeln entscheiden, nicht sammeln.

## To-dos — Target `sailfishai` (OpenRepos, Vollzugriff)

- [ ] **F1** `[X-Sailjail]` in `sailfishai.desktop` ergänzen. Ohne die Sektion
      bekommt die App das **Default-Profil**
      (`config/50-default-profile.conf`:
      `Audio;Bluetooth;Camera;Compatibility;Internet;Location;MediaIndexing;Microphone;NFC;RemovableMedia;UserDirs;WebView`)
      — darin fehlen `Secrets`, `Contacts`, `Calendar`,
      `CommunicationHistory`, `Accounts`. Genau deshalb lief `sailfishai` auf
      SFOS 5.2 sandboxed (Befund aus 0.9.2, hier nur die Ursache).
      Für OpenRepos sind auch die **Nicht**-Harbour-Permissions nutzbar, weil
      der Harbour-Validator hier nicht greift: `Contacts`, `Calendar`,
      `CommunicationHistory`, `Messages`, `Phone`, `Notifications`,
      `Sharing`, `Accounts` sowie die Pseudo-Permission `Privileged`
      (`daemon/permissions.h: PERMISSION_PRIVILEGED`).
- [ ] **F2** Konkretes Ziel für die drei Vollzugriff-Tools:
      `Contacts` (find_contact), `Calendar` (get_upcoming_events, mkcal liest
      `${PRIVILEGED}/Calendar`), `CommunicationHistory` (+ `Messages`, wenn
      Telepathie/ofono gebraucht wird) für `read_recent_messages`,
      `Internet` + `Secrets` unverändert.
- [ ] **F3** `query_failed` beim Kalender gegen F2 gegenprüfen: ohne
      `Calendar`-Permission ist der privilegierte Kalender-Pfad im Sandbox
      nicht sichtbar — das erklärt den M4-Befund schlüssiger als ein
      „Lock-Konflikt“. Erst F2 umsetzen, dann mit dem in
      `docs/m4-follow-up-tools.md` geplanten mKCal-Logging verifizieren.
- [ ] **F4** `X-Nemo-Application-Type=silica-qt5`, `OrganizationName`,
      `ApplicationName` in beiden Desktop-Dateien müssen zu
      `QStandardPaths`/`QSettings`-Pfaden passen — beim Nachziehen von F1
      mitprüfen.
- [ ] **F5** Full-Target braucht die Harbour-Allowlist nicht, aber
      `sfdk check` läuft nur gegen das Harbour-Spec — Full-Builds weiterhin
      nur per `pkcon install` + Gerätetest prüfen.

## Backlog — erlaubt, aber ungenutzt (H8)

Alles hier ist in der Harbour-Allowlist (Import bzw. Lib bzw. Require) und
deshalb **kein** Full-Access-Thema. Reihenfolge nach Nutzen für diese App:

| Schnittstelle | Erlaubt als | Nutzen |
|---|---|---|
| `Nemo.KeepAlive 1.2`, `libkeepalive.so.1` | QML + Lib + `qml(Nemo.KeepAlive)` | lange Streaming-Antwort zu Ende bringen, Display aus (`Capabilities::background()` ist heute `false`) |
| `Nemo.Notifications 1.0` | QML + `libnemonotifications-qt5.so.1` | „Antwort fertig“, wenn die App im Hintergrund war (rechtfertigt H4) |
| `Amber.Web.Authorization 1.0` | QML + `libamberwebauthorization.so.1` | OAuth-Autorisierungscode statt API-Key-Eintippen — grösster Einzelgewinn |
| `Sailfish.Pickers 1.0` + Permissions `Documents`/`Downloads`/`Pictures`/`UserDirs`/`PublicDir` | QML + Permissions | Anhänge: Bild → multimodales Modell, Datei → Kontext (`Capabilities::filesystem()` ist `true`, aber leer) |
| `QtMultimedia 5.x` + `Sailfish.Media 1.0` + Permissions `Audio`/`Microphone`/`Camera` | QML + `qt5-qtmultimedia` | M6 Sprachein-/-ausgabe, Foto → Vision-Modell |
| `Sailfish.WebView 1.0` + Permission `WebView` | QML + `sailfish-components-webview-qt5` | Markdown/HTML-Antworten rendern, OAuth-Seite anzeigen |
| `Sailfish.Share 1.0` | QML | Antwort in andere Apps weitergeben |
| `Sailfish.Accounts 1.0`, `libsailfishaccounts.so.0` + Permission `Accounts` | QML + Lib + Permission | Provider-Zugang als Systemkonto statt app-lokal |
| `Sailfish.Crypto 1.0` | QML + `libsailfishcrypto.so.0` | Verlauf/Keys zusätzlich verschlüsseln (Cloud-Kontext) |
| `QtPositioning 5.2/5.4`, `libQt5Positioning.so.5` + Permission `Location` | QML + Lib + Permission | Standort-/Wettertool |
| `QtWebSockets 1.1` | QML + Lib | lokaler Modellserver/Streaming für M5 |
| `Amber.Mpris 1.0` | QML + `amber-qml-plugin-mpris` | Medienspieler steuern |
| `Nemo.DBus 2.0`, `org.nemomobile.contacts 1.0`, `org.freedesktop.contextkit 1.0` | QML | Systemdienste ohne C++ erreichen; Akku/Netz offiziell statt sysfs (C++-ContextKit gibt es nicht im Sysroot) |
| `Sailfish.Telephony 1.0` + `qml(Sailfish.Telephony)` | QML + Require | `Capabilities::telephony()` ist `true`, aber kein Tool nutzt es |
| `QtFeedback 5.0` | QML (nur `ThemeEffect.play()`, PressWeak/Press/PressStrong) | dezente Haptik bei Senden/Consent |
| `libmlite5.so.0`, `libcurl.so.4`, `libcrypto.so.3`/`libssl.so.3`, `libmlite` | Libs | Helfer, alternativer HTTP-Client, Signieren/Pinning für Cloud-STT/TTS |
| `io.thp.pyotherside 1.0-1.6` | QML-Import | Python im Sandbox (M5-Glue) |

## In Harbour grundsätzlich nicht möglich

Damit diese Punkte nicht wieder als Harbour-Aufgabe auftauchen:

- **SMS/Nachrichten**: es gibt keine Permission `Messages`/
  `CommunicationHistory` in `allowed_permissions.conf`.
- **Kalender**: keine `Calendar`-Permission; `mkcal-qt5`/`KF5CalendarCore`
  stehen in keiner Allowlist.
- **Privilegierte Daten allgemein**: die Pseudo-Permission `Privileged` ist
  Harbour-fremd, auch wenn sailjail sie kennt.
- **Notifications lesen**: `Nemo.Notifications` ist Senden, nicht Lesen.
- **Lokale Inferenz über System-Libs**: `libgomp`/OpenBLAS stehen nicht in
  `allowed_libraries.conf`. Eigene `.so` **ist** erlaubt, wenn sie unter
  `%{_datadir}/<app>/lib/` liegt (`rpmvalidation.sh:720-723`) und der rpath
  stimmt (`RPATH_CHECK_NEEDED`) — llama.cpp/ggml also als private Libs, aber
  ohne System-Threading-Libs.
- **Prozess-Spawn** (`QProcess`): im Validator nicht verboten, aber
  Harbour-QA-relevant — bleibt bewusst Full-Target.

## Falsche Schlüsse aus 0.9.x (zu korrigieren)

1. **„QtContacts ist die dokumentierte Harbour-konforme C++-API“** (so in
   `CLAUDE.md` und `docs/m4-follow-up-tools.md` §2). Falsch: die Allowlist
   kennt nur `Sailfish.Contacts 1.0` / `org.nemomobile.contacts 1.0` als
   QML-Import. Deshalb H1/H2.
2. **„`Error: can't chdir to privileged` beweist einen defekten
   Contacts-Mount“** (0.9.2). Die Zeile erscheint beim Start **jeder**
   sandboxed App im Journal — belegt für `sailfish-browser`,
   `jolla-gallery` und `harbour-pure-maps` (Forum-Threads) — zusammen mit
   `constructing /run/firejail/mnt/privileged: …`, `mounting …`, `hiding …`.
   Sie ist Rauschen, kein app-spezifischer Befund.
3. **Die „0 Kontakte“-Diagnose wurde unsandboxed gefahren** (explizit „per
   SSH statt über `invoker`/Sailjail“). Das ist nicht derselbe Zugriffspfad:
   `~/.local/share/system/privileged/Contacts` gehört `privileged:privileged`,
   Modus `0770` — eine SSH-Session ist keine Mitglied dieser Gruppe und sieht
   deshalb nur den leeren Stub. Der Beweis wäre nur über einen Trace der
   App **innerhalb** des Sandbox zu führen:
   `sailjail --trace -p harbour-nemoai.desktop /usr/bin/harbour-nemoai`
   (Optionen siehe `sailjail/APPDEBUG.md`) plus `pgrep -a firejail` für die
   tatsächlich generierte Kommandozeile.
4. **Konsequenz für 0.9.2:** `find_contact` wurde auf Basis von 2. und 3.
   ausgegraut. Das bleibt ein gültiger Notausgang, aber die Begründung im
   Code-Kommentar und in `toolregistry.cpp` darf so nicht stehen bleiben —
   entweder durch den Trace ersetzen oder den Grund auf „Harbour-C++-API
   nicht erlaubt, QML-Umsetzung offen (H2)“ umschreiben.

## Doku-Korrekturen (Teil dieses To-dos)

- [ ] `CLAUDE.md`, Abschnitt „Harbour-Regeln“: `org.nemomobile.contacts 1.0`
      als **QML**-Import kennzeichnen, QtContacts explizit als Harbour-fremd
      aufführen; `Sailfish.KeepAlive 1.2`/`Nemo.KeepAlive`-Lib,
      `Nemo.Notifications` und die erlaubten Permissions vollständig listen.
- [ ] `CLAUDE.md`: festhalten, dass `sailfishai` **nicht** unsandboxed läuft,
      solange `[X-Sailjail]` fehlt (Default-Profil) — Architekturentscheidung
      3 („Full-Access = unsandboxed“) gilt so nicht mehr.
- [ ] `docs/m4-follow-up-tools.md` §2: QtContacts-Begründung ersetzen und auf
      H1/H2 verweisen.
- [ ] `README.md` (Status, EN + DE): Verweis auf dieses Dokument.

## Verifikation

- Harbour: `sfdk -c target=SailfishOS-5.0.0.62-aarch64 -c specfile=rpm/harbour-nemoai.spec build`
  aus einem Worktree exakt auf dem Release-Tag, dann `sfdk check` —
  Erwartung nach H1: keine `Cannot link to shared library`-Zeile mehr.
  Gegenprobe vorher: derselbe Lauf muss sie aktuell zeigen.
- Eigenkontrolle ohne SDK: `objdump -x <binary> | grep NEEDED` auf
  `libQt5Contacts` prüfen; die Allowlist als Soll-Liste danebenlegen.
- Full: `[X-Sailjail]`-Zeile gegen `pgrep -a firejail` auf dem Gerät
  verifizieren (die generierte Kommandozeile muss die Permissions enthalten),
  danach die drei Tools erneut auslösen.
- `./scripts/run-tests.sh` nach jeder Code-Änderung (H1b/H2) — die
  `src/core/`-Suite deckt `Capabilities`/`ToolRegistry` ab.

Copy-paste-Kette am Build-Rechner (Target an das installierte SDK anpassen):

```
SDKTARGET=SailfishOS-5.0.0.62-aarch64
sfdk -c target=$SDKTARGET -c specfile=rpm/harbour-nemoai.spec build
sfdk -c target=$SDKTARGET -c specfile=rpm/harbour-nemoai.spec check
# Soll nach H1: keine Zeile "Cannot link to shared library: libQt5Contacts.so.5"
rpm -qp --requires RPMS/*/harbour-nemoai-*.aarch64.rpm | grep -i contacts   # leer
objdump -x harbour-nemoai | grep NEEDED | grep -i contacts                  # leer
./scripts/run-tests.sh                                                      # Desktop-Core
rpm -qp --requires RPMS/*/sailfishai-*.aarch64.rpm | grep -E 'commhistory|mkcal'
```

Auf dem Gerät (Full-Target nach F1):

```
pgrep -a firejail | head        # generierte Kommandozeile muss die Permissions zeigen
journalctl -f -t invoker        # Startzeile; "can't chdir to privileged" ist Rauschen
```

`sfdk check` gilt nur für das Harbour-Spec — der Full-Build wird am Gerät
geprüft (`pkcon install`, dann die drei Tools im Chat auslösen).

## Reihenfolge

1. H1 + H2 (Harbour wieder regelkonform; ohne das ist kein Store-Upload
   möglich, und `find_contact` bleibt in beiden Targets kaputt).
2. F1 + F2 + F3 (Full-Target: Sandbox explizit konfigurieren, dann die drei
   Tool-Lücken aus M4 schliessen).
3. H3, H4, H5 (Deklarationen aufräumen).
4. H7 (Diagnose richtigstellen) und die Doku-Korrekturen.
5. Erst danach H8-Backlog priorisieren.

## Anhang — exakte Stellen für die Umsetzung am Build-Rechner

Zeilenangaben gegen Commit `6368d7b` (`git show 6368d7b` als Referenz).

### H1a — QtContacts aus dem Harbour-Build

`sailfish-ai-companion.pro`
- Ist, Z. 23: `QT     += network sql dbus contacts`
- Soll: `QT     += network sql dbus` (Z. 23), dafür im `fullaccess`-Block
  (Z. 56-60, neben `PKGCONFIG += commhistory-qt5 libmkcal-qt5
  KF5CalendarCore`) ein `QT += contacts` ergänzen.

`rpm/harbour-nemoai.spec`
- Ist, Z. 20: `BuildRequires: pkgconfig(Qt5Contacts)` → löschen.
- Im `rpm/sailfishai.spec` bleibt `pkgconfig(Qt5Contacts)` stehen (dort erlaubt).

`tests/tests.pro` bleibt unberührt: die Desktop-Suite linkt kein QtContacts
(Z. 12: `QT += testlib network sql`), `QT -= gui`.

### H1b — QtContacts aus dem Sandboxed-Provider

`src/platform/sandboxed/sandboxedprovider.cpp`
- Ist: Z. 7-11 die fünf `QContact*`-Includes, Z. 13 `QTCONTACTS_USE_NAMESPACE`,
  Z. 129-175 `findContact()`.
- Soll: Includes und `QTCONTACTS_USE_NAMESPACE` raus; `findContact()` wird ein
  Stub. `ISystemProvider::findContact` ist **pure virtual**
  (`src/platform/isystemprovider.h`, Z. 26), der Stub muss also bleiben —
  z. B. `return unsupported();` wie bei `recentMessages()`.
- Mitziehen: Kommentarblock in `sandboxedprovider.h` (Z. 15-22 nennt
  „Sailfish.Contacts“ als erlaubten Kanal) und der Kommentar in
  `isystemprovider.h` (Z. 10-11) auf die tatsächliche Lage bringen.

### H2 — Kontakte im Harbour-Target

Kurzfristig (macht den Harbour-Build manifest-seitig ehrlich):
- `src/core/capabilities.cpp`, Z. 7 (im `#ifdef SFAI_HARBOUR`-Zweig):
  `contacts()` → `false`, mit Verweis auf diesen Abschnitt.
- Folge: `ToolRegistry::buildManifest()` registriert `find_contact` im Harbour
  gar nicht mehr. Der `available`-Ausgraumechanismus aus 0.9.2 bleibt für den
  Full-Fall bestehen.

Achtung Testlage: `tests/tests.pro` definiert `SFAI_HARBOUR` **nicht**, die
Desktop-Suite kompiliert also den `#else`-Zweig (`contacts() == true`).
`tests/tst_toolregistry.cpp` Z. 60 sowie `unavailableToolCannotBeEnabled`
(Z. 145-165) bleiben deshalb unverändert gültig. Wer das Harbour-Manifest
testen will, braucht ein zweites Testziel mit `DEFINES += SFAI_HARBOUR` —
existiert heute nicht.

Eigentlicher Fix (Entwurf, ohne Gerät nicht verifizierbar):
- Erlaubter Weg ist QML: `import Sailfish.Contacts 1.0` bzw.
  `import org.nemomobile.contacts 1.0`, dazu
  `Requires: qml(Sailfish.Contacts)` bzw. `qml(org.nemomobile.contacts)`
  (`allowed_requires.conf` Z. 45/103) im Harbour-Spec.
- Brücke nach C++: `ToolRegistry` bekommt eine setzbare Handler-Schnittstelle
  (`Q_INVOKABLE QVariantMap invoke(const QString &tool, const QVariantMap &args)`),
  die in `main.cpp` auf ein QML-Objekt gesetzt wird; `find_contact` läuft dann
  in QML und liefert das Ergebnis synchron zurück. Alternativ den Tool-Aufruf
  umdrehen und nur das Schema aus C++ liefern.
- Offen zu klären, ob der privilegierte Contacts-Store innerhalb der Sandbox
  erreichbar ist (siehe H7) — erst messen, dann bauen.

### H3 — `Bluetooth` in der Harbour-Desktop-Datei

`harbour-nemoai.desktop`, Z. 12
- Ist: `Permissions=Internet;Secrets;Contacts;Bluetooth`
- Soll (Bluetooth-Tool kommt nicht): `Permissions=Internet;Secrets`
- Falls `find_contact` bis dahin entfernt ist (H2), entfällt auch `Contacts`.
- Falls das Tool doch kommt: `org.kde.bluezqt 1.0` oder
  `Sailfish.Bluetooth 1.0` implementieren und `Bluetooth` begründet behalten.

### H4, H5 — Harbour-Requires

`rpm/harbour-nemoai.spec`
- Z. 11 `Requires:   nemo-qml-plugin-notifications-qt5` → entweder löschen
  (kein Aufruf im Code) oder `Requires: qml(Nemo.Notifications)` ergänzen und
  die Notification tatsächlich bauen (H8-Backlog, Zeile 2).
- Nach Z. 10 (`Requires: sailfishsecretsdaemon`) ergänzen:
  `Requires:   sailfishsecretsdaemon-cryptoplugins-default`,
  `Requires:   sailfishsecretsdaemon-secretsplugins-default`
  (`allowed_requires.conf` Z. 56-58).

### F1 — `[X-Sailjail]` für `sailfishai.desktop`

- Ist: nur ein Kommentar, keine Sektion.
- Soll (Vorschlag):
  ```
  [X-Sailjail]
  OrganizationName=ch.silly
  ApplicationName=sailfishai
  Permissions=Internet;Secrets;Contacts;Calendar;CommunicationHistory;Privileged
  ```
- `OrganizationName` muss zu `app->setOrganizationName(QStringLiteral("ch.silly"))`
  in `src/main.cpp` passen, `ApplicationName` zum Binary-/Settings-Pfad
  (`app->setApplicationName(SFAI_TARGET_NAME)`), sonst verschieben sich
  Datenverzeichnisse (`ConversationStore::open()`) und `QSettings`-Dateien.
  Vor dem Umstellen auf dem Gerät prüfen, wo `~/.local/share/ch.silly/sailfishai/`
  und die `history.db` aktuell liegen.
- Beispiel einer grossen solchen Liste liefert sailjail selbst
  (`daemon/appinfo.c`, Z. 216: `Permissions=Phone;CallRecordings;Contacts;Bluetooth;Privileged;Sharing`).
- `Privileged` nur setzen, wenn wirklich nötig — es ist die Pseudo-Permission,
  die `${PRIVILEGED}` öffnet, und in Harbour verboten.


