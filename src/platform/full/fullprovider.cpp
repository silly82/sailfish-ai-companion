#include "fullprovider.h"
#include <QDBusInterface>
#include <QDBusReply>

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
QVariantMap FullProvider::storageStatus()   { return QVariantMap{{"error","not_implemented"}}; }
QVariantMap FullProvider::bluetoothDevices(){ return QVariantMap{{"error","not_implemented"}}; }

QVariantMap FullProvider::findContact(const QString &query)
{ Q_UNUSED(query) return QVariantMap{{"error","not_implemented"}}; }

QVariantMap FullProvider::recentMessages(int limit)
{ Q_UNUSED(limit) return QVariantMap{{"error","not_implemented"}}; }  // libcommhistory

QVariantMap FullProvider::upcomingEvents(int days)
{ Q_UNUSED(days) return QVariantMap{{"error","not_implemented"}}; }   // libmkcal
