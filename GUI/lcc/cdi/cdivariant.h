/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CDIVARIANT_H
#define CDIVARIANT_H

#include <QString>

#include "grouptype.h"

QString cdiVariantName(const CDIVariant& v);
QString cdiVariantDescription(const CDIVariant& v);
bool cdiVariantIsGroup(const CDIVariant& v);

#endif // CDIVARIANT_H
