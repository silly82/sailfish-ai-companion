# Sailfish AI Companion

Nativer AI-Begleiter für Sailfish OS. Zwei Build-Targets aus einem Trunk:

| Target | Paket | Store | Sandbox |
|---|---|---|---|
| Harbour | `harbour-nemoai` | Jolla Store | Sailjail, minimale Permissions |
| Full | `sailfishai` | OpenRepos | unsandboxed |

Konzept: siehe `docs/konzept-v2.md`.

## Bauen

```sh
# Harbour (Default)
sfdk config target=SailfishOS-5.0.0.x-aarch64
sfdk build

# Full-Access
sfdk build -- --define "fullaccess 1"

# Harbour-Validierung vor jedem Store-Upload
sfdk check
```

## Architektur in einem Satz

Der Unterschied zwischen den Targets ist das **Tool-Manifest**, nicht der Code.
`ToolRegistry` registriert beim Start nur, was `Capabilities` meldet; die QML-UI
blendet den Rest aus.

## Status

- [ ] M1 Vertikaler Durchstich (Chat + Streaming + History)
- [ ] M2 Tool-Framework
- [ ] M3 Harbour-Release
- [ ] M4 Full-Target / OpenRepos
- [ ] M5 Lokale Inference
- [ ] M6 Voice
