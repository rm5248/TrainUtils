/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MAPTYPE_H
#define MAPTYPE_H

#include <QString>
#include <QVector>
#include <QMap>
#include <variant>

class QXmlStreamReader;

class MapType
{
public:
    typedef std::variant<QString, int, uint64_t> Property;

    MapType();

    static MapType createFromXML(QXmlStreamReader* xml);

private:
    QString m_name;
    QString m_description;
    QMap<QString,Property> m_relations;
};

#endif // MAPTYPE_H
