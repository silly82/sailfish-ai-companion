#ifndef SFAI_TOOLREGISTRY_H
#define SFAI_TOOLREGISTRY_H

#include <QObject>
#include <QJsonArray>
#include <QVariantMap>
#include <functional>

class Capabilities;
class ISystemProvider;
class ConsentGate;

/*!
 * Herzstück der Dual-Target-Architektur.
 *
 * Jede Systemintegration ist ein Tool mit deklariertem Capability-Bedarf.
 * Beim Start registriert sich nur, was das Target kann:
 *   Harbour  ~8 Tools
 *   Full    ~25 Tools
 * Alles darüber ist identisch — inklusive der gesamten UI.
 */
class ToolRegistry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int activeToolCount READ activeToolCount NOTIFY toolsChanged)

public:
    enum class Sensitivity {
        Low,        // Akku, Netz, Uhrzeit — kein Dialog
        Personal,   // Kontakte, Kalender  — Bestätigung + Redaktion
        Critical    // SMS, Dateien, exec  — Bestätigung, default aus
    };
    Q_ENUM(Sensitivity)

    struct Tool {
        QString                                 name;
        QString                                 description;
        QJsonObject                             parameterSchema;
        Sensitivity                             sensitivity;
        std::function<QVariantMap(QVariantMap)> handler;
        bool                                    enabled = false;
    };

    explicit ToolRegistry(Capabilities *caps,
                          ISystemProvider *provider,
                          ConsentGate *gate,
                          QObject *parent = nullptr);

    //! Baut das Tool-Set anhand der Capabilities. Einmal beim Start.
    void buildManifest();

    //! JSON-Schema für den "tools"-Parameter des Chat-Requests.
    QJsonArray toolSchema() const;

    //! Führt einen Tool-Call aus — nur nach ConsentGate-Freigabe.
    Q_INVOKABLE QVariantMap invoke(const QString &name, const QVariantMap &args);

    int activeToolCount() const;
    Q_INVOKABLE void setToolEnabled(const QString &name, bool enabled);

signals:
    void toolsChanged();
    void consentRequired(const QString &toolName, const QVariantMap &preview);

private:
    void registerTool(Tool t);

    Capabilities    *m_caps;
    ISystemProvider *m_provider;
    ConsentGate     *m_gate;
    QHash<QString, Tool> m_tools;
};

#endif
