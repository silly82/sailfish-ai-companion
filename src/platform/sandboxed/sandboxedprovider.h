#ifndef SFAI_SANDBOXEDPROVIDER_H
#define SFAI_SANDBOXEDPROVIDER_H

#include "../isystemprovider.h"

/*!
 * Harbour-konforme Implementierung.
 *
 * Erlaubte Kanäle (geprüft gegen docs.sailfishos.org, Stand 4.5.0/5.1):
 *   Akku/Netz   -> org.freedesktop.contextkit 1.0   ⚠️ Property-Namen auf Gerät verifizieren
 *   Kontakte    -> org.nemomobile.contacts 1.0      (Permission: Contacts)
 *   Bluetooth   -> org.kde.bluezqt 1.0              (Permission: Bluetooth)
 *   Storage     -> QStorageInfo, nur zugängliche Mounts
 *
 * NICHT verfügbar: SMS, Kalender, Notification-Reading, Prozess-Spawn.
 */
class SandboxedProvider : public ISystemProvider
{
    Q_OBJECT
public:
    explicit SandboxedProvider(QObject *parent = nullptr);

    QVariantMap batteryStatus() override;
    QVariantMap networkStatus() override;
    QVariantMap storageStatus() override;
    QVariantMap bluetoothDevices() override;
    QVariantMap findContact(const QString &query) override;

    QVariantMap recentMessages(int) override { return unsupported(); }
    QVariantMap upcomingEvents(int) override { return unsupported(); }
    QVariantMap runCommand(const QString &, const QStringList &) override { return unsupported(); }

private:
    static QVariantMap unsupported();
};

#endif
