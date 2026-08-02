#ifndef SFAI_ISYSTEMPROVIDER_H
#define SFAI_ISYSTEMPROVIDER_H

#include <QObject>
#include <QVariantMap>

/*!
 * Abstraktion über alle Systemzugriffe. Zwei Implementierungen:
 *   sandboxed/ : ContextKit, Sailfish.Contacts, BluezQt — Harbour-konform
 *   full/      : QtDBus roh, libcommhistory, libmkcal — OpenRepos
 *
 * Nicht unterstützte Methoden geben einen leeren QVariantMap mit
 * "error" = "unsupported" zurück, statt zu werfen.
 */
class ISystemProvider : public QObject
{
    Q_OBJECT
public:
    explicit ISystemProvider(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ISystemProvider() = default;

    virtual QVariantMap batteryStatus()  = 0;
    virtual QVariantMap networkStatus()  = 0;
    virtual QVariantMap storageStatus()  = 0;
    virtual QVariantMap bluetoothDevices() = 0;
    virtual QVariantMap findContact(const QString &query) = 0;

    // Nur fullaccess — sandboxed liefert {"error":"unsupported"}
    virtual QVariantMap recentMessages(int limit)  = 0;
    virtual QVariantMap upcomingEvents(int days)   = 0;

signals:
    void batteryChanged();
    void networkChanged();
};

#endif
