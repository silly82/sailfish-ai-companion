# Jolla Store listing

Draft copy for the Harbour submission form. Screenshots are in
`store/screenshots/` — `01`–`04` are emulator captures at the SDK's default
336×798 resolution (below the Store's stated 1080px-wide minimum), kept only
as reference. `de-01`–`de-04` are real-device captures (Jolla Phone 2026,
1032×2272, aarch64, Sailfish OS 5.2.0.16) taken 2026-08-14 — use these for
submission. Device system language was German at capture time, so the app
UI is German throughout; per-decision, submit the form with **both** an
English and a German language variant of the Details section (below) rather
than re-capturing in English. Upload order/pick for the 3-screenshot slot:
`de-01-conversations.png`, `de-02-pulley-menu.png`, `de-03-settings.png`.
`de-04-chat-active.png` (open chat, model + active-tools header, empty
message list) is a spare if a 4th slot or a replacement is ever wanted.

Known cosmetic bug visible in `de-01-conversations.png`: the third
conversation preview renders literal `**5 Stunden und 47 Minuten**` —
the chat view doesn't parse Markdown bold out of the model's reply. Not
fixed as part of this listing pass; flagged for a separate fix.

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

---

## Form fields beyond the description

**Title:** AI Companion (12 chars)

**Summary** (≤200 chars, shown under the title in listings):
- EN: `Chat with an AI that can look at your device — with your permission, every time.` (83 chars)
- DE: `Chat mit einer KI, die aufs Gerät schauen darf — mit deiner Erlaubnis, jedes Mal neu.` (87 chars)

**Category:** Utilities

**Recent changes** (first release, ≤2000 chars):

> Initial release (0.8.0).
>
> - Streaming chat against any OpenRouter model, with a live, always-current model list — nothing hardcoded.
> - Consent-gated tool calling: battery, network, time, and contacts. Every access above "system info" sensitivity is shown and confirmed before anything leaves the device.
> - On-device redaction: personal data is replaced with placeholders before a request reaches the model, resolved back only in the reply, locally.
> - Local conversation history via SQLite; API key stored via Sailfish.Secrets, never logged, never in plain settings storage.

**Contact details:**
- Email: siliwalker@gmail.com
- Website / Open source project URL: https://github.com/silly82/sailfish-ai-companion
- Privacy policy: `docs/privacy.md` in the repo (link the rendered GitHub URL once pushed, e.g. `https://github.com/silly82/sailfish-ai-companion/blob/main/docs/privacy.md`)

**Message to QA:**

> AI Companion requires the reviewer's own OpenRouter API key
> (https://openrouter.ai) to chat — there's no bundled test account, since
> shipping one would mean sharing a paid key. A free OpenRouter signup and
> its free-tier models are enough to exercise chat and tool calling.
> On the first tool call above "system info" sensitivity (e.g. reading a
> contact), the app shows a consent dialog before anything is sent —
> expect and confirm this prompt during review. No other login or account
> is required.
