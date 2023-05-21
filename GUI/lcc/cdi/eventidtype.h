/* SPDX-License-Identifier: GPL-2.0 */
#ifndef EVENTIDTYPE_H
#define EVENTIDTYPE_H

#include <QString>

#include "maptype.h"

class EventIDType
{
public:
    EventIDType();

private:
    QString m_name;
    QString m_description;
    MapType m_map;
    int m_offset;
};

#endif // EVENTIDTYPE_H
