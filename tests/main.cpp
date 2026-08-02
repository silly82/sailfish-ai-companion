#include "tst_consentgate.h"
#include "tst_conversationstore.h"
#include "tst_modellist.h"
#include "tst_sseparser.h"
#include "tst_toolregistry.h"
#include "tst_toolroundtrip.h"

#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // ToolRegistry merkt sich die Freischaltungen in QSettings. Ohne eigenes
    // Verzeichnis schriebe der Testlauf in die echten Einstellungen des
    // Nutzers — und faende dort den Zustand des letzten Laufs vor.
    QTemporaryDir settingsDir;
    QCoreApplication::setOrganizationName(QStringLiteral("sailfish-ai-companion-tests"));
    QCoreApplication::setApplicationName(QStringLiteral("core-tests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());

    int failed = 0;
    TestSseParser         sse;
    TestConversationStore store;
    TestModelList         models;
    TestConsentGate       consent;
    TestToolRegistry      tools;
    TestToolRoundtrip     roundtrip;

    failed += QTest::qExec(&sse,       argc, argv);
    failed += QTest::qExec(&store,     argc, argv);
    failed += QTest::qExec(&models,    argc, argv);
    failed += QTest::qExec(&consent,   argc, argv);
    failed += QTest::qExec(&tools,     argc, argv);
    failed += QTest::qExec(&roundtrip, argc, argv);
    return failed;
}
