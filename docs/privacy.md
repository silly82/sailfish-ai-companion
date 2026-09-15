# Privacy Policy — AI Companion

*[Schweizer Hochdeutsch weiter unten](#datenschutzerklärung)*

Last updated: 2026-09-15.

AI Companion (`harbour-nemoai` / `sailfishai`) is a Sailfish OS chat client
for OpenRouter's cloud models. This policy covers both build targets.

## What stays on your device

- **Conversation history** is stored locally via SQLite. It is never
  uploaded, synced, or shared with the developer.
- **Your OpenRouter API key** is stored via `Sailfish.Secrets` (Harbour
  build) or a permission-restricted local file (full-access build). It is
  never logged and never written to plain settings storage.
- The app collects no analytics, telemetry, or crash reports, and contacts
  no server other than the ones described below.

## What leaves your device, and when

- **Chat messages** you send are transmitted to OpenRouter
  (openrouter.ai) and routed to whichever model you selected, so that
  provider and model can process your request. See
  [OpenRouter's privacy policy](https://openrouter.ai/privacy) for how they
  handle that data.
- **Photos you attach** are sent the same way — picking an image from your
  gallery and sending it embeds the full image in that request to
  OpenRouter and the selected model. Nothing about the attachment is
  redacted or altered; treat sending a photo the same as sending its exact
  content to whichever model you've chosen.
- **Tool calls above "system info" sensitivity** (e.g. reading a contact)
  require your explicit confirmation each time, shown before anything is
  sent. Personal data such as phone numbers and addresses is replaced with
  placeholders before the request leaves the device, and resolved back to
  the real values only in the reply, locally.
- The full-access build (OpenRepos only, not distributed via Jolla Store)
  can additionally access calendar, SMS, and run local commands if you
  grant it — governed by the same consent gate as everything else above
  "system info" sensitivity.

## What the developer never sees

The developer has no server component and receives no data from the app —
no account system, no backend, nothing phones home. Any data handling
happens strictly between your device and OpenRouter, using your own API
key.

## Contact

Questions: siliwalker@gmail.com
Source: https://github.com/silly82/sailfish-ai-companion

---

## Datenschutzerklärung

Letzte Aktualisierung: 15.09.2026.

AI Companion (`harbour-nemoai` / `sailfishai`) ist ein Sailfish-OS-Chat-Client
für OpenRouters Cloud-Modelle. Diese Erklärung gilt für beide Build-Targets.

## Was auf dem Gerät bleibt

- **Der Gesprächsverlauf** wird lokal per SQLite gespeichert. Er wird nie
  hochgeladen, synchronisiert oder mit dem Entwickler geteilt.
- **Dein OpenRouter-API-Key** wird über `Sailfish.Secrets` (Harbour-Build)
  bzw. eine berechtigungsbeschränkte lokale Datei (Full-Access-Build)
  gespeichert. Er wird nie geloggt und nie im Klartext in den Einstellungen
  abgelegt.
- Die App sammelt keine Analytics, keine Telemetrie und keine
  Absturzberichte und kontaktiert ausser den unten genannten keine Server.

## Was das Gerät verlässt, und wann

- **Chat-Nachrichten**, die du sendest, gehen an OpenRouter (openrouter.ai)
  und werden an das von dir gewählte Modell weitergeleitet, damit
  Anbieter und Modell deine Anfrage verarbeiten können. Siehe
  [OpenRouters Datenschutzerklärung](https://openrouter.ai/privacy) dazu,
  wie diese Daten dort behandelt werden.
- **Angehängte Fotos** gehen denselben Weg — ein aus der Galerie gewähltes
  und gesendetes Bild wird vollständig in diese Anfrage an OpenRouter und
  das gewählte Modell eingebettet. Am Anhang wird nichts geschwärzt oder
  verändert; das Senden eines Fotos ist gleichzusetzen mit dem Senden
  seines exakten Inhalts an das jeweils gewählte Modell.
- **Tool-Aufrufe ab Sensitivität «Persönlich»** (z. B. Kontakt lesen)
  brauchen jedes Mal deine ausdrückliche Bestätigung, bevor überhaupt
  etwas gesendet wird. Personenbezogene Daten wie Telefonnummern und
  Adressen werden durch Platzhalter ersetzt, bevor die Anfrage das Gerät
  verlässt, und erst in der Antwort lokal wieder aufgelöst.
- Der Full-Access-Build (nur OpenRepos, nicht im Jolla Store) kann
  zusätzlich auf Kalender und SMS zugreifen und lokale Befehle ausführen,
  sofern du das erlaubst — geregelt über dieselbe Datenschleuse wie alles
  ab Sensitivität «Persönlich».

## Was der Entwickler nie zu sehen bekommt

Der Entwickler betreibt keine Serverkomponente und erhält keine Daten aus
der App — kein Konto-System, kein Backend, nichts «telefoniert nach
Hause». Jede Datenverarbeitung findet ausschliesslich zwischen deinem
Gerät und OpenRouter statt, mit deinem eigenen API-Key.

## Kontakt

Fragen: siliwalker@gmail.com
Quellcode: https://github.com/silly82/sailfish-ai-companion
