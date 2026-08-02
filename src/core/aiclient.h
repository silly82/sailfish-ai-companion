#ifndef SFAI_AICLIENT_H
#define SFAI_AICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>

class ToolRegistry;
class KeyStore;

/*!
 * OpenAI-kompatibler Chat-Client (OpenRouter als Default-Backend).
 *
 * Wichtig: Modelle NICHT hardcoden. Beim ersten Start /api/v1/models abrufen,
 * cachen, nach Kontextlänge + Preis + tool_use-Fähigkeit filtern.
 *
 * Antworten werden per SSE gestreamt und inkrementell ins Model geschrieben —
 * ohne das fühlt sich die App auf Mobilfunk kaputt an.
 */
class AIClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool    streaming READ streaming NOTIFY streamingChanged)
    Q_PROPERTY(QString baseUrl   READ baseUrl   WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString model     READ model     WRITE setModel   NOTIFY modelChanged)

public:
    explicit AIClient(KeyStore *keys, ToolRegistry *tools, QObject *parent = nullptr);

    Q_INVOKABLE void sendMessage(const QString &text, int conversationId);
    Q_INVOKABLE void refreshModels();
    Q_INVOKABLE void cancel();

    bool    streaming() const { return m_streaming; }
    QString baseUrl()   const { return m_baseUrl; }
    QString model()     const { return m_model; }
    void setBaseUrl(const QString &u);
    void setModel(const QString &m);

signals:
    void deltaReceived(int conversationId, const QString &chunk);
    void messageComplete(int conversationId);
    void toolCallRequested(const QString &name, const QVariantMap &args);
    void modelsRefreshed(const QVariantList &models);
    void errorOccurred(const QString &message);
    void streamingChanged();
    void baseUrlChanged();
    void modelChanged();

private:
    QNetworkAccessManager  m_nam;
    KeyStore              *m_keys;
    ToolRegistry          *m_tools;
    QNetworkReply         *m_current = nullptr;
    QString m_baseUrl = QStringLiteral("https://openrouter.ai/api/v1");
    QString m_model;
    bool    m_streaming = false;
};

#endif
