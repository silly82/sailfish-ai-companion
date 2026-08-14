#include "aiclient.h"
#include "conversationstore.h"
#include "keystore.h"
#include "openrouterbackend.h"
#include "toolregistry.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

namespace {

//! Nach so vielen Tool-Runden ohne Text bricht die Runde ab. Jede Runde ist
//! ein voller Request mit dem gesamten Verlauf; ein Modell in einer Schleife
//! merkt das selbst nicht, die Rechnung schon.
const int kMaxToolRounds = 4;

QString compactJson(const QVariantMap &map)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(map))
                                 .toJson(QJsonDocument::Compact));
}

}

AIClient::AIClient(KeyStore *keys, ToolRegistry *tools, ConversationStore *store,
                   QObject *parent)
    : QObject(parent)
    , m_cloud(new OpenRouterBackend(keys, this))
    , m_backend(m_cloud)
    , m_tools(tools)
    , m_store(store)
{
    connectBackend();

    connect(m_cloud, &OpenRouterBackend::modelsRefreshed,
            this, [this](const QVariantList &models) {
        m_models = models;
        emit modelsChanged();
    });

    // Nur die Modell-Kennung, nie der Key — der gehört in den KeyStore.
    m_backend->setModel(QSettings().value(QStringLiteral("model")).toString());
}

void AIClient::connectBackend()
{
    connect(m_backend, &ILlmBackend::delta,    this, &AIClient::onDelta);
    connect(m_backend, &ILlmBackend::toolCall, this, &AIClient::onToolCall);
    connect(m_backend, &ILlmBackend::finished, this, &AIClient::onFinished);
    connect(m_backend, &ILlmBackend::failed,   this, &AIClient::onFailed);
}

void AIClient::disconnectBackend()
{
    // Gezielt, nicht pauschal: an m_cloud hängt zusätzlich modelsRefreshed,
    // und die Modellliste soll einen Backend-Wechsel überleben.
    disconnect(m_backend, &ILlmBackend::delta,    this, &AIClient::onDelta);
    disconnect(m_backend, &ILlmBackend::toolCall, this, &AIClient::onToolCall);
    disconnect(m_backend, &ILlmBackend::finished, this, &AIClient::onFinished);
    disconnect(m_backend, &ILlmBackend::failed,   this, &AIClient::onFailed);
}

void AIClient::setBackend(ILlmBackend *backend)
{
    if (!backend || backend == m_backend) return;

    cancel();
    const QString id = m_backend->model();
    disconnectBackend();
    m_backend = backend;
    m_backend->setModel(id);
    connectBackend();
    emit modelChanged();
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
        emit errorOccurred(tr("No conversation open"));
        return;
    }

    m_activeConversation = conversationId;
    m_toolRounds = 0;
    m_pending.clear();
    m_awaitingConsent = false;

    m_store->appendMessage(conversationId, QStringLiteral("user"), text);
    setStreaming(true);
    startRequest();
}

void AIClient::startRequest()
{
    QJsonArray tools = m_tools->toolSchema();
    while (tools.size() > m_backend->maxTools()) tools.removeLast();

    m_store->beginAssistantMessage(m_activeConversation);
    m_backend->chat(m_store->history(m_activeConversation), tools);
}

void AIClient::onDelta(const QString &chunk)
{
    m_store->appendDelta(chunk);
}

void AIClient::onToolCall(const QString &id, const QString &name,
                          const QVariantMap &args)
{
    PendingCall call;
    // Ohne id lehnt die API die Tool-Antwort ab. Manche Anbieter lassen sie
    // im Stream weg, deshalb hier eine eigene, die pro Runde eindeutig ist.
    call.id   = id.isEmpty()
                    ? QStringLiteral("call_%1").arg(m_pending.size())
                    : id;
    call.name = name;
    call.args = args;
    m_pending.append(call);
}

void AIClient::onFinished()
{
    if (!m_pending.isEmpty()) m_store->setPendingToolCalls(pendingToolCallsJson());
    m_store->commitPending();

    if (m_pending.isEmpty()) {
        endRound();
        return;
    }
    runPendingCalls();
}

void AIClient::onFailed(const QString &message)
{
    // Bereits gestreamten Text stehen lassen — er verschwindet sonst vor
    // den Augen des Nutzers. Nur eine leere Zeile wird entfernt.
    m_store->commitPending();
    m_pending.clear();
    m_awaitingConsent = false;
    setStreaming(false);
    emit errorOccurred(message);
}

void AIClient::runPendingCalls()
{
    while (!m_pending.isEmpty()) {
        const PendingCall call = m_pending.first();
        const QVariantMap result = m_tools->invoke(call.name, call.args);

        if (result.value(QStringLiteral("error")).toString()
                == QLatin1String("consent_required")) {
            m_awaitingConsent = true;
            emit consentRequired(call.name, m_tools->consentPreview(call.name, call.args));
            return;
        }

        m_pending.removeFirst();
        appendResult(call, result);
    }

    if (++m_toolRounds > kMaxToolRounds) {
        setStreaming(false);
        emit errorOccurred(tr("Cancelled — the model kept calling tools without stopping"));
        return;
    }
    startRequest();
}

void AIClient::resolveConsent(bool approved)
{
    if (!m_awaitingConsent || m_pending.isEmpty()) return;
    m_awaitingConsent = false;

    if (approved) {
        m_tools->grantConsent(m_pending.first().name);
    } else {
        const PendingCall call = m_pending.takeFirst();
        appendResult(call, QVariantMap{{"error", "denied_by_user"}});
    }
    runPendingCalls();
}

void AIClient::appendResult(const PendingCall &call, const QVariantMap &result)
{
    m_store->appendToolResult(m_activeConversation, call.id, call.name,
                              compactJson(result));
    emit toolCallRequested(call.name, call.args);
}

QJsonArray AIClient::pendingToolCallsJson() const
{
    QJsonArray arr;
    for (int i = 0; i < m_pending.size(); ++i) {
        const PendingCall &c = m_pending.at(i);
        arr.append(QJsonObject{
            {QStringLiteral("id"),   c.id},
            {QStringLiteral("type"), QStringLiteral("function")},
            {QStringLiteral("function"), QJsonObject{
                {QStringLiteral("name"),      c.name},
                {QStringLiteral("arguments"), compactJson(c.args)}
            }}
        });
    }
    return arr;
}

void AIClient::endRound()
{
    setStreaming(false);
    emit messageComplete(m_activeConversation);
}

void AIClient::refreshModels() { m_cloud->refreshModels(); }

void AIClient::cancel()
{
    m_backend->cancel();
    m_pending.clear();
    m_awaitingConsent = false;
    m_store->commitPending();
    m_store->discardPending();
    setStreaming(false);
}
