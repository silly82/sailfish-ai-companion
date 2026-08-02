#ifndef SFAI_AICLIENT_H
#define SFAI_AICLIENT_H

#include <QObject>
#include <QVariantList>

class ILlmBackend;
class OpenRouterBackend;
class ConversationStore;
class ToolRegistry;
class KeyStore;

/*!
 * Orchestriert eine Chat-Runde. Netzwerk macht ausschliesslich das Backend —
 * der Unterschied zwischen Cloud und lokaler Inference ist nur die Base-URL,
 * deshalb hat AIClient selbst keinen QNetworkAccessManager.
 *
 * Ablauf einer Runde:
 *   Nutzernachricht speichern -> Verlauf + Tool-Schema an das Backend ->
 *   Deltas inkrementell in den Store -> Antwort festschreiben.
 *
 * Modelle NICHT hardcoden — die Liste kommt zur Laufzeit von /models,
 * gefiltert nach tool_use, Kontextlänge und Preis.
 */
class AIClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool         streaming READ streaming NOTIFY streamingChanged)
    Q_PROPERTY(QString      model     READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QVariantList models    READ models NOTIFY modelsChanged)

public:
    AIClient(KeyStore *keys, ToolRegistry *tools, ConversationStore *store,
             QObject *parent = nullptr);

    Q_INVOKABLE void sendMessage(const QString &text, int conversationId);
    Q_INVOKABLE void refreshModels();
    Q_INVOKABLE void cancel();

    bool         streaming() const { return m_streaming; }
    QString      model() const;
    void         setModel(const QString &id);
    QVariantList models() const { return m_models; }

signals:
    void messageComplete(int conversationId);
    void toolCallRequested(const QString &name, const QVariantMap &args);
    void modelsChanged();
    void errorOccurred(const QString &message);
    void streamingChanged();
    void modelChanged();

private:
    void setStreaming(bool v);

    OpenRouterBackend *m_cloud;
    ILlmBackend       *m_backend;
    ToolRegistry      *m_tools;
    ConversationStore *m_store;

    QVariantList m_models;
    int  m_activeConversation = -1;
    bool m_streaming = false;
};

#endif
