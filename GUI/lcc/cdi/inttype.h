/* SPDX-License-Identifier: GPL-2.0 */
#ifndef INTTYPE_H
#define INTTYPE_H

#include <QString>
#include <optional>

#include "maptype.h"

class QXmlStreamReader;

class IntType
{
public:
    IntType();

    static IntType createFromXML(QXmlStreamReader* xml);

private:
    QString m_name;
    QString m_description;
    std::optional<int> m_min;
    std::optional<int> m_max;
    std::optional<int> m_default;
    std::optional<MapType> m_map;
    int m_storageSize = 0;
    int m_offset = 0;
};

#endif // INTTYPE_H
