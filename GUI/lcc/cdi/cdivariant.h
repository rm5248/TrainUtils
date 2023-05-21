/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CDIVARIANT_H
#define CDIVARIANT_H

#include <QString>

#include "inttype.h"
#include "stringtype.h"
#include "eventidtype.h"

class CDIVariant
{
public:
    enum class VariableType{
        Invalid,
        Integer,
        String,
        EventID,
        Group,
    };

    CDIVariant();

    VariableType type();

    IntType to_int();
    StringType to_string();
    EventIDType to_eventID();

private:
    VariableType m_type;
};

#endif // CDIVARIANT_H
