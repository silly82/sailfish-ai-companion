#ifndef SFAI_AICLIENT_H
#define SFAI_AICLIENT_H

#include <QObject>
#include <QJsonArray>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

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
 * Fordert das Modell dabei Tools an, folgt eine weitere Runde: Tool ausführen,
 * Ergebnis als Nachricht mit Rolle "tool" anhängen, erneut fragen. Das läuft
 * gedeckelt — ein Modell, das sich in Tool-Aufrufen verrennt, kostet Geld und
 * auf dem Gerät Akku.
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

    //! Antwort auf consentRequired. Ohne Freigabe bekommt das Modell eine
    //! Fehlerkarte statt der Daten — es soll die Ablehnung mitbekommen,
    //! statt auf eine Antwort zu warten, die nie kommt.
    Q_INVOKABLE void resolveConsent(bool approved);

    //! Wechselt das aktive Backend. Ab M5 zeigt das auf LocalServerBackend;
    //! in den Tests hängt hier ein Backend ohne Netzzugriff.
    void setBackend(ILlmBackend *backend);

    bool         streaming() const { return m_streaming; }
    QString      model() const;
    void         setModel(const QString &id);
    QVariantList models() const { return m_models; }

signals:
    void messageComplete(int conversationId);
    void toolCallRequested(const QString &name, const QVariantMap &args);
    void consentRequired(const QString &toolName, const QString &preview);
    void modelsChanged();
    void errorOccurred(const QString &message);
    void streamingChanged();
    void modelChanged();

private:
    struct PendingCall {
        QString     id;
        QString     name;
        QVariantMap args;
    };

    void onDelta(const QString &chunk);
    void onToolCall(const QString &id, const QString &name, const QVariantMap &args);
    void onFinished();
    void onFailed(const QString &message);

    void connectBackend();
    void disconnectBackend();
    void startRequest();
    void runPendingCalls();
    void appendResult(const PendingCall &call, const QVariantMap &result);
    void endRound();
    void setStreaming(bool v);

    QJsonArray pendingToolCallsJson() const;

    OpenRouterBackend *m_cloud;
    ILlmBackend       *m_backend;
    ToolRegistry      *m_tools;
    ConversationStore *m_store;

    QVariantList         m_models;
    QVector<PendingCall> m_pending;
    int  m_activeConversation = -1;
    int  m_toolRounds = 0;
    bool m_awaitingConsent = false;
    bool m_streaming = false;
};

#endif
