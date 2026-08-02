#ifndef SFAI_TST_CONSENTGATE_H
#define SFAI_TST_CONSENTGATE_H

#include <QObject>

class TestConsentGate : public QObject
{
    Q_OBJECT
private slots:
    void confirmsAbovePersonal();
    void localOnlyWaivesEverything();
    void replacesPhoneAndMail();
    void reusesPlaceholderForSameValue();
    void redactsBySensitiveKey();
    void leavesHarmlessNumbersAlone();
    void restoresPlaceholdersInAnswer();
    void forgetsPlaceholders();
};

#endif
