// Harbour-Implementierung von KeyStore (core/keystore.h) über
// Sailfish.Secrets 1.0. Full-Access bleibt auf der dateibasierten
// Implementierung in core/keystore.cpp — dort ist sie der dokumentierte
// Fallback, hier ersetzt sie die 0600-Datei komplett (M3).
//
// Jolla garantiert für diese API ausdrücklich keine Kompatibilität zwischen
// Releases; deshalb bleibt sie hinter dem KeyStore-Wrapper isoliert, nicht
// direkt in AIClient/QML sichtbar.
//
// Die Sailfish::Secrets-Requests sind von Haus aus asynchron (D-Bus zum
// sailfishsecretsd), aber jede Request-Klasse bietet waitForFinished(), das
// intern eine QEventLoop dreht, bis die Antwort da ist. Das haelt KeyStores
// oeffentliche API synchron -- AIClient/OpenRouterBackend lesen den Key
// mitten im Request-Aufbau, ein asynchroner Umbau haette sich durch mehrere
// Schichten gezogen fuer einen Vorgang, der nur beim Start und beim
// Speichern/Loeschen des Keys ueberhaupt passiert.

#include "../../core/keystore.h"

#include <Sailfish/Secrets/secretmanager.h>
#include <Sailfish/Secrets/storesecretrequest.h>
#include <Sailfish/Secrets/storedsecretrequest.h>
#include <Sailfish/Secrets/deletesecretrequest.h>

#include <QHash>

using namespace Sailfish::Secrets;

namespace {

// Der offizielle Standalone-Secret-Codepfad (docs: "Default Secrets Plugins")
// nutzt DefaultStoragePluginName + ein separat gesetztes EncryptionPluginName,
// nicht DefaultEncryptedStoragePluginName -- letzteres ist fuer
// CreateCollectionRequest gedacht, wo dieselbe Plugin-Id sowohl als Storage-
// als auch als Encryption-Plugin dient. Mit DefaultEncryptedStoragePluginName
// hier scheiterte StoreSecretRequest auf echter Hardware mit "no such plugin
// exists" (org.sailfishos.secrets.plugin.encryptedstorage.sqlcipher), obwohl
// laut Doku standardmaessig vorhanden -- vermutlich fehlt/laedt das
// SQLCipher-Plugin auf diesem Geraet nicht. Der Datenblob wird trotzdem
// verschluesselt: das ist gerade der Zweck des Encryption-Plugins.
Secret::Identifier identifierFor(const QString &provider)
{
    return Secret::Identifier(provider, QString(),
                               SecretManager::DefaultStoragePluginName);
}

// KeyStore ist ein Singleton -- main.cpp instanziiert genau eines. Ein
// Prozess-Cache erspart wiederholte D-Bus-Requests fuer denselben Provider,
// ohne den mit core/keystore.cpp geteilten Header um Secrets-spezifischen
// State zu erweitern.
QHash<QString, QString> g_cache;
QHash<QString, bool>    g_resolved;

}

KeyStore::KeyStore(QObject *parent) : QObject(parent) {}

void KeyStore::storeKey(const QString &provider, const QString &key)
{
    SecretManager manager;
    Secret secret(identifierFor(provider));
    secret.setData(key.toUtf8());

    StoreSecretRequest req;
    req.setManager(&manager);
    req.setSecretStorageType(StoreSecretRequest::StandaloneDeviceLockSecret);
    req.setDeviceLockUnlockSemantic(SecretManager::DeviceLockKeepUnlocked);
    req.setAccessControlMode(SecretManager::OwnerOnlyMode);
    req.setUserInteractionMode(SecretManager::SystemInteraction);
    req.setEncryptionPluginName(SecretManager::DefaultEncryptionPluginName);
    req.setSecret(secret);
    req.startRequest();
    req.waitForFinished();

    if (req.result().code() != Result::Succeeded) {
        emit errorOccurred(req.result().errorMessage());
        return;
    }

    g_cache[provider]    = key;
    g_resolved[provider] = true;
    emit keyChanged();
}

void KeyStore::clearKey(const QString &provider)
{
    SecretManager manager;
    DeleteSecretRequest req;
    req.setManager(&manager);
    req.setIdentifier(identifierFor(provider));
    req.setUserInteractionMode(SecretManager::SystemInteraction);
    req.startRequest();
    req.waitForFinished();

    // InvalidSecretError heisst nur "war nie gesetzt" -- kein echter Fehler.
    if (req.result().code() != Result::Succeeded
        && req.result().errorCode() != Result::InvalidSecretError) {
        emit errorOccurred(req.result().errorMessage());
    }

    g_cache.remove(provider);
    g_resolved[provider] = true;
    emit keyChanged();
}

QString KeyStore::key(const QString &provider) const
{
    if (g_resolved.value(provider, false)) return g_cache.value(provider);

    SecretManager manager;
    StoredSecretRequest req;
    req.setManager(&manager);
    req.setIdentifier(identifierFor(provider));
    req.setUserInteractionMode(SecretManager::SystemInteraction);
    req.startRequest();
    req.waitForFinished();

    g_resolved[provider] = true;
    if (req.result().code() == Result::Succeeded) {
        g_cache[provider] = QString::fromUtf8(req.secret().data());
    }
    return g_cache.value(provider);
}

bool KeyStore::hasKey() const
{
    return !key(QStringLiteral("openrouter")).isEmpty();
}
