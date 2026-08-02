# Sailfish AI Companion — Konzept v2 (Dual-Target)

> Revision von `sailaiapp.md`. Harbour-Angaben geprüft gegen
> docs.sailfishos.org (Allowed APIs / Allowed Permissions, Stand SFOS 4.5.0 +
> Nachträge bis 5.1). Alles mit ⚠️ auf Gerät verifizieren.

---

## 🎯 Kernentscheidung: 2 Build-Targets, **kein** Git-Fork

Zwei parallele Git-Branches divergieren nach 4 Wochen und du pflegst zwei
Chat-UIs. Stattdessen: **ein Repo, ein Trunk, zwei RPM-Specs.**

| | Target A | Target B |
|---|---|---|
| Paketname | `harbour-nemoai` | `sailfishai` |
| Store | Jolla Store (Harbour) | OpenRepos / Storeman |
| Sandbox | Sailjail, minimale Permissions | ohne `X-Sailjail` (unsandboxed) |
| Build | `qmake CONFIG+=harbour` | `qmake CONFIG+=fullaccess` |
| Parallel installierbar | ✅ (eigene App-ID, eigene Datenpfade) | ✅ |

Beide Pakete lassen sich gleichzeitig installieren — nützlich zum Vergleichen,
und Nutzer der Store-Version können später upgraden (Chat-DB-Import über
Sailfish.Pickers).

**Regel:** Der Trunk ist *immer* Harbour-clean. Target B fügt nur hinzu,
entfernt nie. Damit kann A nie durch B kaputtgehen.

---

## 🏗️ Architektur: Capability-Gating statt Code-Fork

Das ist die eigentliche Optimierung. Der Unterschied zwischen den Targets ist
**eine Datei**: das Tool-Manifest.

```
sailfish-ai-companion/
├── src/core/                  # 100% branchneutral, Harbour-clean
│   ├── aiclient.cpp           # Chat-Runde inkl. Function-Calling-Roundtrip
│   ├── illmbackend.h          # gemeinsames Interface beider Backends
│   ├── openrouterbackend.cpp  # Cloud-Pfad, SSE-Streaming
│   ├── localserverbackend.cpp # llama-server auf 127.0.0.1:8080
│   ├── sseparser.cpp          # Ereignis-Framing, weil TCP mitten in Zeilen schneidet
│   ├── conversationstore.cpp  # SQLite via QSqlDatabase
│   ├── keystore.cpp           # API-Key, nie in QSettings
│   ├── capabilities.cpp       # was dieses Target kann
│   ├── toolregistry.cpp       # ← Herzstück
│   └── consentgate.cpp        # Datenschleuse vor jedem Tool-Call
├── src/platform/
│   ├── isystemprovider.h      # abstraktes Interface
│   ├── sandboxed/             # ContextKit, Sailfish.Contacts, BluezQt
│   └── full/                  # QtDBus roh, libcommhistory, mkcal, exec
├── qml/                       # gesamte UI — einmal
│   ├── pages/
│   ├── components/
│   └── cover/
├── tests/                     # QtTest gegen src/core/, ohne SDK lauffähig
├── rpm/
│   ├── harbour-nemoai.spec
│   ├── sailfishai.spec
│   └── sailfishai-llama.spec.todo
├── translations/              # geplant (M3), de/en
└── models/                    # geplant (M5), nur .gguf-Downloadmanifest, keine Blobs
```

### Tool-Registry (LLM-Function-Calling)

Jede Systemintegration ist ein **Tool** mit deklarierter Sensitivität:

```cpp
struct Tool {
    QString      name;             // "get_battery_status"
    QString      description;      // geht als Beschreibung ins Schema
    QJsonObject  parameterSchema;  // an das Modell gesendet
    Sensitivity  level;            // Low | Personal | Critical
    Handler      fn;
    bool         enabled;          // Per-Tool-Toggle, in QSettings gemerkt
};
```

Der Capability-Bedarf steht seit M2 nicht als Feld im Tool, sondern als
Bedingung um die Registrierung herum: `buildManifest()` fragt `Capabilities`
und registriert das Tool gar nicht erst, wenn das Target es nicht kann. Ein
Feld hätte bedeutet, nicht verfügbare Tools trotzdem in der Liste zu führen und
an jeder Auswertung erneut auszufiltern.

`Sensitivity` selbst ist in `ConsentGate` beheimatet und nicht in der Registry:
die Einstufung ist eine Consent-Frage — die Schleuse entscheidet daran, und QML
zeigt sie als Badge. Die Registry deklariert sie nur pro Tool.

Die Reihenfolge der Registrierung ist bedeutungstragend. Verkraftet ein Backend
weniger Tools als registriert sind (`maxTools()`, lokal 4 statt 32), kürzt
`AIClient` das Schema von hinten — die billigen Systemtools stehen deshalb
vorn, das Personenbezogene hinten.

Beim Start registriert sich nur, was das Target kann. Die QML-UI fragt einen
`Capabilities`-Singleton ab und blendet Nicht-Verfügbares aus — **keine
`#ifdef`-Wüste in QML**, keine zwei Chat-Seiten.

Für das Modell heisst das: Target A bekommt ~8 Tools ins Schema, Target B ~25.
Sonst identisch.

---

## 📊 Feature-Matrix (geprüft)

| Feature | Harbour (A) | Full (B) | Weg in A |
|---|---|---|---|
| OpenRouter-Chat | ✅ | ✅ | QtNetwork, Permission `Internet` |
| Chat-History SQLite | ✅ | ✅ | `libsqlite3.so.0` + Qt5Sql erlaubt |
| API-Key sicher | ✅ | ✅ | **Sailfish.Secrets 1.0** + Perm `Secrets` |
| Akku / Ladestatus | ✅ | ✅ | **ContextKit** ⚠️ |
| Netz-/Signalstatus | ✅ | ✅ | ContextKit ⚠️ |
| Storage-Belegung | 🟡 | ✅ | `QStorageInfo`, nur zugängliche Mounts |
| Kontakte lesen | ✅ | ✅ | `Sailfish.Contacts` / `org.nemomobile.contacts` + Perm `Contacts` |
| Bluetooth-Geräte | ✅ | ✅ | `org.kde.bluezqt` + Perm `Bluetooth` |
| Telefonie-Status | 🟡 | ✅ | `Sailfish.Telephony 1.0` (Umfang ⚠️) |
| Notifications **senden** | ✅ | ✅ | `Nemo.Notifications 1.0` |
| Notifications **lesen** | ❌ | 🟡 | nicht möglich — siehe unten |
| SMS / Call-Log | ❌ | ✅ | `libcommhistory` (nicht whitelisted) |
| Kalender | ❌ | ✅ | `libmkcal` |
| Dateizugriff | 🟡 | ✅ | `Sailfish.Pickers` + Perms `Documents`/`Pictures`/`Downloads` |
| Voice Input (STT) | ✅ | ✅ | QtMultimedia-Aufnahme → Cloud-STT |
| Voice Output (TTS) | 🟡 | ✅ | A: Cloud-TTS-Audio abspielen. Kein lokales TTS auf der Allowlist |
| Lokale Inference | 🟡 | ✅ | siehe llama.cpp unten |
| Hintergrund-Daemon | ❌ | ✅ | A: nur `Nemo.KeepAlive 1.2` während App läuft |
| Automatisierung/exec | ❌ | ✅ | Prozess-Spawn ist Harbour-Ausschluss |

**Korrektur zu deinem Plan:** „Notifications lesen" in Phase 2 ist auch in B
nicht sauber lösbar. Man müsste entweder den D-Bus-Bus mit einer Match-Rule
belauschen (Bus-Policy nötig, fragil über Updates) oder Lipsticks internen
Store anzapfen. Ich würde das als eigenes Spike-Ticket behandeln, nicht als
Feature-Zusage.

**Korrektur 2:** `google/gemini-3.6-flash` existiert so nicht, und
`claude-3-haiku` ist überholt. Modelle **nicht hardcoden** — beim ersten Start
`GET /api/v1/models` abrufen, cachen, und nach Kontextlänge + Preis filtern.
Das spart dir auch das ständige Nachpflegen.

---

## 🔒 Datenschleuse (Consent Gate)

Der Punkt, an dem so eine App entweder vertrauenswürdig ist oder nicht. Gilt
für **beide** Targets, ist in B aber existenziell:

1. **Per-Tool-Toggle** in den Settings, alles default *aus* ausser Systeminfo.
2. **Bestätigungsdialog** bei `Sensitivity::Personal` und höher, mit Anzeige
   der exakten Daten, die rausgehen — vor dem Request, nicht danach.
3. **Redaktionsschicht:** Telefonnummern/Adressen werden vor dem Upload durch
   Platzhalter ersetzt (`<contact:7>`), die Rück-Auflösung passiert lokal.
   Das Modell braucht die echte Nummer fast nie.
4. **Lokal-Only-Modus:** Sobald ein lokales Modell aktiv ist, sind alle Tools
   freigeschaltet und nichts verlässt das Gerät. Das ist das stärkste
   Verkaufsargument von Target B.
5. Kein Telemetrie-Kanal. Keine Ausnahmen.

---

## 🧠 llama.cpp — realistisch betrachtet

**In Harbour technisch möglich**, entgegen deiner Annahme, aber mit Auflagen:

- llama.cpp linkt nur gegen `libc`, `libm`, `libstdc++`, `libgcc_s`,
  `libpthread` — alle auf der Allowlist.
- Bundling nach `/usr/share/harbour-nemoai/lib`, rpath setzt libsailfishapp
  ≥ 0.0.11 korrekt.
- **`GGML_OPENMP=OFF`** — `libgomp` ist *nicht* whitelisted. Pthread-Backend.
- Kein BLAS, kein Vulkan/OpenCL (nur GLES ist erlaubt) → reines CPU-Inference.
- Modelle **nie mitpaketieren** — Download beim ersten Start in den App-Datenordner.
- Risiko: mittel. Review-Ausgang bei einer gebündelten nativen Lib ist nicht
  garantiert. Nicht auf den kritischen Pfad legen.

**Modellgrössen:** dein Plan nennt 4–7 GB — das ist für ein Jolla C2 oder
Xperia 10 III unbrauchbar. Realistisch:

| Modell | Quant | RAM | Nutzbar? |
|---|---|---|---|
| Qwen2.5-1.5B-Instruct | Q4_K_M | ~1.1 GB | ✅ flüssig |
| Llama-3.2-3B-Instruct | Q4_K_M | ~2.0 GB | 🟡 nur bei 6–8 GB RAM |
| Phi-3.5-mini (3.8B) | Q4_K_M | ~2.4 GB | 🟡 grenzwertig |
| alles > 7B | — | — | ❌ |

Erwartung: 3–8 Token/s auf ARM-CPU. Für Tool-Calling und kurze Antworten
reicht das; für lange Texte nicht. UI muss Streaming zeigen, sonst wirkt es
kaputt.

---

## 🎨 UI/UX (unverändert gut, mit Ergänzungen)

Deine Silica-Patterns passen. Ergänzungen:

- **Pulley-Down** = Modellauswahl (dein Vorschlag), **Pulley-Up** auf der
  ChatPage = „Neue Konversation" + „Tools für diesen Chat".
- **Cover:** letzte Antwort gekürzt + CoverAction „Diktieren" (springt direkt
  in die Aufnahme). Zweite CoverAction: „Neuer Chat".
- **Capability-Badge** in der Titelzeile: zeigt an, ob gerade Cloud oder lokal,
  und wie viele Tools aktiv sind. Ein Tap → Consent-Übersicht.
- **Streaming-Rendering:** SSE vom OpenRouter-Endpoint inkrementell in ein
  `ListModel`, nicht erst am Ende. Sonst fühlt sich alles langsam an.
- Definition of Done + Common Pitfalls durchgehen **bevor** du einreichst:
  `docs.sailfishos.org/Develop/Apps/UI/Definition_of_Done/`

---

## 🚀 Entwicklungsplan (realistisch)

Dein 7-Wochen-Plan ist bei Feierabend-Tempo eher 7 Monate. Neu geschnitten
nach lieferbaren Meilensteinen statt nach Wochen:

### M1 — Vertikaler Durchstich (Target A)
- SDK-Projekt, qmake, beide Specs von Anfang an
- Silica-Chat-UI mit Streaming
- OpenRouter-Client + dynamische Modellliste
- SQLite-History
- **Ziel:** funktionierender Chat auf dem Gerät. Nichts sonst.

### M2 — Tool-Framework
- `ToolRegistry` + `ISystemProvider` + `ConsentGate`
- Erste 3 Tools: Battery, Netz, Uhrzeit (ContextKit)
- Function-Calling-Roundtrip gegen ein Modell, das das kann
- **Ziel:** die Architektur beweist sich, bevor sie 20 Tools trägt.

### M3 — Harbour-Release
- Sailfish.Secrets für den Key
- `harbour-rpmvalidator` grün
- Icons, Übersetzungen (de/en), Store-Assets
- **Ziel:** in der Jolla Store. Ab hier existiert das Projekt öffentlich.

### M4 — Target B aufmachen
- `platform/full`: QtDBus roh, commhistory, mkcal
- Tool-Set auf ~25 erweitern
- Optionaler systemd-User-Service
- **Ziel:** OpenRepos-Release.

### M5 — Lokale Inference
- llama.cpp-Integration, Modell-Downloader, Speicherwarnung
- erst in B, bei Erfolg Backport nach A

### M6 — Voice
- QtMultimedia-Aufnahme → STT
- in B zusätzlich whisper.cpp / Piper lokal

---

## ✅ Erfolgskriterien (nachschärft)

**Must-Have Target A:**
- `harbour-rpmvalidator` ohne Fehler
- Chat + Streaming + History
- ≥ 5 Tools mit Consent-Gate
- Store-Freigabe erhalten

**Must-Have Target B:**
- alles aus A, plus Kontakte/Kalender/SMS als Tools
- lokales Modell lauffähig
- dokumentierter Unterschied zu A in der README (Nutzer müssen verstehen,
  warum sie an der Sandbox vorbei installieren)

**Nice-to-Have:**
- Voice I/O, Automatisierungsregeln, Attachment-Support

---

## ⚠️ Offene Punkte / zu verifizieren

1. **ContextKit-Property-Namen** — die exakten Keys (`Battery.ChargePercentage`
   etc.) gegen `nemo-qml-plugin-contextkit-qt5` auf dem Gerät prüfen.
2. **`Sailfish.Telephony 1.0`** — Doku ist dünn, tatsächlicher Umfang unklar.
   Jolla garantiert hier keine Backwards-Compat.
3. **Sailjail + `Nemo.DBus`** — der Import ist erlaubt, aber welche Services
   das Profil durchlässt, hängt an den Permissions. Nicht davon ausgehen, dass
   beliebige System-Services erreichbar sind.
4. **Unsandboxed in B** — ob das Weglassen der `X-Sailjail`-Sektion auf
   SFOS 5.x noch zum unsandboxed Start führt, auf dem Zielgerät testen.
5. **Allowed-APIs-Liste** ist auf Stand 4.5.0 datiert, mit 5.1-Nachträgen.
   Vor dem Release gegen die Validator-Configs im Repo
   `sailfishos/sdk-harbour-rpmvalidator` gegenprüfen — die sind die Wahrheit.

---

*Basiert auf sailaiapp.md vom 2026-08-02.*
