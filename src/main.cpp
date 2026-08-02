#include <QtQuick>
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
    app->setOrganizationName(QStringLiteral("silly"));

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
    auto *client = new AIClient(keys, tools, app.data());

    store->open();

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    QQmlContext *ctx = view->rootContext();
    ctx->setContextProperty("Caps",     caps);
    ctx->setContextProperty("Consent",  gate);
    ctx->setContextProperty("Tools",    tools);
    ctx->setContextProperty("History",  store);
    ctx->setContextProperty("Keys",     keys);
    ctx->setContextProperty("AI",       client);

    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    return app->exec();
}
