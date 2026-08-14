#include "keystore.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

/*
 * Datei mit 0600 im App-Datenordner. Das ist die dokumentierte
 * Fallback-Variante des Full-Targets (unsandboxed, Dateisystem sowieso
 * frei zugaenglich) und wird ausserdem von den Desktop-Tests gebaut, die
 * kein Sailfish.Secrets zur Verfuegung haben. Der Harbour-Build nutzt
 * stattdessen platform/sandboxed/keystore_secrets.cpp (M3) — dieselbe
 * KeyStore-Klasse, andere Implementierung, ueber CONFIG(harbour)/
 * CONFIG(fullaccess) im .pro-File verdrahtet statt per #ifdef.
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
