#ifndef SFAI_ILLMBACKEND_H
#define SFAI_ILLMBACKEND_H

#include <QObject>
#include <QJsonArray>

/*!
 * Beide Backends sprechen die GLEICHE OpenAI-kompatible API.
 * Der Unterschied ist die Base-URL — sonst nichts.
 *
 *   OpenRouterBackend   -> https://openrouter.ai/api/v1   (Hauptpfad)
 *   LocalServerBackend  -> http://127.0.0.1:8080/v1       (llama-server)
 *
 * Deshalb wird llama.cpp NICHT in die App gelinkt, sondern läuft als
 * eigenes Paket (sailfishai-llama). Vorteile:
 *   - kein natives Blob im Harbour-RPM  -> Review-Risiko entfällt
 *   - llama.cpp updatebar ohne App-Update
 *   - Prozess-Spawn liegt sauber im Full-Target
 */
class ILlmBackend : public QObject
{
    Q_OBJECT
public:
    explicit ILlmBackend(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ILlmBackend() = default;

    virtual QString baseUrl() const = 0;
    virtual bool    isLocal() const = 0;
    virtual bool    available() const = 0;

    //! Max. Kontext, den dieses Backend sinnvoll verkraftet.
    //! Lokal HART deckeln — der KV-Cache frisst sonst RAM und Bandbreite.
    virtual int maxContextTokens() const = 0;

    //! Lokal ein reduziertes Tool-Set: 3-4 statt 8+.
    //! Jedes Tool-Schema kostet Prefill-Zeit, und Prefill ist auf ARM
    //! der Flaschenhals — nicht die Generierung.
    virtual int maxTools() const = 0;

    //! Modell-Kennung, wie sie das Backend im Request erwartet.
    //! Nie hardcoden — die Liste kommt zur Laufzeit von /models.
    QString model() const { return m_model; }
    void    setModel(const QString &id) { m_model = id; }

    virtual void chat(const QJsonArray &messages, const QJsonArray &tools) = 0;
    virtual void cancel() = 0;

protected:
    QString m_model;

signals:
    void delta(const QString &chunk);

    //! Die id gehoert dazu — die Antwort muss sie als tool_call_id
    //! zurueckgeben, sonst ordnet das Modell sie nicht zu.
    void toolCall(const QString &id, const QString &name, const QVariantMap &args);
    void finished();
    void failed(const QString &message);
};

#endif
