#include "consentgate.h"

ConsentGate::ConsentGate(QObject *parent) : QObject(parent) {}

void ConsentGate::setLocalOnly(bool v)
{
    if (m_localOnly == v) return;
    m_localOnly = v;
    emit localOnlyChanged();
}

QVariantMap ConsentGate::redact(const QVariantMap &payload)
{
    if (m_localOnly) return payload;   // lokal: keine Redaktion nötig
    // TODO M2: Telefonnummern, E-Mail, Adressen erkennen und ersetzen.
    //          Das Modell braucht die echte Nummer praktisch nie.
    return payload;
}

QString ConsentGate::restore(const QString &modelOutput) const
{
    QString out = modelOutput;
    for (auto it = m_placeholders.constBegin(); it != m_placeholders.constEnd(); ++it)
        out.replace(it.key(), it.value());
    return out;
}
