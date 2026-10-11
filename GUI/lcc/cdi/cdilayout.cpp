/* SPDX-License-Identifier: GPL-2.0 */
#include "cdilayout.h"
#include "cdivariant.h"

static void layoutElements(CdiLayoutGroup& parent, const QVector<CDIVariant>& elements, uint32_t address){
    for(const CDIVariant& variant : elements){
        address += cdiVariantOffset(variant);
        uint32_t elementAddress = address;
        address += cdiVariantSize(variant);

        if(!cdiVariantIsGroup(variant)){
            parent.fields.push_back(CdiField{variant, parent.space, elementAddress});
            continue;
        }

        std::shared_ptr<GroupType> group = std::get<std::shared_ptr<GroupType>>(variant);
        int repetitions = group->replication() > 1 ? group->replication() : 1;

        if(repetitions == 1 && group->name().isEmpty() && group->elements().isEmpty()){
            // A bare <group offset='n'/> used purely to pad/align storage -
            // nothing to show.
            continue;
        }

        QString label = group->repname().isEmpty() ? group->name() : group->repname();
        int repetitionSize = cdiElementsSize(group->elements());

        for(int rep = 1; rep <= repetitions; rep++){
            CdiLayoutGroup child;
            child.label = repetitions > 1 ? QStringLiteral("%1 %2").arg(label).arg(rep) : label;
            child.space = parent.space;
            child.address = elementAddress + (rep - 1) * repetitionSize;
            layoutElements(child, group->elements(), child.address);
            parent.children.push_back(child);
        }
    }
}

QVector<CdiLayoutGroup> cdiLayout(const CDI& cdi){
    QVector<CdiLayoutGroup> segments;

    for(const Segment& segment : cdi.segments()){
        CdiLayoutGroup group;
        group.label = segment.name().isEmpty() ?
                    QStringLiteral("Space 0x%1").arg(segment.space(), 2, 16, QLatin1Char('0')) :
                    segment.name();
        group.space = segment.space();
        group.address = segment.origin();
        layoutElements(group, segment.elements(), group.address);
        segments.push_back(group);
    }

    return segments;
}
