#include "tst_consentgate.h"
#include "core/consentgate.h"

#include <QTest>

void TestConsentGate::confirmsAbovePersonal()
{
    ConsentGate gate;
    QVERIFY(!gate.requiresConfirmation(ConsentGate::Low));
    QVERIFY(gate.requiresConfirmation(ConsentGate::Personal));
    QVERIFY(gate.requiresConfirmation(ConsentGate::Critical));

    QVERIFY(!gate.isGranted(QStringLiteral("find_contact")));
    gate.grant(QStringLiteral("find_contact"));
    QVERIFY(gate.isGranted(QStringLiteral("find_contact")));
    gate.revoke(QStringLiteral("find_contact"));
    QVERIFY(!gate.isGranted(QStringLiteral("find_contact")));
}

void TestConsentGate::localOnlyWaivesEverything()
{
    ConsentGate gate;
    gate.setLocalOnly(true);

    // Nichts verlaesst das Geraet, also gibt es weder etwas zu bestaetigen
    // noch etwas zu schwaerzen.
    QVERIFY(!gate.requiresConfirmation(ConsentGate::Critical));
    QVERIFY(gate.isGranted(QStringLiteral("read_recent_messages")));

    const QVariantMap payload{{"phone", "+41 79 123 45 67"}};
    QCOMPARE(gate.redact(payload).value(QStringLiteral("phone")).toString(),
             QStringLiteral("+41 79 123 45 67"));
}

void TestConsentGate::replacesPhoneAndMail()
{
    ConsentGate gate;
    const QVariantMap in{
        {"note", "Ruf mich unter +41 79 123 45 67 an oder schreib an a.muster@example.ch"}
    };

    const QString out = gate.redact(in).value(QStringLiteral("note")).toString();
    QVERIFY(!out.contains(QStringLiteral("79 123 45 67")));
    QVERIFY(!out.contains(QStringLiteral("example.ch")));
    QVERIFY(out.contains(QStringLiteral("<contact:")));
    QVERIFY(out.startsWith(QStringLiteral("Ruf mich unter ")));
}

void TestConsentGate::reusesPlaceholderForSameValue()
{
    ConsentGate gate;
    const QVariantMap in{
        {"a", "b.muster@example.ch"},
        {"b", "b.muster@example.ch"}
    };

    const QVariantMap out = gate.redact(in);
    QCOMPARE(out.value(QStringLiteral("a")).toString(),
             out.value(QStringLiteral("b")).toString());
    QVERIFY(out.value(QStringLiteral("a")).toString().startsWith(QStringLiteral("<contact:")));
}

void TestConsentGate::redactsBySensitiveKey()
{
    ConsentGate gate;
    // Eine Adresse erkennt kein Muster zuverlaessig — der Feldname schon.
    const QVariantMap contact{
        {"name",    "Anna Muster"},
        {"address", "Bahnhofstrasse 1, 8001 Zürich"}
    };
    const QVariantMap in{{"contacts", QVariantList{contact}}};

    const QVariantMap result =
        gate.redact(in).value(QStringLiteral("contacts")).toList()
            .at(0).toMap();

    QCOMPARE(result.value(QStringLiteral("name")).toString(),
             QStringLiteral("Anna Muster"));
    QVERIFY(result.value(QStringLiteral("address")).toString()
                .startsWith(QStringLiteral("<contact:")));
}

void TestConsentGate::leavesHarmlessNumbersAlone()
{
    ConsentGate gate;
    const QVariantMap in{
        {"percentage", 87},
        {"short",      "PLZ 8001"},
        {"version",    "5.0.0.62"}
    };

    const QVariantMap out = gate.redact(in);
    QCOMPARE(out.value(QStringLiteral("percentage")).toInt(), 87);
    QCOMPARE(out.value(QStringLiteral("short")).toString(), QStringLiteral("PLZ 8001"));
    QCOMPARE(out.value(QStringLiteral("version")).toString(), QStringLiteral("5.0.0.62"));
}

void TestConsentGate::redactsStringListValues()
{
    // QStringList (find_contact's "phones"/"addresses" in fullprovider.cpp)
    // ist ein eigener QVariant-Typ, verschieden von QVariant::List --
    // fehlte der Fall, ging der ganze Wert unredigiert durch (beobachtet auf
    // echter Hardware: echte Telefonnummern im Klartext ans Cloud-Modell).
    ConsentGate gate;
    const QVariantMap in{
        {"phones", QStringList{"+41 79 123 45 67", "+41 44 987 65 43"}}
    };

    const QVariantList out = gate.redact(in).value(QStringLiteral("phones")).toList();
    QCOMPARE(out.size(), 2);
    for (const QVariant &v : out)
        QVERIFY(v.toString().startsWith(QStringLiteral("<contact:")));
}

void TestConsentGate::redactsSensitiveKeyStringListByKey()
{
    // "addresses" (Freitext) matcht keine Regex -- anders als "phones"
    // haengt die Redaktion hier komplett am Schluesselnamen. Beobachtet auf
    // echter Hardware: eine echte Adresse blieb trotz des StringList-Fixes
    // im Klartext, weil "addresses" (Plural) nicht in isSensitiveKey() stand.
    ConsentGate gate;
    const QVariantMap in{
        {"addresses", QStringList{"In der Mühlematte 8, Altdorf"}}
    };

    const QVariantList out = gate.redact(in).value(QStringLiteral("addresses")).toList();
    QCOMPARE(out.size(), 1);
    QVERIFY(out.first().toString().startsWith(QStringLiteral("<contact:")));
}

void TestConsentGate::leavesIsoDatesAlone()
{
    // phonePattern() matched "2026-01-15" innerhalb eines ISO-Zeitstempels
    // (8 Ziffern + Bindestriche liegen im selben Muster wie eine
    // Telefonnummer) -- auf echter Hardware beobachtet bei
    // get_upcoming_events, wo start/end zu "<contact:N>T00:00:00" wurden.
    ConsentGate gate;
    const QVariantMap in{
        {"start", "2026-01-15T00:00:00"},
        {"end",   "2026-01-22T00:00:00"}
    };

    const QVariantMap out = gate.redact(in);
    QCOMPARE(out.value(QStringLiteral("start")).toString(),
             QStringLiteral("2026-01-15T00:00:00"));
    QCOMPARE(out.value(QStringLiteral("end")).toString(),
             QStringLiteral("2026-01-22T00:00:00"));
}

void TestConsentGate::restoresPlaceholdersInAnswer()
{
    ConsentGate gate;
    const QVariantMap in{{"phone", "+41 79 123 45 67"}};
    const QString token = gate.redact(in).value(QStringLiteral("phone")).toString();

    const QString answer = QStringLiteral("Ich rufe %1 an.").arg(token);
    QCOMPARE(gate.restore(answer), QStringLiteral("Ich rufe +41 79 123 45 67 an."));
}

void TestConsentGate::forgetsPlaceholders()
{
    ConsentGate gate;
    const QVariantMap in{{"phone", "+41 79 123 45 67"}};
    const QString token = gate.redact(in).value(QStringLiteral("phone")).toString();

    gate.forgetPlaceholders();
    // Ohne Zuordnung bleibt der Platzhalter stehen, statt still danebenzugreifen.
    QCOMPARE(gate.restore(token), token);
}
