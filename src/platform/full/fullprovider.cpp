#include "fullprovider.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>
#include <QStorageInfo>
#include <QNetworkInterface>
#include <QFile>
#include <QDir>

#include <CommHistory/Event>
#include <CommHistory/Group>
#include <CommHistory/GroupModel>

#include <QContactManager>
#include <QContactDetailFilter>
#include <QContactDisplayLabel>
#include <QContactPhoneNumber>
#include <QContactAddress>

QTCONTACTS_USE_NAMESPACE

#include <extendedcalendar.h>
#include <sqlitestorage.h>
#include <extendedstorageobserver.h>

#include <algorithm>
#include <QDebug>
#include <QTimeZone>

namespace {
// mKCal::ExtendedStorage::open()/load() only return bool; the actual
// SQLite/mKCal error text is delivered asynchronously to a registered
// observer. Registered around the open()/load() calls below purely for
// diagnostics -- no behavior change.
class CalendarErrorLogger : public mKCal::ExtendedStorageObserver
{
public:
    void storageFinished(mKCal::ExtendedStorage *storage, bool error, const QString &info) override
    {
        Q_UNUSED(storage)
        if (error)
            qWarning("FullProvider::upcomingEvents: mKCal storage error: %s", qPrintable(info));
    }
};
}

FullProvider::FullProvider(QObject *parent) : ISystemProvider(parent) {}

// Liest eine einzeilige sysfs-Datei; leerer String, wenn sie fehlt oder kein
// Wert vorliegt (z.B. time_to_empty_now auf Geräten ohne Fuel-Gauge-Support).
static QString readSysfsLine(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(f.readLine()).trimmed();
}

QVariantMap FullProvider::batteryStatus()
{
    // Kein UPower auf der Zielhardware verifiziert; sysfs ist auch unsandboxed
    // world-readable und bereits auf Gerät getestet (SandboxedProvider).
    QString batteryDir;
    for (const QString &name : QDir("/sys/class/power_supply").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString base = "/sys/class/power_supply/" + name + "/";
        if (readSysfsLine(base + "type") == "Battery") {
            batteryDir = base;
            break;
        }
    }
    if (batteryDir.isEmpty()) return QVariantMap{{"error", "not_implemented"}};

    const QString capacity = readSysfsLine(batteryDir + "capacity");
    const QString status   = readSysfsLine(batteryDir + "status");
    if (capacity.isEmpty() || status.isEmpty()) return QVariantMap{{"error", "not_implemented"}};

    QVariantMap out{
        {"percentage", capacity.toInt()},
        {"charging",   status == "Charging"},
        {"status",     status}
    };

    // Manche Fuel-Gauge-Treiber (beobachtet: MTK) liefern hier gelegentlich
    // einen stehengebliebenen/unplausiblen Rohwert statt 0, bevor die
    // Kalibrierung eingeschwungen ist -> auf einen plausiblen Bereich für
    // ein Telefon kappen (max. 24h), statt Kernel-Rauschen weiterzureichen.
    const qint64 maxPlausibleMinutes = 24 * 60;
    const QString timeToFull  = readSysfsLine(batteryDir + "time_to_full_now");
    const QString timeToEmpty = readSysfsLine(batteryDir + "time_to_empty_now");
    if (status == "Charging" && !timeToFull.isEmpty()) {
        const qint64 minutes = timeToFull.toLongLong() / 60;
        if (minutes > 0 && minutes <= maxPlausibleMinutes) out.insert("timeRemainingMinutes", minutes);
    } else if (status == "Discharging" && !timeToEmpty.isEmpty()) {
        const qint64 minutes = timeToEmpty.toLongLong() / 60;
        if (minutes > 0 && minutes <= maxPlausibleMinutes) out.insert("timeRemainingMinutes", minutes);
    }
    return out;
}

QVariantMap FullProvider::networkStatus()
{
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (flags & QNetworkInterface::IsLoopBack) continue;
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)) continue;

        QString ip;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
                ip = entry.ip().toString();
                break;
            }
        }
        if (ip.isEmpty()) continue;

        const QString name = iface.name();
        QString type = "other";
        if (name.startsWith("wlan"))                                type = "wifi";
        else if (name.startsWith("ccmni") || name.startsWith("rmnet")) type = "mobile";
        else if (name.startsWith("usb") || name.startsWith("eth") || name.startsWith("enp")) type = "ethernet";

        return QVariantMap{
            {"connected", true},
            {"type",      type},
            {"interface", name},
            {"ipAddress", ip}
        };
    }
    return QVariantMap{{"connected", false}, {"type", "none"}};
}

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
{
    // org.nemomobile.contacts.sqlite: dasselbe Backend, das Sailfish.Contacts
    // (QML) intern nutzt. Ergebnis wird von ToolRegistry::invoke() als
    // ConsentGate::Personal redigiert, bevor es an ein Cloud-Modell geht.
    QContactManager manager(QStringLiteral("org.nemomobile.contacts.sqlite"));

    QContactDetailFilter filter;
    filter.setDetailType(QContactDisplayLabel::Type, QContactDisplayLabel::FieldLabel);
    filter.setMatchFlags(QContactFilter::MatchContains);
    filter.setValue(query);

    QVariantList out;
    for (const QContact &contact : manager.contacts(filter)) {
        QStringList numbers;
        for (const QContactPhoneNumber &phone : contact.details<QContactPhoneNumber>())
            numbers.append(phone.number());

        QStringList addresses;
        for (const QContactAddress &address : contact.details<QContactAddress>()) {
            QStringList parts{address.street(), address.locality()};
            parts.removeAll(QString());
            if (!parts.isEmpty())
                addresses.append(parts.join(QStringLiteral(", ")));
        }

        out.append(QVariantMap{
            {"name",      contact.detail<QContactDisplayLabel>().label()},
            {"phones",    numbers},
            {"addresses", addresses}
        });
    }
    return QVariantMap{{"contacts", out}};
}

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

    CalendarErrorLogger errorLogger;
    storage->registerObserver(&errorLogger);

    if (!storage->open() || !storage->load(start, end)) {
        storage->unregisterObserver(&errorLogger);
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

    storage->unregisterObserver(&errorLogger);
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
