/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MAPTYPE_H
#define MAPTYPE_H

#include <QString>
#include <QVector>
#include <QMap>

class MapType
{
public:
    MapType();

private:
    QString m_name;
    QString m_description;
    QVector<QString> m_relations;
    QMap<QString,QString> m_properties;
};

#endif // MAPTYPE_H
