#ifndef SFAI_TOOLREGISTRY_H
#define SFAI_TOOLREGISTRY_H

#include "consentgate.h"

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>
#include <QVector>
#include <functional>

class Capabilities;
class ISystemProvider;

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
    Q_PROPERTY(QVariantList tools READ tools NOTIFY toolsChanged)

public:
    typedef ConsentGate::Sensitivity Sensitivity;

    struct Tool {
        QString                                 name;
        QString                                 description;
        QJsonObject                             parameterSchema;
        Sensitivity                             sensitivity;
        std::function<QVariantMap(QVariantMap)> handler;
        bool                                    enabled = false;
        //! false = kann vom Nutzer nicht (mehr) eingeschaltet werden, UI graut
        //! den Umschalter aus. Fuer Tools, die aktuell bekanntermassen kaputt
        //! sind, statt sie ganz zu entfernen.
        bool                                    available = true;
    };

    explicit ToolRegistry(Capabilities *caps,
                          ISystemProvider *provider,
                          ConsentGate *gate,
                          QObject *parent = nullptr);

    //! Baut das Tool-Set anhand der Capabilities. Einmal beim Start.
    void buildManifest();

    //! JSON-Schema für den "tools"-Parameter des Chat-Requests.
    //! In Registrierungsreihenfolge — AIClient kürzt von hinten, wenn das
    //! Backend weniger Tools verkraftet, also stehen die wichtigen vorn.
    QJsonArray toolSchema() const;

    //! Führt einen Tool-Call aus. Prüft Freischaltung und Consent selbst und
    //! liefert im Zweifel eine Fehlerkarte statt Daten:
    //!   unknown_tool | tool_disabled | consent_required
    Q_INVOKABLE QVariantMap invoke(const QString &name, const QVariantMap &args);

    //! Wortlaut für den Bestätigungsdialog — was dieses Tool herausgeben will.
    Q_INVOKABLE QString consentPreview(const QString &name,
                                       const QVariantMap &args) const;

    //! Freigabe aus dem Bestätigungsdialog. Geht an die Schleuse durch — die
    //! Registry entscheidet nicht selbst über Consent, sie fragt nur.
    Q_INVOKABLE void grantConsent(const QString &name);

    Q_INVOKABLE bool contains(const QString &name) const;
    Q_INVOKABLE void setToolEnabled(const QString &name, bool enabled);

    int          activeToolCount() const;
    QVariantList tools() const;

signals:
    void toolsChanged();

private:
    void registerTool(Tool t);
    int  indexOf(const QString &name) const;

    Capabilities    *m_caps;
    ISystemProvider *m_provider;
    ConsentGate     *m_gate;
    QVector<Tool>    m_tools;   // Registrierungsreihenfolge ist bedeutungstragend
};

#endif
