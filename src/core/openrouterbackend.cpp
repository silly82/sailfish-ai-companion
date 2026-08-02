#include "openrouterbackend.h"
#include "keystore.h"
#include <QNetworkRequest>
#include <QNetworkReply>

OpenRouterBackend::OpenRouterBackend(KeyStore *keys, QObject *parent)
    : ILlmBackend(parent), m_keys(keys) {}

bool OpenRouterBackend::available() const { return m_keys && m_keys->hasKey(); }

void OpenRouterBackend::chat(const QJsonArray &messages, const QJsonArray &tools)
{
    Q_UNUSED(messages) Q_UNUSED(tools)
    // TODO M1: POST /chat/completions mit "stream": true
    //   Header: Authorization: Bearer <KeyStore>
    //           HTTP-Referer, X-Title  (OpenRouter-Attribution, optional)
    //   readyRead() -> Zeilen ab "data: " parsen -> emit delta()
    //   "data: [DONE]" -> emit finished()
    emit failed(tr("Nicht implementiert (M1)"));
}

void OpenRouterBackend::handleSseChunk(const QByteArray &line) { Q_UNUSED(line) }
void OpenRouterBackend::refreshModels() { /* TODO M1 */ }

void OpenRouterBackend::cancel()
{
    if (m_reply) { m_reply->abort(); m_reply = nullptr; }
}
