/* SPDX-License-Identifier: GPL-2.0 */
#ifndef IDENTIFICATION_H
#define IDENTIFICATION_H

#include <QString>
#include <QXmlStreamReader>

#include "maptype.h"

class Identification
{
public:
    Identification();

    QString manufacturer();
    QString model();
    QString hardwareVersion();
    QString softwareVersion();

    static Identification createFromXML(QXmlStreamReader* reader);

private:
    QString m_manufacturer;
    QString m_model;
    QString m_hardwareVersion;
    QString m_softwareVersion;
    MapType m_map;
};

#endif // IDENTIFICATION_H
