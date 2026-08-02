#include "toolregistry.h"
#include "capabilities.h"
#include "consentgate.h"
#include "../platform/isystemprovider.h"

ToolRegistry::ToolRegistry(Capabilities *caps, ISystemProvider *provider,
                           ConsentGate *gate, QObject *parent)
    : QObject(parent), m_caps(caps), m_provider(provider), m_gate(gate) {}

void ToolRegistry::registerTool(Tool t)
{
    m_tools.insert(t.name, t);
}

void ToolRegistry::buildManifest()
{
    m_tools.clear();

    // --- Low: in beiden Targets, default an ---
    if (m_caps->battery()) {
        registerTool({"get_battery_status",
                      "Liefert Ladestand, Ladezustand und geschätzte Restlaufzeit.",
                      QJsonObject{},
                      Sensitivity::Low,
                      [this](QVariantMap){ return m_provider->batteryStatus(); },
                      true});
    }
    if (m_caps->network()) {
        registerTool({"get_network_status",
                      "Liefert Verbindungstyp, Signalstärke und Online-Status.",
                      QJsonObject{},
                      Sensitivity::Low,
                      [this](QVariantMap){ return m_provider->networkStatus(); },
                      true});
    }
    // TODO M2: get_storage_status, get_datetime

    // --- Personal: default aus, Bestätigung + Redaktion ---
    if (m_caps->contacts()) {
        // TODO M2: registerTool find_contact
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
    for (const Tool &t : m_tools) {
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
    if (!m_tools.contains(name))
        return QVariantMap{{"error", "unknown_tool"}};

    const Tool &t = m_tools.value(name);
    if (!t.enabled)
        return QVariantMap{{"error", "tool_disabled"}};

    // TODO M2: ConsentGate durchlaufen, bei Personal/Critical blockierend
    //          bestätigen lassen und Ergebnis durch die Redaktionsschicht
    //          schicken, bevor es an ein Cloud-Modell geht.
    return t.handler(args);
}

int ToolRegistry::activeToolCount() const
{
    int n = 0;
    for (const Tool &t : m_tools) if (t.enabled) ++n;
    return n;
}

void ToolRegistry::setToolEnabled(const QString &name, bool enabled)
{
    if (!m_tools.contains(name)) return;
    m_tools[name].enabled = enabled;
    emit toolsChanged();
}
