#include "tst_sseparser.h"
#include "core/sseparser.h"

#include <QTest>

void TestSseParser::splitsCompleteEvents()
{
    SseParser p;
    p.feed("data: {\"a\":1}\n\ndata: [DONE]\n\n");

    const QVector<QByteArray> events = p.takeEvents();
    QCOMPARE(events.size(), 2);
    QCOMPARE(events.at(0), QByteArray("{\"a\":1}"));
    QCOMPARE(events.at(1), QByteArray("[DONE]"));

    QVERIFY(p.takeEvents().isEmpty());
}

void TestSseParser::buffersPartialLineAcrossChunks()
{
    // Der eigentliche Grund für diese Klasse: TCP zerschneidet Zeilen
    // an beliebiger Stelle, nicht an Ereignisgrenzen.
    SseParser p;
    p.feed("data: {\"content\":\"Hal");
    QVERIFY(p.takeEvents().isEmpty());

    p.feed("lo\"}\n\n");
    const QVector<QByteArray> events = p.takeEvents();
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.at(0), QByteArray("{\"content\":\"Hallo\"}"));
}

void TestSseParser::ignoresKeepAliveComments()
{
    SseParser p;
    p.feed(": OPENROUTER PROCESSING\n\ndata: x\n\n");

    const QVector<QByteArray> events = p.takeEvents();
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.at(0), QByteArray("x"));
}

void TestSseParser::joinsMultipleDataLines()
{
    SseParser p;
    p.feed("data: erste\ndata: zweite\n\n");

    const QVector<QByteArray> events = p.takeEvents();
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.at(0), QByteArray("erste\nzweite"));
}

void TestSseParser::handlesCrLf()
{
    SseParser p;
    p.feed("data: x\r\n\r\n");

    const QVector<QByteArray> events = p.takeEvents();
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.at(0), QByteArray("x"));
}

void TestSseParser::withholdsEventWithoutBlankLine()
{
    SseParser p;
    p.feed("data: unvollstaendig\n");
    QVERIFY(p.takeEvents().isEmpty());
}
