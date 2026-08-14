#include "conversationstore.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QVariantMap>

namespace {
QString nowIso() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODate); }
}

ConversationStore::ConversationStore(QObject *parent) : QAbstractListModel(parent) {}

ConversationStore::~ConversationStore()
{
    if (m_connectionName.isEmpty()) return;
    if (m_db.isOpen()) m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool ConversationStore::open()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty() || !QDir().mkpath(dir)) {
        emit errorOccurred(tr("Data folder not writable"));
        return false;
    }
    return openAt(dir + QStringLiteral("/history.db"), QStringLiteral("sfai_history"));
}

bool ConversationStore::openAt(const QString &path, const QString &connectionName)
{
    m_connectionName = connectionName;
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        emit errorOccurred(m_db.lastError().text());
        return false;
    }

    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));

    if (!createSchema()) return false;
    emit conversationsChanged();
    return true;
}

bool ConversationStore::createSchema()
{
    QSqlQuery q(m_db);

    const char *statements[] = {
        "CREATE TABLE IF NOT EXISTS conversations ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  title      TEXT NOT NULL,"
        "  created_at TEXT NOT NULL,"
        "  model      TEXT)",

        "CREATE TABLE IF NOT EXISTS messages ("
        "  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  conversation_id INTEGER NOT NULL REFERENCES conversations(id) ON DELETE CASCADE,"
        "  role            TEXT NOT NULL,"
        "  content         TEXT NOT NULL,"
        "  tool_name       TEXT,"
        "  tool_calls      TEXT,"
        "  tool_call_id    TEXT,"
        "  created_at      TEXT NOT NULL)",

        ("CREATE INDEX IF NOT EXISTS idx_messages_conversation"
         "  ON messages(conversation_id, id)")
    };

    for (const char *sql : statements) {
        if (!q.exec(QLatin1String(sql))) {
            emit errorOccurred(q.lastError().text());
            return false;
        }
    }

    // Bestandsdatenbanken aus M1 kennen die Tool-Spalten noch nicht.
    const QSqlRecord columns = m_db.record(QStringLiteral("messages"));
    const char *added[] = { "tool_calls", "tool_call_id" };
    for (const char *column : added) {
        if (columns.indexOf(QLatin1String(column)) >= 0) continue;
        if (!q.exec(QStringLiteral("ALTER TABLE messages ADD COLUMN %1 TEXT")
                        .arg(QLatin1String(column)))) {
            emit errorOccurred(q.lastError().text());
            return false;
        }
    }
    return true;
}

qint64 ConversationStore::insertMessage(int conversationId, const Message &m)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO messages (conversation_id, role, content, tool_name,"
        "                      tool_calls, tool_call_id, created_at)"
        " VALUES (?, ?, ?, ?, ?, ?, ?)"));
    q.addBindValue(conversationId);
    q.addBindValue(m.role);
    // Ein default-konstruierter QString ist null, nicht bloss leer — QSQLITE
    // bindet ihn als NULL, und daran scheitert die NOT-NULL-Spalte. Trifft
    // genau die Assistant-Nachricht, die nur Tool-Calls traegt und keinen Text.
    q.addBindValue(m.content.isNull() ? QString::fromLatin1("") : m.content);
    q.addBindValue(m.toolName.isEmpty()   ? QVariant() : QVariant(m.toolName));
    q.addBindValue(m.toolCalls.isEmpty()  ? QVariant() : QVariant(m.toolCalls));
    q.addBindValue(m.toolCallId.isEmpty() ? QVariant() : QVariant(m.toolCallId));
    q.addBindValue(m.timestamp);
    if (!q.exec()) {
        emit errorOccurred(q.lastError().text());
        return -1;
    }
    return q.lastInsertId().toLongLong();
}

int ConversationStore::createConversation(const QString &title)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO conversations (title, created_at) VALUES (?, ?)"));
    q.addBindValue(title.isEmpty() ? tr("New conversation") : title);
    q.addBindValue(nowIso());
    if (!q.exec()) {
        emit errorOccurred(q.lastError().text());
        return -1;
    }

    const int id = q.lastInsertId().toInt();
    emit conversationsChanged();
    loadConversation(id);
    return id;
}

void ConversationStore::loadConversation(int id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT id, role, content, tool_name, tool_calls, tool_call_id, created_at"
        "  FROM messages WHERE conversation_id = ? ORDER BY id"));
    q.addBindValue(id);
    if (!q.exec()) {
        emit errorOccurred(q.lastError().text());
        return;
    }

    beginResetModel();
    m_messages.clear();
    m_pendingRow = -1;
    while (q.next()) {
        Message m;
        m.id         = q.value(0).toLongLong();
        m.role       = q.value(1).toString();
        m.content    = q.value(2).toString();
        m.toolName   = q.value(3).toString();
        m.toolCalls  = q.value(4).toString();
        m.toolCallId = q.value(5).toString();
        m.timestamp  = q.value(6).toString();
        m_messages.append(m);
    }
    endResetModel();

    setCurrentConversation(id);
}

void ConversationStore::setCurrentConversation(int id)
{
    if (m_currentConversation == id) return;
    m_currentConversation = id;
    emit currentConversationChanged();
}

void ConversationStore::appendMessage(int conversationId, const QString &role,
                                      const QString &content)
{
    Message m;
    m.role      = role;
    m.content   = content;
    m.timestamp = nowIso();

    m.id = insertMessage(conversationId, m);
    if (m.id < 0) return;
    if (conversationId != m_currentConversation) return;

    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(m);
    endInsertRows();
    emit conversationsChanged();
}

void ConversationStore::appendToolResult(int conversationId,
                                         const QString &toolCallId,
                                         const QString &toolName,
                                         const QString &content)
{
    Message m;
    m.role       = QStringLiteral("tool");
    m.content    = content;
    m.toolName   = toolName;
    m.toolCallId = toolCallId;
    m.timestamp  = nowIso();

    m.id = insertMessage(conversationId, m);
    if (m.id < 0) return;
    if (conversationId != m_currentConversation) return;

    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(m);
    endInsertRows();
    emit conversationsChanged();
}

void ConversationStore::beginAssistantMessage(int conversationId)
{
    if (conversationId != m_currentConversation) loadConversation(conversationId);
    if (m_pendingRow >= 0) discardPending();

    Message m;
    m.role      = QStringLiteral("assistant");
    m.timestamp = nowIso();
    m.pending   = true;

    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(m);
    endInsertRows();
    m_pendingRow = m_messages.size() - 1;
}

void ConversationStore::appendDelta(const QString &chunk)
{
    if (m_pendingRow < 0 || m_pendingRow >= m_messages.size()) return;
    m_messages[m_pendingRow].content += chunk;
    const QModelIndex idx = index(m_pendingRow, 0);
    emit dataChanged(idx, idx, QVector<int>() << RoleContent);
}

void ConversationStore::setPendingToolCalls(const QJsonArray &calls)
{
    if (m_pendingRow < 0 || m_pendingRow >= m_messages.size()) return;
    m_messages[m_pendingRow].toolCalls =
        QString::fromUtf8(QJsonDocument(calls).toJson(QJsonDocument::Compact));
}

void ConversationStore::commitPending()
{
    if (m_pendingRow < 0 || m_pendingRow >= m_messages.size()) return;

    // Eine leere Antwort ist Muell im Kontext der naechsten Runde. Eine
    // Nachricht, die nur Tool-Calls traegt, ist dagegen der Normalfall.
    const Message &pending = m_messages.at(m_pendingRow);
    if (pending.content.isEmpty() && pending.toolCalls.isEmpty()) {
        discardPending();
        return;
    }

    Message &m = m_messages[m_pendingRow];
    const qint64 id = insertMessage(m_currentConversation, m);
    if (id < 0) return;

    m.id      = id;
    m.pending = false;

    const QModelIndex idx = index(m_pendingRow, 0);
    emit dataChanged(idx, idx, QVector<int>() << RoleId << RolePending);
    m_pendingRow = -1;
    emit conversationsChanged();
}

void ConversationStore::discardPending()
{
    if (m_pendingRow < 0 || m_pendingRow >= m_messages.size()) return;
    beginRemoveRows(QModelIndex(), m_pendingRow, m_pendingRow);
    m_messages.remove(m_pendingRow);
    endRemoveRows();
    m_pendingRow = -1;
}

void ConversationStore::deleteConversation(int id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM messages WHERE conversation_id = ?"));
    q.addBindValue(id);
    q.exec();

    q.prepare(QStringLiteral("DELETE FROM conversations WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        emit errorOccurred(q.lastError().text());
        return;
    }

    if (id == m_currentConversation) {
        beginResetModel();
        m_messages.clear();
        m_pendingRow = -1;
        endResetModel();
        setCurrentConversation(-1);
    }
    emit conversationsChanged();
}

QJsonArray ConversationStore::history(int conversationId, int maxMessages) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT role, content, tool_name, tool_calls, tool_call_id FROM ("
        "  SELECT id, role, content, tool_name, tool_calls, tool_call_id"
        "    FROM messages WHERE conversation_id = ?"
        "  ORDER BY id DESC LIMIT ?) ORDER BY id"));
    q.addBindValue(conversationId);
    q.addBindValue(maxMessages);

    QJsonArray arr;
    if (!q.exec()) return arr;
    while (q.next()) {
        const QString role       = q.value(0).toString();
        const QString toolCalls  = q.value(3).toString();
        const QString toolCallId = q.value(4).toString();

        QJsonObject o{{QStringLiteral("role"),    role},
                      {QStringLiteral("content"), q.value(1).toString()}};
        if (!toolCalls.isEmpty()) {
            o.insert(QStringLiteral("tool_calls"),
                     QJsonDocument::fromJson(toolCalls.toUtf8()).array());
        }
        if (role == QLatin1String("tool")) {
            o.insert(QStringLiteral("tool_call_id"), toolCallId);
            const QString name = q.value(2).toString();
            if (!name.isEmpty()) o.insert(QStringLiteral("name"), name);
        }
        arr.append(o);
    }

    // Das Fenster darf keine halbe Tool-Runde erwischen. Vorne eine
    // tool-Nachricht ohne ihren Aufruf und hinten ein Aufruf ohne Ergebnis
    // beantwortet die API beide mit einem Fehler statt mit einer Antwort.
    while (!arr.isEmpty()
           && arr.first().toObject().value(QStringLiteral("role")).toString()
                  == QLatin1String("tool")) {
        arr.removeFirst();
    }
    while (!arr.isEmpty()
           && arr.last().toObject().contains(QStringLiteral("tool_calls"))) {
        arr.removeLast();
    }
    return arr;
}

QVariantList ConversationStore::conversations() const
{
    QSqlQuery q(m_db);
    const bool ok = q.exec(QStringLiteral(
        "SELECT c.id, c.title, c.created_at,"
        "       (SELECT COUNT(*) FROM messages m WHERE m.conversation_id = c.id),"
        "       (SELECT m.content FROM messages m WHERE m.conversation_id = c.id"
        "        ORDER BY m.id DESC LIMIT 1)"
        "  FROM conversations c ORDER BY c.id DESC"));

    QVariantList out;
    if (!ok) return out;
    while (q.next()) {
        QVariantMap row;
        row[QStringLiteral("conversationId")] = q.value(0).toInt();
        row[QStringLiteral("title")]          = q.value(1).toString();
        row[QStringLiteral("createdAt")]      = q.value(2).toString();
        row[QStringLiteral("messageCount")]   = q.value(3).toInt();
        // simplified() collapses newlines/runs of whitespace to single spaces —
        // the preview must fit MainPage's one-line list delegate, and a raw
        // multi-line message would otherwise overflow the fixed row height
        // and visually overlap the next entry.
        row[QStringLiteral("preview")]        = q.value(4).toString().simplified();
        out.append(row);
    }
    return out;
}

int ConversationStore::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_messages.size();
}

QVariant ConversationStore::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_messages.size()) return QVariant();
    const Message &m = m_messages.at(index.row());

    switch (role) {
    case RoleId:        return static_cast<qlonglong>(m.id);
    case RoleRole:      return m.role;
    case RoleContent:   return m.content;
    case RoleTimestamp: return m.timestamp;
    case RoleToolName:  return m.toolName;
    case RolePending:   return m.pending;
    default:            return QVariant();
    }
}

QHash<int, QByteArray> ConversationStore::roleNames() const
{
    return {{RoleId, "messageId"}, {RoleRole, "role"}, {RoleContent, "content"},
            {RoleTimestamp, "timestamp"}, {RoleToolName, "toolName"},
            {RolePending, "pending"}};
}
