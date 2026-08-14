#include "sandboxedprovider.h"
#include <QStorageInfo>
#include <QNetworkInterface>
#include <QFile>
#include <QDir>

#include <QContactManager>
#include <QContactFetchRequest>
#include <QContactDisplayLabel>
#include <QContactPhoneNumber>
#include <QContactAddress>

QTCONTACTS_USE_NAMESPACE

SandboxedProvider::SandboxedProvider(QObject *parent) : ISystemProvider(parent) {}

QVariantMap SandboxedProvider::unsupported()
{
    return QVariantMap{{"error", "unsupported"},
                       {"hint",  "Nur im Full-Access-Build (sailfishai) verfügbar."}};
}

// Liest eine einzeilige sysfs-Datei; leerer String, wenn sie fehlt oder kein
// Wert vorliegt (z.B. time_to_empty_now auf Geräten ohne Fuel-Gauge-Support).
static QString readSysfsLine(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(f.readLine()).trimmed();
}

QVariantMap SandboxedProvider::batteryStatus()
{
    // Kein contextsubscriber-C++-Paket im SDK-Sysroot (nur das QML-Modul) ->
    // direkt aus sysfs lesen, world-readable, keine Extra-Permission nötig.
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
    const QString status   = readSysfsLine(batteryDir + "status"); // Charging/Discharging/Full/Not charging
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

QVariantMap SandboxedProvider::networkStatus()
{
    // Kein contextsubscriber-C++-Paket im SDK-Sysroot -> QNetworkInterface
    // (bereits über QT += network verlinkt) statt ContextKit.
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
    // org.nemomobile.contacts.sqlite: dasselbe Backend, das Sailfish.Contacts
    // (QML) intern nutzt. Ergebnis wird von ToolRegistry::invoke() als
    // ConsentGate::Personal redigiert, bevor es an ein Cloud-Modell geht.
    //
    // Zwei Bugs gegen dieses Backend auf echtem Geraet reproduziert, beide
    // in derselben Zeile behoben:
    // 1. QContactDetailFilter auf QContactDisplayLabel liefert serverseitig
    //    nie Treffer (DisplayLabel ist berechnet, keine indizierte Spalte).
    // 2. Die synchrone Convenience-Methode manager.contacts() liefert bei
    //    diesem Backend grundsaetzlich leer mit UnspecifiedError zurueck --
    //    auch OHNE Filter, selbst wenn Kontakte existieren. Nur die
    //    asynchrone QContactFetchRequest (mit waitForFinished() blockierend
    //    genutzt) findet sie zuverlaessig.
    QContactManager manager(QStringLiteral("org.nemomobile.contacts.sqlite"));

    QContactFetchRequest fetch;
    fetch.setManager(&manager);
    fetch.start();
    fetch.waitForFinished();

    QVariantList out;
    for (const QContact &contact : fetch.contacts()) {
        if (!contact.detail<QContactDisplayLabel>().label().contains(query, Qt::CaseInsensitive))
            continue;

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
