#ifndef SFAI_CONSENTGATE_H
#define SFAI_CONSENTGATE_H

#include <QObject>
#include <QVariantMap>

/*!
 * Datenschleuse. Alles, was aus dem Gerät heraus an ein Cloud-Modell geht,
 * passiert diese Klasse.
 *
 *  1. Per-Tool-Toggle, default aus ausser Sensitivity::Low
 *  2. Blockierender Bestätigungsdialog ab Personal — mit Anzeige der exakten
 *     Daten VOR dem Request
 *  3. Redaktionsschicht: Telefonnummern/Adressen -> "<contact:7>",
 *     Rück-Auflösung passiert lokal nach der Antwort
 *  4. localOnly: sobald ein lokales Modell aktiv ist, entfällt 2 und 3 —
 *     nichts verlässt das Gerät
 */
class ConsentGate : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool localOnly READ localOnly WRITE setLocalOnly NOTIFY localOnlyChanged)

public:
    explicit ConsentGate(QObject *parent = nullptr);

    bool localOnly() const { return m_localOnly; }
    void setLocalOnly(bool v);

    //! Ersetzt personenbezogene Felder durch Platzhalter.
    QVariantMap redact(const QVariantMap &payload);

    //! Löst Platzhalter in der Modellantwort wieder auf. Rein lokal.
    QString restore(const QString &modelOutput) const;

signals:
    void localOnlyChanged();
    void confirmationRequested(const QString &toolName, const QString &dataPreview);

private:
    bool m_localOnly = false;
    QHash<QString, QString> m_placeholders;  // "<contact:7>" -> "+41 79 ..."
};

#endif
