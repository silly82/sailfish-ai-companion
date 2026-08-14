# Jolla Store listing

Draft copy for the Harbour submission form. Screenshots are in
`store/screenshots/` — emulator captures at the SDK's default 336×798
resolution, good enough as placeholders/reference; real-device screenshots
at native resolution should replace them before the actual submission.

Category: **Utilities** (alternatively **Internet** — the app is a chat
client, but its defining feature is the on-device tool/consent layer, which
leans utility).

---

## English

**Short description** (one line):

> Chat with an AI that can look at your device — with your permission, every time.

**Long description:**

AI Companion is a native Sailfish OS chat client for OpenRouter's cloud
models, built around one idea: the assistant can only see what you let it
see, one request at a time.

- **Streaming chat** with a dynamically fetched, always-current model list —
  no hardcoded models to go stale.
- **Tool calling with a consent gate.** The assistant can check things like
  battery status, network state, or contacts — but every access above
  "system info" sensitivity shows exactly what's about to be shared and
  waits for your confirmation first, not after.
- **Redaction, not trust.** Personal data (phone numbers, addresses) is
  replaced with placeholders before it ever reaches the model, resolved
  back only in the reply on your device.
- **Local history**, stored on-device via SQLite — nothing about your past
  conversations goes anywhere.
- **Your API key stays on-device**, held by Sailfish.Secrets — never
  logged, never in plain settings storage.

This is the Harbour (sandboxed, Jolla Store) build: battery/network status,
contacts, Bluetooth, and time — about 8 tools. A separate, unsandboxed
full-access build on OpenRepos adds calendar, SMS, and local command
execution for users who want it — same app, same UI, more permissions,
never the other way round.

---

## Deutsch

**Kurzbeschreibung** (eine Zeile):

> Chat mit einer KI, die aufs Gerät schauen darf — mit deiner Erlaubnis, jedes Mal neu.

**Ausführliche Beschreibung:**

AI Companion ist ein nativer Sailfish-OS-Chat-Client für OpenRouters
Cloud-Modelle, gebaut um eine Idee herum: Der Assistent sieht nur, was du
ihm zeigst — pro Anfrage neu entschieden.

- **Streaming-Chat** mit einer dynamisch abgerufenen, immer aktuellen
  Modellliste — keine hartcodierten Modelle, die veralten.
- **Function-Calling mit Datenschleuse.** Der Assistent kann z. B.
  Akkustatus, Netzstatus oder Kontakte abfragen — aber jeder Zugriff ab
  Sensitivität „Persönlich" zeigt vorher genau, was rausgehen würde, und
  wartet auf deine Bestätigung — nicht danach.
- **Redaktion statt Vertrauen.** Personenbezogene Daten (Telefonnummern,
  Adressen) werden durch Platzhalter ersetzt, bevor sie das Modell
  erreichen, und erst in der Antwort auf deinem Gerät wieder aufgelöst.
- **Lokaler Verlauf**, per SQLite auf dem Gerät gespeichert — von deinen
  bisherigen Konversationen geht nichts nach aussen.
- **Dein API-Key bleibt auf dem Gerät**, verwaltet über Sailfish.Secrets —
  nie geloggt, nie im Klartext in den Einstellungen.

Dies ist der Harbour-Build (Sailjail-Sandbox, Jolla Store): Akku-/Netzstatus,
Kontakte, Bluetooth und Uhrzeit — rund 8 Tools. Ein separater, unsandboxed
Vollzugriffs-Build auf OpenRepos ergänzt Kalender, SMS und lokale
Befehlsausführung für alle, die das wollen — dieselbe App, dieselbe
Oberfläche, mehr Rechte, nie umgekehrt.
