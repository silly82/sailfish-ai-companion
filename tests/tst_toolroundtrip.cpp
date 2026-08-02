#include "tst_toolroundtrip.h"
#include "fakes.h"

#include "core/aiclient.h"
#include "core/capabilities.h"
#include "core/consentgate.h"
#include "core/conversationstore.h"
#include "core/toolregistry.h"

#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

namespace {

/*!
 * Ein vollstaendiger Roundtrip ohne Netz und ohne Geraet: FakeBackend statt
 * OpenRouter, FakeProvider statt ContextKit, In-Memory-SQLite statt Datei.
 *
 * Der KeyStore ist absichtlich nullptr — AIClient legt zwar ein
 * OpenRouterBackend an, aber setBackend() haengt es sofort ab, und ohne Key
 * meldet es ohnehin nur available() == false.
 */
struct Fixture {
    Capabilities      caps;
    FakeProvider      provider;
    ConsentGate       gate;
    ToolRegistry      registry;
    ConversationStore store;
    FakeBackend       backend;
    AIClient          client;
    int               conversation = -1;

    Fixture()
        : registry(&caps, &provider, &gate)
        , client(nullptr, &registry, &store)
    {
        registry.buildManifest();
        store.openAt(QStringLiteral(":memory:"),
                     QStringLiteral("rt_") + QUuid::createUuid().toString(QUuid::Id128));
        conversation = store.createConversation(QStringLiteral("Test"));
        client.setBackend(&backend);
        client.setModel(QStringLiteral("test/model"));
    }

    void send(const QString &text) { client.sendMessage(text, conversation); }

    //! Letzte Nachricht des n-ten Requests (0-basiert).
    QJsonObject messageAt(int request, int index) const
    {
        const QJsonArray msgs = backend.requests.at(request);
        const int i = index < 0 ? msgs.size() + index : index;
        return msgs.at(i).toObject();
    }
};

QString roleOf(const QJsonObject &o)
{
    return o.value(QStringLiteral("role")).toString();
}

}

void TestToolRoundtrip::init()
{
    QSettings().clear();
}

void TestToolRoundtrip::answersWithoutToolsInOneRound()
{
    Fixture f;
    f.backend.script.append({QStringLiteral("Hallo."), {}});

    QSignalSpy done(&f.client, &AIClient::messageComplete);
    f.send(QStringLiteral("Hi"));

    QCOMPARE(f.backend.requests.size(), 1);
    QCOMPARE(done.count(), 1);
    QVERIFY(!f.client.streaming());

    // Der Verlauf endet mit der Antwort des Modells, nicht mit der Frage.
    const QJsonArray history = f.store.history(f.conversation);
    QCOMPARE(history.size(), 2);
    QCOMPARE(roleOf(history.at(1).toObject()), QStringLiteral("assistant"));
    QCOMPARE(history.at(1).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("Hallo."));
}

void TestToolRoundtrip::executesToolAndAsksAgain()
{
    Fixture f;
    f.backend.script.append({QString(), {{"c1", "get_battery_status", {}}}});
    f.backend.script.append({QStringLiteral("Der Akku ist bei 87 Prozent."), {}});

    f.send(QStringLiteral("Wie voll ist der Akku?"));

    QCOMPARE(f.backend.requests.size(), 2);

    // Die Tool-Antwort muss unmittelbar auf den Aufruf folgen und ihn per id
    // referenzieren — sonst weist die API den Request zurueck.
    const QJsonObject result = f.messageAt(1, -1);
    QCOMPARE(roleOf(result), QStringLiteral("tool"));
    QCOMPARE(result.value(QStringLiteral("tool_call_id")).toString(),
             QStringLiteral("c1"));
    QCOMPARE(result.value(QStringLiteral("name")).toString(),
             QStringLiteral("get_battery_status"));
    QVERIFY(result.value(QStringLiteral("content")).toString().contains(QStringLiteral("87")));

    const QJsonObject call = f.messageAt(1, -2);
    QCOMPARE(roleOf(call), QStringLiteral("assistant"));
    const QJsonArray calls = call.value(QStringLiteral("tool_calls")).toArray();
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls.at(0).toObject().value(QStringLiteral("id")).toString(),
             QStringLiteral("c1"));
    QCOMPARE(calls.at(0).toObject().value(QStringLiteral("function")).toObject()
                 .value(QStringLiteral("name")).toString(),
             QStringLiteral("get_battery_status"));
}

void TestToolRoundtrip::truncatesSchemaToBackendLimit()
{
    // Lokal sind es 3-4 Tools statt 8. Gekuerzt wird von hinten, damit die
    // billigen und wichtigen Tools den Prefill ueberleben.
    Fixture f;
    f.backend.maxToolCount = 2;
    f.backend.script.append({QStringLiteral("ok"), {}});

    f.send(QStringLiteral("Hi"));

    QCOMPARE(f.backend.lastTools.size(), 2);
    QCOMPARE(f.backend.lastTools.at(0).toObject()
                 .value(QStringLiteral("function")).toObject()
                 .value(QStringLiteral("name")).toString(),
             QStringLiteral("get_datetime"));
}

void TestToolRoundtrip::stopsAfterTooManyToolRounds()
{
    // Ein Modell, das sich in Tool-Aufrufen verrennt, merkt das selbst nicht.
    Fixture f;
    for (int i = 0; i < 8; ++i)
        f.backend.script.append({QString(), {{"c1", "get_datetime", {}}}});

    QSignalSpy failed(&f.client, &AIClient::errorOccurred);
    f.send(QStringLiteral("Wie spät?"));

    QCOMPARE(failed.count(), 1);
    QCOMPARE(f.backend.requests.size(), 5);   // kMaxToolRounds + der erste Request
    QVERIFY(!f.client.streaming());
}

void TestToolRoundtrip::deniedConsentReachesTheModel()
{
    Fixture f;
    f.registry.setToolEnabled(QStringLiteral("find_contact"), true);
    f.backend.script.append({QString(),
                             {{"c1", "find_contact", {{"query", "Anna"}}}}});
    f.backend.script.append({QStringLiteral("Ich darf nicht nachsehen."), {}});

    QSignalSpy asked(&f.client, &AIClient::consentRequired);
    f.send(QStringLiteral("Wie ist Annas Nummer?"));

    // Der Roundtrip haelt an dieser Stelle an: ein Request, keine Antwort.
    QCOMPARE(asked.count(), 1);
    QCOMPARE(asked.at(0).at(0).toString(), QStringLiteral("find_contact"));
    QCOMPARE(f.backend.requests.size(), 1);
    QVERIFY(f.provider.lastQuery.isEmpty());

    f.client.resolveConsent(false);

    // Die Ablehnung geht als Ergebnis zurueck — das Modell soll sie
    // mitbekommen, statt auf eine Antwort zu warten, die nie kommt.
    QCOMPARE(f.backend.requests.size(), 2);
    const QJsonObject result = f.messageAt(1, -1);
    QCOMPARE(roleOf(result), QStringLiteral("tool"));
    QVERIFY(result.value(QStringLiteral("content")).toString()
                .contains(QStringLiteral("denied_by_user")));
    QVERIFY(f.provider.lastQuery.isEmpty());
}

void TestToolRoundtrip::grantedConsentRunsTheToolRedacted()
{
    Fixture f;
    f.registry.setToolEnabled(QStringLiteral("find_contact"), true);
    f.backend.script.append({QString(),
                             {{"c1", "find_contact", {{"query", "Anna"}}}}});
    f.backend.script.append({QStringLiteral("Ich rufe sie an."), {}});

    f.send(QStringLiteral("Ruf Anna an"));
    f.client.resolveConsent(true);

    QCOMPARE(f.provider.lastQuery, QStringLiteral("Anna"));
    QCOMPARE(f.backend.requests.size(), 2);

    const QString content =
        f.messageAt(1, -1).value(QStringLiteral("content")).toString();
    QVERIFY(content.contains(QStringLiteral("Anna Muster")));
    // Die Nummer selbst erreicht das Modell nie.
    QVERIFY(!content.contains(QStringLiteral("79 123 45 67")));
    QVERIFY(content.contains(QStringLiteral("<contact:")));

    // ... und wird erst in der Ausgabe wieder eingesetzt.
    QCOMPARE(f.gate.restore(QStringLiteral("<contact:1>")),
             QStringLiteral("+41 79 123 45 67"));
}
