# Jolla Store listing

Draft copy for the Harbour submission form — updated for **v1.0.1**
(2026-09-15). Supersedes the 0.8.0 draft below the screenshot note; the
0.8.0 submission (2026-08-14) has been sitting in QA for a month with no
decision — check its status on harbour.jolla.com before submitting this as
a fresh entry vs. an update to that pending one.

**What changed since the 0.8.0 draft, and why the copy below is rewritten:**
the interface audit (`docs/todo-harbour-vs-full.md`) found the Harbour build
was linking a disallowed library for `find_contact` and declaring
`Contacts`/`Bluetooth` permissions with no working code path behind them.
Both are now gone from Harbour entirely — `Capabilities::contacts()` is
`false` there, so `find_contact` isn't even registered. **Harbour's tool
set today is 4 tools, all "system info" sensitivity: battery, network,
storage, and the current date/time.** Nothing in Harbour triggers the
consent dialog any more — that's exclusively a full-access (OpenRepos)
thing now. The 0.8.0 "Message to QA" text below, which told reviewers to
expect a consent prompt from reading a contact, is **wrong** for this
build and must not be reused as-is.

New since 0.8.0, in Harbour too: **image attachments** (pick a photo via
Sailfish.Pickers, sent to a vision-capable model) and **message sharing**
(long-press a message, share to another app) — both are plain UI features,
not gated by the tool/consent system.

## Screenshots

Existing captures (`store/screenshots/de-01` through `de-04`, real Jolla
Phone 2026, 2026-08-14) still show the conversation list, pulley menu, and
settings page, which haven't changed shape. They do **not** show the new
attach button or the now-much-shorter Tools & Permissions list — consider
recapturing at least a Tools & Permissions shot before submitting, since the
old one (if it was ever used) would show tools that no longer exist in this
build. Not recaptured in this pass — screenshotting the real device needs a
manual tap on the Quick Settings screenshot tile, not something scriptable
over SSH.

`01`–`04` are emulator captures at the SDK's default 336×798 resolution
(below the Store's stated 1080px-wide minimum), kept only as reference.
`de-01`–`de-04` are real-device captures (Jolla Phone 2026, aarch64,
Sailfish OS 5.2.0.16) taken 2026-08-14. Native capture size was 1032×2272,
just under the form's stated 1080×1920 minimum on the width; upscaled to
1080×2378 (Lanczos, aspect-ratio preserved, no crop) to clear the check.
Device system language was German at capture time — current live testing
(2026-09-15) shows the app running in English on the same device, so the
system language has apparently changed since; a fresh capture would come
out English, matching the primary English Details text directly instead of
needing the bilingual-Details workaround below. Upload order/pick for the
3-screenshot slot: `de-01-conversations.png`, `de-02-pulley-menu.png`,
`de-03-settings.png`. `de-04-chat-active.png` (open chat, model +
active-tools header, empty message list) is a spare.

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
- **Photo attachments.** Pick an image from your gallery and send it along
  with your message to a vision-capable model.
- **Share.** Long-press any message to hand its text to another app.
- **Tool calling with a consent gate.** The assistant can check things like
  battery, network, or storage status — every access above "system info"
  sensitivity would show exactly what's about to be shared and wait for
  your confirmation first, not after (this build's tool set doesn't
  currently include anything above that tier — see note below).
- **Redaction, not trust.** Where the consent gate applies, personal data
  (phone numbers, addresses) is replaced with placeholders before it ever
  reaches the model, resolved back only in the reply on your device.
- **Local history**, stored on-device via SQLite — nothing about your past
  conversations goes anywhere.
- **Your API key stays on-device**, held by Sailfish.Secrets — never
  logged, never in plain settings storage.

This is the Harbour (sandboxed, Jolla Store) build: battery, network,
storage status, and the current date/time — 4 tools, all low-sensitivity,
no confirmation needed. A separate, unsandboxed full-access build on
OpenRepos adds contacts, calendar, SMS, and local command execution for
users who want the consent-gated tier — same app, same UI, more
permissions, never the other way round.

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
- **Foto-Anhänge.** Ein Bild aus der Galerie wählen und zusammen mit der
  Nachricht an ein vision-fähiges Modell schicken.
- **Teilen.** Eine Nachricht lang antippen, um ihren Text an eine andere
  App weiterzugeben.
- **Function-Calling mit Datenschleuse.** Der Assistent kann z. B.
  Akku-, Netz- oder Speicherstatus abfragen — jeder Zugriff ab
  Sensitivität „Persönlich" würde vorher genau zeigen, was rausgehen
  würde, und auf deine Bestätigung warten (das Tool-Set dieses Builds
  enthält aktuell nichts oberhalb dieser Stufe — siehe Hinweis unten).
- **Redaktion statt Vertrauen.** Wo die Datenschleuse greift, werden
  personenbezogene Daten (Telefonnummern, Adressen) durch Platzhalter
  ersetzt, bevor sie das Modell erreichen, und erst in der Antwort auf
  deinem Gerät wieder aufgelöst.
- **Lokaler Verlauf**, per SQLite auf dem Gerät gespeichert — von deinen
  bisherigen Konversationen geht nichts nach aussen.
- **Dein API-Key bleibt auf dem Gerät**, verwaltet über Sailfish.Secrets —
  nie geloggt, nie im Klartext in den Einstellungen.

Dies ist der Harbour-Build (Sailjail-Sandbox, Jolla Store): Akku-, Netz-
und Speicherstatus sowie die aktuelle Uhrzeit — 4 Tools, alle mit
niedriger Sensitivität, keine Bestätigung nötig. Ein separater,
unsandboxed Vollzugriffs-Build auf OpenRepos ergänzt Kontakte, Kalender,
SMS und lokale Befehlsausführung für alle, die die Datenschleusen-Stufe
wollen — dieselbe App, dieselbe Oberfläche, mehr Rechte, nie umgekehrt.

---

## Form fields beyond the description

**Title:** AI Companion (12 chars)

**Summary** (≤200 chars, shown under the title in listings):
- EN: `Chat with an AI that can look at your device — with your permission, every time.` (83 chars)
- DE: `Chat mit einer KI, die aufs Gerät schauen darf — mit deiner Erlaubnis, jedes Mal neu.` (87 chars)

**Category:** Utilities

**Recent changes** (v1.0.1, ≤2000 chars):

> First submission to reach QA under this feature set (supersedes the
> 0.8.0 draft, which has been pending since 2026-08-14).
>
> - Photo attachments: pick an image and send it to a vision-capable model.
> - Share a message's text to another app via long-press.
> - Streaming responses now survive the display blanking mid-reply.
> - Fixed: contact search and calendar tools (full-access build only) no
>   longer send personal data to the model unredacted in some cases.
> - Removed two permissions (Contacts, Bluetooth) that were declared but
>   had no working code behind them in this build — Harbour's tool set is
>   now battery, network, storage, and date/time (4 tools, all low
>   sensitivity, no confirmation prompts).
> - Numerous fixes found and verified on real hardware; see
>   https://github.com/silly82/sailfish-ai-companion/releases for the full
>   changelog.

**Contact details:**
- Email: siliwalker@gmail.com
- Website / Open source project URL: https://github.com/silly82/sailfish-ai-companion
- Privacy policy: `docs/privacy.md` in the repo (link the rendered GitHub URL, e.g. `https://github.com/silly82/sailfish-ai-companion/blob/main/docs/privacy.md` — check it's still accurate for image attachments before linking; last updated 2026-08-14, predates that feature)

**Message to QA:**

> AI Companion requires the reviewer's own OpenRouter API key
> (https://openrouter.ai) to chat — there's no bundled test account, since
> shipping one would mean sharing a paid key. A free OpenRouter signup and
> its free-tier models are enough to exercise chat and tool calling.
>
> This build's tool set (battery/network/storage status, date/time) is all
> "system info" sensitivity, so **no consent dialog will appear** during a
> normal review — that's expected, not a missing feature. The consent-gate
> and redaction feature described in the listing is real but only
> exercised by tools in the separate, non-Harbour full-access build (not
> submitted here). To see the picker/attach flow: tap the paperclip icon
> next to the message field, pick any photo, and send — no permission
> beyond Sailfish.Pickers' own gallery access is required. No other login
> or account is needed.
