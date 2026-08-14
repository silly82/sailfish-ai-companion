#include "toolregistry.h"
#include "capabilities.h"
#include "appsettings.h"
#include "../platform/isystemprovider.h"

#include <QDateTime>
#include <QLocale>
#include <QSettings>
#include <QStringList>
#include <QTimeZone>

namespace {

//! Ein leeres QJsonObject ist kein gültiges JSON Schema. Manche Anbieter
//! lehnen den Request damit ab, statt es als "keine Parameter" zu lesen.
QJsonObject noParameters()
{
    return QJsonObject{
        {QStringLiteral("type"),       QStringLiteral("object")},
        {QStringLiteral("properties"), QJsonObject{}}
    };
}

QString settingsKey(const QString &toolName)
{
    return QStringLiteral("tools/") + toolName;
}

}

ToolRegistry::ToolRegistry(Capabilities *caps, ISystemProvider *provider,
                           ConsentGate *gate, QObject *parent)
    : QObject(parent), m_caps(caps), m_provider(provider), m_gate(gate) {}

int ToolRegistry::indexOf(const QString &name) const
{
    for (int i = 0; i < m_tools.size(); ++i)
        if (m_tools.at(i).name == name) return i;
    return -1;
}

bool ToolRegistry::contains(const QString &name) const { return indexOf(name) >= 0; }

void ToolRegistry::grantConsent(const QString &name)
{
    if (indexOf(name) < 0) return;
    m_gate->grant(name);
}

void ToolRegistry::registerTool(Tool t)
{
    // Der uebergebene Wert ist die Voreinstellung; die Entscheidung des
    // Nutzers ueberschreibt sie. Nie umgekehrt. Ausnahme: ein als kaputt
    // markiertes Tool bleibt aus, auch wenn eine alte settings.ini es noch
    // als eingeschaltet gespeichert hat.
    t.enabled = t.available && QSettings(appSettingsPath(), QSettings::IniFormat)
                    .value(settingsKey(t.name), t.enabled).toBool();
    m_tools.append(t);
}

void ToolRegistry::buildManifest()
{
    m_tools.clear();

    // --- Low: in beiden Targets, default an ---

    // Kein Systemzugriff, deshalb auch kein Provider: reines Qt. Modelle haben
    // kein Zeitgefuehl, und ohne dieses Tool raten sie das Datum.
    registerTool({"get_datetime",
                  "Liefert das aktuelle Datum, die Uhrzeit und die Zeitzone des Geräts.",
                  noParameters(),
                  ConsentGate::Low,
                  [](QVariantMap) {
                      const QDateTime now = QDateTime::currentDateTime();
                      return QVariantMap{
                          {"iso",                now.toString(Qt::ISODate)},
                          {"date",               now.date().toString(Qt::ISODate)},
                          {"time",               now.time().toString(QStringLiteral("HH:mm"))},
                          {"weekday",            QLocale::c().dayName(now.date().dayOfWeek())},
                          {"timezone",           QString::fromUtf8(now.timeZone().id())},
                          {"utc_offset_minutes", now.offsetFromUtc() / 60}
                      };
                  },
                  true});

    if (m_caps->battery()) {
        registerTool({"get_battery_status",
                      "Liefert Ladestand, Ladezustand und geschätzte Restlaufzeit.",
                      noParameters(),
                      ConsentGate::Low,
                      [this](QVariantMap){ return m_provider->batteryStatus(); },
                      true});
    }
    if (m_caps->network()) {
        registerTool({"get_network_status",
                      "Liefert Verbindungstyp, Signalstärke und Online-Status.",
                      noParameters(),
                      ConsentGate::Low,
                      [this](QVariantMap){ return m_provider->networkStatus(); },
                      true});
    }

    registerTool({"get_storage_status",
                  "Liefert freien und gesamten Speicherplatz der zugänglichen Laufwerke.",
                  noParameters(),
                  ConsentGate::Low,
                  [this](QVariantMap){ return m_provider->storageStatus(); },
                  true});

    // --- Personal: default aus, Bestätigung + Redaktion ---
    if (m_caps->contacts()) {
        registerTool({"find_contact",
                      "Sucht einen Kontakt nach Name und liefert die hinterlegten "
                      "Nummern und Adressen. Vorübergehend deaktiviert: auf "
                      "Sailfish OS ab 5.2 scheitert der Sailjail-Mount für den "
                      "privilegierten Kontakte-Store (\"can't chdir to "
                      "privileged\"), wodurch beide Targets nur eine leere "
                      "Kontaktliste sehen. Fix folgt in 0.9.2.",
                      QJsonObject{
                          {"type", "object"},
                          {"properties", QJsonObject{
                              {"query", QJsonObject{
                                  {"type",        "string"},
                                  {"description", "Name oder Namensteil"}
                              }}
                          }},
                          {"required", QJsonArray{"query"}}
                      },
                      ConsentGate::Personal,
                      [this](QVariantMap args) {
                          return m_provider->findContact(
                              args.value(QStringLiteral("query")).toString());
                      },
                      false,
                      false});
    }

    if (m_caps->calendar()) {
        registerTool({"get_upcoming_events",
                      "Liefert anstehende Kalendertermine der nächsten Tage.",
                      QJsonObject{
                          {"type", "object"},
                          {"properties", QJsonObject{
                              {"days", QJsonObject{
                                  {"type",        "integer"},
                                  {"description", "Zeitraum in Tagen ab heute"}
                              }}
                          }},
                          {"required", QJsonArray{"days"}}
                      },
                      ConsentGate::Personal,
                      [this](QVariantMap args) {
                          return m_provider->upcomingEvents(
                              args.value(QStringLiteral("days")).toInt());
                      },
                      false});
    }

    // --- Critical: nur Full-Target ---
    if (m_caps->messages()) {
        registerTool({"read_recent_messages",
                      "Liefert die jüngsten SMS-Konversationen mit letzter Nachricht.",
                      QJsonObject{
                          {"type", "object"},
                          {"properties", QJsonObject{
                              {"limit", QJsonObject{
                                  {"type",        "integer"},
                                  {"description", "Maximale Anzahl Konversationen"}
                              }}
                          }},
                          {"required", QJsonArray{"limit"}}
                      },
                      ConsentGate::Critical,
                      [this](QVariantMap args) {
                          return m_provider->recentMessages(
                              args.value(QStringLiteral("limit")).toInt());
                      },
                      false});
    }
    if (m_caps->automation()) {
        registerTool({"run_command",
                      "Führt ein Programm mit Argumenten auf dem Gerät aus und "
                      "liefert Exit-Code sowie Ausgabe zurück. Es wird kein "
                      "Terminal bereitgestellt — nur für nicht-interaktive "
                      "Einzelaufrufe geeignet. Interaktive oder laufend "
                      "aktualisierende Programme (z.B. `top` ohne `-n`) laufen "
                      "nach 15s in einen Timeout statt eine Ausgabe zu liefern; "
                      "ggf. eine nicht-interaktive Variante wählen (z.B. `top -n 1`).",
                      QJsonObject{
                          {"type", "object"},
                          {"properties", QJsonObject{
                              {"command", QJsonObject{
                                  {"type",        "string"},
                                  {"description", "Programmname oder -pfad"}
                              }},
                              {"args", QJsonObject{
                                  {"type",        "array"},
                                  {"items",       QJsonObject{{"type", "string"}}},
                                  {"description", "Argumente"}
                              }}
                          }},
                          {"required", QJsonArray{"command"}}
                      },
                      ConsentGate::Critical,
                      [this](QVariantMap args) {
                          QStringList cmdArgs;
                          for (const QVariant &a : args.value(QStringLiteral("args")).toList())
                              cmdArgs.append(a.toString());
                          return m_provider->runCommand(
                              args.value(QStringLiteral("command")).toString(), cmdArgs);
                      },
                      false});
    }

    emit toolsChanged();
}

QJsonArray ToolRegistry::toolSchema() const
{
    QJsonArray arr;
    for (int i = 0; i < m_tools.size(); ++i) {
        const Tool &t = m_tools.at(i);
        if (!t.enabled) continue;
        arr.append(QJsonObject{
            {"type", "function"},
            {"function", QJsonObject{
                {"name",        t.name},
                {"description", t.description},
                {"parameters",  t.parameterSchema}
            }}
        });
    }
    return arr;
}

QVariantMap ToolRegistry::invoke(const QString &name, const QVariantMap &args)
{
    const int i = indexOf(name);
    if (i < 0)
        return QVariantMap{{"error", "unknown_tool"}};

    const Tool &t = m_tools.at(i);
    if (!t.enabled)
        return QVariantMap{{"error", "tool_disabled"}};

    if (m_gate->requiresConfirmation(t.sensitivity) && !m_gate->isGranted(name))
        return QVariantMap{{"error", "consent_required"}};

    const QVariantMap result = t.handler(args);

    // Redaktion nur dort, wo Personenbezug ueberhaupt moeglich ist. Auf eine
    // Low-Ausgabe angewandt wuerde sie Zeitstempel und IDs zerpfluecken.
    if (t.sensitivity == ConsentGate::Low) return result;
    return m_gate->redact(result);
}

QString ToolRegistry::consentPreview(const QString &name, const QVariantMap &args) const
{
    const int i = indexOf(name);
    if (i < 0) return QString();

    QString out = m_tools.at(i).description;
    if (args.isEmpty()) return out;

    QStringList lines;
    for (auto it = args.constBegin(); it != args.constEnd(); ++it)
        lines.append(it.key() + QStringLiteral(": ") + it.value().toString());
    return out + QLatin1Char('\n') + lines.join(QLatin1Char('\n'));
}

int ToolRegistry::activeToolCount() const
{
    int n = 0;
    for (int i = 0; i < m_tools.size(); ++i)
        if (m_tools.at(i).enabled) ++n;
    return n;
}

QVariantList ToolRegistry::tools() const
{
    QVariantList out;
    for (int i = 0; i < m_tools.size(); ++i) {
        const Tool &t = m_tools.at(i);
        out.append(QVariantMap{
            {"name",         t.name},
            {"description",  t.description},
            {"sensitivity",  static_cast<int>(t.sensitivity)},
            {"enabled",      t.enabled},
            {"available",    t.available},
            {"needsConsent", m_gate->requiresConfirmation(t.sensitivity)}
        });
    }
    return out;
}

void ToolRegistry::setToolEnabled(const QString &name, bool enabled)
{
    const int i = indexOf(name);
    if (i < 0 || m_tools.at(i).enabled == enabled) return;
    if (!m_tools.at(i).available) return;

    m_tools[i].enabled = enabled;
    QSettings(appSettingsPath(), QSettings::IniFormat).setValue(settingsKey(name), enabled);

    // Eine zurueckgenommene Freischaltung nimmt die Sitzungsfreigabe mit —
    // sonst laeuft das Tool nach dem Wiedereinschalten ohne Nachfrage.
    if (!enabled) m_gate->revoke(name);

    emit toolsChanged();
}
