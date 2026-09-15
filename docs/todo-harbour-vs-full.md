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

- [x] **H1a** `QT += contacts` aus der gemeinsamen Zeile in den
      `fullaccess`-Block verschieben; `BuildRequires: pkgconfig(Qt5Contacts)`
      aus `rpm/harbour-nemoai.spec` entfernen.
- [x] **H1b** `QContactManager`/`QContactFetchRequest`/`QContactDisplayLabel`/
      `QContactPhoneNumber`/`QContactAddress` aus
      `src/platform/sandboxed/sandboxedprovider.cpp` entfernen.
- [x] **H2 (kurzfristig)** `Capabilities::contacts()` im `SFAI_HARBOUR`-Zweig
      auf `false`, damit `find_contact` im Harbour-Manifest gar nicht mehr
      auftaucht. Der eigentliche Fix (Kontakte im Harbour-Target über QML neu
      aufsetzen, `Sailfish.Contacts 1.0`/`org.nemomobile.contacts 1.0` +
      `ToolRegistry`-Brücke) bleibt offen — ohne SDK/Gerät hier nicht
      verifizierbar, siehe Entwurf weiter unten unter „H2 — Kontakte im
      Harbour-Target“.
- [x] **H3** `Bluetooth` (und `Contacts`, da mit H2 kein Codepfad mehr) aus
      `Permissions=` in `harbour-nemoai.desktop` gestrichen — kein Tool nutzt
      es, siehe Kommentar in `sandboxedprovider.cpp`.
- [x] **H4** `Requires: nemo-qml-plugin-notifications-qt5` aus
      `rpm/harbour-nemoai.spec` gestrichen (kein Aufruf im Code). Die
      Notification-Umsetzung bleibt H8-Backlog.
- [x] **H5** Secrets-Requires ergänzt:
      `Requires: sailfishsecretsdaemon-cryptoplugins-default`,
      `Requires: sailfishsecretsdaemon-secretsplugins-default`.
- [x] **H7 (Root Cause gefunden)** `sailjail --trace`/`-d` auf echter
      Hardware (Jolla Phone 2026, SFOS 5.2.0.17) durchgeführt, siehe
      „Falsche Schlüsse“ Punkt 5 unten. Ursache: der `booster-silica-qt5`-
      Daemon cacht seinen Sandbox-Mount-Namespace vom Zeitpunkt seines
      letzten Starts — ein reines RPM-Upgrade, das `[X-Sailjail]`-
      Permissions ändert, reicht nicht, der Booster muss dafür neu
      gestartet werden (`systemctl --user restart
      booster-silica-qt5.service`), sonst bleiben `privileged-data`-Mounts
      leer, obwohl `pgrep -a firejail` bereits die korrekten
      `--profile=...`-Einträge zeigt. Kein App-Bug, kein Sailjail-Bug —
      ein Deployment-/Testworkflow-Fallstrick, jetzt in `CLAUDE.md`
      dokumentiert.
- [x] **H8 (Triage + die drei Quick-Wins umgesetzt)** Alle 17 Punkte einzeln
      entschieden (Abschnitt weiter unten, Stand 2026-09-15). Die drei
      "jetzt"-Punkte (`Nemo.KeepAlive`, `Sailfish.Share`,
      `Capabilities::telephony()` auf `false`) sind umgesetzt und auf
      echter Hardware verifiziert (Jolla Phone 2026). Ein Punkt bleibt
      ein Punkt zurückgestellt (`Amber.Web.Authorization`/OAuth, vorerst
      nicht weiterverfolgt), Rest entweder an M5/M6 gebunden oder vorerst
      verworfen.
      **Korrektur zur ursprünglichen Recherche:** Die frühere Tabelle
      notierte `qml(Nemo.KeepAlive)` als nötige `Requires:`-Zeile — das
      ist falsch. Die echte `allowed_requires.conf`
      (`/srv/mer/targets/.../usr/share/sdk-harbour-rpmvalidator/`, per
      `sfdk engine exec grep` gefunden) kennt für KeepAlive nur
      `libkeepalive`/`libkeepalive-glib` (Paketname, keine `qml()`-Form),
      `Sailfish.Share` taucht dort gar nicht auf. `sfdk check` läuft für
      beide QML-Imports **ohne jede explizite Requires-Zeile** sauber
      durch — offenbar über die Basis-Silica-Abhängigkeit ohnehin
      garantiert vorhanden.

## To-dos — Target `sailfishai` (OpenRepos, Vollzugriff)

- [x] **F1** `[X-Sailjail]` in `sailfishai.desktop` ergänzt
      (`Permissions=Internet;Secrets;Contacts;Calendar;CommunicationHistory;Privileged`,
      `OrganizationName=ch.silly`/`ApplicationName=sailfishai` geprüft gegen
      `src/main.cpp`). Im SDK-Emulator (5.1.0.11-i486) verifiziert:
      installiert, gestartet, zeigt jetzt beim ersten Start den erwarteten
      Sailjail-Berechtigungsdialog (Calendar, Communication history,
      Contacts, Internet, Secrets), `pgrep -a firejail` listet die
      passenden `--profile=...permission`-Einträge (s. F3). Zusätzlich auf
      echter Hardware bestätigt (Jolla Phone 2026, SFOS 5.2.0.17): gleicher
      `pgrep -a firejail`-Befund, Berechtigungsdialog erschien beim ersten
      Start. Ohne die Sektion
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
- [x] **F2** Ziel wie beschrieben durch F1 umgesetzt: `sailfishai.desktop`
      deklariert `Permissions=Internet;Secrets;Contacts;Calendar;
      CommunicationHistory;Privileged` — deckt `find_contact` (`Contacts`),
      `get_upcoming_events` (`Calendar`) und `read_recent_messages`
      (`CommunicationHistory`) genau wie hier gefordert ab. `Messages`
      wurde nicht ergänzt, da `read_recent_messages` laut
      `fullprovider.cpp` über `libcommhistory-qt5`, nicht Telepathie/ofono
      geht.
- [x] **F3 (vollständig bestätigt)** Zwei Belege im SDK-Emulator
      (5.1.0.11-i486):
      1. `sudo cat /proc/<pid>/mounts` für den laufenden, sandboxed
         `sailfishai`-Prozess zeigt
         `/home/defaultuser/.local/share/system/privileged/Contacts` **und**
         `.../privileged/Calendar` als `rw`-Bind-Mounts innerhalb der
         Sandbox — vorher (ohne `[X-Sailjail]`, siehe F1) waren diese Pfade
         dort nicht sichtbar. Die generierte `firejail`-Kommandozeile
         (`pgrep -a firejail`) listet entsprechend
         `--profile=.../Contacts.permission --profile=.../Calendar.permission
         --profile=.../CommunicationHistory.permission`.
      2. Echter End-to-End-Test mit einem echten API-Key gegen
         `deepseek/deepseek-v4.1-flash` (OpenRouter): Frage nach
         Kalenderterminen → Modell ruft `get_upcoming_events` mit `days: 7`
         auf → ConsentGate-Dialog → nach Bestätigung liefert der Tool-Call
         `{"events":[]}` statt des vorherigen `query_failed` — die
         (leere) Emulator-Kalenderdatenbank öffnet und lädt jetzt
         fehlerfrei.

      Beides zusammen bestätigt die F3-Hypothese: der fehlende
      `Calendar`-Permission-Mount, nicht ein „Lock-Konflikt“, war die
      tatsächliche Ursache für `query_failed`. Auf echter Hardware (Jolla
      Phone 2026, SFOS 5.2.0.17) lief `get_upcoming_events` ebenfalls ohne
      `query_failed` — dort allerdings mit echten Kalenderdaten, was zwei
      eigene Bugs freigelegt hat (siehe „Nachtrag“ unten): eine
      Redaktions-Falscherkennung auf ISO-Datumsstrings und eine fehlende
      Rekurrenz-Expansion für wiederkehrende Termine. Beide behoben.
- [x] **F4** Geprüft: beide Desktop-Dateien deklarieren
      `X-Nemo-Application-Type=silica-qt5`; `OrganizationName=ch.silly`/
      `ApplicationName={harbour-nemoai,sailfishai}` stimmen exakt mit
      `app->setOrganizationName("ch.silly")`/
      `app->setApplicationName(SFAI_TARGET_NAME)` in `src/main.cpp`
      überein. Auf echter Hardware (Jolla Phone 2026) zusätzlich empirisch
      bestätigt: `~/.config/ch.silly/sailfishai/settings.ini` und
      `~/.local/share/ch.silly/sailfishai/history.db` liegen genau dort,
      wo diese Namen es vorhersagen.
- [ ] **F5** Full-Target braucht die Harbour-Allowlist nicht, aber
      `sfdk check` läuft nur gegen das Harbour-Spec — Full-Builds weiterhin
      nur per `pkcon install` + Gerätetest prüfen.

## Backlog — erlaubt, aber ungenutzt (H8, Triage 2026-09-15)

Alles hier ist in der Harbour-Allowlist (Import bzw. Lib bzw. Require) und
deshalb **kein** Full-Access-Thema. Jeder Punkt einzeln entschieden statt
gesammelt aufgeschoben:

| Schnittstelle | Nutzen | Entscheidung | Begründung |
|---|---|---|---|
| `Nemo.KeepAlive 1.2` | Streaming-Antwort zu Ende bringen, Display aus (`Capabilities::background()` ist `false`) | **[x] Umgesetzt** | `KeepAlive { enabled: AI.streaming }` in `ChatPage.qml`; `Capabilities::background()` im `SFAI_HARBOUR`-Zweig auf `true`. Live auf echter Hardware verifiziert: Streaming läuft durch, App bleibt stabil |
| `Sailfish.Share 1.0` | Antwort an andere App weitergeben | **[x] Umgesetzt** | `ShareAction` + `menu: ContextMenu { MenuItem { text: qsTr("Share") ... } }` in `MessageDelegate.qml` (long-press, nur bei fertigen Nicht-Tool-Nachrichten). Live verifiziert: Kontextmenü und nativer Share-Dialog funktionieren |
| `Sailfish.Telephony 1.0` | Ungenutzt — `Capabilities::telephony()` ist `true`, obwohl kein Tool es nutzt | **[x] Umgesetzt** | `Capabilities::telephony()` in beiden Zweigen auf `false` — kein konkretes Tool geplant, ein Flag ohne Wirkung ist irreführend (dieselbe Logik wie H2s „Manifest ehrlich machen“ für Contacts) |
| `Nemo.Notifications 1.0` | „Antwort fertig“, wenn App im Hintergrund war | **Zurückgestellt** | Setzt `Nemo.KeepAlive`/Hintergrundlogik voraus, die es noch nicht gibt — erst sinnvoll, wenn das oben steht |
| `Amber.Web.Authorization 1.0` | OAuth statt API-Key eintippen — grösster Einzelgewinn laut ursprünglicher Einschätzung, durch die heutige Key-Eingabe-Odyssee (Tastatur-Layout-Kalibrierung per `uinput`) nur bestätigt | **Zurückgestellt** | Kein Quick-Win — braucht Klärung, ob/wie OpenRouter einen OAuth-Code-Flow anbietet, bevor Code entsteht. Auf Wunsch erstmal nicht weiterverfolgt (2026-09-15) |
| `Sailfish.Pickers 1.0` | Anhänge: Bild → multimodales Modell | **[x] Umgesetzt (2026-09-15)** | `ImagePickerPage` in `ChatPage.qml`, Bildpfad in `ConversationStore` (neue `image_path`-Spalte), `history()` baut bei vorhandenem Bild ein OpenAI/OpenRouter-Multimodal-`content`-Array (Text + `image_url` als Data-URI, `imageDataUri()` in `conversationstore.cpp`) statt eines reinen Strings — kein Eingriff in `openrouterbackend.cpp` nötig, da `chat()` ohnehin nur ein opakes `QJsonArray` entgegennimmt. Live auf echter Hardware verifiziert: Modell (`deepseek/deepseek-v4.1-flash`) erkannte und beschrieb ein angehängtes Kartenbild korrekt. Keine Vision-Fähigkeits-Filterung in der Modellliste — bewusst nicht gemacht, um den Umfang klein zu halten |
| `QtMultimedia 5.x` + `Sailfish.Media 1.0` | M6 Sprachein-/-ausgabe, Foto → Vision-Modell | **Zurückgestellt, Teil von M6** | Bereits als eigener Meilenstein in `README.md` getrackt |
| `QtWebSockets 1.1` | Lokaler Modellserver/Streaming | **Zurückgestellt, Teil von M5** | Bereits als eigener Meilenstein in `README.md` getrackt |
| `io.thp.pyotherside 1.0-1.6` | Python im Sandbox (M5-Glue) | **Zurückgestellt, Teil von M5** | Bereits als eigener Meilenstein in `README.md` getrackt |
| `QtFeedback 5.0` | Dezente Haptik bei Senden/Consent | **Zurückgestellt** | Reines Politur-Feature, kein blockierendes Problem |
| `QtPositioning 5.2/5.4` | Standort-/Wettertool | **Zurückgestellt** | Sinnvolle Tool-Idee, aber kein konkreter Bedarf/Anfrage bisher — neues Tool heisst neuer Consent-Flow, nicht nebenbei zu machen |
| `Sailfish.WebView 1.0` | Markdown/HTML-Antworten rendern, OAuth-Seite anzeigen | **Verworfen (vorerst)** | Aktuelle Markdown-Darstellung im Chat funktioniert bereits (s. Screenshots vom Live-Test); kein akuter Schmerzpunkt |
| `Sailfish.Accounts 1.0` | Provider-Zugang als Systemkonto statt app-lokal | **Verworfen (vorerst)** | Passt nicht zum aktuellen Ein-Key-Modell ohne echten OAuth-Flow — erst relevant, falls Amber.Web.Authorization kommt |
| `Sailfish.Crypto 1.0` | Verlauf/Keys zusätzlich verschlüsseln | **Verworfen** | Der API-Key ist bereits über `Sailfish.Secrets` verschlüsselt; kein Anlass, den Verlauf zusätzlich zu verschlüsseln |
| `Amber.Mpris 1.0` | Medienspieler steuern | **Verworfen** | Kein erkennbarer Bezug zu einem „AI Companion"-Chat — Tool-Fläche ohne klaren Bedarf |
| `Nemo.DBus 2.0`, `org.freedesktop.contextkit 1.0` | Akku/Netz offiziell statt sysfs | **Verworfen** | Der sysfs-Ansatz ist heute mehrfach auf echter Hardware verifiziert und funktioniert (s. `get_battery_status`/`get_network_status`-Tests) — kein Grund, ein laufendes System umzubauen |
| `libmlite5.so.0`, `libcurl.so.4`, `libcrypto.so.3`/`libssl.so.3` | Alternativer HTTP-Client, Signieren/Pinning | **Verworfen (vorerst)** | Kein konkreter Bedarf — `QNetworkAccessManager`/`QSsl` decken den heutigen OpenRouter-Zugriff ab; erst bei einem echten Anlass (z. B. Cert-Pinning für Cloud-STT/TTS in M6) neu bewerten |

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
5. **Der echte Grund für den leeren `/run/firejail/mnt/privileged/` bei den
   0.9.5-Erstversuchen war nicht Sailjail/Firejail selbst, sondern ein
   veralteter Booster-Prozess.** `sailjail -d -p sailfishai.desktop --
   /usr/bin/sailfishai` (direkt, ohne `invoker`) zeigte im Debug-Log
   (`Mounting ... constructing /run/firejail/mnt/privileged: Contacts,
   Calendar ...` gefolgt von zwei erfolgreichen `mounted at:`-Zeilen) einen
   **funktionierenden** Mount-Aufbau — der normale Start über `invoker
   --type=silica-qt5` (der über den langlebigen `booster-silica-qt5`-Daemon
   läuft) zeigte für denselben Prozess dagegen keinen Mount. Der Booster war
   in beiden Fällen bereits **vor** der Installation der `[X-Sailjail]`-
   Permissions gestartet worden; sein Sandbox-Mount-Namespace stammt vom
   Zeitpunkt seines eigenen Starts, nicht von der zuletzt installierten
   App-Version. Nach `systemctl --user restart booster-silica-qt5.service`
   zeigte derselbe `invoker`-Startpfad die Mounts korrekt. Für Tests nach
   jeder Änderung an `[X-Sailjail]`-Permissions: Booster neu starten (oder
   Gerät neu starten), ein reines `rpm -Uvh --force` genügt nicht.

## Doku-Korrekturen (Teil dieses To-dos)

- [x] `CLAUDE.md`, Abschnitt „Harbour-Regeln“: `org.nemomobile.contacts 1.0`
      als **QML**-Import kennzeichnen, QtContacts explizit als Harbour-fremd
      aufführen — war bereits so dokumentiert, jetzt auch im Code umgesetzt
      (H1/H2).
- [x] `CLAUDE.md`: festhalten, dass `sailfishai` **nicht** unsandboxed läuft,
      solange `[X-Sailjail]` fehlt — jetzt zusätzlich, dass die Sektion mit F1
      ergänzt wurde.
- [x] `docs/m4-follow-up-tools.md` §2: QtContacts-Begründung war bereits per
      Korrektur-Vermerk ersetzt (Stand vor diesem Commit); Code jetzt
      nachgezogen (H1/H2).
- [x] `README.md` (Status, EN + DE): Verweis auf dieses Dokument war bereits
      vorhanden.

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

## Nachtrag: F1 auf echter Hardware bestätigt, zwei neue Bugs gefunden (2026-09-15)

`sailfishai` 0.9.4 auf einem Jolla Phone 2026 (SFOS 5.2.0.17 „Finlayson", per USB/`devel-su rpm -Uvh`) installiert und getestet, nicht nur im Emulator:

- **F1 real bestätigt**: `pgrep -a firejail` zeigt auf dem echten Gerät dieselben
  `--profile=.../Contacts.permission --profile=.../Calendar.permission
  --profile=.../CommunicationHistory.permission`-Einträge wie im Emulator.
  Berechtigungsdialog erschien beim ersten Start wie erwartet.
- **H7-Update**: `sudo cat /proc/<pid>/mounts` für den laufenden Prozess zeigt
  auf diesem Gerät **kein** `.../privileged/Contacts`/`Calendar` — anders als
  im Emulator ist `/run/firejail/mnt/privileged/` hier komplett leer
  (`ls` bestätigt), obwohl die echten Daten unter
  `~/.local/share/system/privileged/{Contacts,Calendar}` vorhanden sind
  (Owner `privileged:privileged`). **Root Cause seither gefunden (H7,
  Abschnitt „Falsche Schlüsse“ Punkt 5): ein veralteter
  `booster-silica-qt5`-Prozess, der vor der Installation der
  `[X-Sailjail]`-Permissions gestartet wurde und seinen alten Sandbox-Mount-
  Namespace weiterverwendet hat — kein Geräte-/OS-Setup-Problem.** Nach
  `systemctl --user restart booster-silica-qt5.service` war der Mount
  vorhanden. Dass `get_upcoming_events` trotzdem fehlerfrei lief (kein
  `query_failed`), obwohl der Mount zu diesem Zeitpunkt fehlte, deutet
  darauf hin, dass `mKCal::SqliteStorage` hier nicht auf die gemountete
  Datei angewiesen ist, sondern über einen D-Bus-Dienst auf die
  Kalenderdaten zugreift — nicht weiter verifiziert.

**Bug A — `get_upcoming_events` liefert mkcal's automatisches „Geburtstage
aus Kontakten"-Notebook statt echter Termine (behoben).** Mit dem
Redaktions-Fix aus Bug B sichtbar gemacht: die Rohantwort enthielt ~190
Einträge, `summary` durchweg echte Kontaktnamen (z. B. „Markus Furger",
„Doris Schuler"), `start`/`end` auf das tatsächliche Geburtsjahr gepinnt
(1604, 1973, 1975, 1979, 1983, 1984, 1985, 1992, ...) statt auf den
diesjährigen Wiederholungstermin — unabhängig vom `days`-Parameter (7 Tage
angefragt, alle ~190 Einträge zurückgekommen). Ursache:
`calendar->events(start, end)` (`KCalendarCore::Calendar`) expandiert
wiederkehrende Termine (die Birthday-Notebook nutzt eine jährliche RRULE
mit dem echten Geburtsjahr als `DTSTART`) nicht auf ihr tatsächliches
Vorkommen im angefragten Fenster — es kommt das rohe, ungeprüfte `DTSTART`
zurück, das JEDES Mal im Ergebnis landet, ganz unabhängig davon, ob die
diesjährige Wiederholung überhaupt in `[start, end]` fällt.

Fix in `FullProvider::upcomingEvents()`
(`src/platform/full/fullprovider.cpp`): pro Event zusätzlich `event->recurs()`
prüfen und für wiederkehrende Termine über
`event->recurrence()->getNextDateTime(rangeStart - 1s)` das nächste
tatsächliche Vorkommen bestimmen; liegt das ausserhalb `[start, end]` oder
ist ungültig (Wiederholung bereits beendet), wird der Eintrag verworfen.
Nicht-wiederkehrende Termine werden zusätzlich defensiv gegen `[start, end]`
geprüft. Verifiziert auf echter Hardware (Jolla Phone 2026, SFOS 5.2.0.17):
derselbe `get_upcoming_events(days=7)`-Aufruf lieferte vorher ~190, danach
genau 10 Einträge — alle echt im 17.–22.09.2026-Fenster, Geburtstage korrekt
auf 2026 umgerechnet (z. B. „Maya Regli" jetzt am 2026-09-17 statt im
Geburtsjahr), neben den echten Terminen (Konzert, Sauna, Mittagessen).

**Bug B — Redaktion trifft ISO-Datumsstrings, nicht die eigentlich
sensiblen Daten (behoben).** `ConsentGate::redactText()`s Telefonnummer-Regex
(`\+?[0-9][0-9 ()./\-]{5,17}[0-9]`) matched auf `"2026-01-15"` innerhalb eines
ISO-Zeitstempels (8 Ziffern + Bindestriche fallen ins selbe Muster wie eine
Telefonnummer) — dadurch wurden `start`/`end` in Bug A zu
`<contact:N>T00:00:00`. Fix: neuer `isoDatePattern()`-Ausschluss
(`^\d{4}-\d{2}-\d{2}$`) in der Telefonnummer-Pass von `redactText()`
(`src/core/consentgate.cpp`), Regressionstest `leavesIsoDatesAlone` in
`tests/tst_consentgate.cpp`. Bewusst **nicht** angefasst: dass `summary`
(und `name` bei `find_contact`) nicht per Schlüsselname redigiert werden,
ist eine bestehende, test-dokumentierte Design-Entscheidung
(`redactsBySensitiveKey` in `tst_consentgate.cpp` erwartet das explizit) —
blosse Namen gelten im Projekt als unkritisch, nur Adresse/Telefon/E-Mail
werden reflexhaft geschwärzt.

## Nachtrag: `find_contact` wieder aktiviert, zwei Redaktions-Lecks gefunden (2026-09-15)

H7 geklärt (Booster-Neustart, s. o.) → `find_contact` in `toolregistry.cpp`
wieder mit `available=true` registriert (Standard, kein expliziter Parameter
mehr nötig), Regressionstest `unavailableToolCannotBeEnabled` entfernt
(keine Tool mehr mit `available=false`), stattdessen
`findContactRedactsPhoneKeepsName` in `tst_toolregistry.cpp` ergänzt.

Live-Test auf echter Hardware (Jolla Phone 2026, SFOS 5.2.0.17,
`deepseek/deepseek-v4.1-flash`) deckte dabei **zwei echte, aktuelle
Redaktions-Lecks** auf — beide behoben, auf demselben Gerät verifiziert:

**Bug C — `QVariant::StringList` ohne eigenen Fall in
`ConsentGate::redactValue()` (behoben).** `FullProvider::findContact()`
liefert `phones`/`addresses` als `QStringList`, nicht als `QVariantList` —
ein eigener `QVariant`-Typ. Ohne passenden `switch`-Fall landete das im
`default: return value;` und ging **komplett unredigiert** durch, sowohl am
Schlüssel- als auch am Regex-Pfad vorbei. Erster Live-Test zeigte echte
Rufnummern im Klartext im gespeicherten Tool-Ergebnis
(`"phones":["+41418870209","+41793607569"]`). Fix: neuer
`case QVariant::StringList:` in `redactValue()`, wandelt in eine
`QVariantList` um und redigiert jedes Element einzeln.

**Bug D — Plural-Schlüssel `phones`/`addresses` fehlten in
`isSensitiveKey()` (behoben).** Nach Fix C wurden Telefonnummern korrekt
redigiert (sie matchen `phonePattern()`), aber **Adressen weiterhin nicht**
— Freitext wie „In der Mühlematte 8, Altdorf" matcht keine Regex, und
`isSensitiveKey()` kannte nur die Singular-Form `address`, nicht
`addresses`. Fix: `phones`/`addresses` zu `isSensitiveKey()` ergänzt, dazu
die `QVariant::Map`-Iteration in `redactValue()` umgebaut — ein sensitiver
Schlüssel mit `QStringList`-Wert maskiert jetzt jedes Element einzeln über
`placeholderFor()`, statt (wie zuvor) nur bei einem einzelnen String-Wert
zu greifen.

Beide Fixes mit Regressionstests abgesichert
(`redactsStringListValues`, `redactsSensitiveKeyStringListByKey` in
`tst_consentgate.cpp`; `findContactRedactsPhoneKeepsName` in
`tst_toolregistry.cpp` prüft beide Felder), danach zweimal live auf dem
Gerät nachgestellt: derselbe `find_contact`-Aufruf zeigte vorher Klartext,
nachher `<contact:N>`-Platzhalter für Telefonnummern und Adressen
gleichermassen — das Modell hat in beiden Fällen korrekt erkannt, dass es
die echten Werte nicht kennt, und das dem Nutzer auch so mitgeteilt.

**Einordnung:** Bug C betraf nicht nur `find_contact` — jeder künftige Tool-
Output mit einem `QStringList`-Feld wäre vom selben Loch betroffen gewesen.
Aktuell ist `find_contact` der einzige Konsument.

## Nachtrag: Bild-Anhang (`Sailfish.Pickers`) — zwei Permission-Fallstricke (2026-09-15)

Live-Debugging auf echter Hardware (Jolla Phone 2026) beim Umsetzen des
`ImagePickerPage`-Anhangs deckte zwei getrennte, nicht offensichtliche
Permission-Lücken auf — beide behoben (`harbour-nemoai.desktop`,
`sailfishai.desktop`: `Permissions=...;UserDirs;MediaIndexing`):

1. **`Pictures`-Permission allein reicht nicht, damit der Picker überhaupt
   Bilder anzeigt.** Symptom: Attach-Button funktioniert, `ImagePickerPage`
   öffnet, aber bleibt leer, obwohl `~/Pictures/...` nachweislich Dateien
   enthält und im Sandbox-Mount sichtbar ist. Ursache: `ImagePickerPage`
   listet Inhalte über eine Tracker-/MediaIndexing-Abfrage, nicht per
   direktem Verzeichnis-Scan — dafür fehlte die `MediaIndexing`-Permission
   (die im sailjail-Default-Profil normalerweise automatisch dabei ist,
   aber sobald `[X-Sailjail]` explizit gesetzt ist — wie seit F1 — gilt nur
   noch die explizite Liste, s. bereits dokumentierter F1-Fallstrick).
2. **Der Picker zeigt auch Bilder aus `~/Documents` o.ä., nicht nur aus
   `~/Pictures`.** Symptom: Bild im Picker sichtbar und auswählbar, Senden
   lief fehlerfrei durch, aber das Modell reagierte, als hätte es nur den
   Text bekommen — kein Fehler, keine Warnung. Ursache:
   `ConversationStore::imageDataUri()` öffnet die Datei zum Zeitpunkt des
   Requests **im eigenen Sandbox-Kontext** der App (nicht dem des Pickers),
   und `~/Documents` war nicht freigegeben — `QFile::open()` schlägt still
   fehl, die Funktion gibt bewusst nur eine leere Data-URI statt eines
   Fehlers zurück (ein fehlendes Bild soll den Rest der Anfrage nicht
   blockieren), was den eigentlichen Grund verschleiert hat. Fix: statt
   einzelner `Pictures`-Permission das Sammel-Permission `UserDirs`
   (`include`s `Documents`/`Downloads`/`Music`/`Pictures`/`PublicDir`/
   `Videos`) — deckt, wo der Picker tatsächlich hinschaut.

Diagnosemethode: `printf '<pw>\n' | ssh -tt ... devel-su grep -iE
'documents|pictures' /proc/<pid-des-echten-app-binaries>/mounts` — dabei
wichtig, die **richtige PID** zu erwischen (`pgrep -af
'/usr/bin/<app>$'` zeigt sowohl den `invoker`-Prozess als auch das echte
Binary; nur letzteres hat die relevanten Bind-Mounts).

## Nachtrag: `run_command` funktioniert strukturell nicht mehr (2026-09-15)

Live-Test auf echter Hardware: **jeder** `run_command`-Aufruf endet mit
`{"timedOut":true}` — auch triviale, garantiert nicht-interaktive Befehle
wie `cat /proc/loadavg` oder `uptime`. Die Zeitabstände zwischen
Tool-Call und Ergebnis (~5-7s für mehrere Aufrufe) passen ausserdem nicht
zu echten 15s-Timeouts, was auf ein sofortiges Fehlschlagen statt
tatsächlichem Warten hindeutet.

**Root Cause gefunden**: `ls -la /proc/<pid-des-echten-app-binaries>/root/usr/bin/`
für den laufenden `sailfishai`-Prozess zeigt genau drei Dateien —
`sailfishai` selbst, `thumbnaild-pdf`, `thumbnaild-video`. Kein `cat`,
`sh`, `top`, `ps`, nichts sonst. Das ist `firejail --private-bin=sailfishai`
(steht schon lange sichtbar in der generierten Kommandozeile, siehe F1/F3-
Abschnitte oben) — Sailjail baut für jede App mit deklariertem
`[X-Sailjail]` eine private `/usr/bin`, die **ausschliesslich die eigene
Binary** enthält. `QProcess::start()` kann dadurch grundsätzlich **kein
einziges externes Programm** starten, unabhängig von Consent oder
Argumenten.

**Vermutlich eine Nebenwirkung von F1**: Vor der expliziten
`[X-Sailjail]`-Sektion lief `sailfishai` im sailjail-Default-Profil, das
`private-bin` vermutlich nicht erzwingt — `run_command` könnte damals
tatsächlich funktioniert haben (unklar, nie mit Logging verifiziert). F1
hat Contacts/Calendar/Secrets repariert, dabei aber wahrscheinlich
`run_command` strukturell kaputt gemacht: sauberes Sailjail-Sandboxing und
beliebige Prozessausführung schliessen sich gegenseitig aus.

**Entscheidung (Nutzer, 2026-09-15): vorerst nur dokumentieren, kein
Code-Fix.** Zwei denkbare, aber grössere Auswege für später:
1. Eigene, kuratierte Helfer-Binaries unter `%{_datadir}/sailfishai/`
   ausliefern (whitelisteter App-Pfad, nicht `/usr/bin`) und darüber
   referenzieren — ersetzt „beliebiges Programm" durch eine feste,
   sichere Auswahl. Widerspricht bewusst der bisherigen
   Architekturentscheidung „keine Allow-/Blocklist, ConsentGate reicht"
   (Kommentar in `fullprovider.cpp`) — wäre eine echte Kursänderung.
2. `[X-Sailjail]` für `sailfishai` wieder weglassen — verwirft aber F1
   und macht Contacts/Calendar/Secrets erneut kaputt. Kein ernsthafter
   Kompromiss.

Die Tool-Beschreibung in `toolregistry.cpp` wurde angepasst, damit das
Modell das nicht mehr wie im Live-Test beobachtet mehrfach mit
unterschiedlichen Varianten durchprobiert (6 Versuche, alle
`timedOut`), sondern die Situation sofort ehrlich mitteilt. Kein
Verhaltens-Fix am `QProcess`-Aufruf selbst.


