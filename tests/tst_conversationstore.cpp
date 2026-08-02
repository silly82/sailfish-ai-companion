#include "tst_conversationstore.h"
#include "core/conversationstore.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

namespace {
//! Jeder Test bekommt eine eigene In-Memory-DB; sonst teilen sich die Tests
//! über den Verbindungsnamen denselben Zustand.
QString uniqueConnection()
{
    return QStringLiteral("test_") + QUuid::createUuid().toString(QUuid::Id128);
}

bool openMemory(ConversationStore &store)
{
    return store.openAt(QStringLiteral(":memory:"), uniqueConnection());
}
}

void TestConversationStore::createsSchemaOnOpen()
{
    ConversationStore store;
    QVERIFY(openMemory(store));
    QVERIFY(store.conversations().isEmpty());

    const int id = store.createConversation(QStringLiteral("Erste"));
    QVERIFY(id > 0);

    const QVariantList list = store.conversations();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.at(0).toMap().value(QStringLiteral("title")).toString(),
             QStringLiteral("Erste"));
}

void TestConversationStore::persistsMessagesPerConversation()
{
    ConversationStore store;
    QVERIFY(openMemory(store));

    const int a = store.createConversation(QStringLiteral("A"));
    const int b = store.createConversation(QStringLiteral("B"));

    store.appendMessage(a, QStringLiteral("user"), QStringLiteral("hallo a"));
    store.appendMessage(b, QStringLiteral("user"), QStringLiteral("hallo b"));

    // createConversation() hat zuletzt b geöffnet.
    QCOMPARE(store.currentConversation(), b);
    QCOMPARE(store.rowCount(), 1);
    QCOMPARE(store.data(store.index(0, 0), ConversationStore::RoleContent).toString(),
             QStringLiteral("hallo b"));

    store.loadConversation(a);
    QCOMPARE(store.rowCount(), 1);
    QCOMPARE(store.data(store.index(0, 0), ConversationStore::RoleContent).toString(),
             QStringLiteral("hallo a"));
}

void TestConversationStore::streamsDeltaIntoPendingRow()
{
    ConversationStore store;
    QVERIFY(openMemory(store));
    const int id = store.createConversation(QStringLiteral("Stream"));

    store.beginAssistantMessage(id);
    QCOMPARE(store.rowCount(), 1);
    QVERIFY(store.data(store.index(0, 0), ConversationStore::RolePending).toBool());

    QSignalSpy changed(&store, &ConversationStore::dataChanged);
    store.appendDelta(QStringLiteral("Hal"));
    store.appendDelta(QStringLiteral("lo"));
    QCOMPARE(changed.count(), 2);
    QCOMPARE(store.data(store.index(0, 0), ConversationStore::RoleContent).toString(),
             QStringLiteral("Hallo"));

    store.commitPending();
    QVERIFY(!store.data(store.index(0, 0), ConversationStore::RolePending).toBool());

    // Nach dem Festschreiben muss die Zeile aus der DB kommen, nicht aus dem Model.
    store.loadConversation(id);
    QCOMPARE(store.rowCount(), 1);
    QCOMPARE(store.data(store.index(0, 0), ConversationStore::RoleContent).toString(),
             QStringLiteral("Hallo"));
}

void TestConversationStore::discardsEmptyPendingRow()
{
    // Bricht der Stream ab, bevor ein Zeichen kam, darf keine leere
    // Assistenten-Nachricht im Verlauf landen — sie vergiftet sonst den
    // Kontext des nächsten Requests.
    ConversationStore store;
    QVERIFY(openMemory(store));
    const int id = store.createConversation(QStringLiteral("Leer"));

    store.beginAssistantMessage(id);
    store.commitPending();

    QCOMPARE(store.rowCount(), 0);
    store.loadConversation(id);
    QCOMPARE(store.rowCount(), 0);
}

void TestConversationStore::historyIsChronologicalAndCapped()
{
    ConversationStore store;
    QVERIFY(openMemory(store));
    const int id = store.createConversation(QStringLiteral("Verlauf"));

    for (int i = 0; i < 5; ++i)
        store.appendMessage(id, QStringLiteral("user"), QString::number(i));

    const QJsonArray all = store.history(id);
    QCOMPARE(all.size(), 5);
    QCOMPARE(all.at(0).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("0"));
    QCOMPARE(all.at(4).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("4"));

    // Gedeckelt wird am neuen Ende, die Reihenfolge bleibt chronologisch.
    const QJsonArray capped = store.history(id, 2);
    QCOMPARE(capped.size(), 2);
    QCOMPARE(capped.at(0).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("3"));
    QCOMPARE(capped.at(1).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("4"));
}

void TestConversationStore::deleteRemovesMessagesToo()
{
    ConversationStore store;
    QVERIFY(openMemory(store));
    const int id = store.createConversation(QStringLiteral("Weg"));
    store.appendMessage(id, QStringLiteral("user"), QStringLiteral("x"));

    store.deleteConversation(id);

    QVERIFY(store.conversations().isEmpty());
    QCOMPARE(store.rowCount(), 0);
    QCOMPARE(store.currentConversation(), -1);
    QVERIFY(store.history(id).isEmpty());
}
