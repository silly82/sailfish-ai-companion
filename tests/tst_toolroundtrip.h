#ifndef SFAI_TST_TOOLROUNDTRIP_H
#define SFAI_TST_TOOLROUNDTRIP_H

#include <QObject>

class TestToolRoundtrip : public QObject
{
    Q_OBJECT
private slots:
    void init();

    void answersWithoutToolsInOneRound();
    void executesToolAndAsksAgain();
    void truncatesSchemaToBackendLimit();
    void stopsAfterTooManyToolRounds();
    void deniedConsentReachesTheModel();
    void grantedConsentRunsTheToolRedacted();
};

#endif
