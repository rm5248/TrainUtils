/* SPDX-License-Identifier: GPL-2.0 */
#ifndef INTTYPE_H
#define INTTYPE_H

#include <QString>

#include "maptype.h"

class IntType
{
public:
    IntType();

private:
    QString m_name;
    QString m_description;
    int m_min;
    int m_max;
    int m_default;
    MapType m_map;
    int m_storageSize;
    int m_offset;
};

#endif // INTTYPE_H
