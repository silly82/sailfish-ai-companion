#include "consentgate.h"

#include <QRegularExpression>
#include <QStringList>

namespace {

//! Bei strukturierter Tool-Ausgabe ist der Schlüssel die verlässlichere
//! Information als der Wert — eine Adresse erkennt kein Muster zuverlässig,
//! das Feld "address" dagegen schon.
bool isSensitiveKey(const QString &key)
{
    static const QStringList keys = QStringList()
        << QStringLiteral("phone")     << QStringLiteral("phonenumber")
        << QStringLiteral("phone_number") << QStringLiteral("tel")
        << QStringLiteral("telephone") << QStringLiteral("mobile")
        << QStringLiteral("msisdn")    << QStringLiteral("email")
        << QStringLiteral("e_mail")    << QStringLiteral("mail")
        << QStringLiteral("address")   << QStringLiteral("street")
        << QStringLiteral("postal_code") << QStringLiteral("zip")
        // Plural-Varianten, wie find_contact sie tatsächlich liefert
        // (fullprovider.cpp) -- ohne die blieb "addresses" unredigiert:
        // Freitext-Adressen matchen ohnehin keine Regex.
        << QStringLiteral("phones")    << QStringLiteral("addresses");
    return keys.contains(key.toLower());
}

const QRegularExpression &emailPattern()
{
    static const QRegularExpression re(
        QStringLiteral("[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}"));
    return re;
}

const QRegularExpression &phonePattern()
{
    static const QRegularExpression re(
        QStringLiteral("\\+?[0-9][0-9 ()./\\-]{5,17}[0-9]"));
    return re;
}

//! YYYY-MM-DD fällt mit Bindestrichen und 8 Ziffern genau in phonePattern()s
//! Ziffern-/Zeichenbereich — ohne diesen Ausschluss redigiert redactText()
//! z.B. "2026-01-15T00:00:00" zu "<contact:N>T00:00:00" (beobachtet bei
//! get_upcoming_events auf echter Hardware, siehe docs/todo-harbour-vs-full.md).
const QRegularExpression &isoDatePattern()
{
    static const QRegularExpression re(QStringLiteral("^\\d{4}-\\d{2}-\\d{2}$"));
    return re;
}

int digitCount(const QString &s)
{
    int n = 0;
    for (int i = 0; i < s.size(); ++i)
        if (s.at(i).isDigit()) ++n;
    return n;
}

}

ConsentGate::ConsentGate(QObject *parent) : QObject(parent) {}

void ConsentGate::setLocalOnly(bool v)
{
    if (m_localOnly == v) return;
    m_localOnly = v;
    emit localOnlyChanged();
}

bool ConsentGate::requiresConfirmation(Sensitivity level) const
{
    // Lokales Modell: die Daten verlassen das Gerät nicht, also gibt es auch
    // nichts zu bestätigen.
    if (m_localOnly) return false;
    return level != Low;
}

void ConsentGate::grant(const QString &toolName) { m_granted.insert(toolName); }
void ConsentGate::revoke(const QString &toolName) { m_granted.remove(toolName); }

bool ConsentGate::isGranted(const QString &toolName) const
{
    return m_localOnly || m_granted.contains(toolName);
}

QVariantMap ConsentGate::redact(const QVariantMap &payload)
{
    if (m_localOnly) return payload;   // lokal: keine Redaktion nötig
    return redactValue(payload).toMap();
}

QVariant ConsentGate::redactValue(const QVariant &value)
{
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap in = value.toMap();
        QVariantMap out;
        for (auto it = in.constBegin(); it != in.constEnd(); ++it) {
            if (!isSensitiveKey(it.key())) {
                out.insert(it.key(), redactValue(it.value()));
                continue;
            }
            // Sensitiver Schlüssel: jeden Wert maskieren, nicht nur einen
            // einzelnen String -- "phones"/"addresses" kommen als
            // QStringList mit potenziell mehreren Einträgen (find_contact).
            if (it.value().type() == QVariant::StringList) {
                QVariantList redacted;
                for (const QString &s : it.value().toStringList()) {
                    if (!s.isEmpty()) redacted.append(placeholderFor(s));
                }
                out.insert(it.key(), redacted);
            } else if (!it.value().toString().isEmpty()) {
                out.insert(it.key(), placeholderFor(it.value().toString()));
            } else {
                out.insert(it.key(), redactValue(it.value()));
            }
        }
        return out;
    }
    // QStringList (e.g. find_contact's "phones"/"addresses") is a distinct
    // QVariant type from QVariant::List -- without this case it fell through
    // to the default branch below and went out completely unredacted,
    // bypassing both the key- and the regex-based checks. Observed live on
    // real hardware: real phone numbers reaching the cloud model verbatim.
    case QVariant::StringList: {
        QVariantList out;
        for (const QString &s : value.toStringList())
            out.append(redactValue(s));
        return out;
    }
    case QVariant::List: {
        const QVariantList in = value.toList();
        QVariantList out;
        out.reserve(in.size());
        for (int i = 0; i < in.size(); ++i) out.append(redactValue(in.at(i)));
        return out;
    }
    case QVariant::String:
        return redactText(value.toString());
    default:
        return value;
    }
}

QString ConsentGate::redactText(const QString &text)
{
    QString out = text;

    // E-Mail zuerst: der Platzhalter enthält danach zu wenige Ziffern, um
    // noch als Telefonnummer durchzugehen.
    for (int pass = 0; pass < 2; ++pass) {
        const QRegularExpression &re = (pass == 0) ? emailPattern() : phonePattern();
        QString rebuilt;
        int last = 0;
        QRegularExpressionMatchIterator it = re.globalMatch(out);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            const QString hit = m.captured(0);
            // Eine Landesvorwahl hat 7 Stellen, eine IBAN oder ein Zeitstempel
            // deutlich mehr — dazwischen liegt der Bereich, den wir meinen.
            if (pass == 1) {
                const int digits = digitCount(hit);
                if (digits < 7 || digits > 15) continue;
                if (isoDatePattern().match(hit).hasMatch()) continue;
            }
            rebuilt += out.mid(last, m.capturedStart() - last);
            rebuilt += placeholderFor(hit);
            last = m.capturedEnd();
        }
        if (last == 0) continue;
        rebuilt += out.mid(last);
        out = rebuilt;
    }
    return out;
}

QString ConsentGate::placeholderFor(const QString &value)
{
    const QString known = m_reverse.value(value);
    if (!known.isEmpty()) return known;

    const QString token =
        QStringLiteral("<contact:%1>").arg(m_nextPlaceholder++);
    m_placeholders.insert(token, value);
    m_reverse.insert(value, token);
    return token;
}

QString ConsentGate::restore(const QString &modelOutput) const
{
    QString out = modelOutput;
    for (auto it = m_placeholders.constBegin(); it != m_placeholders.constEnd(); ++it)
        out.replace(it.key(), it.value());
    return out;
}

void ConsentGate::forgetPlaceholders()
{
    m_placeholders.clear();
    m_reverse.clear();
    m_nextPlaceholder = 1;
}
