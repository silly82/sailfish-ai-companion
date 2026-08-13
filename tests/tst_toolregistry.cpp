#include "tst_toolregistry.h"
#include "fakes.h"

#include "core/capabilities.h"
#include "core/consentgate.h"
#include "core/toolregistry.h"

#include <QDateTime>
#include <QJsonObject>
#include <QSettings>
#include <QTest>

namespace {

QStringList schemaNames(const QJsonArray &schema)
{
    QStringList out;
    for (int i = 0; i < schema.size(); ++i) {
        out.append(schema.at(i).toObject()
                       .value(QStringLiteral("function")).toObject()
                       .value(QStringLiteral("name")).toString());
    }
    return out;
}

}

/*!
 * Die Registry merkt sich Freischaltungen in QSettings. Ohne Rücksetzen
 * schleppt jeder Test die Schalterstellung des vorigen mit.
 */
void TestToolRegistry::init()
{
    QSettings().clear();
}

//! Fasst den immer gleichen Aufbau zusammen: Caps, Provider, Gate, Registry.
namespace {
struct Fixture {
    Capabilities  caps;
    FakeProvider  provider;
    ConsentGate   gate;
    ToolRegistry  registry;

    Fixture() : registry(&caps, &provider, &gate) { registry.buildManifest(); }
};
}

void TestToolRegistry::registersLowToolsEnabled()
{
    Fixture f;

    QVERIFY(f.registry.contains(QStringLiteral("get_datetime")));
    QVERIFY(f.registry.contains(QStringLiteral("get_battery_status")));
    QVERIFY(f.registry.contains(QStringLiteral("get_network_status")));
    QVERIFY(f.registry.contains(QStringLiteral("get_storage_status")));

    // Personenbezogenes und Kritisches ist registriert, aber ab Werk aus.
    QVERIFY(f.registry.contains(QStringLiteral("find_contact")));
    QVERIFY(f.registry.contains(QStringLiteral("get_upcoming_events")));
    QVERIFY(f.registry.contains(QStringLiteral("read_recent_messages")));
    QVERIFY(f.registry.contains(QStringLiteral("run_command")));
    QCOMPARE(f.registry.activeToolCount(), 4);
}

void TestToolRegistry::schemaKeepsRegistrationOrder()
{
    // AIClient kuerzt das Schema von hinten, wenn das Backend weniger Tools
    // vertraegt. Deshalb ist die Reihenfolge Teil des Vertrags, nicht Zufall.
    Fixture f;
    const QStringList names = schemaNames(f.registry.toolSchema());

    QCOMPARE(names.first(), QStringLiteral("get_datetime"));
    QCOMPARE(names.size(), 4);
    QVERIFY(!names.contains(QStringLiteral("find_contact")));
}

void TestToolRegistry::schemaOmitsDisabledTools()
{
    Fixture f;
    f.registry.setToolEnabled(QStringLiteral("get_battery_status"), false);

    const QStringList names = schemaNames(f.registry.toolSchema());
    QVERIFY(!names.contains(QStringLiteral("get_battery_status")));
    QVERIFY(names.contains(QStringLiteral("get_datetime")));
    QCOMPARE(f.registry.activeToolCount(), 3);
}

void TestToolRegistry::refusesUnknownTool()
{
    Fixture f;
    const QVariantMap out = f.registry.invoke(QStringLiteral("launch_missiles"),
                                              QVariantMap());
    QCOMPARE(out.value(QStringLiteral("error")).toString(),
             QStringLiteral("unknown_tool"));
}

void TestToolRegistry::refusesDisabledTool()
{
    Fixture f;
    f.registry.setToolEnabled(QStringLiteral("get_battery_status"), false);

    const QVariantMap out = f.registry.invoke(QStringLiteral("get_battery_status"),
                                              QVariantMap());
    QCOMPARE(out.value(QStringLiteral("error")).toString(),
             QStringLiteral("tool_disabled"));
}

void TestToolRegistry::refusesPersonalToolWithoutConsent()
{
    Fixture f;
    f.registry.setToolEnabled(QStringLiteral("find_contact"), true);

    const QVariantMap out =
        f.registry.invoke(QStringLiteral("find_contact"),
                          QVariantMap{{"query", "Anna"}});

    QCOMPARE(out.value(QStringLiteral("error")).toString(),
             QStringLiteral("consent_required"));
    // Ohne Freigabe darf der Handler nicht einmal gelaufen sein.
    QVERIFY(f.provider.lastQuery.isEmpty());
}

void TestToolRegistry::redactsPersonalResultAfterConsent()
{
    Fixture f;
    f.registry.setToolEnabled(QStringLiteral("find_contact"), true);
    f.registry.grantConsent(QStringLiteral("find_contact"));

    const QVariantMap out =
        f.registry.invoke(QStringLiteral("find_contact"),
                          QVariantMap{{"query", "Anna"}});

    QCOMPARE(f.provider.lastQuery, QStringLiteral("Anna"));
    QVERIFY(!out.contains(QStringLiteral("error")));
    QCOMPARE(out.value(QStringLiteral("name")).toString(),
             QStringLiteral("Anna Muster"));
    // Die Nummer geht als Platzhalter raus und wird erst in der Antwort
    // wieder eingesetzt.
    QVERIFY(out.value(QStringLiteral("phone")).toString()
                .startsWith(QStringLiteral("<contact:")));
}

void TestToolRegistry::leavesLowResultUntouched()
{
    // Die Redaktion greift nur ab Personal. Auf get_datetime angewandt wuerde
    // sie den ISO-Zeitstempel als Telefonnummer missdeuten.
    Fixture f;
    const QVariantMap out = f.registry.invoke(QStringLiteral("get_datetime"),
                                              QVariantMap());

    const QString iso = out.value(QStringLiteral("iso")).toString();
    QVERIFY(QDateTime::fromString(iso, Qt::ISODate).isValid());
    QCOMPARE(f.registry.invoke(QStringLiteral("get_battery_status"), QVariantMap())
                 .value(QStringLiteral("percentage")).toInt(), 87);
}

void TestToolRegistry::disablingRevokesConsent()
{
    Fixture f;
    f.registry.setToolEnabled(QStringLiteral("find_contact"), true);
    f.registry.grantConsent(QStringLiteral("find_contact"));

    f.registry.setToolEnabled(QStringLiteral("find_contact"), false);
    f.registry.setToolEnabled(QStringLiteral("find_contact"), true);

    // Wieder eingeschaltet heisst nicht wieder freigegeben.
    QCOMPARE(f.registry.invoke(QStringLiteral("find_contact"),
                               QVariantMap{{"query", "Anna"}})
                 .value(QStringLiteral("error")).toString(),
             QStringLiteral("consent_required"));
}

void TestToolRegistry::togglePersistsAcrossInstances()
{
    {
        Fixture f;
        f.registry.setToolEnabled(QStringLiteral("get_network_status"), false);
    }

    Fixture again;
    QVERIFY(!schemaNames(again.registry.toolSchema())
                 .contains(QStringLiteral("get_network_status")));
}

void TestToolRegistry::criticalToolRequiresConsentEvenWhenEnabled()
{
    Fixture f;
    f.registry.setToolEnabled(QStringLiteral("run_command"), true);

    const QVariantMap denied =
        f.registry.invoke(QStringLiteral("run_command"),
                          QVariantMap{{"command", "ls"}});
    QCOMPARE(denied.value(QStringLiteral("error")).toString(),
             QStringLiteral("consent_required"));
    // Ohne Freigabe darf der Handler nicht einmal gelaufen sein.
    QVERIFY(f.provider.lastCommand.isEmpty());

    f.registry.grantConsent(QStringLiteral("run_command"));
    const QVariantMap out =
        f.registry.invoke(QStringLiteral("run_command"),
                          QVariantMap{{"command", "ls"},
                                      {"args", QVariantList{"-la"}}});
    QCOMPARE(f.provider.lastCommand, QStringLiteral("ls"));
    QCOMPARE(f.provider.lastArgs, QStringList{"-la"});
    QVERIFY(!out.contains(QStringLiteral("error")));
}

void TestToolRegistry::calendarToolUsesPersonalSensitivity()
{
    // Die Einstufung entscheidet, ob ein Tool per Default-aus + Bestaetigung
    // (ab Personal) laeuft — ConsentGate::Sensitivity dokumentiert die
    // Zuordnung: Kalender ist Personal, SMS und Prozessausfuehrung Critical.
    Fixture f;
    QVariantMap byName;
    for (const QVariant &v : f.registry.tools())
        byName.insert(v.toMap().value(QStringLiteral("name")).toString(), v);

    QCOMPARE(byName.value(QStringLiteral("get_upcoming_events")).toMap()
                 .value(QStringLiteral("sensitivity")).toInt(),
             static_cast<int>(ConsentGate::Personal));
    QCOMPARE(byName.value(QStringLiteral("read_recent_messages")).toMap()
                 .value(QStringLiteral("sensitivity")).toInt(),
             static_cast<int>(ConsentGate::Critical));
    QCOMPARE(byName.value(QStringLiteral("run_command")).toMap()
                 .value(QStringLiteral("sensitivity")).toInt(),
             static_cast<int>(ConsentGate::Critical));
}
