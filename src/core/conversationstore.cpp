#include "conversationstore.h"

ConversationStore::ConversationStore(QObject *parent) : QAbstractListModel(parent) {}

bool ConversationStore::open()
{
    // TODO M1: DB in QStandardPaths::AppDataLocation anlegen.
    // Schema:
    //   conversations(id, title, created_at, model)
    //   messages(id, conversation_id, role, content, tool_name, created_at)
    //   INDEX messages(conversation_id, id)
    return false;
}

int  ConversationStore::createConversation(const QString &t) { Q_UNUSED(t) return -1; }
void ConversationStore::loadConversation(int id)             { Q_UNUSED(id) }
void ConversationStore::appendMessage(int c, const QString &r, const QString &t)
{ Q_UNUSED(c) Q_UNUSED(r) Q_UNUSED(t) }
void ConversationStore::appendDelta(const QString &chunk)    { Q_UNUSED(chunk) }
void ConversationStore::deleteConversation(int id)           { Q_UNUSED(id) }

int ConversationStore::rowCount(const QModelIndex &) const { return 0; }
QVariant ConversationStore::data(const QModelIndex &, int) const { return QVariant(); }

QHash<int, QByteArray> ConversationStore::roleNames() const
{
    return {{RoleId, "messageId"}, {RoleRole, "role"}, {RoleContent, "content"},
            {RoleTimestamp, "timestamp"}, {RoleToolName, "toolName"},
            {RolePending, "pending"}};
}
