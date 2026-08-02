#ifndef SFAI_OPENROUTERBACKEND_H
#define SFAI_OPENROUTERBACKEND_H

#include "illmbackend.h"
#include <QNetworkAccessManager>

class KeyStore;
class QNetworkReply;

//! Hauptpfad. SSE-Streaming, Modellliste zur Laufzeit von /models.
class OpenRouterBackend : public ILlmBackend
{
    Q_OBJECT
public:
    explicit OpenRouterBackend(KeyStore *keys, QObject *parent = nullptr);

    QString baseUrl() const override { return m_baseUrl; }
    bool    isLocal() const override { return false; }
    bool    available() const override;
    int     maxContextTokens() const override { return 32768; }
    int     maxTools() const override { return 32; }

    void chat(const QJsonArray &messages, const QJsonArray &tools) override;
    void cancel() override;

    //! Modelle NIE hardcoden. GET /models, cachen, nach tool_use +
    //! Kontextlänge + Preis filtern.
    void refreshModels();

signals:
    void modelsRefreshed(const QVariantList &models);

private:
    void handleSseChunk(const QByteArray &line);

    QNetworkAccessManager m_nam;
    KeyStore     *m_keys;
    QNetworkReply *m_reply = nullptr;
    QString m_baseUrl = QStringLiteral("https://openrouter.ai/api/v1");
};

#endif
