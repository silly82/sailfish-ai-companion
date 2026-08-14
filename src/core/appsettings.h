#ifndef SFAI_APPSETTINGS_H
#define SFAI_APPSETTINGS_H

#include <QSettings>
#include <QStandardPaths>
#include <QString>

/*!
 * QSettings() ohne expliziten Pfad schreibt nach
 * ~/.config/<Organization>/<Application>.conf -- diese Datei liegt aber
 * ausserhalb dessen, was Sailjail fuer den Harbour-Build freigibt (nur das
 * gleichnamige Verzeichnis ist whitelisted, nicht die Elternebene). Der
 * Schreibversuch verpufft dort lautlos, ohne Fehler-Signal. Explizit in
 * dieses Verzeichnis schreiben, das per --mkdir/--whitelist garantiert
 * zugaenglich ist.
 *
 * QSettings ist nicht kopierbar -- deshalb hier der Pfad statt eines
 * fertigen Objekts, jeder Aufrufer baut sich sein eigenes QSettings(...).
 */
inline QString appSettingsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
         + QStringLiteral("/settings.ini");
}

#endif
