#ifndef SFAI_SSEPARSER_H
#define SFAI_SSEPARSER_H

#include <QByteArray>
#include <QVector>

/*!
 * Framing für text/event-stream. Bewusst ohne QObject und ohne QtNetwork,
 * damit sich das Zerlegen auf dem Desktop testen lässt — das ist der Teil,
 * der auf Mobilfunk tatsächlich kaputtgeht.
 *
 * Zerlegt wird nur; die Payload interpretiert der Aufrufer. Ein Ereignis
 * endet an einer Leerzeile, mehrere data-Zeilen werden mit \n verbunden
 * (SSE-Spezifikation). Kommentarzeilen beginnen mit ":" — OpenRouter sendet
 * regelmässig ": OPENROUTER PROCESSING" als Keep-Alive.
 */
class SseParser
{
public:
    //! Hängt einen Netzwerk-Chunk an. Teilzeilen werden gepuffert, bis der
    //! Rest eintrifft.
    void feed(const QByteArray &bytes);

    //! Gibt die seit dem letzten Aufruf vollständigen data-Payloads zurück
    //! und leert die interne Liste.
    QVector<QByteArray> takeEvents();

    void reset();

private:
    void handleLine(const QByteArray &line);
    void flushEvent();

    QByteArray          m_buffer;
    QByteArray          m_data;
    bool                m_haveData = false;
    QVector<QByteArray> m_events;
};

#endif
