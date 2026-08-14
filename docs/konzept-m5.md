# M5 — Lokale Inferenz: Konzept

> Vertieft den M5-Absatz aus [`konzept-v2.md`](konzept-v2.md#-llamacpp--realistisch-betrachtet).
> Dort steht die grundsätzliche Machbarkeitsprüfung (Linking, Allowlist,
> Modellgrössen) — hier geht es um den konkreten Bauplan.

## Ausgangslage

Ein guter Teil des Grundgerüsts existiert bereits, aus M1–M4 mitgezogen, weil
`ILlmBackend` von Anfang an auf zwei Implementierungen ausgelegt war
(Architekturentscheidung 3, `CLAUDE.md`):

| Baustein | Datei | Stand |
|---|---|---|
| Gemeinsames Backend-Interface | `src/core/illmbackend.h` | fertig, seit M1 |
| Lokales Backend, Gerüst | `src/core/localserverbackend.{h,cpp}` | Konstanten/Kommentare stehen, `startServer()`/`chat()` sind Stubs |
| Backend-Umschaltung | `AIClient::setBackend()` | fertig, seit M1 (Cloud ist bisher der einzige Aufrufer) |
| „Nur lokal"-Schalter | `ConsentGate::localOnly` + `SettingsPage.qml` | Property + UI stehen, aber nichts löst bei Toggle einen `setBackend()` aus |
| Capability-Flag | `Capabilities::localInference()` | Full: `true`, Harbour: `false` (Kommentar: „ab M5 evtl. true") |
| RPM-Grundgerüst | `rpm/sailfishai-llama.spec.todo` | Build-Flags entschieden (s. u.), aber keine echte `.spec` |
| Modell-Manifest | `models/manifest.json` | Struktur steht, `url`-Felder sind `"TODO"` |

M5 schliesst diese Lücken, in dieser Reihenfolge:

1. `sailfishai-llama`-Paket bauen (echte `.spec`, CI/lokaler Build)
2. `LocalServerBackend::startServer()`/`chat()` implementieren
3. Modell-Downloader (UI + Netzwerkcode)
4. Speicherwarnung
5. `localOnly`-Toggle tatsächlich mit `AIClient::setBackend()` verdrahten
6. Backport-Entscheidung Richtung Harbour

Ziel zuerst nur Full (`sailfishai`) — deckt sich mit dem Konzept-Fahrplan
(„erst in B, bei Erfolg Backport nach A"). Full braucht ohnehin schon
Prozess-Spawn für andere Zwecke noch nicht, aber Harbour verbietet ihn
kategorisch (`docs/konzept-v2.md` Harbour-Regeln) — ein zweiter, laufender
Prozess ist in B ohne Weiteres im Rahmen, in A architektonisch die grössere
Frage.

---

## 1. Paket `sailfishai-llama`

Eigenständiges RPM, **kein** Linken in die App (Architekturentscheidung 3 —
damit bleibt `ILlmBackend` bei zwei austauschbaren HTTP-Clients, und
llama.cpp-Updates brauchen kein App-Release).

Build-Flags stehen schon fest (`rpm/sailfishai-llama.spec.todo`):

- `-DGGML_OPENMP=OFF` — `libgomp` ist nicht auf der Harbour-Allowlist und auf
  SFOS ohnehin nicht garantiert vorhanden. Pthread-Backend stattdessen.
- `-DGGML_NATIVE=OFF -DGGML_CPU_ARM_ARCH=armv8.2-a+dotprod` — A78 (Jolla
  Phone 2026) und A75 (C2) können `dotprod`, aber kein `i8mm`. Ein zu
  optimistischer `-march` crasht mit `SIGILL` erst zur Laufzeit, nicht beim
  Bauen — deshalb hart auf den gemeinsamen Nenner beider Zielgeräte pinnen,
  nicht `-march=native` im Build-Environment.
- Kein BLAS, kein Vulkan/OpenCL — nur GLES ist erlaubt, beide SoCs haben
  ohnehin keine für llama.cpp nutzbare GPU-Compute-API. Reines CPU-Inference.
- Modelle werden **nicht** mitpaketiert — reiner Server-Binary plus Libs.

Offene Bau-Entscheidungen für die echte `.spec`:

- **Versionierung getrennt von der App.** `sailfishai-llama` folgt der
  llama.cpp-Upstream-Version (z. B. `sailfishai-llama-b4821-1`), nicht
  `X.Y.Z` der App — sonst zwingt ein reines llama.cpp-Bugfix-Update einen
  App-Versionsbump, den CLAUDE.mds Versionierungsregel („ein Trunk, eine
  Version für beide Targets") gar nicht vorsieht (die gilt nur für
  `harbour-nemoai`/`sailfishai`, nicht für dieses dritte Paket).
- **`Recommends:` statt `Requires:`** in `sailfishai.spec` — das Full-Paket
  soll ohne lokale Inferenz installierbar bleiben (steht so schon im
  `%description` von `sailfishai.spec`, nur `sailfishai-llama` existiert
  noch nicht als installierbares Ziel).
- **Kein systemd-User-Service.** `llama-server` läuft als Kindprozess der
  App (`QProcess` in `LocalServerBackend`), nicht als Hintergrunddienst —
  passt zur bereits getroffenen Entscheidung „Hintergrund-Daemon: nur
  `Nemo.KeepAlive` während App läuft" (`konzept-v2.md`, Feature-Matrix).
  Stirbt die App, stirbt der Server mit; kein verwaister Prozess, keine
  zusätzliche Permission für einen eigenständigen Dienst.

---

## 2. `LocalServerBackend` — von Stub zu Implementierung

`startServer()` bekommt den vollen Kommandozeilenaufbau, der in den
Kommentaren von `localserverbackend.h`/`.cpp` schon vorgezeichnet ist:

```
llama-server -m <modelPath> --host 127.0.0.1 --port 8080
             -c 4096 -t <große-Kerne> --prompt-cache <cachePath>
```

Details, die im Stub noch nicht entschieden sind:

- **Thread-Pinning auf die grossen Kerne.** `-t` allein wählt nur die
  *Anzahl*, nicht *welche* Kerne. Braucht zusätzlich `taskset`/
  `sched_setaffinity` vor dem Start, sonst verteilt der Scheduler frei über
  big.LITTLE — mit den kleinen Kernen im Mix sinkt der Durchsatz kaum
  messbar, aber die Hitze steigt (Kommentar in `localserverbackend.h`).
  Kern-IDs sind SoC-spezifisch (Unisoc T606 vs. MediaTek) — Erkennung über
  `/sys/devices/system/cpu/cpu*/cpufreq/cpuinfo_max_freq`, nicht hardcoden.
- **`--prompt-cache`-Pfad** im App-Datenordner, ein Cache pro Modell-ID (der
  System-Prompt inkl. Tool-Schemas ändert sich nur, wenn sich das
  Tool-Manifest ändert — Cache-Invalidierung also an
  `ToolRegistry::buildManifest()`-Ausgabe hängen, nicht an Zeit/Version).
- **Bereitschaft ≠ Prozess läuft.** `available()` prüft aktuell nur
  `QProcess::state() == Running` — das sagt nichts darüber, ob der Server
  das Modell schon geladen hat und Requests beantwortet (Ladezeit bei
  Q4_K_M/4B: mehrere Sekunden). Braucht einen echten Health-Check
  (`GET /health` oder ersten `/v1/models`-Request pollen), bevor `AIClient`
  einen Chat-Request lostreten darf — sonst läuft der erste Versuch nach
  jedem Kaltstart ins Leere.
- **Fehlerfälle, die der Stub noch nicht behandelt:**
  - Port 8080 belegt (zweite Instanz nach Absturz, die den `QProcess` nicht
    sauber beendet hat) → vor dem Start prüfen, ggf. Alt-Prozess per PID-Datei
    identifizieren und beenden.
  - Modell-Datei fehlt/korrupt (abgebrochener Download) → `startServer()`
    muss das *vor* dem Prozessstart prüfen (Checksumme, s. Downloader unten),
    nicht erst am kryptischen `llama-server`-Exitcode scheitern.
  - OOM-Kill durch den Kernel bei knappem RAM (C2 mit 8 GB und mehreren
    offenen Apps) → `QProcess::finished()` mit Exitcode/Signal auswerten und
    als eigene, verständliche Fehlermeldung an `ILlmBackend::failed()`
    durchreichen, nicht als generisches „Verbindung fehlgeschlagen".
- **Lifecycle beim App-Wechsel in den Hintergrund.** Solange ein Chat aktiv
  ist, hält `Nemo.KeepAlive 1.2` den Prozess am Leben (schon in den
  Harbour-Regeln als erlaubt gelistet). Ohne aktiven Request sollte
  `stopServer()` nach einer Leerlauf-Frist greifen — ein permanent laufender
  Inferenz-Server im Hintergrund ist genau die Dauerlast, vor der der
  Thermik-Hinweis in `localserverbackend.h` warnt.

---

## 3. Modell-Downloader

`models/manifest.json` ist die Datenquelle, aber `url: "TODO"` muss erst
geklärt werden:

- **Woher die `.gguf`-Dateien?** Vermutlich Hugging Face (Quantisierungen von
  `Qwen/Qwen3-*-Instruct-GGUF` o. ä.) — vor dem Eintragen der echten URL
  Lizenz der jeweiligen Quantisierung prüfen (Re-Distribution über eine
  eigene URL vs. Direktlink zu HF, letzteres vermeidet eigenes Hosting).
- **Checksumme im Manifest ergänzen** (`sha256`, fehlt aktuell als Feld) —
  ohne Verifikation nach Download lädt ein abgebrochener/manipulierter
  Download stillschweigend ein kaputtes Modell, das erst beim
  `llama-server`-Start auffällt (s. Fehlerfälle oben).

Ablauf (UI in `ModelPage.qml`, die aktuell nur Cloud-Modelle listet — braucht
einen zweiten Abschnitt „Lokal", gespeist aus `manifest.json` statt aus
`/models`):

1. Manifest laden (gebündelt in der App, `models/manifest.json` — kein
   Netzwerk-Roundtrip nur um die Liste zu zeigen; falls die Modell-Liste sich
   später unabhängig vom App-Release ändern soll, wäre ein Nachladen von
   einer eigenen URL eine spätere Erweiterung, kein M5-Muss).
2. Pro Modell: `min_ram_bytes` gegen tatsächlich verfügbares RAM prüfen
   (Abschnitt 4) — nicht verfügbare Modelle ausgegraut, nicht versteckt
   (Nutzer soll verstehen, *warum* etwas fehlt, nicht nur dass es fehlt).
3. Download über `QNetworkAccessManager`, Fortschritt via
   `downloadProgress()`-Signal in ein `ProgressBar`-Element auf der
   `ModelPage`.
4. Nach Abschluss: Checksumme prüfen, bei Erfolg in den App-Datenordner
   verschieben (Download erst in eine `.part`-Datei, damit ein Absturz
   mitten im Download keine halbfertige Datei hinterlässt, die
   fälschlich als „vorhanden" gilt).
5. Löschfunktion pro heruntergeladenem Modell (Speicherplatz ist auf beiden
   Zielgeräten knapp genug, dass Nutzer aktiv aufräumen wollen).

Was **nicht** zu M5 gehört: Resume von abgebrochenen Downloads (HTTP
Range-Requests). Bei 1–2,5 GB und WLAN ist ein Neustart ärgerlich, aber kein
Blocker — als Nice-to-have vormerken, nicht auf den kritischen Pfad.

---

## 4. Speicherwarnung

`min_ram_bytes` im Manifest existiert schon, aber es gibt noch keinen Code,
der tatsächliches Geräte-RAM ausliest. `QSysInfo` liefert das nicht — Qt hat
dafür keine portable API. Auf Linux/SFOS der übliche Weg:
`/proc/meminfo`, Feld `MemTotal`, direkt gelesen (kein zusätzliches Harbour-
Permission nötig, `/proc` ist wie `/sys/class/power_supply` world-readable —
gleiches Muster wie beim Akku-Tool).

Zwei Schwellen, nicht nur eine:

- **Vor dem Download:** `min_ram_bytes` nicht erfüllt → Download-Button
  bleibt nutzbar (der Nutzer kennt sein Gerät ggf. besser als die
  konservative Schätzung im Manifest), aber mit Warnhinweis statt stillem
  Blockieren.
- **Vor dem Start (`startServer()`):** zusätzlich *aktuell freies* RAM
  prüfen (`MemAvailable` aus `/proc/meminfo`, nicht `MemFree` — letzteres
  ignoriert reclaimbaren Cache und ist auf Linux notorisch irreführend). Ein
  Gerät mit genug RAM *insgesamt*, aber gerade drei andere Apps offen, sollte
  vor dem Start gewarnt werden statt in einen OOM-Kill zu laufen (s.
  Fehlerfälle in Abschnitt 2).

`recommended_for: ["jolla-c2", "jolla-phone-2026"]` im Manifest ist aktuell
Geräte-ID-basiert, aber es gibt keine Geräteerkennung im Code. RAM-basiert
filtern (`min_ram_bytes` gegen `MemTotal`) ist robuster als ein
Geräte-ID-Abgleich, der bei jedem neuen Zielgerät im Manifest nachgepflegt
werden müsste — das Feld `recommended_for` kann als informativer Zusatz
bleiben (UI-Text „empfohlen für dein Gerät"), sollte aber nicht die einzige
Filterlogik sein.

---

## 5. `localOnly` tatsächlich verdrahten

`ConsentGate::localOnly` und der Schalter in `SettingsPage.qml` existieren
bereits, lösen aber noch keinen Backend-Wechsel aus. Fehlendes Bindeglied:
ein Listener auf `localOnlyChanged()`, der `AIClient::setBackend()` mit
`LocalServerBackend`/`OpenRouterBackend` aufruft — und dabei
`LocalServerBackend::startServer()` mit dem zuletzt gewählten/heruntergeladenen
lokalen Modell anstösst, falls noch nicht `available()`.

Reihenfolge beim Umschalten *auf* lokal wichtig für die UX: Server-Start
dauert (Modell laden), das darf die UI nicht blockieren — Ladezustand im
Capability-Badge zeigen (`konzept-v2.md`, UI-Abschnitt nennt das Badge schon
für Cloud/Lokal-Anzeige, hier zusätzlich ein dritter Zustand „lädt").

---

## 6. Testbarkeit

Ohne SDK/Gerät testbar (`tests/`, Desktop-Build):

- Manifest-Parsing (`models/manifest.json` → Datenstruktur), inkl.
  Fehlerfällen (fehlendes Feld, kaputtes JSON).
- RAM-Schwellenwert-Logik, sobald sie eine eigene Funktion ist statt inline
  im UI-Code — `/proc/meminfo`-Parsing lässt sich mit einer Fake-Datei
  testen, wie `tests/fakes.h` es für andere Provider schon vorführt.
- Downloader-Statemachine (Start/Progress/Fehler/Fertig) gegen einen
  Test-HTTP-Server oder QNetworkAccessManager-Mock — nicht die echte
  llama.cpp-Binary.
- `LocalServerBackend`s Kommandozeilen-Aufbau (welche Argumente bei welcher
  Konfiguration) — reine String-/Argument-Assertion, kein echter
  Prozessstart nötig.

Nur auf echtem Gerät sinnvoll:

- Tatsächliche Tokens/Sekunde, Prefill-Zeit mit vollem Tool-Manifest.
- Thermik unter Dauerlast (das Desktop-x86-System hat ein komplett anderes
  thermisches Profil als ein Phone-SoC).
- Echtes OOM-Verhalten bei knappem RAM.
- `--prompt-cache`-Wirkung — der Effekt hängt an echten Prefill-Zeiten, die
  der Desktop-Build (Qt 5.15, andere CPU-Architektur) nicht repräsentiert.

Deckt sich mit dem bereits etablierten Muster aus M2/M4 (siehe
`CLAUDE.md`, „Nächster Schritt"-Historie): Headless-Testharness, das die
echten Core-Klassen direkt auf dem Gerät verlinkt, ohne QML/Netzwerk —
für M5 zusätzlich ohne echten Cloud-Request, dafür mit echtem
`llama-server`-Prozess.

---

## 7. Backport-Kriterium Richtung Harbour

`konzept-v2.md` stuft das Harbour-Risiko schon als „mittel" ein („Review-
Ausgang bei einer gebündelten nativen Lib ist nicht garantiert. Nicht auf
den kritischen Pfad legen."). Für M5 heisst das konkret: der Fahrplan baut
`sailfishai-llama` zunächst nur als optionale Empfehlung fürs Full-Target.
Backport nach Harbour erst, wenn:

1. Full-Betrieb auf beiden Zielgeräten über mehrere Wochen stabil lief
   (kein wiederkehrender OOM-Kill, keine Prozess-Leichen nach Absturz).
2. Die gebündelte `llama-server`-Binary tatsächlich nur gegen die
   Harbour-Allowlist-Libs linkt (`ldd` gegen die echte Build-Ausgabe prüfen,
   nicht nur gegen die Kommentar-Liste in `sailfishai-llama.spec.todo`).
3. Ein Harbour-Reviewer-Vorgespräch (falls möglich) oder zumindest ein
   `sfdk check`-grüner Trockenlauf mit dem gebündelten Binary vorliegt —
   die Validator-Configs sind die Wahrheit, nicht diese Doku (gleicher
   Grundsatz wie in `CLAUDE.md`s Harbour-Regeln).

Bis dahin bleibt `Capabilities::localInference()` für Harbour `false`.

---

## Offene Punkte

1. Modell-Download-URLs und Lizenzprüfung (Abschnitt 3).
2. Checksummen-Feld im Manifest ergänzen und Quelle dafür festlegen (selbst
   berechnet beim Kuratieren vs. vom Hoster übernommen).
3. Kern-Erkennung „gross vs. klein" pro SoC — kein generischer Weg über Qt,
   branchspezifischer Code in `LocalServerBackend` nötig.
4. Ob die Modell-Liste (`manifest.json`) irgendwann serverseitig aktualisiert
   werden soll, statt nur mit der App auszuliefern — nicht für M5 nötig,
   aber eine Weiche, die früh genug entschieden werden sollte, bevor UI/Code
   sich auf „immer gebündelt" festlegen.
5. Downloads über mobile Daten vs. nur WLAN — bei 1–2,5 GB pro Modell
   vermutlich ein Opt-in-Warnhinweis, ähnlich wie andere Apps das lösen;
   nicht im Manifest oder Backend verankert, reine UI-Entscheidung.
