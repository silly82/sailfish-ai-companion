#ifndef SFAI_TST_SSEPARSER_H
#define SFAI_TST_SSEPARSER_H

#include <QObject>

class TestSseParser : public QObject
{
    Q_OBJECT
private slots:
    void splitsCompleteEvents();
    void buffersPartialLineAcrossChunks();
    void ignoresKeepAliveComments();
    void joinsMultipleDataLines();
    void handlesCrLf();
    void withholdsEventWithoutBlankLine();
};

#endif
