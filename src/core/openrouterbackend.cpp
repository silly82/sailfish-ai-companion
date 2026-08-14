#include "openrouterbackend.h"
#include "keystore.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVariantMap>

namespace {
const char *kProvider = "openrouter";

QNetworkRequest makeRequest(const QUrl &url, const QString &key)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Authorization", QByteArray("Bearer ") + key.toUtf8());
    // OpenRouter-Attribution. Ohne diese Header greifen die Rate-Limits früher.
    req.setRawHeader("HTTP-Referer", "https://github.com/silly82/sailfish-ai-companion");
    req.setRawHeader("X-Title", "Sailfish AI Companion");
    return req;
}
}

OpenRouterBackend::OpenRouterBackend(KeyStore *keys, QObject *parent)
    : ILlmBackend(parent), m_keys(keys) {}

bool OpenRouterBackend::available() const { return m_keys && m_keys->hasKey(); }

void OpenRouterBackend::chat(const QJsonArray &messages, const QJsonArray &tools)
{
    if (!available()) {
        emit failed(tr("No API key set"));
        return;
    }
    if (m_model.isEmpty()) {
        emit failed(tr("No model selected"));
        return;
    }
    cancel();

    QJsonObject body{
        {QStringLiteral("model"),    m_model},
        {QStringLiteral("messages"), messages},
        {QStringLiteral("stream"),   true}
    };
    if (!tools.isEmpty()) body.insert(QStringLiteral("tools"), tools);

    m_sse.reset();
    m_toolCalls.clear();
    m_sawDone = false;

    const QNetworkRequest req =
        makeRequest(QUrl(m_baseUrl + QStringLiteral("/chat/completions")),
                    m_keys->key(QLatin1String(kProvider)));

    m_reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::readyRead, this, &OpenRouterBackend::readStream);
    connect(m_reply, &QNetworkReply::finished,  this, &OpenRouterBackend::finishStream);
}

void OpenRouterBackend::readStream()
{
    if (!m_reply) return;
    m_sse.feed(m_reply->readAll());

    const QVector<QByteArray> events = m_sse.takeEvents();
    for (const QByteArray &payload : events) {
        if (payload == "[DONE]") { m_sawDone = true; continue; }
        handleEvent(payload);
    }
}

void OpenRouterBackend::handleEvent(const QByteArray &payload)
{
    const QJsonObject root = QJsonDocument::fromJson(payload).object();

    // Fehler kommen mitten im Stream als reguläres Ereignis, nicht als
    // HTTP-Status — etwa wenn ein Anbieter die Antwort abbricht.
    if (root.contains(QStringLiteral("error"))) {
        const QString message = root.value(QStringLiteral("error")).toObject()
                                    .value(QStringLiteral("message")).toString();
        emit failed(message.isEmpty() ? tr("Unknown error") : message);
        return;
    }

    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) return;
    const QJsonObject deltaObj = choices.at(0).toObject()
                                     .value(QStringLiteral("delta")).toObject();

    const QString chunk = deltaObj.value(QStringLiteral("content")).toString();
    if (!chunk.isEmpty()) emit delta(chunk);

    const QJsonArray calls = deltaObj.value(QStringLiteral("tool_calls")).toArray();
    for (int i = 0; i < calls.size(); ++i) {
        const QJsonObject call = calls.at(i).toObject();
        const int index = call.value(QStringLiteral("index")).toInt(i);
        PartialToolCall &partial = m_toolCalls[index];

        const QString id = call.value(QStringLiteral("id")).toString();
        if (!id.isEmpty()) partial.id = id;

        const QJsonObject fn = call.value(QStringLiteral("function")).toObject();
        const QString name = fn.value(QStringLiteral("name")).toString();
        if (!name.isEmpty()) partial.name = name;
        partial.arguments += fn.value(QStringLiteral("arguments")).toString();
    }
}

void OpenRouterBackend::emitToolCalls()
{
    for (auto it = m_toolCalls.constBegin(); it != m_toolCalls.constEnd(); ++it) {
        if (it.value().name.isEmpty()) continue;
        const QJsonObject args =
            QJsonDocument::fromJson(it.value().arguments.toUtf8()).object();
        emit toolCall(it.value().id, it.value().name, args.toVariantMap());
    }
    m_toolCalls.clear();
}

void OpenRouterBackend::finishStream()
{
    if (!m_reply) return;
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();

    if (reply->error() == QNetworkReply::OperationCanceledError) return;

    if (reply->error() != QNetworkReply::NoError) {
        // Bei einem Fehlerstatus ist der Body JSON, kein Stream.
        const QJsonObject err = QJsonDocument::fromJson(reply->readAll()).object()
                                    .value(QStringLiteral("error")).toObject();
        const QString message = err.value(QStringLiteral("message")).toString();
        emit failed(message.isEmpty() ? reply->errorString() : message);
        return;
    }

    m_sse.feed(reply->readAll());
    const QVector<QByteArray> rest = m_sse.takeEvents();
    for (const QByteArray &payload : rest) {
        if (payload == "[DONE]") { m_sawDone = true; continue; }
        handleEvent(payload);
    }
    emitToolCalls();

    if (!m_sawDone) {
        // Sauber geschlossen, aber ohne [DONE] — die Antwort ist angeschnitten.
        // Auf Mobilfunk passiert das regelmässig.
        emit failed(tr("Connection closed early"));
        return;
    }
    emit finished();
}

void OpenRouterBackend::cancel()
{
    if (!m_reply) return;
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->abort();
    reply->deleteLater();
}

void OpenRouterBackend::refreshModels()
{
    const QNetworkRequest req =
        makeRequest(QUrl(m_baseUrl + QStringLiteral("/models")),
                    m_keys ? m_keys->key(QLatin1String(kProvider)) : QString());

    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit failed(reply->errorString());
            return;
        }
        emit modelsRefreshed(parseModelList(reply->readAll()));
    });
}

QVariantList OpenRouterBackend::parseModelList(const QByteArray &json)
{
    const QJsonArray data = QJsonDocument::fromJson(json).object()
                                .value(QStringLiteral("data")).toArray();

    QVariantList out;
    for (int i = 0; i < data.size(); ++i) {
        const QJsonObject m = data.at(i).toObject();

        // Ohne Function-Calling ist ein Modell für diese App nutzlos —
        // die halbe Architektur hängt daran.
        const QJsonArray params = m.value(QStringLiteral("supported_parameters")).toArray();
        if (!params.contains(QJsonValue(QStringLiteral("tools")))) continue;

        const int context = m.value(QStringLiteral("context_length")).toInt();
        if (context < 8192) continue;

        const QJsonObject pricing = m.value(QStringLiteral("pricing")).toObject();
        const double prompt     = pricing.value(QStringLiteral("prompt")).toString().toDouble();
        const double completion = pricing.value(QStringLiteral("completion")).toString().toDouble();

        QVariantMap row;
        row[QStringLiteral("id")]         = m.value(QStringLiteral("id")).toString();
        row[QStringLiteral("name")]       = m.value(QStringLiteral("name")).toString();
        row[QStringLiteral("context")]    = context;
        row[QStringLiteral("prompt")]     = prompt;
        row[QStringLiteral("completion")] = completion;
        row[QStringLiteral("free")]       = (prompt == 0.0 && completion == 0.0);
        out.append(row);
    }
    return out;
}
