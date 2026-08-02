#include "tst_conversationstore.h"
#include "tst_modellist.h"
#include "tst_sseparser.h"

#include <QCoreApplication>
#include <QTest>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int failed = 0;
    TestSseParser         sse;
    TestConversationStore store;
    TestModelList         models;

    failed += QTest::qExec(&sse,    argc, argv);
    failed += QTest::qExec(&store,  argc, argv);
    failed += QTest::qExec(&models, argc, argv);
    return failed;
}
