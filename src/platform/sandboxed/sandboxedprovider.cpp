#include "sandboxedprovider.h"
#include <QStorageInfo>

SandboxedProvider::SandboxedProvider(QObject *parent) : ISystemProvider(parent) {}

QVariantMap SandboxedProvider::unsupported()
{
    return QVariantMap{{"error", "unsupported"},
                       {"hint",  "Nur im Full-Access-Build (sailfishai) verfügbar."}};
}

QVariantMap SandboxedProvider::batteryStatus()
{
    // TODO M2: ContextKit-Properties anbinden.
    //          Kandidaten: Battery.ChargePercentage, Battery.IsCharging,
    //          Battery.OnBattery, Battery.TimeUntilLow
    //          -> gegen nemo-qml-plugin-contextkit-qt5 auf dem Gerät prüfen.
    return QVariantMap{{"error", "not_implemented"}};
}

QVariantMap SandboxedProvider::networkStatus()
{
    // TODO M2: ContextKit Internet.NetworkState / Internet.NetworkType
    return QVariantMap{{"error", "not_implemented"}};
}

QVariantMap SandboxedProvider::storageStatus()
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

QVariantMap SandboxedProvider::bluetoothDevices()
{
    // TODO M2: org.kde.bluezqt 1.0 anbinden (Permission Bluetooth im .desktop)
    return QVariantMap{{"error", "not_implemented"}};
}

QVariantMap SandboxedProvider::findContact(const QString &query)
{
    Q_UNUSED(query)
    // TODO M2: org.nemomobile.contacts 1.0, read-only.
    //          ACHTUNG: Ergebnis muss durch ConsentGate + Redaktion,
    //          bevor es an ein Cloud-Modell geht.
    return QVariantMap{{"error", "not_implemented"}};
}
