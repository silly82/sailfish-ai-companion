#include "fullprovider.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>
#include <QStorageInfo>

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
{ Q_UNUSED(limit) return QVariantMap{{"error","not_implemented"}}; }  // libcommhistory

QVariantMap FullProvider::upcomingEvents(int days)
{ Q_UNUSED(days) return QVariantMap{{"error","not_implemented"}}; }   // libmkcal

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
