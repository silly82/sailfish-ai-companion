#ifndef SFAI_LOCALSERVERBACKEND_H
#define SFAI_LOCALSERVERBACKEND_H

#include "illmbackend.h"
#include <QProcess>

/*!
 * Lokaler Fallback über llama-server (Paket sailfishai-llama).
 * NUR im Full-Target — Harbour erlaubt keinen Prozess-Spawn.
 *
 * Realistische Zahlen (Q4_K_M, CPU-only, Stand 2026):
 *
 *   Jolla Phone 2026 (12 GB, LPDDR5, A78-Klasse)
 *     Qwen3-4B      ~2.5 GB   ~6-10 tok/s   <- Empfehlung
 *     Qwen3-1.7B    ~1.1 GB   ~15-20 tok/s  <- wenn Latenz zählt
 *
 *   Jolla C2 (8 GB, LPDDR4X, A75 @1.6 GHz)
 *     Qwen3-1.7B    ~1.1 GB   ~4-6 tok/s    <- Obergrenze
 *     alles ab 3B: 1-2 tok/s, unbenutzbar
 *
 * WICHTIG — Prefill ist der Flaschenhals, nicht die Generierung:
 * 8 Tool-Schemas sind ~1500 Token. Auf dem C2 bei ~20 tok/s Prefill
 * heisst das eine Minute bis zum ersten Zeichen. Deshalb:
 *   1. --prompt-cache: System-Prompt einmal prefillen, persistieren
 *   2. maxTools() = 4
 *   3. maxContextTokens() = 4096, nicht 32k
 *
 * Thermik: Dauerlast zieht 3-4 W, drosselt nach Minuten. Kurzstreckenmodus.
 */
class LocalServerBackend : public ILlmBackend
{
    Q_OBJECT
public:
    explicit LocalServerBackend(QObject *parent = nullptr);
    ~LocalServerBackend() override;

    QString baseUrl() const override { return QStringLiteral("http://127.0.0.1:8080/v1"); }
    bool    isLocal() const override { return true; }
    bool    available() const override;
    int     maxContextTokens() const override { return 4096; }
    int     maxTools() const override { return 4; }

    void chat(const QJsonArray &messages, const QJsonArray &tools) override;
    void cancel() override;

    //! Startet llama-server mit --prompt-cache und gedeckeltem Kontext.
    Q_INVOKABLE bool startServer(const QString &modelPath);
    Q_INVOKABLE void stopServer();

private:
    QProcess m_server;
};

#endif
