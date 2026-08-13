#ifndef SFAI_FULLPROVIDER_H
#define SFAI_FULLPROVIDER_H

#include "../isystemprovider.h"

/*!
 * Full-Access-Implementierung für OpenRepos. Läuft unsandboxed.
 *
 * Kanäle:
 *   Akku      -> /sys/class/power_supply (wie SandboxedProvider — kein
 *                UPower auf diesem Gerät verifiziert, sysfs ist unsandboxed
 *                genauso zugänglich und bereits auf Zielhardware getestet)
 *   Netz      -> QNetworkInterface (wie SandboxedProvider, s.o.)
 *   Telefonie -> org.ofono
 *   Bluetooth -> org.bluez
 *   SMS/Calls -> libcommhistory
 *   Kalender  -> libmkcal
 *
 * Diese Klasse darf NIE in den Harbour-Build gelinkt werden.
 */
class FullProvider : public ISystemProvider
{
    Q_OBJECT
public:
    explicit FullProvider(QObject *parent = nullptr);

    QVariantMap batteryStatus() override;
    QVariantMap networkStatus() override;
    QVariantMap storageStatus() override;
    QVariantMap bluetoothDevices() override;
    QVariantMap findContact(const QString &query) override;
    QVariantMap recentMessages(int limit) override;
    QVariantMap upcomingEvents(int days) override;
    QVariantMap runCommand(const QString &command, const QStringList &args) override;
};

#endif
