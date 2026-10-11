/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CDILAYOUT_H
#define CDILAYOUT_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "cdi.h"
#include "grouptype.h"

/**
 * A single value(int, string or event ID) in a CDI, along with where it is
 * stored in the node's memory.
 */
struct CdiField {
    CDIVariant variant;
    int space = 0;
    uint32_t address = 0;
};

/**
 * A segment or group in a CDI, with all of the addresses resolved.  Replicated
 * groups are expanded, so a group with replication='4' becomes 4 CdiLayoutGroups.
 */
struct CdiLayoutGroup {
    /** What to show for this group, e.g. "Line 3" */
    QString label;
    int space = 0;
    /** Address where this group starts */
    uint32_t address = 0;
    /** The values directly in this group(not in sub-groups) */
    QVector<CdiField> fields;
    QVector<CdiLayoutGroup> children;
};

/**
 * Resolve the addresses of everything in the CDI.  This is the one place that
 * knows how offsets, sizes and replication determine where a value is stored.
 *
 * @return One CdiLayoutGroup per segment
 */
QVector<CdiLayoutGroup> cdiLayout(const CDI& cdi);

/**
 * Call fn for every field in the given groups(and their children), with the
 * labels of the groups that lead to the field, starting with the segment.
 */
template<typename Fn>
void cdiForEachField(const QVector<CdiLayoutGroup>& groups, Fn fn, QStringList path = QStringList()){
    for(const CdiLayoutGroup& group : groups){
        QStringList groupPath = path;
        groupPath.push_back(group.label);
        for(const CdiField& field : group.fields){
            fn(field, groupPath);
        }
        cdiForEachField(group.children, fn, groupPath);
    }
}

#endif // CDILAYOUT_H
