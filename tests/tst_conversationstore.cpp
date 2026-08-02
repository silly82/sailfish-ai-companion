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

namespace {
//! Ein Aufruf, wie ihn AIClient an die offene Assistenten-Zeile haengt.
QJsonArray oneCall(const QString &id, const QString &name)
{
    return QJsonArray{QJsonObject{
        {QStringLiteral("id"),   id},
        {QStringLiteral("type"), QStringLiteral("function")},
        {QStringLiteral("function"), QJsonObject{
            {QStringLiteral("name"),      name},
            {QStringLiteral("arguments"), QStringLiteral("{}")}
        }}
    }};
}

//! Eine vollstaendige Tool-Runde: Aufruf und Ergebnis.
void appendToolRound(ConversationStore &store, int id, const QString &callId,
                     const QString &tool, const QString &text)
{
    store.beginAssistantMessage(id);
    if (!text.isEmpty()) store.appendDelta(text);
    store.setPendingToolCalls(oneCall(callId, tool));
    store.commitPending();
    store.appendToolResult(id, callId, tool, QStringLiteral("{\"ok\":true}"));
}
}

void TestConversationStore::keepsToolCallsWithTheirMessage()
{
    ConversationStore store;
    QVERIFY(openMemory(store));
    const int id = store.createConversation(QStringLiteral("Tools"));

    store.appendMessage(id, QStringLiteral("user"), QStringLiteral("wie voll?"));
    appendToolRound(store, id, QStringLiteral("c1"),
                    QStringLiteral("get_battery_status"), QStringLiteral("Moment"));

    const QJsonArray history = store.history(id);
    QCOMPARE(history.size(), 3);

    const QJsonObject call = history.at(1).toObject();
    QCOMPARE(call.value(QStringLiteral("role")).toString(), QStringLiteral("assistant"));
    QCOMPARE(call.value(QStringLiteral("content")).toString(), QStringLiteral("Moment"));
    QCOMPARE(call.value(QStringLiteral("tool_calls")).toArray().size(), 1);

    const QJsonObject result = history.at(2).toObject();
    QCOMPARE(result.value(QStringLiteral("role")).toString(), QStringLiteral("tool"));
    QCOMPARE(result.value(QStringLiteral("tool_call_id")).toString(), QStringLiteral("c1"));
    QCOMPARE(result.value(QStringLiteral("name")).toString(),
             QStringLiteral("get_battery_status"));
    // Eine Nachricht mit Rolle "tool" darf kein tool_calls-Feld tragen.
    QVERIFY(!result.contains(QStringLiteral("tool_calls")));
}

void TestConversationStore::keepsMessageThatOnlyCarriesToolCalls()
{
    // Text und Tool-Calls schliessen sich nicht aus, aber der Normalfall ist
    // eine Antwort ganz ohne Text. Sie darf nicht als leer verworfen werden.
    ConversationStore store;
    QVERIFY(openMemory(store));
    const int id = store.createConversation(QStringLiteral("Stumm"));

    store.beginAssistantMessage(id);
    store.setPendingToolCalls(oneCall(QStringLiteral("c1"),
                                      QStringLiteral("get_datetime")));
    store.commitPending();

    store.loadConversation(id);
    QCOMPARE(store.rowCount(), 1);
    QVERIFY(!store.data(store.index(0, 0), ConversationStore::RolePending).toBool());
}

void TestConversationStore::trimsHalfToolRoundsFromWindow()
{
    ConversationStore store;
    QVERIFY(openMemory(store));
    const int id = store.createConversation(QStringLiteral("Fenster"));

    store.appendMessage(id, QStringLiteral("user"), QStringLiteral("frage"));
    appendToolRound(store, id, QStringLiteral("c1"),
                    QStringLiteral("get_datetime"), QString());
    store.appendMessage(id, QStringLiteral("assistant"), QStringLiteral("fertig"));

    // Das Fenster faengt mitten in der Runde an: die Tool-Antwort ohne ihren
    // Aufruf muss vorne wegfallen.
    const QJsonArray cut = store.history(id, 2);
    QCOMPARE(cut.size(), 1);
    QCOMPARE(cut.at(0).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("fertig"));

    // Passt die Runde ganz hinein, bleibt sie vollstaendig.
    QCOMPARE(store.history(id, 3).size(), 3);

    // Und ein Aufruf ohne Ergebnis faellt hinten weg — so sieht der Verlauf
    // aus, solange der Tool-Aufruf noch laeuft.
    const int open = store.createConversation(QStringLiteral("Offen"));
    store.appendMessage(open, QStringLiteral("user"), QStringLiteral("frage"));
    store.beginAssistantMessage(open);
    store.setPendingToolCalls(oneCall(QStringLiteral("c9"),
                                      QStringLiteral("get_datetime")));
    store.commitPending();

    const QJsonArray pending = store.history(open);
    QCOMPARE(pending.size(), 1);
    QCOMPARE(pending.at(0).toObject().value(QStringLiteral("role")).toString(),
             QStringLiteral("user"));
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
