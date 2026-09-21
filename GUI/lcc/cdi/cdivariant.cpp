/* SPDX-License-Identifier: GPL-2.0 */
#include "cdivariant.h"

#include "inttype.h"
#include "stringtype.h"
#include "eventidtype.h"

QString cdiVariantName(const CDIVariant& v){
    return std::visit([](const auto& ptr) -> QString {
        return ptr->name();
    }, v);
}

QString cdiVariantDescription(const CDIVariant& v){
    return std::visit([](const auto& ptr) -> QString {
        return ptr->description();
    }, v);
}

bool cdiVariantIsGroup(const CDIVariant& v){
    return std::holds_alternative<std::shared_ptr<GroupType>>(v);
}
