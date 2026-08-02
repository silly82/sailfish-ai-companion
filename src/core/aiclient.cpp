#include "aiclient.h"
#include "conversationstore.h"
#include "keystore.h"
#include "openrouterbackend.h"
#include "toolregistry.h"

#include <QJsonArray>
#include <QSettings>

AIClient::AIClient(KeyStore *keys, ToolRegistry *tools, ConversationStore *store,
                   QObject *parent)
    : QObject(parent)
    , m_cloud(new OpenRouterBackend(keys, this))
    , m_backend(m_cloud)
    , m_tools(tools)
    , m_store(store)
{
    connect(m_cloud, &ILlmBackend::delta, this, [this](const QString &chunk) {
        m_store->appendDelta(chunk);
    });
    connect(m_cloud, &ILlmBackend::toolCall, this, &AIClient::toolCallRequested);

    connect(m_cloud, &ILlmBackend::finished, this, [this]() {
        m_store->commitPending();
        setStreaming(false);
        emit messageComplete(m_activeConversation);
    });

    connect(m_cloud, &ILlmBackend::failed, this, [this](const QString &message) {
        // Bereits gestreamten Text stehen lassen — er verschwindet sonst vor
        // den Augen des Nutzers. Nur eine leere Zeile wird entfernt.
        m_store->commitPending();
        setStreaming(false);
        emit errorOccurred(message);
    });

    connect(m_cloud, &OpenRouterBackend::modelsRefreshed,
            this, [this](const QVariantList &models) {
        m_models = models;
        emit modelsChanged();
    });

    // Nur die Modell-Kennung, nie der Key — der gehört in den KeyStore.
    m_backend->setModel(QSettings().value(QStringLiteral("model")).toString());
}

QString AIClient::model() const { return m_backend->model(); }

void AIClient::setModel(const QString &id)
{
    if (m_backend->model() == id) return;
    m_backend->setModel(id);
    QSettings().setValue(QStringLiteral("model"), id);
    emit modelChanged();
}

void AIClient::setStreaming(bool v)
{
    if (m_streaming == v) return;
    m_streaming = v;
    emit streamingChanged();
}

void AIClient::sendMessage(const QString &text, int conversationId)
{
    if (text.isEmpty() || m_streaming) return;
    if (conversationId < 0) {
        emit errorOccurred(tr("Keine Konversation geöffnet"));
        return;
    }

    m_activeConversation = conversationId;
    m_store->appendMessage(conversationId, QStringLiteral("user"), text);

    QJsonArray tools = m_tools->toolSchema();
    while (tools.size() > m_backend->maxTools()) tools.removeLast();

    m_store->beginAssistantMessage(conversationId);
    setStreaming(true);
    m_backend->chat(m_store->history(conversationId), tools);
}

void AIClient::refreshModels() { m_cloud->refreshModels(); }

void AIClient::cancel()
{
    m_backend->cancel();
    m_store->commitPending();
    m_store->discardPending();
    setStreaming(false);
}
