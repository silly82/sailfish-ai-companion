# Sailfish AI Companion - Projektplan

## 🎯 Vision
Native Sailfish OS App mit Hermes-ähnlicher Funktionalität und tiefer Systemintegration, speziell für Jolla Phones optimiert.

## 📱 Kern-Features

### MVP (Minimum Viable Product)
- [ ] OpenRouter/Chat-API Integration
- [ ] Native Silica UI mit Chat-Interface
- [ ] Chat-History mit SQLite
- [ ] System-Info (Battery, Network, Storage)

### Phase 2 - Systemintegration
- [ ] Kontakte-Zugriff (read-only)
- [ ] Notifications lesen
- [ ] Telefonie-Status
- [ ] Bluetooth-Geräte

### Phase 3 - Erweiterte Features
- [ ] Lokale Inference mit llama.cpp
- [ ] Voice Input/Output (TTS/STT)
- [ ] File Attachments
- [ ] Automatisierungen

### Phase 4 - Distribution
- [ ] RPM Packaging
- [ ] Harbour-Compliance Check
- [ ] OpenRepos Publication

## 🛠️ Tech-Stack

**Frontend:**
- QML mit Sailfish Silica Components
- C++ Backend für Performance

**Backend:**
- Qt 5 / C++14
- SQLite für Chat-History
- D-Bus für Systemintegration
- HTTP Client für OpenRouter API

**AI Integration:**
- OpenRouter API (initial)
- llama.cpp für lokale Modelle (optional)

## 📁 Projektstruktur

```
sailfish-ai-companion/
├── rpm/                    # RPM Packaging Specs
├── src/
│   ├── main.cpp           # App Entry Point
│   ├── qml/
│   │   ├── MainPage.qml   # Hauptseite
│   │   ├── ChatPage.qml   # Chat-Interface
│   │   ├── SettingsPage.qml # Einstellungen
│   │   └── Cover.qml      # Cover Action
│   └── backend/
│       ├── AIClient.cpp   # OpenRouter API
│       ├── SystemIntegration.cpp # D-Bus Calls
│       ├── Database.cpp   # SQLite Wrapper
│       └── LocalModel.cpp # llama.cpp Integration
├── po/                    # Übersetzungen
├── icons/                 # App-Icons
└── documentation/         # Entwickler-Dokumentation
```

## 🔌 Systemintegrationen

### D-Bus Services:
- `org.freedesktop.UPower` - Battery Status
- `org.nemomobile.contacts` - Telefonbuch
- `org.freedesktop.Notifications` - Benachrichtigungen
- `org.nemomobile.voicecall` - Telefonie
- `org.bluez` - Bluetooth
- `org.freedesktop.UDisks2` - Storage

### Dateizugriffe:
- Pictures/ Ordner
- Downloads/ Ordner
- App-spezifischer Storage

## 🎨 UI/UX Design

### Silica Patterns:
- Pull-Down-Menü für Modelauswahl
- Swipe-Gesten für Navigation
- Cover Actions für schnellen Zugriff
- Haptic Feedback
- Dark/Light Theme Support

### Seitenstruktur:
1. **MainPage**: Chat-Übersicht
2. **ChatPage**: Konversation mit AI
3. **SettingsPage**: API Keys, Modelle, Systemintegration
4. **SystemPage**: Systemstatus-Überwachung

## ⚠️ Herausforderungen

### Harbour-Compliance:
- Viele System-APIs sind restricted
- Lösung: OpenRepos für uneingeschränkten Zugriff
- Alternativ: Permission-Request System

### Performance:
- C++ statt Python für kritische Komponenten
- QML Optimierungen
- Background Processing für AI Requests

### Offline-Funktionalität:
- llama.cpp Integration
- Model-Größenanpassung für Mobile
- Quantisierte Modelle (4-7GB range)

## 🚀 Entwicklungsplan

### Woche 1-2: Setup & MVP
- [ ] Sailfish SDK Projekt erstellen
- [ ] Basic QML UI mit Chat-Interface
- [ ] OpenRouter API Integration
- [ ] SQLite Database Setup

### Woche 3-4: Systemintegration
- [ ] Battery/Network Status
- [ ] Contacts Integration (read-only)
- [ ] Notifications Access
- [ ] File System Access

### Woche 5-6: Advanced Features
- [ ] Local Inference mit llama.cpp
- [ ] Voice Input/Output
- [ ] Automation Rules
- [ ] Advanced Settings

### Woche 7: Polish & Distribution
- [ ] RPM Packaging
- [ ] Store Submission
- [ ] Documentation

## 📦 Distribution

### Primary Target: OpenRepos
- Keine API-Beschränkungen
- Vollständige Systemintegration
- Community Distribution

### Secondary Target: Harbour
- Eingeschränkte System-APIs
- AppStore Validation
- Official Certification

## 🔧 Entwicklungsumgebung

### Required:
- Sailfish SDK
- Qt Creator
- Jolla Phone/Emulator
- OpenRouter API Key

### Optional:
- llama.cpp für lokale Tests
- Bluetooth Test Devices
- Various Storage Media

## 📋 Erfolgskriterien

### Must-Have:
- Funktionierender Chat mit OpenRouter
- System-Status Anzeige
- Native Sailfish UI
- RPM Package

### Nice-to-Have:
- Lokale Inference
- Voice I/O
- Automation Engine
- Harbour Compliance

## 🌐 API Integration

### OpenRouter Endpoints:
- `https://openrouter.ai/api/v1/chat/completions`
- `https://openrouter.ai/api/v1/models`

### Supported Models:
- deepseek/deepseek-chat-v3.1
- anthropic/claude-3-haiku  
- google/gemini-3.6-flash
- openai/gpt-4o-mini
- meta-llama/llama-3.1-8b-instruct

## 🔒 Sicherheit

- API Keys secure storage
- D-Bus permission management
- Local data encryption
- Network security (HTTPS only)

---

**Nächste Schritte:**
1. Sailfish SDK Projekt erstellen
2. Basic UI Implementation
3. OpenRouter API Integration
4. System Info Integration

Letzte Aktualisierung: 2026-08-02