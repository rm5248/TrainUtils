/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ACDI_H
#define ACDI_H

class QXmlStreamReader;

/**
 * Abbreviated Common Description Interface
 */
class ACDI
{
public:
    ACDI();

    static ACDI createFromXML(QXmlStreamReader* xml);

private:
    int m_fixedFormat;
    int m_varFormat;
};

#endif // ACDI_H
