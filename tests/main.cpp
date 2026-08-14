#include "tst_consentgate.h"
#include "tst_conversationstore.h"
#include "tst_modellist.h"
#include "tst_sseparser.h"
#include "tst_toolregistry.h"
#include "tst_toolroundtrip.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTest>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // ToolRegistry/AIClient merken sich Zustand ueber appSettings()
    // (core/appsettings.h), das nach QStandardPaths::AppConfigLocation
    // schreibt. Testmodus verlegt das in ein isoliertes Verzeichnis --
    // ohne den Testlauf faende jeder Testlauf die echten Einstellungen
    // des Nutzers und den Zustand des vorigen Laufs vor.
    QCoreApplication::setOrganizationName(QStringLiteral("sailfish-ai-companion-tests"));
    QCoreApplication::setApplicationName(QStringLiteral("core-tests"));
    QStandardPaths::setTestModeEnabled(true);

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
