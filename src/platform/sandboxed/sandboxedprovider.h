#ifndef SFAI_SANDBOXEDPROVIDER_H
#define SFAI_SANDBOXEDPROVIDER_H

#include "../isystemprovider.h"

/*!
 * Harbour-konforme Implementierung.
 *
 * Erlaubte Kanäle (geprüft gegen die echten Validator-Configs in
 * sailfishos/sdk-harbour-rpmvalidator, siehe docs/todo-harbour-vs-full.md):
 *   Akku/Netz   -> /sys/class/power_supply, /sys/class/net + QNetworkInterface
 *                  (org.freedesktop.contextkit 1.0 bietet nur ein QML-Modul,
 *                  keine C++-API im SDK-Sysroot — daher sysfs statt ContextKit)
 *   Bluetooth   -> org.kde.bluezqt 1.0              (Permission: Bluetooth)
 *   Storage     -> QStorageInfo, nur zugängliche Mounts
 *
 * Kontakte NICHT über QtContacts: libQt5Contacts.so.5 steht nicht in
 * allowed_libraries.conf, sfdk check bricht mit "Cannot link to shared
 * library" ab. Harbour-legal ist nur der QML-Import (Sailfish.Contacts 1.0 /
 * org.nemomobile.contacts 1.0) — die C++-Brücke dafür existiert noch nicht
 * (siehe H2 in docs/todo-harbour-vs-full.md), deshalb bis dahin
 * findContact() ein reiner unsupported()-Stub und
 * Capabilities::contacts() == false im SFAI_HARBOUR-Zweig.
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
    QVariantMap findContact(const QString &) override { return unsupported(); }

    QVariantMap recentMessages(int) override { return unsupported(); }
    QVariantMap upcomingEvents(int) override { return unsupported(); }
    QVariantMap runCommand(const QString &, const QStringList &) override { return unsupported(); }

private:
    static QVariantMap unsupported();
};

#endif
