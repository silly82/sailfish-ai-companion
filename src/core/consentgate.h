#ifndef SFAI_CONSENTGATE_H
#define SFAI_CONSENTGATE_H

#include <QObject>
#include <QHash>
#include <QSet>
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
    /*!
     * Liegt hier und nicht in ToolRegistry, weil die Einstufung eine
     * Consent-Frage ist: die Schleuse entscheidet daran, und QML zeigt sie als
     * Badge. Die Registry deklariert sie nur pro Tool.
     */
    enum Sensitivity {
        Low,        //!< Akku, Netz, Uhrzeit — kein Dialog
        Personal,   //!< Kontakte, Kalender  — Bestätigung + Redaktion
        Critical    //!< SMS, Dateien, exec  — Bestätigung, default aus
    };
    Q_ENUM(Sensitivity)

    explicit ConsentGate(QObject *parent = nullptr);

    bool localOnly() const { return m_localOnly; }
    void setLocalOnly(bool v);

    //! Braucht ein Aufruf dieser Stufe eine Bestätigung durch den Nutzer?
    Q_INVOKABLE bool requiresConfirmation(Sensitivity level) const;

    //! Freigabe für die laufende Sitzung. Bewusst nicht persistent — eine
    //! einmal erteilte Zustimmung soll einen Neustart nicht überleben.
    Q_INVOKABLE void grant(const QString &toolName);
    Q_INVOKABLE void revoke(const QString &toolName);
    Q_INVOKABLE bool isGranted(const QString &toolName) const;

    //! Ersetzt personenbezogene Felder durch Platzhalter.
    QVariantMap redact(const QVariantMap &payload);

    //! Löst Platzhalter in der Modellantwort wieder auf. Rein lokal.
    QString restore(const QString &modelOutput) const;

    //! Beim Verlassen einer Konversation aufrufen — die Zuordnung
    //! Platzhalter -> echter Wert hat darüber hinaus nichts zu suchen.
    Q_INVOKABLE void forgetPlaceholders();

signals:
    void localOnlyChanged();
    void confirmationRequested(const QString &toolName, const QString &dataPreview);

private:
    QVariant redactValue(const QVariant &value);
    QString  redactText(const QString &text);
    QString  placeholderFor(const QString &value);

    bool m_localOnly = false;
    QSet<QString> m_granted;
    QHash<QString, QString> m_placeholders;  // "<contact:7>" -> "+41 79 ..."
    QHash<QString, QString> m_reverse;       // "+41 79 ..."  -> "<contact:7>"
    int m_nextPlaceholder = 1;
};

#endif
