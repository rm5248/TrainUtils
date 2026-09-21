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

    QString name() const;
    QString description() const;
    std::optional<int> min() const;
    std::optional<int> max() const;
    std::optional<int> defaultValue() const;
    std::optional<MapType> map() const;
    int size() const;
    int offset() const;

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
