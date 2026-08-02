#ifndef SFAI_TST_MODELLIST_H
#define SFAI_TST_MODELLIST_H

#include <QObject>

class TestModelList : public QObject
{
    Q_OBJECT
private slots:
    void dropsModelsWithoutToolSupport();
    void dropsShortContextModels();
    void marksFreeModels();
    void survivesGarbage();
};

#endif
