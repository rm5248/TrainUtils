/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SEGMENT_H
#define SEGMENT_H

#include <QString>
#include <QVector>

#include "grouptype.h"

class QXmlStreamReader;

/**
 * Represents a <segment> tag in the LCC XML schema.
 */
class Segment
{
public:
    Segment();

    static Segment createFromXML(QXmlStreamReader* xml);

    QString name() const;
    QString description() const;
    int space() const;
    int origin() const;
    const QVector<CDIVariant>& elements() const;

private:
    int m_space = 0;
    int m_origin = 0;
    QString m_name;
    QString m_description;
    QVector<CDIVariant> m_elements;
};

#endif // SEGMENT_H
