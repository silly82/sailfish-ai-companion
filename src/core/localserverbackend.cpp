#include "localserverbackend.h"

LocalServerBackend::LocalServerBackend(QObject *parent) : ILlmBackend(parent) {}
LocalServerBackend::~LocalServerBackend() { stopServer(); }

bool LocalServerBackend::available() const
{ return m_server.state() == QProcess::Running; }

bool LocalServerBackend::startServer(const QString &modelPath)
{
    Q_UNUSED(modelPath)
    // TODO M5:
    //   llama-server -m <model> --host 127.0.0.1 --port 8080
    //                -c 4096 -t 4 --prompt-cache <cache>
    //   Threads: nur die grossen Kerne. Alle 8 Kerne = mehr Hitze,
    //   kaum mehr Durchsatz, weil bandbreitenlimitiert.
    return false;
}

void LocalServerBackend::stopServer()
{
    if (m_server.state() != QProcess::NotRunning) {
        m_server.terminate();
        m_server.waitForFinished(3000);
    }
}

void LocalServerBackend::chat(const QJsonArray &m, const QJsonArray &t)
{ Q_UNUSED(m) Q_UNUSED(t) emit failed(tr("Nicht implementiert (M5)")); }

void LocalServerBackend::cancel() {}
