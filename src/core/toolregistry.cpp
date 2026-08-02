#include "toolregistry.h"
#include "capabilities.h"
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
    // Nutzers ueberschreibt sie. Nie umgekehrt.
    t.enabled = QSettings().value(settingsKey(t.name), t.enabled).toBool();
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
                      "Nummern und Adressen.",
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
                      false});
    }

    // --- Critical: nur Full-Target ---
    if (m_caps->messages()) {
        // TODO M4: registerTool read_recent_messages
    }
    if (m_caps->automation()) {
        // TODO M4: registerTool run_command — höchste Sensitivity
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
            {"needsConsent", m_gate->requiresConfirmation(t.sensitivity)}
        });
    }
    return out;
}

void ToolRegistry::setToolEnabled(const QString &name, bool enabled)
{
    const int i = indexOf(name);
    if (i < 0 || m_tools.at(i).enabled == enabled) return;

    m_tools[i].enabled = enabled;
    QSettings().setValue(settingsKey(name), enabled);

    // Eine zurueckgenommene Freischaltung nimmt die Sitzungsfreigabe mit —
    // sonst laeuft das Tool nach dem Wiedereinschalten ohne Nachfrage.
    if (!enabled) m_gate->revoke(name);

    emit toolsChanged();
}
