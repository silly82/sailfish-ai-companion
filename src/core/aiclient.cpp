#include "aiclient.h"
#include "keystore.h"
#include "toolregistry.h"
#include <QNetworkRequest>
#include <QNetworkReply>

AIClient::AIClient(KeyStore *keys, ToolRegistry *tools, QObject *parent)
    : QObject(parent), m_keys(keys), m_tools(tools) {}

void AIClient::setBaseUrl(const QString &u)
{ if (m_baseUrl == u) return; m_baseUrl = u; emit baseUrlChanged(); }

void AIClient::setModel(const QString &m)
{ if (m_model == m) return; m_model = m; emit modelChanged(); }

void AIClient::sendMessage(const QString &text, int conversationId)
{
    Q_UNUSED(text) Q_UNUSED(conversationId)
    // TODO M1: POST {baseUrl}/chat/completions, "stream": true
    //   Header: Authorization: Bearer <key aus KeyStore>
    //           HTTP-Referer / X-Title  (OpenRouter-Attribution, optional)
    //   Body:   messages[] aus ConversationStore + tools aus ToolRegistry
    //   SSE-Parsing: readyRead() -> "data: " Zeilen -> deltaReceived()
    emit errorOccurred(tr("Nicht implementiert (M1)"));
}

void AIClient::refreshModels()
{
    // TODO M1: GET {baseUrl}/models, cachen, nach tool_use + Kontext filtern
}

void AIClient::cancel()
{
    if (m_current) { m_current->abort(); m_current = nullptr; }
    if (m_streaming) { m_streaming = false; emit streamingChanged(); }
}
