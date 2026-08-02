#ifndef SFAI_OPENROUTERBACKEND_H
#define SFAI_OPENROUTERBACKEND_H

#include "illmbackend.h"
#include "sseparser.h"
#include <QNetworkAccessManager>
#include <QMap>

class KeyStore;
class QNetworkReply;

//! Hauptpfad. SSE-Streaming, Modellliste zur Laufzeit von /models.
class OpenRouterBackend : public ILlmBackend
{
    Q_OBJECT
public:
    explicit OpenRouterBackend(KeyStore *keys, QObject *parent = nullptr);

    QString baseUrl() const override { return m_baseUrl; }
    void    setBaseUrl(const QString &url) { m_baseUrl = url; }
    bool    isLocal() const override { return false; }
    bool    available() const override;
    int     maxContextTokens() const override { return 32768; }
    int     maxTools() const override { return 32; }

    void chat(const QJsonArray &messages, const QJsonArray &tools) override;
    void cancel() override;

    //! Modelle NIE hardcoden. GET /models, cachen, nach tool_use +
    //! Kontextlänge + Preis filtern.
    void refreshModels();

    //! Wertet die /models-Antwort aus. Statisch und ohne QtNetwork, damit
    //! sich die Auswahlregeln ohne Netzzugriff testen lassen.
    static QVariantList parseModelList(const QByteArray &json);

signals:
    void modelsRefreshed(const QVariantList &models);

private:
    struct PartialToolCall {
        QString id;
        QString name;
        QString arguments;   // trifft über mehrere Deltas fragmentiert ein
    };

    void readStream();
    void handleEvent(const QByteArray &payload);
    void finishStream();
    void emitToolCalls();

    QNetworkAccessManager m_nam;
    KeyStore      *m_keys;
    QNetworkReply *m_reply = nullptr;
    QString m_baseUrl = QStringLiteral("https://openrouter.ai/api/v1");

    SseParser m_sse;
    QMap<int, PartialToolCall> m_toolCalls;
    bool m_sawDone = false;
};

#endif
