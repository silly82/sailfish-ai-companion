#ifndef SFAI_CONVERSATIONSTORE_H
#define SFAI_CONVERSATIONSTORE_H

#include <QAbstractListModel>
#include <QSqlDatabase>

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
public:
    enum Roles { RoleId = Qt::UserRole + 1, RoleRole, RoleContent,
                 RoleTimestamp, RoleToolName, RolePending };

    explicit ConversationStore(QObject *parent = nullptr);

    Q_INVOKABLE bool open();
    Q_INVOKABLE int  createConversation(const QString &title);
    Q_INVOKABLE void loadConversation(int id);
    Q_INVOKABLE void appendMessage(int conversationId, const QString &role,
                                   const QString &content);
    Q_INVOKABLE void appendDelta(const QString &chunk);   // Streaming
    Q_INVOKABLE void deleteConversation(int id);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QSqlDatabase m_db;
    int m_currentConversation = -1;
};

#endif
