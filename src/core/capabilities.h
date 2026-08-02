#ifndef SFAI_CAPABILITIES_H
#define SFAI_CAPABILITIES_H

#include <QObject>

/*!
 * Laufzeit-Fähigkeitsmodell. Einzige Stelle, an der sich die beiden
 * Build-Targets für die UI unterscheiden. QML fragt hier ab und blendet
 * Nicht-Verfügbares aus — keine #ifdefs in QML.
 */
class Capabilities : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool sandboxed      READ sandboxed      CONSTANT)
    Q_PROPERTY(bool battery        READ battery        CONSTANT)
    Q_PROPERTY(bool network        READ network        CONSTANT)
    Q_PROPERTY(bool contacts       READ contacts       CONSTANT)
    Q_PROPERTY(bool bluetooth      READ bluetooth      CONSTANT)
    Q_PROPERTY(bool telephony      READ telephony      CONSTANT)
    Q_PROPERTY(bool messages       READ messages       CONSTANT)
    Q_PROPERTY(bool calendar       READ calendar       CONSTANT)
    Q_PROPERTY(bool filesystem     READ filesystem     CONSTANT)
    Q_PROPERTY(bool localInference READ localInference CONSTANT)
    Q_PROPERTY(bool background     READ background     CONSTANT)
    Q_PROPERTY(bool automation     READ automation     CONSTANT)

public:
    explicit Capabilities(QObject *parent = nullptr);

    bool sandboxed()      const;
    bool battery()        const { return true; }   // ContextKit bzw. UPower
    bool network()        const { return true; }
    bool contacts()       const;
    bool bluetooth()      const { return true; }
    bool telephony()      const;
    bool messages()       const;   // nur fullaccess (libcommhistory)
    bool calendar()       const;   // nur fullaccess (libmkcal)
    bool filesystem()     const;
    bool localInference() const;
    bool background()     const;   // nur fullaccess (systemd user unit)
    bool automation()     const;   // nur fullaccess (Prozess-Spawn)
};

#endif
