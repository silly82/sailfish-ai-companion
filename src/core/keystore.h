#ifndef SFAI_KEYSTORE_H
#define SFAI_KEYSTORE_H

#include <QObject>

/*!
 * API-Key-Ablage. Wrapper, weil Jolla für Sailfish.Secrets ausdrücklich
 * KEINE Backwards-Compat zwischen Releases garantiert.
 *
 *   Harbour : Sailfish.Secrets 1.0  (Permission "Secrets" im .desktop)
 *   Full    : dito, Fallback auf Datei mit 0600 im App-Datenordner
 *
 * Der Key darf niemals im Klartext geloggt oder in QSettings landen.
 */
class KeyStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasKey READ hasKey NOTIFY keyChanged)
public:
    explicit KeyStore(QObject *parent = nullptr);

    Q_INVOKABLE void    storeKey(const QString &provider, const QString &key);
    Q_INVOKABLE void    clearKey(const QString &provider);
    QString             key(const QString &provider) const;
    bool                hasKey() const;

signals:
    void keyChanged();
    void errorOccurred(const QString &message);
};

#endif
