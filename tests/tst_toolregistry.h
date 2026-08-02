#ifndef SFAI_TST_TOOLREGISTRY_H
#define SFAI_TST_TOOLREGISTRY_H

#include <QObject>

class TestToolRegistry : public QObject
{
    Q_OBJECT
private slots:
    void init();

    void registersLowToolsEnabled();
    void schemaKeepsRegistrationOrder();
    void schemaOmitsDisabledTools();
    void refusesUnknownTool();
    void refusesDisabledTool();
    void refusesPersonalToolWithoutConsent();
    void redactsPersonalResultAfterConsent();
    void leavesLowResultUntouched();
    void disablingRevokesConsent();
    void togglePersistsAcrossInstances();
};

#endif
