#ifndef SFAI_TST_CONVERSATIONSTORE_H
#define SFAI_TST_CONVERSATIONSTORE_H

#include <QObject>

class TestConversationStore : public QObject
{
    Q_OBJECT
private slots:
    void createsSchemaOnOpen();
    void persistsMessagesPerConversation();
    void streamsDeltaIntoPendingRow();
    void discardsEmptyPendingRow();
    void historyIsChronologicalAndCapped();
    void keepsToolCallsWithTheirMessage();
    void keepsMessageThatOnlyCarriesToolCalls();
    void trimsHalfToolRoundsFromWindow();
    void deleteRemovesMessagesToo();
};

#endif
