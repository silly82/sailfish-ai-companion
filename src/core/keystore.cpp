#include "keystore.h"

KeyStore::KeyStore(QObject *parent) : QObject(parent) {}

void KeyStore::storeKey(const QString &provider, const QString &key)
{ Q_UNUSED(provider) Q_UNUSED(key) /* TODO M3: Sailfish.Secrets */ }

void KeyStore::clearKey(const QString &provider)
{ Q_UNUSED(provider) }

QString KeyStore::key(const QString &provider) const
{ Q_UNUSED(provider) return QString(); }

bool KeyStore::hasKey() const { return false; }
