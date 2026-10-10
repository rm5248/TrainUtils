/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CDIVARIANT_H
#define CDIVARIANT_H

#include <QString>

#include "grouptype.h"

QString cdiVariantName(const CDIVariant& v);
QString cdiVariantDescription(const CDIVariant& v);
bool cdiVariantIsGroup(const CDIVariant& v);

/**
 * The value of the 'offset' attribute of the element: the number of bytes to
 * skip, relative to the end of the previous element, before this element starts.
 */
int cdiVariantOffset(const CDIVariant& v);

/**
 * The number of bytes of memory that this element takes up, not including its
 * own offset.  For a group, this includes all of its replications.
 */
int cdiVariantSize(const CDIVariant& v);

/**
 * The number of bytes that a single replication of the given elements takes up.
 */
int cdiElementsSize(const QVector<CDIVariant>& elements);

#endif // CDIVARIANT_H
