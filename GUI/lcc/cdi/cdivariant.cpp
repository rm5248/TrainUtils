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

int cdiVariantOffset(const CDIVariant& v){
    return std::visit([](const auto& ptr) -> int {
        return ptr->offset();
    }, v);
}

int cdiVariantSize(const CDIVariant& v){
    if(auto i = std::get_if<std::shared_ptr<IntType>>(&v)){
        // Per the CDI schema, an int defaults to 1 byte
        return (*i)->size() > 0 ? (*i)->size() : 1;
    }else if(auto s = std::get_if<std::shared_ptr<StringType>>(&v)){
        return (*s)->size();
    }else if(std::holds_alternative<std::shared_ptr<EventIDType>>(v)){
        return 8;
    }else if(auto g = std::get_if<std::shared_ptr<GroupType>>(&v)){
        int repetitions = (*g)->replication() > 1 ? (*g)->replication() : 1;
        return repetitions * cdiElementsSize((*g)->elements());
    }

    return 0;
}

int cdiElementsSize(const QVector<CDIVariant>& elements){
    int size = 0;
    for(const CDIVariant& v : elements){
        size += cdiVariantOffset(v) + cdiVariantSize(v);
    }
    return size;
}
