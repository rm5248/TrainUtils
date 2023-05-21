/* SPDX-License-Identifier: GPL-2.0 */
#ifndef GROUPTYPE_H
#define GROUPTYPE_H

#include <QString>
#include <QVariant>

#include "cdivariant.h"

class GroupType
{
public:
    GroupType();

private:
    QString m_name;
    QString m_description;
    QString m_repname;
    QVector<CDIVariant> m_elements;
    int m_offset;
    int m_replication;
};

#endif // GROUPTYPE_H
