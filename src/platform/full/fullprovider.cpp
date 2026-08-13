#include "fullprovider.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>
#include <QStorageInfo>

#include <CommHistory/Event>
#include <CommHistory/Group>
#include <CommHistory/GroupModel>

#include <extendedcalendar.h>
#include <sqlitestorage.h>

#include <algorithm>
#include <QTimeZone>

FullProvider::FullProvider(QObject *parent) : ISystemProvider(parent) {}

QVariantMap FullProvider::batteryStatus()
{
    // TODO M4: UPower DisplayDevice-Properties auslesen
    //   QDBusInterface("org.freedesktop.UPower",
    //                  "/org/freedesktop/UPower/devices/DisplayDevice",
    //                  "org.freedesktop.DBus.Properties",
    //                  QDBusConnection::systemBus())
    return QVariantMap{{"error", "not_implemented"}};
}

QVariantMap FullProvider::networkStatus()   { return QVariantMap{{"error","not_implemented"}}; }

QVariantMap FullProvider::storageStatus()
{
    QVariantMap out;
    QVariantList mounts;
    for (const QStorageInfo &si : QStorageInfo::mountedVolumes()) {
        if (!si.isValid() || !si.isReady() || si.isReadOnly()) continue;
        mounts.append(QVariantMap{
            {"path",  si.rootPath()},
            {"free",  si.bytesAvailable()},
            {"total", si.bytesTotal()}
        });
    }
    out.insert("volumes", mounts);
    return out;
}

QVariantMap FullProvider::bluetoothDevices(){ return QVariantMap{{"error","not_implemented"}}; }

QVariantMap FullProvider::findContact(const QString &query)
{ Q_UNUSED(query) return QVariantMap{{"error","not_implemented"}}; }

QVariantMap FullProvider::recentMessages(int limit)
{
    // CommHistory::Group ist pro Konversation, nicht pro Nachricht — es gibt
    // keinen flachen "alle Events aller Konversationen"-Zugriff in dieser
    // API. lastMessageText()/endTime() liefern aber genau die letzte
    // Nachricht je Gespraech, was fuer "juengste Nachrichten" reicht, ohne
    // fuer jede Konversation einzeln ein ConversationModel abzufragen.
    CommHistory::GroupModel model;
    model.setQueryMode(CommHistory::EventModel::SyncQuery);
    if (!model.getGroups())
        return QVariantMap{{"error", "query_failed"}};

    QVector<CommHistory::Group> smsGroups;
    for (int i = 0; i < model.rowCount(); ++i) {
        const CommHistory::Group g = model.group(model.index(i, 0));
        if (g.lastEventType() == CommHistory::Event::SMSEvent)
            smsGroups.append(g);
    }
    std::sort(smsGroups.begin(), smsGroups.end(),
              [](const CommHistory::Group &a, const CommHistory::Group &b) {
                  return a.endTime() > b.endTime();
              });
    if (smsGroups.size() > limit)
        smsGroups.resize(limit);

    QVariantList out;
    for (const CommHistory::Group &g : smsGroups) {
        const QStringList senders = g.recipients().remoteUids();
        out.append(QVariantMap{
            {"sender",    senders.isEmpty() ? QString() : senders.first()},
            {"timestamp", g.endTime().toString(Qt::ISODate)},
            {"text",      g.lastMessageText()}
        });
    }
    return QVariantMap{{"messages", out}};
}

QVariantMap FullProvider::upcomingEvents(int days)
{
    const QDate start = QDate::currentDate();
    const QDate end = start.addDays(days);

    mKCal::ExtendedCalendar::Ptr calendar(
        new mKCal::ExtendedCalendar(QTimeZone::systemTimeZone()));
    mKCal::ExtendedStorage::Ptr storage(new mKCal::SqliteStorage(calendar));

    if (!storage->open() || !storage->load(start, end)) {
        storage->close();
        return QVariantMap{{"error", "query_failed"}};
    }

    const KCalendarCore::Event::List events = calendar->events(start, end);

    QVariantList out;
    for (const KCalendarCore::Event::Ptr &event : events) {
        out.append(QVariantMap{
            {"summary",  event->summary()},
            {"start",    event->dtStart().toString(Qt::ISODate)},
            {"end",      event->dtEnd().toString(Qt::ISODate)},
            {"location", event->location()}
        });
    }

    storage->close();
    return QVariantMap{{"events", out}};
}

QVariantMap FullProvider::runCommand(const QString &command, const QStringList &args)
{
    // Keine Shell dazwischen: das Modell liefert Programm + argv direkt, kein
    // String-Parsing. Schutz ist ausschliesslich ConsentGate::Critical +
    // Default-aus — keine Allow-/Blocklist, die hier nur falsche Sicherheit
    // vorgaukeln wuerde.
    static const int kTimeoutMs = 15000;
    static const int kMaxOutputChars = 4000;

    QProcess process;
    process.start(command, args);
    if (!process.waitForFinished(kTimeoutMs)) {
        process.kill();
        process.waitForFinished();
        return QVariantMap{{"timedOut", true}};
    }

    return QVariantMap{
        {"exitCode", process.exitCode()},
        {"stdout",   QString::fromUtf8(process.readAllStandardOutput()).left(kMaxOutputChars)},
        {"stderr",   QString::fromUtf8(process.readAllStandardError()).left(kMaxOutputChars)},
        {"timedOut", false}
    };
}
