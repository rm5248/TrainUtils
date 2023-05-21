/* SPDX-License-Identifier: GPL-2.0 */
#ifndef STRINGTYPE_H
#define STRINGTYPE_H

#include <QString>

#include "maptype.h"

class StringType
{
public:
    StringType();

private:
    QString m_name;
    QString m_description;
    MapType m_map;
    int m_maxLen;
    int m_offset;
};

#endif // STRINGTYPE_H
