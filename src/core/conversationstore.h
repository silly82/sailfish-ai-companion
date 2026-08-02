#ifndef SFAI_CONVERSATIONSTORE_H
#define SFAI_CONVERSATIONSTORE_H

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QJsonArray>
#include <QVariantList>
#include <QVector>

/*!
 * Chat-History als QAbstractListModel direkt für die QML-SilicaListView.
 * SQLite via QSqlDatabase (QSQLITE) — libsqlite3.so.0 und Qt5Sql sind beide
 * Harbour-erlaubt.
 *
 * Ablage: QStandardPaths::AppDataLocation. Im Harbour-Build sandboxed pro App,
 * daher kein gemeinsamer Pfad mit dem Full-Build — Migration läuft über einen
 * Export/Import via Sailfish.Pickers.
 */
class ConversationStore : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList conversations READ conversations NOTIFY conversationsChanged)
    Q_PROPERTY(int currentConversation READ currentConversation NOTIFY currentConversationChanged)

public:
    enum Roles { RoleId = Qt::UserRole + 1, RoleRole, RoleContent,
                 RoleTimestamp, RoleToolName, RolePending };

    explicit ConversationStore(QObject *parent = nullptr);
    ~ConversationStore() override;

    Q_INVOKABLE bool open();
    Q_INVOKABLE int  createConversation(const QString &title);
    Q_INVOKABLE void loadConversation(int id);
    Q_INVOKABLE void appendMessage(int conversationId, const QString &role,
                                   const QString &content);
    Q_INVOKABLE void appendDelta(const QString &chunk);   // Streaming
    Q_INVOKABLE void deleteConversation(int id);

    //! Öffnet die Zeile, in die appendDelta() schreibt. Sie bleibt als pending
    //! markiert und ungespeichert, bis commitPending() sie festschreibt.
    Q_INVOKABLE void beginAssistantMessage(int conversationId);
    Q_INVOKABLE void commitPending();
    Q_INVOKABLE void discardPending();

    //! Verlauf im OpenAI-Format für den nächsten Request.
    QJsonArray history(int conversationId, int maxMessages = 40) const;

    QVariantList conversations() const;
    int currentConversation() const { return m_currentConversation; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    //! Für die Tests: erlaubt eine eigene Datei bzw. :memory: statt
    //! QStandardPaths::AppDataLocation.
    bool openAt(const QString &path, const QString &connectionName);

signals:
    void conversationsChanged();
    void currentConversationChanged();
    void errorOccurred(const QString &message);

private:
    struct Message {
        qint64  id = -1;
        QString role;
        QString content;
        QString timestamp;
        QString toolName;
        bool    pending = false;
    };

    bool createSchema();
    void setCurrentConversation(int id);

    QSqlDatabase     m_db;
    QString          m_connectionName;
    QVector<Message> m_messages;
    int  m_currentConversation = -1;
    int  m_pendingRow = -1;
};

#endif
