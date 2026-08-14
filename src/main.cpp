#include <QtQuick>
#include <QTranslator>
#include <sailfishapp.h>

#include "core/capabilities.h"
#include "core/consentgate.h"
#include "core/toolregistry.h"
#include "core/conversationstore.h"
#include "core/keystore.h"
#include "core/aiclient.h"

#ifdef SFAI_HARBOUR
#  include "platform/sandboxed/sandboxedprovider.h"
#else
#  include "platform/full/fullprovider.h"
#endif

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationName(QStringLiteral(SFAI_TARGET_NAME));
    // Must match the Sailjail "OrganizationName" in the .desktop file, since
    // that determines which paths under ~/.local/share etc. the sandbox
    // whitelists for QStandardPaths::AppDataLocation and friends.
    app->setOrganizationName(QStringLiteral("ch.silly"));

    // Only translations/harbour-nemoai-de.ts is maintained (see CLAUDE.md);
    // sailfishapp_i18n installs the resulting .qm under
    // /usr/share/$${TARGET}/translations/, but SailfishApp's own automatic
    // translator looks for a catalog matching the *binary* name. For
    // TARGET=sailfishai that search (sailfishai-de.qm) never matches the
    // shipped harbour-nemoai-de.qm, so the fullaccess build silently stayed
    // on English regardless of device locale. Load the real catalog name
    // explicitly so both targets pick it up.
    auto *translator = new QTranslator(app.data());
    const QString translationsDir = SailfishApp::pathTo(QStringLiteral("translations")).toLocalFile();
    if (translator->load(QLocale(), QStringLiteral("harbour-nemoai"), QStringLiteral("-"), translationsDir)) {
        app->installTranslator(translator);
    }

    auto *caps  = new Capabilities(app.data());
    auto *gate  = new ConsentGate(app.data());
    auto *keys  = new KeyStore(app.data());
    auto *store = new ConversationStore(app.data());

#ifdef SFAI_HARBOUR
    ISystemProvider *provider = new SandboxedProvider(app.data());
#else
    ISystemProvider *provider = new FullProvider(app.data());
#endif

    auto *tools  = new ToolRegistry(caps, provider, gate, app.data());
    tools->buildManifest();
    store->open();
    auto *client = new AIClient(keys, tools, store, app.data());

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    QQmlContext *ctx = view->rootContext();
    ctx->setContextProperty("Caps",     caps);
    ctx->setContextProperty("Consent",  gate);
    ctx->setContextProperty("Tools",    tools);
    ctx->setContextProperty("History",  store);
    // Named "KeyStore", not "Keys" — the latter collides with QtQuick's
    // built-in Keys attached property and silently resolves to the wrong
    // object in any Item's scope (Keys.storeKey(...) becomes a no-op).
    ctx->setContextProperty("KeyStore", keys);
    ctx->setContextProperty("AI",       client);

    // pathToMainQml() expects qml/<TARGET>.qml (harbour-nemoai.qml vs.
    // sailfishai.qml); pathTo() keeps a single main.qml working for both.
    view->setSource(SailfishApp::pathTo(QStringLiteral("qml/main.qml")));
    view->show();
    return app->exec();
}
