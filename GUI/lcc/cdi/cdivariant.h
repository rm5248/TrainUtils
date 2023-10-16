/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CDIVARIANT_H
#define CDIVARIANT_H

#include <variant>
#include <memory>

#include "inttype.h"
#include "stringtype.h"
#include "eventidtype.h"
#include "grouptype.h"

//class CDIVariant
//{
//public:
//    enum class VariableType{
//        Invalid,
//        Integer,
//        String,
//        EventID,
//        Group,
//    };

//    CDIVariant();
//    CDIVariant(GroupType type);

//    VariableType type();

//    IntType to_int();
//    StringType to_string();
//    EventIDType to_eventID();
//    GroupType to_group();

//private:
//    VariableType m_type;
////    std::variant<
//};

#endif // CDIVARIANT_H
