/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CDI_H
#define CDI_H

#include <QVector>
#include <QXmlStreamReader>

#include "identification.h"
#include "acdi.h"
#include "segment.h"

class CDI
{
public:
    CDI();

    Identification identification();
    ACDI acdi();
    QVector<Segment> segments();

    static CDI createFromXML(QXmlStreamReader* xml);

private:
    Identification m_ident;
    ACDI m_acdi;
    QVector<Segment> m_segments;
};

#endif // CDI_H
