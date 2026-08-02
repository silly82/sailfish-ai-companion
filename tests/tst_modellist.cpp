#include "tst_modellist.h"
#include "core/openrouterbackend.h"

#include <QTest>
#include <QVariantMap>

namespace {
QByteArray catalogue()
{
    return QByteArray(R"({"data":[
      {"id":"vendor/with-tools","name":"With Tools","context_length":32768,
       "supported_parameters":["tools","temperature"],
       "pricing":{"prompt":"0.0000005","completion":"0.0000015"}},
      {"id":"vendor/no-tools","name":"No Tools","context_length":32768,
       "supported_parameters":["temperature"],
       "pricing":{"prompt":"0.0000001","completion":"0.0000002"}},
      {"id":"vendor/tiny-context","name":"Tiny","context_length":4096,
       "supported_parameters":["tools"],
       "pricing":{"prompt":"0","completion":"0"}},
      {"id":"vendor/free","name":"Free","context_length":16384,
       "supported_parameters":["tools"],
       "pricing":{"prompt":"0","completion":"0"}}
    ]})");
}

QVariantMap findModel(const QVariantList &list, const QString &id)
{
    for (int i = 0; i < list.size(); ++i) {
        const QVariantMap m = list.at(i).toMap();
        if (m.value(QStringLiteral("id")).toString() == id) return m;
    }
    return QVariantMap();
}
}

void TestModelList::dropsModelsWithoutToolSupport()
{
    // Ohne Function-Calling ist ein Modell für diese App wertlos — die
    // Tool-Registry ist die halbe Architektur.
    const QVariantList models = OpenRouterBackend::parseModelList(catalogue());
    QVERIFY(findModel(models, QStringLiteral("vendor/no-tools")).isEmpty());
    QVERIFY(!findModel(models, QStringLiteral("vendor/with-tools")).isEmpty());
}

void TestModelList::dropsShortContextModels()
{
    const QVariantList models = OpenRouterBackend::parseModelList(catalogue());
    QVERIFY(findModel(models, QStringLiteral("vendor/tiny-context")).isEmpty());
}

void TestModelList::marksFreeModels()
{
    const QVariantList models = OpenRouterBackend::parseModelList(catalogue());

    const QVariantMap free = findModel(models, QStringLiteral("vendor/free"));
    QVERIFY(!free.isEmpty());
    QVERIFY(free.value(QStringLiteral("free")).toBool());

    const QVariantMap paid = findModel(models, QStringLiteral("vendor/with-tools"));
    QVERIFY(!paid.value(QStringLiteral("free")).toBool());
    QCOMPARE(paid.value(QStringLiteral("context")).toInt(), 32768);
}

void TestModelList::survivesGarbage()
{
    QVERIFY(OpenRouterBackend::parseModelList(QByteArray("nicht json")).isEmpty());
    QVERIFY(OpenRouterBackend::parseModelList(QByteArray("{}")).isEmpty());
}
