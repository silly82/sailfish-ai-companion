#include "keystore.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

/*
 * M1-Ablage: Datei mit 0600 im App-Datenordner. Das ist die dokumentierte
 * Fallback-Variante des Full-Targets und reicht, um den Chat zum Laufen zu
 * bringen. M3 ersetzt das im Harbour-Build durch Sailfish.Secrets — bis dahin
 * darf die Store-Version nicht ausgeliefert werden.
 */

namespace {
QString keyPath(const QString &provider)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + QStringLiteral("/keys");
    if (!QDir().mkpath(dir)) return QString();
    return dir + QLatin1Char('/')
         + QString(provider).replace(QLatin1Char('/'), QLatin1Char('_'));
}
}

KeyStore::KeyStore(QObject *parent) : QObject(parent) {}

void KeyStore::storeKey(const QString &provider, const QString &key)
{
    const QString path = keyPath(provider);
    if (path.isEmpty()) {
        emit errorOccurred(tr("Datenordner nicht beschreibbar"));
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit errorOccurred(f.errorString());
        return;
    }
    f.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    f.write(key.toUtf8());
    f.close();
    emit keyChanged();
}

void KeyStore::clearKey(const QString &provider)
{
    const QString path = keyPath(provider);
    if (path.isEmpty()) return;
    QFile::remove(path);
    emit keyChanged();
}

QString KeyStore::key(const QString &provider) const
{
    QFile f(keyPath(provider));
    if (!f.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

bool KeyStore::hasKey() const
{
    return !key(QStringLiteral("openrouter")).isEmpty();
}
