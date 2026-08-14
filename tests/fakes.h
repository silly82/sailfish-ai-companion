#ifndef SFAI_TESTS_FAKES_H
#define SFAI_TESTS_FAKES_H

#include "core/illmbackend.h"
#include "platform/isystemprovider.h"

#include <QVariantMap>
#include <QVector>

/*!
 * Provider ohne Geraet. Liefert genau so viel, wie die Tests unterscheiden
 * muessen — Ladestand als harmlose Zahl, Kontakt mit Telefonnummer, damit die
 * Redaktion etwas zu tun bekommt.
 */
class FakeProvider : public ISystemProvider
{
public:
    explicit FakeProvider(QObject *parent = nullptr) : ISystemProvider(parent) {}

    QVariantMap batteryStatus() override
    {
        return QVariantMap{{"percentage", 87}, {"charging", false}};
    }
    QVariantMap networkStatus() override
    {
        return QVariantMap{{"type", "wlan"}, {"online", true}};
    }
    QVariantMap storageStatus() override
    {
        return QVariantMap{{"volumes", QVariantList{}}};
    }
    QVariantMap bluetoothDevices() override { return QVariantMap{}; }

    QVariantMap findContact(const QString &query) override
    {
        lastQuery = query;
        return QVariantMap{{"name", "Anna Muster"},
                           {"phone", "+41 79 123 45 67"}};
    }

    QVariantMap recentMessages(int) override { return QVariantMap{}; }

    QVariantMap upcomingEvents(int days) override
    {
        lastDays = days;
        return QVariantMap{{"title", "Team Meeting"},
                           {"address", "Bahnhofstrasse 1, Zürich"}};
    }

    QVariantMap runCommand(const QString &command, const QStringList &args) override
    {
        lastCommand = command;
        lastArgs = args;
        return QVariantMap{{"exitCode", 0}, {"stdout", "ok"}, {"stderr", ""}};
    }

    QString lastQuery;
    QString lastCommand;
    QStringList lastArgs;
    int lastDays = -1;
};

/*!
 * Backend ohne Netz. Spielt ein vorgegebenes Skript ab und protokolliert, was
 * es geschickt bekommen hat.
 *
 * Die Signale kommen synchron aus chat() zurueck. Das ist unrealistisch, aber
 * es macht die Tests deterministisch, und geprueft wird ohnehin die
 * Reihenfolge der Nachrichten, nicht das Timing.
 */
class FakeBackend : public ILlmBackend
{
public:
    struct Call {
        QString     id;
        QString     name;
        QVariantMap args;
    };
    struct Turn {
        QString       text;
        QVector<Call> calls;
    };

    explicit FakeBackend(QObject *parent = nullptr) : ILlmBackend(parent) {}

    QString baseUrl() const override { return QStringLiteral("fake://backend"); }
    bool    isLocal() const override { return true; }
    bool    available() const override { return true; }
    int     maxContextTokens() const override { return 8192; }
    int     maxTools() const override { return maxToolCount; }

    void chat(const QJsonArray &messages, const QJsonArray &tools) override
    {
        requests.append(messages);
        lastTools = tools;

        if (script.isEmpty()) {
            emit failed(QStringLiteral("script exhausted"));
            return;
        }
        const Turn turn = script.takeFirst();
        if (!turn.text.isEmpty()) emit delta(turn.text);
        for (int i = 0; i < turn.calls.size(); ++i) {
            const Call &c = turn.calls.at(i);
            emit toolCall(c.id, c.name, c.args);
        }
        emit finished();
    }

    void cancel() override {}

    QVector<Turn>       script;
    QVector<QJsonArray> requests;
    QJsonArray          lastTools;
    int                 maxToolCount = 8;
};

#endif
