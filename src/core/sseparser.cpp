#include "sseparser.h"

void SseParser::feed(const QByteArray &bytes)
{
    m_buffer += bytes;

    int nl;
    while ((nl = m_buffer.indexOf('\n')) >= 0) {
        QByteArray line = m_buffer.left(nl);
        m_buffer.remove(0, nl + 1);
        if (line.endsWith('\r')) line.chop(1);
        handleLine(line);
    }
}

void SseParser::handleLine(const QByteArray &line)
{
    if (line.isEmpty()) { flushEvent(); return; }
    if (line.startsWith(':')) return;            // Keep-Alive-Kommentar

    const int colon = line.indexOf(':');
    if (colon < 0) return;                       // Feld ohne Wert, hier irrelevant

    const QByteArray field = line.left(colon);
    if (field != "data") return;                 // event/id/retry braucht M1 nicht

    QByteArray value = line.mid(colon + 1);
    if (value.startsWith(' ')) value.remove(0, 1);

    if (m_haveData) m_data += '\n';
    m_data += value;
    m_haveData = true;
}

void SseParser::flushEvent()
{
    if (!m_haveData) return;
    m_events.append(m_data);
    m_data.clear();
    m_haveData = false;
}

QVector<QByteArray> SseParser::takeEvents()
{
    QVector<QByteArray> out;
    out.swap(m_events);
    return out;
}

void SseParser::reset()
{
    m_buffer.clear();
    m_data.clear();
    m_haveData = false;
    m_events.clear();
}
